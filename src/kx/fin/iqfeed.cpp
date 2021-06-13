#include "kx/fin/iqfeed.h"
#include "kx/net/client.h"
#include "kx/util.h"
#include "kx/io.h"
#include "kx/fin/yahoo.h"
#include "kx/log.h"
#include "kx/debug.h"
#include "kx/rand.h"
#include "kx/time.h"
#include "kx/multithread/shared_lock_set.h"

#include <cstdlib>
#include <fstream>
#include <cstdint>
#include <vector>
#include <cmath>
#include <atomic>
#include <mutex>
#include <set>
#include <map>
#include <queue>
#include <deque>
#include <atomic>
#include <thread>

using std::vector;
using std::string;

template<class T>
class std::numeric_limits<std::atomic<T>> : public std::numeric_limits<T> {};

namespace kx { namespace fin { namespace IQFeed {

//we read in binary data with the following assumptions:
static_assert(sizeof(OHLCV_1min_Bar) == 56);
static_assert(sizeof(OHLCV_1day_Bar) == 48);
static_assert(sizeof(EquityFundamentalData) == 80);

//we will also assume EquityTickL1's size if we download and store them in the future
//static_assert(sizeof(EquityTickL1) == ...);

static constexpr const char *IQ_PROTOCOL_VERSION = "6.1";

bool EquityTickL1::is_out_of_sequence() const
{
    for(size_t i=0; i<sizeof(most_recent_conditions)/sizeof(*most_recent_conditions); i++) {
        if(is_trade_cond_out_of_sequence(most_recent_conditions[i]))
            return true;
    }
    return false;
}
bool EquityTickL1::operator < (const EquityTickL1 &other) const
{
    return most_recent_time < other.most_recent_time;
}

/*Quickly converts strings in a fixed point format with up
  to 19 digits up to the decimal point to a double. The
  strings may optionally be prefixed with '-'. This function
  does not check for errors in the input.
*/
[[gnu::pure]]
static double atof_fixed(const string &input)
{
    static constexpr double POW10[] {
        1e-0, 1e-1, 1e-2, 1e-3, 1e-4,
        1e-5, 1e-6, 1e-7, 1e-8, 1e-9,
        1e-10, 1e-11, 1e-12, 1e-13, 1e-14,
        1e-15, 1e-16, 1e-17, 1e-18, 1e-19
    };
    if(input.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    size_t i;
    if(input[0] == '-')
        i = 1;
    else
        i = 0;
    double val = 0;
    for(; i<input.size(); i++) {
        if(input[i] == '.') {
            for(size_t j=i+1; j<input.size(); j++) {
                k_assert(j-i < sizeof(POW10) / sizeof(*POW10));
                val += (input[j]-'0')*POW10[j-i];
            }
            break;
        }
        val = val*10 + input[i]-'0';
    }
    if(input[0] == '-')
        return -val;
    return val;
}

[[gnu::const]]
static inline int uppercase_hex_to_int(char c)
{
    static_assert('A' > '0');
    if(c >= 'A')
        return c - 'A' + 10;
    return c - '0';
}

[[maybe_unused]]
static void test_atof_fixed()
{
    /*atof_fixed could fail if rand_val happens to be very small and
      to_str converts it to scientific notation (e.g. 1.8e-12). atof_fixed
      can't handle this, but that's ok because it's not supposed to.
      This shouldn't print any errors because strings with scientific
      notation (i.e. ones that contain 'e') are ignored
    */
    MersenneTwister_RNG rng;
    int num_tests = 0;
    for(int i=0; i<1e6; i++) {
        auto rand_val = to_str((rng.randf64()-0.5)*20000);
        if(rand_val.find('e') == string::npos) {
            num_tests++;
            auto v1 = std::atof(rand_val.c_str());
            auto v2 = atof_fixed(rand_val);
            if(std::fabs(v1 - v2) > 1e-11) {
                io::println("BAD " + to_str(v1) + " " + to_str(v2) + " " + to_str(v2-v1));
            }
        }
    }
    io::println("checked " + to_str(num_tests) + " random numbers for std::atof/fast_atof match");
}

/** Note that the parsing functions could fail if IQFeed suddenly disconnects and sends us
 *  an incomplete line.
 *  However, they shouldn't crash even if we receive an incomplete line. We only process a
 *  line if it has >=X fields, meaning the first X-1 must be complete. Parsing the last field
 *  shouldn't crash (it might silently error out though) even if it's incomplete, because
 *  for OHLCV data it contains volume, which requires parsing an integer, which won't crash
 *  even if the integer is incomplete. For tick data, it requires parsing trade condition
 *  codes, which consist of 2 hex characters each. We parse (C/2) codes, where C is the
 *  number of hex characters we receive, so if we receive an odd number of characters, a
 *  check might fail, but we'll just end up ignoring the last character.
 */
vector<EquityTickL1> parse_hist_equity_ticks_L1(const vector<string> &lines)
{
    vector<EquityTickL1> data(lines.size());

    #pragma omp parallel for //I think this should work (atof/atoll are thread safe)
    for(size_t i=0; i<lines.size(); i++) {
        auto fields = split_str(lines[i], ',');
        if(fields.size() < 10) {
            log_error("not enough fields in historical tick data "
                      "(did IQFeed disconnect?)");
            continue;
        }
        data[i] = {}; //zero initialize (we need to set last_conditions to 0)

        data[i].bid_time = Time::NA();
        data[i].ask_time = Time::NA();
        data[i].most_recent_time = Time(fields[0], Time::Format::YYYY_mm_DD_HH_MM_SS_ffffff);

        data[i].bid = atof_fixed(fields[4]);
        data[i].ask = atof_fixed(fields[5]);
        data[i].most_recent = atof_fixed(fields[1]);

        data[i].bid_size = std::numeric_limits<double>::quiet_NaN();
        data[i].ask_size = std::numeric_limits<double>::quiet_NaN();
        data[i].most_recent_size = std::atoll(fields[2].c_str());

        data[i].cum_vol = std::atoll(fields[3].c_str());

        data[i].bid_market = 0;
        data[i].ask_market = 0;
        data[i].most_recent_market = std::atoi(fields[8].c_str());

        data[i].most_recent_aggressor = std::atoi(fields[10].c_str());

        if(!(fields[9].size()<=8 && fields[9].size()%2==0)) {
            log_error("bad historical equity tick flags");
            continue;
        }

        for(size_t j=0; j<fields[9].size()/2; j++) {
            data[i].most_recent_conditions[j] = 16*uppercase_hex_to_int(fields[9][2*j]) +
                                                uppercase_hex_to_int(fields[9][2*j+1]);
        }

        //all historical data ticks are last qualified
        data[i].flags |= EquityTickL1::LAST_QUALIFIED_BIT;

        if(fields[7].size() != 1) {
            log_error("bad historical equity tick trade code");
            continue;
        }

        if(fields[7][0]=='O')
            data[i].flags |= EquityTickL1::OTHER_TRADE_BIT;
        else if(fields[7][0]=='E')
            data[i].flags |= EquityTickL1::EXTENDED_TRADE_BIT;
    }

    return data;
}
vector<OHLCV_1min_Bar> parse_OHLCV_1min(const vector<string> &lines)
{
    vector<OHLCV_1min_Bar> data(lines.size());

    #pragma omp parallel for //I think this should work (atof/atoll are thread safe)
    for(size_t i=0; i<lines.size(); i++) {
        auto fields = split_str(lines[i], ',');
        if(fields.size() < 7) {
            log_error("not enough fields in OHLCV 1 minute data "
                      "(did IQFeed disconnect?)");
            continue;
        }

        Time time(fields[0], Time::Format::YYYY_mm_DD_HH_MM_SS);
        double open = atof_fixed(fields[3]);
        double high = atof_fixed(fields[1]);
        double low = atof_fixed(fields[2]);
        double close = atof_fixed(fields[4]);
        double volume = std::atoll(fields[6].c_str());
        double cum_vol = std::atoll(fields[5].c_str());

        data[i] = OHLCV_1min_Bar(time, open, high, low, close, volume, cum_vol);
    }

    return data;
}
vector<OHLCV_1day_Bar> parse_OHLCV_1day(const vector<string> &lines)
{
    vector<OHLCV_1day_Bar> data;
    data.resize(lines.size());

    #pragma omp parallel for //I think this should work (atof/atoll are thread safe)
    for(size_t i=0; i<lines.size(); i++) {
        auto fields = split_str(lines[i], ',');
        if(fields.size() < 5) {
            log_error("not enough fields in OHLCV 1 day data "
                      "(did IQFeed disconnect?)");
            continue;
        }

        Time time(fields[0], Time::Format::YYYY_mm_DD);
        double open = atof_fixed(fields[3]);
        double high = atof_fixed(fields[1]);
        double low = atof_fixed(fields[2]);
        double close = atof_fixed(fields[4]);
        double volume = std::atoll(fields[5].c_str());

        data[i] = OHLCV_1day_Bar(time, open, high, low, close, volume);
    }

    return data;
}

static constexpr const char *exch_cstr = "kx_data/iqfeed/market_centers.txt";
static constexpr const char *trade_conds_cstr = "kx_data/iqfeed/trade_conditions.txt";

static vector<string> exch_short_names;
static vector<string> exch_long_names;
static vector<string> trade_cond_short_names;
static vector<string> trade_cond_long_names;

static constexpr int EXCH_NA_IDX = 0;
static constexpr int TCOND_NA_IDX = 0;

//we can't have negative array indices, so these have to be unsigned.
//If they were signed, we could use a map, but that'd be much slower.
static_assert(std::is_unsigned<EquityTickL1::market_t>::value);
static_assert(std::is_unsigned<EquityTickL1::trade_cond_t>::value);

static constexpr size_t EXCH_NAMES_SIZE = std::numeric_limits<EquityTickL1::market_t>::max() + 1;
static constexpr size_t TRADE_COND_SIZE = std::numeric_limits<EquityTickL1::trade_cond_t>::max() + 1;

void load_ID_maps()
{
    exch_short_names.resize(EXCH_NAMES_SIZE, "bad ID");
    exch_long_names.resize(EXCH_NAMES_SIZE, "bad ID");
    exch_short_names[EXCH_NA_IDX] = "exch_NA";
    exch_long_names[EXCH_NA_IDX] = "exch_NA";

    auto data_opt = io::read_binary_file(exch_cstr);
    if(!data_opt.has_value()) {
        log_error("failed to load file \"" + to_str(exch_cstr) + "\"");
    } else {
        const auto &data = *data_opt;
        auto lines = split_str(data, '\n');
        for(size_t i=0; i<lines.size(); i++) {
            if(lines[i].empty())
                continue;
            auto parts = split_str(lines[i], ',');
            size_t pos = std::atoi(parts[0].c_str());
            if(pos==EXCH_NA_IDX || pos>=EXCH_NAMES_SIZE) {
                log_error("invalid pos in exchange names: " + to_str(pos));
                continue;
            }
            exch_short_names[pos] = parts[1];
            exch_long_names[pos] = parts[2];
        }
    }

    trade_cond_short_names.resize(TRADE_COND_SIZE, "bad ID");
    trade_cond_long_names.resize(TRADE_COND_SIZE, "bad ID");
    trade_cond_short_names[TCOND_NA_IDX] = "tcond_NA";
    trade_cond_long_names[TCOND_NA_IDX] = "tcond_NA";

    data_opt = io::read_binary_file(trade_conds_cstr);
    if(!data_opt.has_value()) {
        log_error("failed to load file \"" + to_str(trade_conds_cstr) + "\"");
    } else {
        const auto &data = *data_opt;
        auto lines = split_str(data, '\n');
        for(size_t i=0; i<lines.size(); i++) {
            if(lines[i].empty())
                continue;
            auto parts = split_str(lines[i], ',');
            size_t pos = std::atoi(parts[0].c_str());
            if(pos==TCOND_NA_IDX || pos>=TRADE_COND_SIZE) {
                log_error("invalid pos in trade cond names: " + to_str(pos));
                continue;
            }
            trade_cond_short_names[pos] = parts[1];
            trade_cond_long_names[pos] = parts[2];
        }
        //make sure that these trade conditions refer to out of sequence trades
        k_ensures(trade_cond_short_names[25]=="SOLDOSEQ");
        k_ensures(trade_cond_short_names[49]=="STPD_SOLDOSEQ");
        k_ensures(trade_cond_short_names[58]=="FORTMTSOLDOSEQ");
    }
}

std::string get_exch_short_name(EquityTickL1::market_t ID)
{
    k_expects(exch_short_names.size() == EXCH_NAMES_SIZE); //check for init
    return exch_short_names[ID];
}
std::string get_exch_long_name(EquityTickL1::market_t ID)
{
    k_expects(exch_long_names.size() == EXCH_NAMES_SIZE); //check for init
    return exch_long_names[ID];
}

std::string get_trade_cond_short_name(EquityTickL1::trade_cond_t ID)
{
    k_expects(trade_cond_short_names.size() == TRADE_COND_SIZE); //check for init
    return trade_cond_short_names[ID];
}
std::string get_trade_cond_long_name(EquityTickL1::trade_cond_t ID)
{
    k_expects(trade_cond_long_names.size() == TRADE_COND_SIZE); //check for init
    return trade_cond_long_names[ID];
}
bool is_trade_cond_out_of_sequence(EquityTickL1::trade_cond_t ID)
{
    //we still have to check for init because init ensures that the out of sequence
    //codes are what we expect them to be (e.g. 25=SOLDOSEQ)
    k_expects(trade_cond_short_names.size() == TRADE_COND_SIZE); //check for init
    return ID==25 || ID==49 || ID==58;
}

static std::string format_ticker_to_iqfeed(const std::string &ticker)
{
    //IQFeed uses C.ticker instead of the more standard ticker.TO for TSX equities
    if(ticker.size()>=3 && ticker.substr(ticker.size()-3)==".TO")
        return "C." + ticker.substr(0, ticker.size()-3);
    //pink sheets require .PK for real-time data only, but we currently don't handle them
    //it's difficult to because .PK isn't used for historical data, and it's hard to detect
    //pink sheet tickers

    //otherwise no special formatting has to be done
    return ticker;
}
static std::string format_ticker_from_iqfeed(const std::string &ticker)
{
    //IQFeed uses C.ticker instead of the more standard ticker.TO for TSX equities
    if(ticker.size()>=2 && ticker.substr(0, 2)=="C.")
        return ticker.substr(2) + ".TO";
    //we don't handle pink sheets rn

    //otherwise no special formatting has to be done
    return ticker;
}


class AbstractUserSet;

class AbstractUser
{
    friend AbstractUserSet;

    AbstractUserSet *const owner;

    AbstractUser(AbstractUserSet *owner_);
public:
    virtual ~AbstractUser();

    AbstractUser(const AbstractUser&) = delete;
    AbstractUser &operator = (const AbstractUser&) = delete;

    //nonmovable because the owner shouldn't change (don't want to effectively call dtor twice)
    AbstractUser(AbstractUser&&) = delete;
    AbstractUser &operator = (const AbstractUser&&) = delete;
};

class AbstractUserSet
{
    #pragma GCC diagnostic ignored "-Wattributes"
    [[maybe_unused]] const std::mutex *mtx;

    std::atomic<int> num_valid_users;
    const int max_users;
public:
    static constexpr auto MAX_MAX_USER_COUNT =
            std::numeric_limits<decltype(num_valid_users)>::max() - 1;

    AbstractUserSet(std::mutex *mtx_, int max_users_):
        mtx(mtx_),
        num_valid_users(0),
        max_users(max_users_)
    {}
    ~AbstractUserSet()
    {
        //there shouldn't be any valid users when we reach the dtor.
        //Users keep a pointer to their owning AbstractUserSet, so we have a
        //dangling pointer if there are still users.
        //Users don't keep a smart pointer because good program design should
        //ensure that when this dtor is called, there are no users left.
        k_expects(num_valid_users.load(std::memory_order_acquire) == 0);
    }

    //noncopyable
    AbstractUserSet(const AbstractUserSet&) = delete;
    AbstractUserSet &operator = (const AbstractUserSet&) = delete;

    //nonmovable because AbstractUsers hold a pointer to their owning AbstractUserSet
    AbstractUserSet(AbstractUserSet&&) = delete;
    AbstractUserSet &operator = (AbstractUserSet&&) = delete;

    void subtract_valid_user(Passkey<AbstractUser>)
    {
        num_valid_users.fetch_sub(1, std::memory_order_acq_rel);
    }
    int load_num_valid_users_acq()
    {
        return num_valid_users.load(std::memory_order_acquire);
    }
    std::unique_ptr<AbstractUser> get_user([[maybe_unused]] const std::unique_lock<std::mutex> &lock)
    {
        //uh-oh, it looks like we can still do lock.mutex()->lock(), which is obviously bad.
        //I don't think we can set the modifiers for "lock" to explicitly prevent this.

        k_expects(lock.owns_lock()); //the mutex should be locked
        k_expects(lock.mutex() == mtx); //we should use the same mutex for all get_user calls

        auto num_users_load = num_valid_users.load(std::memory_order_acquire);
        k_expects(num_users_load>=0 && num_users_load<=max_users);

        if(num_users_load >= max_users)
            return nullptr;

        num_valid_users.fetch_add(1, std::memory_order_acq_rel);

        return std::unique_ptr<AbstractUser>(new AbstractUser(this));
    }
};

AbstractUser::AbstractUser(AbstractUserSet *owner_):
    owner(owner_)
{}
AbstractUser::~AbstractUser()
{
    k_expects(owner != nullptr);
    owner->subtract_valid_user({});
}


namespace Stream
{
    static std::mutex connect_mutex;
    static AbstractUserSet user_set(&connect_mutex, 1);

    //static constexpr const char *PRODUCT_NAME = "KEVIN_LIU_44815";
    //static constexpr const char *PRODUCT_VERSION = "1";
    static constexpr int WATCHED_TICKERS_LIMIT = 500; //500 for IQFeed, 1300 for DTN.IQ
    static constexpr uint16_t port = 5009;
    static constexpr const char *FIELDNAMES = "Bid Time,Bid,Bid Size,Bid Market Center,"
                                              "Ask Time,Ask,Ask Size,Ask Market Center,"
                                              "Most Recent Trade Date,Most Recent Trade Time,"
                                              "Most Recent Trade,Most Recent Trade Size,"
                                              "Most Recent Trade Market Center,"
                                              "Most Recent Trade Aggressor,"
                                              "Most Recent Trade Conditions,"
                                              "Total Volume,"
                                              "Restricted Code,"
                                              "Message Contents";

    static net::Client client;

    static std::set<string> watched_tickers;
    static std::deque<EquityTickL1_wTicker> equity_ticks;

    static Time today = Time::NA();
    static Time prev_T_message_timestamp;
    static std::deque<Time::Delta> T_message_pings;

    namespace RecvBuffer
    {
        static string cur_line;
        static std::deque<string> recv_lines_buffer;

        static void update()
        {
            if(get_status() != net::Client::Status::Connected) {
                log_error("RecvBuffer::update called while disconnected (did IQFeed disconnect)?");
                return;
            }

            while(client.recv_ready()) {
                string recv_data = client.recv();
                auto lines = split_str(recv_data, '\n');
                cur_line += std::move(lines[0]);
                if(lines.size() > 1) {
                    recv_lines_buffer.push_back(std::move(cur_line));
                    cur_line = std::move(lines.back());
                    for(size_t i=1; i<lines.size()-1; i++) {
                        //It appears that IQFeed streaming data doesn't have carriage returns, but might
                        //as well check to be safe because it appears like some their other data does.
                        //A stray carriage return could fuck us, so we should check.
                        if(!lines[i].empty() && lines[i].back()=='\r') {
                            lines[i].pop_back();
                            log_warning("carriage return found (and removed) in IQFeed streaming data line");
                        }
                        recv_lines_buffer.push_back(std::move(lines[i]));
                    }
                }
            }
        }
        static bool get_line(string *line)
        {
            if(!recv_lines_buffer.empty()) {
                *line = std::move(recv_lines_buffer.front());
                recv_lines_buffer.pop_front();
                return true;
            }
            return false;
        }
        static std::deque<string> get_lines()
        {
            /*for(const auto &i: recv_lines_buffer)
                io::println(i);*/
            auto ret = std::move(recv_lines_buffer);
            recv_lines_buffer.clear();
            return ret;
        }
    }

    static void handle_T_message(const std::string &time_str)
    {
        if(time_str.size() == 17) {
            Time timestamp(time_str, Time::Format::YYYYmmDD_HH_MM_SS);

            T_message_pings.emplace_back(Time::now_ET() - timestamp);
            if(T_message_pings.size() > 5) //arbitrary limit of 5
                T_message_pings.pop_front();

            prev_T_message_timestamp = timestamp;
            if(prev_T_message_timestamp > timestamp) {
                auto format = Time::Format::YYYY_mm_DD_HH_MM_SS;
                auto cur_timestamp = timestamp.to_str(format);
                auto prev_timestamp = prev_T_message_timestamp.to_str(format);

                log_warning("current \'T\' timestamp is earlier than previous one\n"
                            "current: " + cur_timestamp + "\n"
                            "previous: " + prev_timestamp);
            }
            prev_T_message_timestamp = timestamp;


            auto date = Time(time_str.substr(0, 8), Time::Format::YYYYmmDD);
            if(today!=Time::NA() && today!=date) {
                auto format = Time::Format::YYYY_mm_DD;
                auto prev_date = today.to_str(format);
                auto cur_date = date.to_str(format);

                log_warning("\'T\' messages have different dates:\n"
                            "current: " + cur_date + "\n"
                            "previous: " + prev_date);
            }
            today = date;

        } else
            log_warning("malformed IQFeed T timestamp (expected length 17): " + time_str);
    }
    void connect()
    {
        std::lock_guard<std::mutex> lg(connect_mutex);

        k_expects(get_status() == net::Client::Status::Disconnected);

        client.connect("127.0.0.1", port);
        if(client.get_status() != net::Client::Status::Connected) {
            log_error("couldn't connect to the IQFeed streaming service on port " + to_str(port));
            return;
        }

        client.send("T\r\n");
        client.send("S,SET PROTOCOL " + to_str(IQ_PROTOCOL_VERSION) + "\r\n");
        client.send("S,SELECT UPDATE FIELDS," + to_str(FIELDNAMES) + "\r\n");

        //we expect to receive "CUST" and "CURRENT PROTOCOL" once.
        //we will parse the first timestamp we receive.
        //we receive "CURRENT UPDATE FIELDNAMES" twice, once for the original fieldnames
        //IQFeed gives us and once for the new fieldnames due to our "SELECT UPDATE FIELDS"
        //request. However, we only count the message if the fieldnames IQFeed sends us match
        //the fieldnames we requested, so we only count it once.
        constexpr int NUM_START_MESSAGES = 4;

        int num_start_msg_received = 0;
        bool received_time_str = false;
        bool received_cust = false;
        bool received_protocol = false;
        bool received_fieldnames = false;

        //If IQFeed doesn't send us enough updates after a while, just exit the loop because
        //IQFeed is probably broken
        const Time::Delta TIMEOUT_LEN = Time::Delta(5000, Time::Length::ms);
        Time start_time = Time::now();

        while(num_start_msg_received < NUM_START_MESSAGES) {
            string line;
            try {
                RecvBuffer::update(); //call this every loop because we expect and need new data

                if(RecvBuffer::get_line(&line)) {
                    if(line.empty()) {
                        log_warning("empty line received from IQFeed streaming");
                        continue;
                    }
                    auto parts = split_str(line, ',');
                    switch(parts.at(0).at(0)) {
                    case 'E':
                        log_warning("Received error from IQFeed streaming: \"" + line + "\"");
                        break;
                    case 'S': {
                        auto msg1 = parts.at(1);
                        if(msg1 == "KEY") {

                        } else if(msg1 == "IP") {

                        } else if(msg1 == "SERVER CONNECTED") {
                            log_info("IQFeed is now connected to the Level 1 Quote Server");
                        } else if(msg1 == "SERVER DISCONNECTED") {
                            log_info("IQFeed is now not connected to the Level 1 Quote Server");
                        } else if(msg1 == "CUST") {
                            log_info("IQFeed streaming info:"
                                     "\nService type: " + parts.at(2) +
                                     "\nVersion: " + parts.at(6) +
                                     "\nVerbose Exchanges: " + parts.at(8) +
                                     "\nMax Symbols: " + parts.at(10) +
                                     "\nFlags: " + parts.at(11));
                            if(!received_cust) {
                                num_start_msg_received++;
                                received_cust = true;
                            }
                        } else if(msg1 == "CURRENT PROTOCOL") {
                            log_info("set IQFeed protocol to " + parts.at(2));
                            if(!received_protocol) {
                                num_start_msg_received++;
                                received_protocol = true;
                            }
                        } else if(msg1 == "CURRENT UPDATE FIELDNAMES") {
                            //only count the message if the fieldnames match what we want
                            if(line == "S,CURRENT UPDATE FIELDNAMES,Symbol," + to_str(FIELDNAMES)) {
                                log_info("successfully set IQFeed Level 1 tick fieldnames");
                                if(!received_fieldnames) {
                                    num_start_msg_received++;
                                    received_fieldnames = true;
                                }
                            }
                        } else
                            log_info("unknown msg1: \"" + msg1 + "\"");
                        break;}
                    case 'T':
                        if(!received_time_str) {
                            received_time_str = true;
                            num_start_msg_received++;
                        }
                        handle_T_message(parts.at(1));
                        break;
                    default:
                        log_warning("received message with unknown type from IQFeed streaming: "
                                    "\"" + line + "\"");
                    }
                }
            } catch(std::out_of_range &e) {
                log_error("out_of_range exception while processing IQFeed streaming line: "
                          "\"" + line + "\"");
            }
            if(Time::now() - start_time > TIMEOUT_LEN) {
                log_error("Timed out while waiting for IQFeed's stream to send the expected "
                          "initial messages. Disconnecting.");
                client.send("S,DISCONNECT\r\n");
                client.disconnect();
                return;
            }
        }
    }
    void disconnect()
    {
        std::lock_guard<std::mutex> lg(connect_mutex);

        k_expects(get_status() != net::Client::Status::Connecting);

        if(get_status() == net::Client::Status::Disconnected)
            return;

        auto load_num_users = user_set.load_num_valid_users_acq();
        if(load_num_users > 0) {
            //this shouldn't happen if we have good program design
            log_warning("Waiting for Stream::num_users to reach 0 (currently " +
                        to_str(load_num_users) + ") before disconnecting "
                        "from IQFeed's stream");
            while(user_set.load_num_valid_users_acq() > 0)
                std::this_thread::yield();
        }

        if(get_status() != net::Client::Status::Connected)
            return;
        client.send("S,DISCONNECT\r\n");
        client.disconnect();
    }
    net::Client::Status get_status()
    {
        return client.get_status(); //client.get_status() is thread safe
    }

    User::User(std::unique_ptr<AbstractUser> abstract_user_):
        abstract_user(std::move(abstract_user_))
    {}
    User::~User()
    {}
    void User::start_watching_ticker(const string &ticker)
    {
        if(!watched_tickers.count(ticker)) {
            client.send("w" + format_ticker_to_iqfeed(ticker) + "\r\n");
            watched_tickers.insert(ticker);
            if(watched_tickers.size() > WATCHED_TICKERS_LIMIT) {
                log_warning("watching more tickers (" + to_str(watched_tickers.size()) +
                            ") than IQFeed is supposed to support (" +
                            to_str(WATCHED_TICKERS_LIMIT) + ")");
            }
        }
    }
    void User::stop_watching_ticker(const string &ticker)
    {
        if(watched_tickers.count(ticker)) {
            client.send("r" + format_ticker_to_iqfeed(ticker) + "\r\n");
            watched_tickers.erase(ticker);
        } else
            log_warning("IQFeed::Stream received request to stop watching ticker "
                      "\"" + ticker + "\", but this ticker wasn't already being watched");
    }
    void User::update()
    {
        //we only want to call update() once at the beginning rather than during every loop
        //for efficiency. It's unlikely more data will come in while we're looping, and even
        //if it does, we can just process it the next time update() is called.
        RecvBuffer::update();

        auto lines = RecvBuffer::get_lines();
        bool is_summary_message = false;
        for(const auto &line: lines) {
            try {
                //io::println(line);
                if(line.empty()) {
                    log_warning("empty line received from IQFeed streaming");
                    continue;
                }
                auto parts = split_str(line, ',');
                switch(parts.at(0).at(0)) {
                case 'E':
                    log_info("IQFeed streaming error: \"" + parts.at(1) + "\"");
                    break;
                case 'S': {
                    auto msg1 = parts.at(1);
                    if(msg1 == "KEY") {

                    } else if(msg1 == "IP") {

                    } else if(msg1 == "SERVER CONNECTED") {
                        log_info("IQFeed is now connected to the Level 1 Quote Server");
                    } else if(msg1 == "SERVER DISCONNECTED") {
                        log_info("IQFeed is now not connected to the Level 1 Quote Server");
                    } else
                        log_info("IQFeed streaming system message: \"" + line + "\"");
                    break;}
                case 'n':
                    log_error("IQFeed reported symbol not found: " + parts.at(1));
                    break;
                case 'F':
                    break;
                case 'T':
                    handle_T_message(parts.at(1));
                    break;
                case 'P':
                    is_summary_message = true;
                    [[fallthrough]];
                case 'Q': {
                    if(parts.size() < 20) {
                        log_error("IQFeed equity tick data has the wrong number of fields "
                                  "(expected >=20, got " + to_str(parts.size()) + ")");
                        break;
                    }
                    EquityTickL1_wTicker tick{}; //zero initialize (we need to zero out the flags)

                    tick.ticker = format_ticker_from_iqfeed(parts[1]);

                    //We have to check if fields are empty because there might not always be a bid,
                    //ask, or most recent at the current time. This pretty much only happens to
                    //illiquid stocks during extended hours.

                    //bid
                    //IQFeed only shows the time of day, but I believe it guarantees it will only
                    //show ask ticks that occurred today
                    if(!parts[2].empty())
                        tick.bid_time = today + Time::Delta(parts[2], Time::Format::HH_MM_SS_ffffff);
                    else
                        tick.bid_time = Time::NA();

                    if(!parts[3].empty())
                        tick.bid = atof_fixed(parts[3]);
                    else
                        tick.bid = std::numeric_limits<double>::quiet_NaN();

                    if(!parts[4].empty())
                        tick.bid_size = std::atoll(parts[4].c_str());
                    else
                        tick.bid_size = 0;

                    if(!parts[5].empty())
                        tick.bid_market = std::atoi(parts[5].c_str());
                    else
                        tick.bid_market = EXCH_NA_IDX; //market 0 is the equivalent of "NA"

                    //ask
                    //IQFeed only shows the time of day, but I believe it guarantees it will only
                    //show ask ticks that occurred today
                    if(!parts[6].empty())
                        tick.ask_time = today + Time::Delta(parts[6], Time::Format::HH_MM_SS_ffffff);
                    else
                        tick.ask_time = Time::NA();

                    if(!parts[7].empty())
                        tick.ask = atof_fixed(parts[7]);
                    else
                        tick.ask = std::numeric_limits<double>::quiet_NaN();

                    if(!parts[8].empty())
                        tick.ask_size = std::atoll(parts[8].c_str());
                    else
                        tick.ask_size = 0;

                    if(!parts[9].empty())
                        tick.ask_market = std::atoi(parts[9].c_str());
                    else
                        tick.ask_market = EXCH_NA_IDX; //market 0 is the equivalent of "NA"

                    //most recent
                    //note that date and time are provided in separate fields (10 and 11)
                    if(!parts[10].empty())
                        tick.most_recent_time = Time(parts[10] + " " + parts[11],
                                                     Time::Format::mm_DD_YYYY_HH_MM_SS_ffffff);
                    else
                        tick.most_recent_time = Time::NA();

                    if(!parts[12].empty())
                        tick.most_recent = atof_fixed(parts[12].c_str());
                    else
                        tick.most_recent = std::numeric_limits<double>::quiet_NaN();

                    if(!parts[13].empty())
                        tick.most_recent_size = std::atoll(parts[13].c_str());
                    else
                        tick.most_recent_size = 0;

                    if(!parts[14].empty())
                        tick.most_recent_market = std::atoi(parts[14].c_str());
                    else
                        tick.most_recent_market = EXCH_NA_IDX;

                    constexpr int AGGRESSOR_NA = 0;
                    if(!parts[15].empty())
                        tick.most_recent_aggressor = std::atoi(parts[15].c_str());
                    else
                        tick.most_recent_aggressor = AGGRESSOR_NA;

                    if(parts[16].size()>8 || parts[16].size()%2!=0)
                        log_error("deformed IQFeed trade conditions");
                    for(size_t i=0; i<parts[16].size()/2; i++) {
                        tick.most_recent_conditions[i] = 16*uppercase_hex_to_int(parts[16][2*i]) +
                                                         uppercase_hex_to_int(parts[16][2*i+1]);
                    }

                    tick.cum_vol = std::atoll(parts[17].c_str());
                    tick.flags |= (EquityTickL1::SUMMARY_BIT & is_summary_message);
                    tick.flags |= (EquityTickL1::SHORT_SALE_RESTRICTED_BIT & (parts[18].at(0)=='R'));
                    for(const auto &chr: parts[19]) {
                        switch(chr)
                        {
                        case 'C':
                            tick.flags |= EquityTickL1::LAST_QUALIFIED_BIT;
                            break;
                        case 'E':
                            tick.flags |= EquityTickL1::EXTENDED_TRADE_BIT;
                            break;
                        case 'O':
                            tick.flags |= EquityTickL1::OTHER_TRADE_BIT;
                            break;
                        case 'b':
                            tick.flags |= EquityTickL1::BID_UPDATE_BIT;
                            break;
                        case 'a':
                            tick.flags |= EquityTickL1::ASK_UPDATE_BIT;
                            break;
                        case 'o':
                        case 'h':
                        case 'l':
                        case 'c':
                        case 's':
                        case 'v':
                            //we don't care about these codes
                            break;
                        default:
                            log_warning("unrecognized message ID code character: " + chr);
                            break;
                        }
                    }
                    equity_ticks.push_back(tick);
                    break;}
                default:
                    log_info("IQFeed message with unknown type: " + line);
                }
            } catch(std::out_of_range &e) {
                log_error("reading IQFeed stream produced std::out_of_range for line \"" +
                          line + "\" with error: " + to_str(e.what()));
            }
        }
    }
    std::deque<EquityTickL1_wTicker> User::get_equity_ticks_L1()
    {
        auto ret = std::move(equity_ticks);
        equity_ticks.clear();
        return ret;
    }
    Time::Delta User::get_T_message_ping()
    {
        if(T_message_pings.size() == 0)
            return Time::Delta(9999, Time::Length::ms);

        Time::Delta ping_sum(0, Time::Length::second);
        for(const auto &ping: T_message_pings)
            ping_sum += ping;
        return ping_sum / T_message_pings.size();
    }
    std::unique_ptr<User> request_user()
    {
        std::unique_lock<std::mutex> lock(connect_mutex);

        if(get_status() != net::Client::Status::Connected)
            return nullptr;
        else {
            auto abstract_user = user_set.get_user(lock);
            if(abstract_user != nullptr) {
                return std::unique_ptr<User>(new User(std::move(abstract_user)));
            } else
                return nullptr;
        }
    }
}

namespace HistDb
{
    static std::mutex connect_mutex;
    static AbstractUserSet user_set(&connect_mutex, AbstractUserSet::MAX_MAX_USER_COUNT);

    //static constexpr const char *PRODUCT_NAME = "KEVIN_LIU_44815";
    //static constexpr const char *PRODUCT_VERSION = "1";
    static constexpr uint16_t port = 9105; //IMPORTANT: this is different from the default port of 9100!

    static_assert(Stream::port != HistDb::port);

    static_assert(NUM_CONNECTIONS>0 && NUM_CONNECTIONS<=100); //sanity checks

    //the request limit is 50 requests per second. 1.15s to make sure that requests will fit into
    //the rate limit even if there's random lagg. It's possible to exceed the rate limit if there's too
    //much lagg (e.g. if the requests in a one second interval are all delayed by exactly 1s, the next
    //1s interval will have around 100 requests). This is assuming that the IQFeed server rate limits
    //requests rather than the local IQFeed app (which is probably correct).
    static constexpr size_t MAX_REQ = 50;
    static const Time::Delta REQ_WINDOW = Time::Delta(1.15, Time::Length::second);
    static std::mutex req_q_mtx;

    static std::queue<Time> request_times;

    class ClientPool
    {
        std::atomic_flag is_being_used[NUM_CONNECTIONS];
        net::Client clients[NUM_CONNECTIONS];
        std::atomic<net::Client::Status> status;
    public:
        ClientPool():
            status(net::Client::Status::Disconnected)
        {}

        void connect()
        {
            std::lock_guard<std::mutex> lg(connect_mutex);

            k_expects(get_status() == net::Client::Status::Disconnected);

            status.store(net::Client::Status::Connecting, std::memory_order_relaxed);

            for(size_t i=0; i<NUM_CONNECTIONS; i++)
                is_being_used[i].clear(std::memory_order_relaxed);

            constexpr int MAX_TRIES = 5;
            int num_tries = 1;
            size_t num_failed;
            while(true) { //keep trying if only some fail
                num_failed = 0;
                #pragma omp parallel for num_threads(NUM_CONNECTIONS) reduction(+:num_failed)
                for(size_t i=0; i<NUM_CONNECTIONS; i++) {
                    if(clients[i].get_status() == net::Client::Status::Connected)
                        continue;
                    clients[i].connect("127.0.0.1", port);
                    if(clients[i].get_status() != net::Client::Status::Connected) {
                        num_failed++;
                        continue;
                    }

                    clients[i].send("S,SET PROTOCOL " + to_str(IQ_PROTOCOL_VERSION) + "\r\n");

                    string set_protocol_response;
                    while(true) {
                        set_protocol_response += clients[i].recv();
                        if(!set_protocol_response.empty() && set_protocol_response.back()=='\n') {
                            if(set_protocol_response != "S,CURRENT PROTOCOL," + to_str(IQ_PROTOCOL_VERSION) + "\r\n") {
                                log_error("failed to set IQFeed protocol version to " + to_str(IQ_PROTOCOL_VERSION));
                                log_info("got IQFeed response: " + set_protocol_response);
                            }
                            break;
                        }
                    }
                }
                //If only some connections succeeded, then buggy things happen. I'm not completely sure why.
                //It appears that we don't always reconnect successfully.
                if(num_failed!=0 && num_failed!=NUM_CONNECTIONS) {
                    num_tries++;
                    if(num_tries < MAX_TRIES) {
                        log_warning("some connections to the IQFeed historical data port failed. "
                                    "Retrying (attempt=" + to_str(num_tries) + ")");
                    } else {
                        #pragma omp parallel for
                        for(size_t i=0; i<NUM_CONNECTIONS; i++) {
                            if(clients[i].get_status() == net::Client::Status::Connected)
                                clients[i].disconnect();
                        }
                    }
                }
                else
                    break;
            }
            if(num_failed == NUM_CONNECTIONS) {
                log_error("couldn't connect to the IQFeed historical data service "
                          "on port " + to_str(port));
                status.store(net::Client::Status::Disconnected, std::memory_order_release);
            } else {
                status.store(net::Client::Status::Connected, std::memory_order_release);
            }
        }
        void disconnect()
        {
            std::lock_guard<std::mutex> lg(connect_mutex);

            k_expects(get_status() != net::Client::Status::Connecting);

            auto load_num_users = user_set.load_num_valid_users_acq();
            if(load_num_users > 0) {
                log_warning("Waiting for HistDb::num_users to reach 0 (currently " +
                            to_str(load_num_users) + ") before disconnecting from "
                            "IQFeed's historical data service");
                while(user_set.load_num_valid_users_acq() > 0)
                    std::this_thread::yield();
            }

            size_t num_owned = 0;

            //we don't want to interrupt any active connections, so take "ownership"
            //of all clients to make sure no connections are active.
            while(num_owned < NUM_CONNECTIONS) {
                for(size_t i=0; i<NUM_CONNECTIONS; i++) {
                    if(!is_being_used[i].test_and_set(std::memory_order_acquire))
                        num_owned++;
                }
            }

            #pragma omp parallel for num_threads(NUM_CONNECTIONS)
            for(size_t i=0; i<NUM_CONNECTIONS; i++) {
                clients[i].disconnect();
            }
            status.store(net::Client::Status::Disconnected, std::memory_order_release);
        }
        net::Client::Status get_status()
        {
            return status.load(std::memory_order_acquire);
        }

        string send_request_and_get_response(const string &request)
        {
            if(get_status() != net::Client::Status::Connected) {
                log_error("send_request_and_get_response called while disconnected (did IQFeed disconnect)?");
                return "";
            }

            size_t client_idx;
            //spinlock until we find a suitable client
            //we won't ever have to actually spinlock (i.e. we'll find a free client on
            //our first try) if there are at most NUM_CONNECTION threads calling this
            //function at any given time
            while(true) {
                size_t num_tries = 0;
                for(size_t i=0; i<NUM_CONNECTIONS; i++) {
                    if(!is_being_used[i].test_and_set(std::memory_order_acquire)) {
                        client_idx = i;
                        goto found_client;
                    }
                }
                num_tries++;
                if(num_tries>=100 && std::pow(10, (int)std::log10(num_tries))==num_tries) {
                    log_warning(to_str(num_tries) + "th spinlock loop in HistDataDownloader."
                                "client_pool.send_request_and_get_response while trying to "
                                "acquire a free net::Client.");
                }
                std::this_thread::yield();
            }

            found_client:

            //throttle
            while(true) {
                std::lock_guard<std::mutex> lg(req_q_mtx);
                while(request_times.size()>0 && Time::now_UTC() - request_times.front() > REQ_WINDOW)
                    request_times.pop();
                if(request_times.size() < MAX_REQ) {
                    request_times.push(Time::now_UTC());
                    break;
                }
                std::this_thread::yield();
            }

            clients[client_idx].send(request);

            string buffer;
            while(true) {
                buffer += clients[client_idx].recv();
                if(buffer.size() >= 11 && buffer.substr(buffer.size()-11)=="!ENDMSG!,\r\n")
                    break;
            }
            buffer.erase(buffer.size()-11, string::npos);

            is_being_used[client_idx].clear(std::memory_order_release);

            return buffer;
        }
    };

    static ClientPool client_pool;

    /*void launch_iqfeed(const string &login, const string &password)
    {
        string launch_iqfeed_str = "iqconnect "
                                        "-product " + to_str(PRODUCT_NAME) + " "
                                        "-version " + PRODUCT_VERSION + " "
                                        "-login " + login + " "
                                        "-password " + password + " "
                                        "-autoconnect";
        ShellExecute(NULL, "open", "IQConnect.exe", nullptr, nullptr, SW_SHOWNORMAL)0;
    }*/
    void connect()
    {
        client_pool.connect();
    }
    void disconnect()
    {
        client_pool.disconnect();
    }
    net::Client::Status get_status()
    {
        return client_pool.get_status();
    }

    //year >=2038 is buggy. The following warns if we compile in >=2037. This assumes we compile
    //this file at least once per year. Change end_time_cstr if necessary. There should be checks
    //in all downloading functions, but add this one just in case.
    static_assert(10*(__DATE__[9]-'0') + (__DATE__[10]-'0') <= 36);

    //arbitrarily pick 1980 as the starting year
    //the start date used to be 2007-04-01, so the bars for some tickers might start from there
    //(iqfeed doesn't have many OHLCV 1 min bars before 2007-05-01)
    static constexpr const char* start_time_cstr = "19800101 000000";
    static constexpr const char* end_time_cstr = "20371231 235959"; //IQFeed seems to bug for 2038+

    static constexpr const char* equity_ticks_L1_folder_path = "kx_data/iqfeed/equity_ticks_L1/";
    static constexpr const char* ohlcv_1min_folder_path = "kx_data/iqfeed/OHLCV_1min/";

    static string get_equity_ticks_L1_folder_path(const string &ticker)
    {
        return equity_ticks_L1_folder_path + ticker + "/";
    }
    static string get_ohlcv_1min_folder_path(const string &ticker)
    {
        return ohlcv_1min_folder_path + ticker + "/";
    }
    static bool equity_ticks_L1_folder_exists(const string &ticker)
    {
        auto ticker_folder_path = get_equity_ticks_L1_folder_path(ticker);
        return io::folder_exists(ticker_folder_path);
    }
    static bool OHLCV_1min_folder_exists(const string &ticker)
    {
        auto ticker_folder_path = get_ohlcv_1min_folder_path(ticker);
        return io::folder_exists(ticker_folder_path);
    }

    void convert_raw_OHLCV_1min_to_binary(const vector<string> &tickers)
    {
        #pragma omp parallel for
        for(size_t i=0; i<tickers.size(); i++) {
            string ticker_folder_path = get_ohlcv_1min_folder_path(tickers[i]);

            string raw_data_file_path = ticker_folder_path + start_time_cstr +
                                        (string)"_" + end_time_cstr +
                                        (string)"_raw.csv";

            std::ifstream input_file(raw_data_file_path, std::ios::binary);
            if(input_file.fail()) {
                io::println("failed to open raw data for file \"" + tickers[i] + "\"");
                continue;
            }

            input_file.seekg(0, std::ios::end);
            auto file_len = input_file.tellg();
            input_file.seekg(0, std::ios::beg);

            string file_data;
            file_data.resize(file_len);
            input_file.read((char*)file_data.data(), file_len);
            input_file.close();

            auto lines = split_str(file_data, '\n');
            while(!lines.empty() && lines.back().empty())
                lines.pop_back();
            auto data = parse_OHLCV_1min(lines);

            vector<vector<OHLCV_1min_Bar>> data_by_day;
            vector<string> day_strs;

            int prev_day = -1;
            for(const auto &datapoint: data)
            {
                int cur_day = datapoint.time.to_int64(Time::Length::day);
                if(prev_day != cur_day) {
                    prev_day = cur_day;
                    day_strs.push_back(datapoint.time.to_str(Time::Format::YYYY_mm_DD));
                    data_by_day.emplace_back();
                }
                data_by_day.back().push_back(datapoint);
            }
            k_ensures(data_by_day.size() == day_strs.size());

            #pragma omp parallel for //this should work (days are unique so file names should be too)
            for(size_t j=0; j<data_by_day.size(); j++) {
                string day_output_file_name = ticker_folder_path + day_strs[j] + ".csv.binary";
                std::ofstream day_output_file(day_output_file_name, std::ios::binary | std::ios::trunc);
                if(day_output_file.fail()) {
                    log_error("failed to create output file \"" + day_output_file_name + "\"");
                }
                size_t len = data_by_day[j].size() * sizeof(data_by_day[j][0]);
                day_output_file.write((char*)data_by_day[j].data(), len);
            }
        }
    }
    User::User(std::unique_ptr<AbstractUser> abstract_user_):
        abstract_user(std::move(abstract_user_))
    {}
    User::~User()
    {}
    string User::send_request_and_get_response(const string &request)
    {
        return client_pool.send_request_and_get_response(request);
    }
    static SharedLock_Set<std::string> equity_ticks_L1_locks;
    void User::download_equity_ticks_L1(const std::string &ticker)
    {
        if(std::atoi(Time::now_ET().to_str("%Y").c_str()) >= 2038)
            log_warning("You might need to change the end time to include the recent date. "
                        "Note that IQFeed had a Y2038 bug, so make sure that it doesn't affect results.");

        if(get_status() != net::Client::Status::Connected) {
            log_error("User::download_equity_ticks_L1 called while disconnected (did IQFeed disconnect)?");
            return;
        }

        auto lock_guard = equity_ticks_L1_locks.get_lock_guard(ticker);

        string folder_path = get_equity_ticks_L1_folder_path(ticker);

        //check how much data we've already downloaded

        //IQFeed only gives the last 180 days of tick data and gives us an error response if we query
        //a time range that ends before 180 days ago, which is undesirable, so set start day to right
        //before the time range that IQFeed has data for. Note that CHUNK_SIZE needs to be large enough
        //that the first chunk is guaranteed to contain data
        Time start_day(Time::now_ET() - Time::Delta(185, Time::Length::day));
        start_day = Time(start_day.to_int64(Time::Length::day), Time::Length::day);

        //points to the beginning of today in ET
        Time end_day = Time(Time::now_ET().to_int64(Time::Length::day), Time::Length::day);

        for(Time t=end_day; t>=start_day; t-=Time::Delta(1, Time::Length::day)) {
            string file_name = folder_path + t.to_str(Time::Format::YYYY_mm_DD) +
                               ".csv.binary";
            std::ifstream check_file_existence(file_name, std::ios::binary);
            if(!check_file_existence.fail()) { //this file exists
                //note that we redownload the last day we already have in our db.
                //This is for safety purposes, as it's possible that the last day
                //in our db is only partially complete.
                start_day = t;
                break;
            }
        }

        if(start_day > end_day) {
            //io::println("already downloaded data for ticker " + ticker);
            return;
        }

        io::make_folder(folder_path);

        //download it in multiple chunks because downloading it all at once takes a ton of memory
        const Time::Delta CHUNK_SIZE(15, Time::Length::day);
        for(auto day1=start_day; day1<=end_day; day1+=CHUNK_SIZE) {
            auto day2 = std::min(day1 + CHUNK_SIZE - Time::Delta(1, Time::Length::day), end_day);
            string start_time_str = day1.to_str(Time::Format::YYYYmmDD) + " " +
                                    ((string)start_time_cstr).substr(9);
            string end_time_str = day2.to_str(Time::Format::YYYYmmDD) + " " +
                                  ((string)end_time_cstr).substr(9);

            string request = "HTT," +
                             format_ticker_to_iqfeed(ticker) + "," +
                             start_time_str + "," +
                             end_time_str + "," +
                             ",,,1,,\r\n";
            string response = send_request_and_get_response(request);

            if(response.size()==0) {
                log_warning("received response of length 0 for ticker \"" + ticker + "\"");
                return;
            }
            if(response[0]=='E') { //E=error (I think all errors will start with this)
                log_warning("received response \"" + response.substr(response.size()-1) +
                            "\" for ticker \"" + ticker + "\""); //use .substr because the last char is '\n'
                return;
            }
            string output_file_path = folder_path + day1.to_str(Time::Format::YYYYmmDD) +
                                      (string)"_" + day2.to_str(Time::Format::YYYYmmDD) +
                                      (string)"_raw.csv";
            std::ofstream output_file(output_file_path, std::ios::binary | std::ios::trunc);
            if(output_file.fail()) {
                log_error("failed to create output file \"" + output_file_path + "\"");
            }

            //workaround; split the data into smaller chunks because ofstream.write hangs for 2GB+ writes
            constexpr size_t BLOCK_SIZE = 1e9;
            for(size_t i=0; i<(response.size()+BLOCK_SIZE-1)/BLOCK_SIZE; i++) {
                output_file.write(response.data() + BLOCK_SIZE*i,
                                  std::min(BLOCK_SIZE, response.size() - BLOCK_SIZE*i));
            }
            output_file.close();

            auto lines = split_str(response, '\n');
            response.clear(); //frees memory

            while(!lines.empty() && lines.back().empty())
                lines.pop_back();
            auto data = parse_hist_equity_ticks_L1(lines);

            vector<size_t> data_by_day_idx;
            vector<string> day_strs;

            int prev_day = -1;
            size_t cur_idx = 0;
            for(const auto &datapoint: data)
            {
                int cur_day = datapoint.most_recent_time.to_int64(Time::Length::day);
                if(prev_day != cur_day) {
                    prev_day = cur_day;
                    day_strs.push_back(datapoint.most_recent_time.to_str(Time::Format::YYYY_mm_DD));
                    data_by_day_idx.push_back(cur_idx);
                }
                cur_idx++;
            }
            data_by_day_idx.push_back(cur_idx);
            k_ensures(data_by_day_idx.size() == day_strs.size()+1);

            //no OpenMP here, because I want to make this more robust to failures. If we do this
            //on a single thread, the file with the chronologically latest date is the only one
            //that can possibly be incomplete, and we end up rewriting that file anyway, so it's
            //fairly easy to ensure our database doesn't have incomplete days. If we use openmp,
            //then there might be files with earlier dates that are incomplete.
            [[maybe_unused]] volatile int enforce_loop_order;
            for(int j=0; j<(int)data_by_day_idx.size()-1; j++) {
                enforce_loop_order = j;
                string day_output_file_name = folder_path + day_strs[j] + ".csv.binary";
                std::ofstream day_output_file(day_output_file_name, std::ios::binary | std::ios::trunc);
                if(day_output_file.fail()) {
                    log_error("failed to create output file \"" + day_output_file_name + "\"");
                }
                size_t len = (data_by_day_idx[j+1] - data_by_day_idx[j]) * sizeof(EquityTickL1);
                day_output_file.write((char*)(data.data() + data_by_day_idx[j]), len);
            }

            //do I have to ensure the OS has written the file to disk? Although all the files are written
            //from the POV of my program, idk how the OS handles it (if I try to read the files now, will
            //I get the full contents?). I believe the OS will block the read request until I finish writing,
            //but I'm not sure.
        }
    }
    void User::download_equity_ticks_L1(const vector<string> &tickers)
    {
        if(get_status() != net::Client::Status::Connected) {
            log_error("User::download_equity_ticks_L1(vec) called while disconnected "
                      "(did IQFeed disconnect)?");
            return;
        }

        #pragma omp parallel for num_threads(HistDb::NUM_CONNECTIONS)
        for(size_t i=0; i<tickers.size(); i++) {
            download_equity_ticks_L1(tickers[i]);
        }
    }

    static SharedLock_Set<std::string> ohlcv_1min_locks;
    void User::download_OHLCV_1min(const string &ticker)
    {
        if(std::atoi(Time::now_ET().to_str("%Y").c_str()) >= 2038)
            log_warning("You might need to change the end time to include the recent date. "
                        "Note that IQFeed had a Y2038 bug, so make sure that it doesn't affect results.");

        if(get_status() != net::Client::Status::Connected) {
            log_error("User::download_OHLCV_1min called while disconnected (did IQFeed disconnect)?");
            return;
        }

        auto lock_guard = ohlcv_1min_locks.get_lock_guard(ticker);

        string folder_path = get_ohlcv_1min_folder_path(ticker);

        //check how much data we've already downloaded
        Time start_day(((string)start_time_cstr).substr(0, 8), Time::Format::YYYYmmDD);
        //points to the beginning of today in ET
        Time end_day = Time(Time::now_ET().to_int64(Time::Length::day), Time::Length::day);

        for(Time t=end_day; t>=start_day; t-=Time::Delta(1, Time::Length::day)) {
            string file_name = folder_path + t.to_str(Time::Format::YYYY_mm_DD) +
                               ".csv.binary";
            std::ifstream check_file_existence(file_name, std::ios::binary);
            if(!check_file_existence.fail()) { //this file exists
                //note that we redownload the last day we already have in our db.
                //This is for safety purposes, as it's possible that the last day
                //in our db is only partially complete.
                start_day = t;
                break;
            }
        }

        if(start_day > end_day) {
            //io::println("already downloaded data for ticker " + ticker);
            return;
        }

        io::make_folder(folder_path);

        string start_time_str = start_day.to_str(Time::Format::YYYYmmDD) + " " +
                                ((string)start_time_cstr).substr(9);
        string end_time_str = end_day.to_str(Time::Format::YYYYmmDD) + " " +
                              ((string)end_time_cstr).substr(9);

        string request = "HIT," +
                         format_ticker_to_iqfeed(ticker) + ","
                         "60," +
                         start_time_str + "," +
                         end_time_str + ","
                         ",,,1\r\n";
        string response = send_request_and_get_response(request);

        if(response.size()==0) {
            log_warning("received response of length 0 for ticker \"" + ticker + "\"");
            return;
        }
        if(response[0]=='E') { //E=error (I think all errors will start with this)
            log_warning("received response \"" + response.substr(response.size()-1) +
                        "\" for ticker \"" + ticker + "\""); //use .substr because the last char is '\n'
            return;
        }

        string output_file_path = folder_path + start_day.to_str(Time::Format::YYYYmmDD) +
                                  (string)"_" + end_day.to_str(Time::Format::YYYYmmDD) +
                                  (string)"_raw.csv";
        std::ofstream output_file(output_file_path, std::ios::binary | std::ios::trunc);
        if(output_file.fail()) {
            log_error("failed to create output file \"" + output_file_path + "\"");
        }
        output_file.write(response.data(), response.size());
        output_file.close();

        auto lines = split_str(response, '\n');
        while(!lines.empty() && lines.back().empty())
            lines.pop_back();
        auto data = parse_OHLCV_1min(lines);

        vector<size_t> data_by_day_idx;
        vector<string> day_strs;

        int prev_day = -1;
        size_t cur_idx = 0;
        for(const auto &datapoint: data)
        {
            int cur_day = datapoint.time.to_int64(Time::Length::day);
            if(prev_day != cur_day) {
                prev_day = cur_day;
                day_strs.push_back(datapoint.time.to_str(Time::Format::YYYY_mm_DD));
                data_by_day_idx.push_back(cur_idx);
            }
            cur_idx++;
        }
        data_by_day_idx.push_back(cur_idx);
        k_ensures(data_by_day_idx.size() == day_strs.size()+1);

        //no openmp here, because I want to make this more robust to failures. If we do this
        //on a single thread, the file with the chronologically latest date is the only one
        //that can possibly be incomplete, and we end up rewriting that file anyway, so it's
        //fairly easy to ensure our database doesn't have incomplete days. If we use openmp,
        //then there might be files with earlier dates that are incomplete.
        [[maybe_unused]] volatile int enforce_loop_order;
        for(int j=0; j<(int)data_by_day_idx.size()-1; j++) {
            enforce_loop_order = j;
            string day_output_file_name = folder_path + day_strs[j] + ".csv.binary";
            std::ofstream day_output_file(day_output_file_name, std::ios::binary | std::ios::trunc);
            if(day_output_file.fail()) {
                log_error("failed to create output file \"" + day_output_file_name + "\"");
            }
            size_t len = (data_by_day_idx[j+1] - data_by_day_idx[j]) * sizeof(OHLCV_1min_Bar);
            day_output_file.write((char*)(data.data() + data_by_day_idx[j]), len);
        }

        //do I have to ensure the OS has written the file to disk? Although all the files are written
        //from the POV of my program, idk how the OS handles it (if I try to read the files now, will
        //I get the full contents?). I believe the OS will block the read request until I finish writing,
        //but I'm not sure.
    }
    void User::download_OHLCV_1min(const vector<string> &tickers)
    {
        if(get_status() != net::Client::Status::Connected) {
            log_error("User::download_OHLCV_1min(vec) called while disconnected (did IQFeed disconnect)?");
            return;
        }

        #pragma omp parallel for num_threads(HistDb::NUM_CONNECTIONS)
        for(size_t i=0; i<tickers.size(); i++) {
            download_OHLCV_1min(tickers[i]);
        }
    }

    std::unique_ptr<User> request_user()
    {
        std::unique_lock<std::mutex> lock(connect_mutex);

        if(get_status() != net::Client::Status::Connected)
            return nullptr;
        else {
            auto abstract_user = user_set.get_user(lock);
            if(abstract_user != nullptr) {
                return std::unique_ptr<User>(new User(std::move(abstract_user)));
            } else
                return nullptr;
        }
    }

    static void filter_by_time_of_day(vector<vector<EquityTickL1>> *data,
                                      int64_t day_start_ns, int64_t day_end_ns)
    {
        k_expects(day_start_ns <= day_end_ns);
        k_expects(day_start_ns >= 0);
        k_expects(day_end_ns <= (int64_t)24*60*60*1e9);

        auto is_out_of_range = [=](const EquityTickL1 &datapoint)
        {
            auto ns_of_day = datapoint.most_recent_time.to_int64(Time::Length::ns) % ((int64_t)(24*60*60*1e9));
            return ns_of_day < day_start_ns || ns_of_day > day_end_ns;
        };

        for(size_t i=0; i < data->size(); i++)
            erase_remove_if(&(*data)[i], is_out_of_range);
    }
    static vector<vector<EquityTickL1>> read_equity_ticks_L1_binary(const LoadDataInfo &info)
    {
        vector<vector<EquityTickL1>> data_by_day(info.dates.size());

        //I think this is worth parallelizing because file IO usually causes
        //lots of stalling due to disk usage that isn't CPU-bound
        #pragma omp parallel for
        for(size_t i=0; i<data_by_day.size(); i++) {
            string input_file_path = get_equity_ticks_L1_folder_path(info.ticker) +
                                     info.dates[i].to_str(Time::Format::YYYY_mm_DD) +
                                     ".csv.binary";
            std::ifstream input_file(input_file_path, std::ios::binary);

            if(!input_file.fail()) {
                input_file.seekg(0, std::ios::end);
                auto file_len = input_file.tellg();
                if(file_len == 0) {
                    log_warning("empty equity ticks L1 1 min file: \"" + input_file_path + "\"");
                    continue;
                }
                input_file.seekg(0, std::ios::beg);
                k_expects(file_len % sizeof(EquityTickL1) == 0);
                auto datapoint_count = file_len / sizeof(EquityTickL1);
                data_by_day[i].resize(datapoint_count);
                input_file.read((char*)data_by_day[i].data(), file_len);
            }
        }

        return data_by_day;
    }
    static vector<vector<EquityTickL1>> adjust_for_splits(vector<vector<EquityTickL1>> data,
                                                          const vector<yahoo::Split> &splits)
    {
        if(data.size() == 0)
            return data;

        if(splits.size() > 0) { //need this because we do size_t-1
            for(auto &split: splits) { //the split info shouldn't contain finer data than days
                [[maybe_unused]] int64_t ns = split.date.to_int64(Time::Length::nanosecond);
                k_expects(ns % (24*60*60*(int64_t)1e9) == 0);
            }
            int split_idx = splits.size()-1;
            double adj_factor = 1;
            double inv_adj_factor = 1;

            for(int day=(int)data.size()-1; day>=0; day--) {
                for(int minute=(int)data[day].size()-1; minute>=0; minute--) {
                    while(split_idx>=0 && data[day][minute].most_recent_time<splits[split_idx].date) {
                        double split_ratio = splits[split_idx].ratio;
                        adj_factor /= split_ratio;
                        inv_adj_factor *= split_ratio;
                        split_idx--;
                    }
                    data[day][minute].most_recent *= adj_factor;
                    data[day][minute].bid *= adj_factor;
                    data[day][minute].ask *= adj_factor;

                    data[day][minute].most_recent_size *= adj_factor;
                    data[day][minute].bid_size *= adj_factor;
                    data[day][minute].ask_size *= adj_factor;

                    //we store cum_vol as a double, so this doesn't result in truncation
                    data[day][minute].cum_vol *= inv_adj_factor;
                }
            }
        }

        return data;
    }
    //I added "ticks" to the name to prevent a name collision with the templated function
    //that has the same name but we use to adjust OHLCV data. Although this name collision doesn't cause a
    //compiler error, there's some chance the compiler might silently call the wrong function if
    //we had the same names.
    static vector<vector<EquityTickL1>> adjust_ticks_for_dividends(vector<vector<EquityTickL1>> data,
                                                                   const vector<yahoo::Split> &splits,
                                                                   const string &ticker)
    {
        if(data.size() == 0)
            return data;

        std::set<Time> split_dates;
        for(const auto &split: splits)
            split_dates.insert(split.date);

        auto dividends_opt = yahoo::load_dividends(ticker);
        decltype(dividends_opt)::value_type dividends;
        if(!dividends_opt.has_value())
            log_warning("failed to load Yahoo dividends for ticker \"" + ticker + "\"");
        else
            dividends = std::move(*dividends_opt);

        if(dividends.size() > 0) { //need this because we do size_t-1
            for(auto &dividend: dividends) { //the dividend info shouldn't contain finer data than days
                [[maybe_unused]] int64_t ns = dividend.ex_date.to_int64(Time::Length::nanosecond);
                k_expects(ns % (24*60*60*(int64_t)1e9) == 0);
            }

            int dividend_idx = dividends.size()-1;
            double adj_factor = 1;
            int last_day_with_trades = -1;
            for(int day=(int)data.size()-1; day>=0; day--) {
                if(!data[day].empty()) {
                    last_day_with_trades = day;
                    break;
                }
            }

            if(last_day_with_trades == -1) { //there are no data points, which could happen
                log_warning("adjust_for_dividends (ticks) found no data points for ticker \"" + ticker + "\"");
                return data; //don't return {} because data still takes up memory, it just has no info.
            }

            //Note: the names "day" and "minute" are kind of deceptive, because we also use this
            //function to adjust 1 day OHLCV bars

            //Note: we don't adjust for all dividends (it would take considerably more effort
            //to do so quickly), so we ignore ones past the end of our time range.

            for(int day=last_day_with_trades; day>=0; day--) {
                for(int minute=(int)data[day].size()-1; minute>=0; minute--) {
                    while(dividend_idx>=0 && data[day][minute].most_recent_time<dividends[dividend_idx].ex_date) {
                        bool past_end_range = (day==last_day_with_trades && minute==(int)data[day].size()-1);
                        bool coincides_with_split = (split_dates.count(dividends[dividend_idx].ex_date));
                        if(!past_end_range && !coincides_with_split) {
                            //I'm using the last closing price in the time range, which adds noise
                            //because it depends on the user-specified start and end times
                            double price_ratio = (data[day][minute].most_recent - dividends[dividend_idx].amount) /
                                                 data[day][minute].most_recent;
                            //do a sanity check (note that yahoo sometimes has dividends of 0, so we use <=1)
                            k_expects(price_ratio>0 && price_ratio<=1, "ticker =\"" + ticker + "\"");
                            adj_factor *= price_ratio;
                        }
                        dividend_idx--;
                    }
                    data[day][minute].most_recent *= adj_factor;
                    data[day][minute].bid *= adj_factor;
                    data[day][minute].ask *= adj_factor;
                    //I'm pretty sure we don't adjust volume for dividends
                }
            }
        }

        return data;
    }
    vector<vector<EquityTickL1>> load_equity_ticks_L1(const LoadDataInfo &info)
    {
        k_expects(info.adjust==0 || info.adjust==LoadDataInfo::ALL_ADJUST_BITS);
        k_expects(std::is_sorted(info.dates.begin(), info.dates.end()));

        if(!equity_ticks_L1_folder_exists(info.ticker)) {
            log_error("ticks for ticker \"" + info.ticker + "\" have not been downloaded yet");
            //don't return here because that could cause a crash if the user expects
            //a non-empty data list. We'll just return a bogus data set instead, and
            //the user should be aware it's bogus because of the log_error.
        }

        vector<vector<EquityTickL1>> data;
        {
            auto lg = equity_ticks_L1_locks.get_lock_shared_guard(info.ticker);
            data = read_equity_ticks_L1_binary(info);
        }

        //some checks
        auto day_start_ns = info.day_start_time.to_int64(Time::Length::ns);
        auto day_end_ns = info.day_end_time.to_int64(Time::Length::ns);
        k_ensures(day_start_ns <= day_end_ns);
        k_ensures(day_start_ns >= 0);
        k_ensures(day_end_ns <= 24*60*60*1e9);

        filter_by_time_of_day(&data, day_start_ns, day_end_ns);

        if(info.adjust) {
            //adjust for dividends first because Yahoo's dividends are split adjusted
            auto splits_opt = yahoo::load_splits(info.ticker);
            decltype(splits_opt)::value_type splits;
            if(!splits_opt.has_value())
                log_warning("failed to load Yahoo splits for ticker \"" + info.ticker + "\"");
            else
                splits = std::move(*splits_opt);

            data = adjust_for_splits(std::move(data), splits);
            data = adjust_ticks_for_dividends(std::move(data), splits, info.ticker);
        }
        return data;
    }
    static vector<vector<OHLCV_1min_Bar>> read_OHLCV_1min_binary(const LoadDataInfo &info)
    {
        vector<vector<OHLCV_1min_Bar>> data_by_day(info.dates.size());

        //I think this is worth parallelizing because file IO usually causes
        //lots of stalling due to disk usage that isn't CPU-bound
        #pragma omp parallel for
        for(size_t i=0; i<data_by_day.size(); i++) {
            string input_file_path = get_ohlcv_1min_folder_path(info.ticker) +
                                     info.dates[i].to_str(Time::Format::YYYY_mm_DD) +
                                     ".csv.binary";
            std::ifstream input_file(input_file_path, std::ios::binary);

            if(!input_file.fail()) {
                input_file.seekg(0, std::ios::end);
                auto file_len = input_file.tellg();
                if(file_len == 0) {
                    log_warning("empty OHLCV 1 min file: \"" + input_file_path + "\"");
                    continue;
                }
                input_file.seekg(0, std::ios::beg);
                k_expects(file_len % sizeof(OHLCV_1min_Bar) == 0);
                auto datapoint_count = file_len / sizeof(OHLCV_1min_Bar);
                data_by_day[i].resize(datapoint_count);
                input_file.read((char*)data_by_day[i].data(), file_len);
            }
        }

        return data_by_day;
    }
    static void filter_by_time_of_day(vector<vector<OHLCV_1min_Bar>> *data,
                                      int day_start_min, int day_end_min)
    {
        auto is_out_of_range = [=](const OHLCV_1min_Bar &datapoint)
        {
            auto minute_of_day = datapoint.time.to_int64(Time::Length::minute) % (24*60);
            return minute_of_day < day_start_min || minute_of_day > day_end_min;
        };

        for(size_t i=0; i < data->size(); i++)
            erase_remove_if(&((*data)[i]), is_out_of_range);
    }
    static OHLCV_1min_Bar fill_with_previous(const fin::OHLCV_1min_Bar &previous)
    {
        fin::OHLCV_1min_Bar ret;
        //don't fill in the time field. That has to be filled in manually.
        ret.open = previous.close;
        ret.high = previous.close;
        ret.low = previous.close;
        ret.close = previous.close;
        ret.volume = 0;
        ret.cum_vol = previous.cum_vol;
        return ret;
    }
    static vector<vector<OHLCV_1min_Bar>> plug_holes(vector<vector<OHLCV_1min_Bar>> data,
                                                     const string &ticker,
                                                     const vector<Time> &dates,
                                                     LoadDataInfo::PlugHoleStyle style,
                                                     Time::Delta day_start_minute,
                                                     Time::Delta day_end_minute)
    {
        switch(style) {
        case LoadDataInfo::PlugHoleStyle::Dont:
            return data;
        case LoadDataInfo::PlugHoleStyle::CopyPrevious: {
            if(data.size() == 0) //we assume data.size()>0 in the following code
                return {};

            int minutes_per_day = (day_end_minute - day_start_minute).to_int64(Time::Length::minute) + 1;

            vector<vector<OHLCV_1min_Bar>> res(data.size());
            for(auto &res_day: res)
                res_day.resize(minutes_per_day);

            //identify the first day and minute with data, and backfill everything before
            //then with that datapoint's price information (this permits some lookahead, which
            //is kind of sketchy, but it's probably the best solution).

            int first_day_with_data = -1;
            int first_day_start_idx;
            for(size_t day=0; day<data.size(); day++) {
                //check if today has any bars
                if(data[day].size() > 0) {
                    first_day_with_data = day;

                    auto first_datapoint = fill_with_previous(data[day][0]);
                    first_datapoint.cum_vol = 0;

                    //backfill all previous days' bars with the first bar from today
                    for(size_t prev_day=0; prev_day<day; prev_day++) {
                        Time start_time = dates[prev_day] + day_start_minute;
                        for(int minute=0; minute<minutes_per_day; minute++) {
                            res[prev_day][minute] = first_datapoint;
                            res[prev_day][minute].time = start_time + Time::Delta(minute, Time::Length::minute);
                        }
                    }

                    first_day_start_idx = (data[day][0].time - day_start_minute).
                                           to_int64(Time::Length::minute)%(24*60);
                    if(first_day_start_idx >= minutes_per_day) {
                        io::println(ticker + " " + to_str(data[day][0].time.to_str(Time::Format::YYYY_mm_DD_HH_MM_SS)));
                    }
                    first_day_start_idx = std::clamp(first_day_start_idx, 0, minutes_per_day);

                    Time start_time = dates[day] + day_start_minute;
                    for(int minute=0; minute<(int)first_day_start_idx; minute++) {
                        res[day][minute] = first_datapoint;
                        res[day][minute].time = start_time + Time::Delta(minute, Time::Length::minute);
                    }
                    break;
                }
            }

            if(first_day_with_data == -1) {
                log_warning("no OHLCV 1min bars found in time range for ticker \"" + ticker + "\"; "
                            "Returning all 100s for prices and 0s for volumes");
                //fill with 100s for px because it's a nice number
                //(if we fill with 0s we could get a divide by 0).
                for(size_t day=0; day<res.size(); day++) {
                    for(int i=0; i<minutes_per_day; i++) {
                        res[day][i].time = dates[day] + day_start_minute +
                                                Time::Delta(i, Time::Length::minute);
                        res[day][i].open = 100;
                        res[day][i].high = 100;
                        res[day][i].low = 100;
                        res[day][i].close = 100;
                        res[day][i].volume = 0;
                        res[day][i].cum_vol = 0;
                    }
                }
                return res;
            }

            //can't use omp because we use res[day-1].back()
            for(int day=first_day_with_data; day<(int)res.size(); day++) {
                int minute;
                Time time_minute = dates[day];
                if(day == first_day_with_data) {
                    minute = first_day_start_idx;
                    time_minute += day_start_minute + Time::Delta((int64_t)(first_day_start_idx),
                                                                  Time::Length::minute);
                } else {
                    minute = 0;
                    time_minute += day_start_minute;
                }

                int data_idx = 0;
                for(; minute<minutes_per_day; minute++) {
                    while(data_idx < (int)data[day].size() && data[day][data_idx].time < time_minute)
                        data_idx++;
                    if(data_idx < (int)data[day].size() && data[day][data_idx].time==time_minute) {
                        res[day][minute] = data[day][data_idx];
                    } else {
                        if(data_idx == 0) {
                            k_expects(day != 0);
                            res[day][minute] = fill_with_previous(res[day-1].back());
                            res[day][minute].cum_vol = 0;
                        } else
                            res[day][minute] = fill_with_previous(data[day][data_idx-1]);
                        res[day][minute].time = dates[day] + day_start_minute +
                                                Time::Delta(minute, Time::Length::minute);
                    }
                    time_minute += Time::Delta(1, Time::Length::minute);
                }
            }
            return res;}
        default:
            log_error("unsupported plug hole style for OHLCV 1 min");
            return {};
        }
    }
    static vector<vector<OHLCV_1min_Bar>> adjust_for_splits(vector<vector<OHLCV_1min_Bar>> data,
                                                                  const vector<yahoo::Split> &splits)
    {
        if(data.size() == 0)
            return data;

        if(splits.size() > 0) { //need this because we do size_t-1
            for(auto &split: splits) { //the split info shouldn't contain finer data than days
                [[maybe_unused]] int64_t ns = split.date.to_int64(Time::Length::nanosecond);
                k_expects(ns % (24*60*60*(int64_t)1e9) == 0);
            }
            int split_idx = splits.size()-1;
            double adj_factor = 1;
            double inv_adj_factor = 1;

            for(int day=(int)data.size()-1; day>=0; day--) {
                for(int minute=(int)data[day].size()-1; minute>=0; minute--) {
                    while(split_idx>=0 && data[day][minute].time<splits[split_idx].date) {
                        double split_ratio = splits[split_idx].ratio;
                        adj_factor /= split_ratio;
                        inv_adj_factor *= split_ratio;
                        split_idx--;
                    }
                    data[day][minute].open *= adj_factor;
                    data[day][minute].high *= adj_factor;
                    data[day][minute].low *= adj_factor;
                    data[day][minute].close *= adj_factor;
                    //we store volume and cum_vol as doubles, so this doesn't result
                    //in truncation
                    data[day][minute].volume *= inv_adj_factor;
                    data[day][minute].cum_vol *= inv_adj_factor;
                }
            }
        }

        return data;
    }
    template<class Container>
    static Container adjust_for_dividends(Container data,
                                          const vector<yahoo::Split> &splits,
                                          const string &ticker)
    {
        if(data.size() == 0)
            return data;

        std::set<Time> split_dates;
        for(const auto &split: splits)
            split_dates.insert(split.date);

        auto dividends_opt = yahoo::load_dividends(ticker);
        decltype(dividends_opt)::value_type dividends;
        if(!dividends_opt.has_value())
            log_warning("failed to load Yahoo dividends for ticker \"" + ticker + "\"");
        else
            dividends = std::move(*dividends_opt);

        if(dividends.size() > 0) { //need this because we do size_t-1
            for(auto &dividend: dividends) { //the dividend info shouldn't contain finer data than days
                [[maybe_unused]] int64_t ns = dividend.ex_date.to_int64(Time::Length::nanosecond);
                k_expects(ns % (24*60*60*(int64_t)1e9) == 0);
            }

            int dividend_idx = dividends.size()-1;
            double adj_factor = 1;
            int last_day_with_trades = -1;
            for(int day=(int)data.size()-1; day>=0; day--) {
                if(!data[day].empty()) {
                    last_day_with_trades = day;
                    break;
                }
            }

            if(last_day_with_trades == -1) { //there are no data points, which could happen
                log_warning("adjust_for_dividends (OHLCV) found no data points for ticker \"" + ticker + "\"");
                return data; //don't return {} because data still takes up memory, it just has no info.
            }

            //Note: the names "day" and "minute" are kind of deceptive, because we also use this
            //function to adjust 1 day OHLCV bars

            //Note: we don't adjust for all dividends (it would take considerably more effort
            //to do so quickly), so we ignore ones past the end of our time range.

            for(int day=last_day_with_trades; day>=0; day--) {
                for(int minute=(int)data[day].size()-1; minute>=0; minute--) {
                    while(dividend_idx>=0 && data[day][minute].time<dividends[dividend_idx].ex_date) {
                        bool past_end_range = (day==last_day_with_trades && minute==(int)data[day].size()-1);
                        bool coincides_with_split = (split_dates.count(dividends[dividend_idx].ex_date));
                        if(!past_end_range && !coincides_with_split) {
                            //I'm using the last closing price in the time range, which adds noise
                            //because it depends on the user-specified start and end times
                            double price_ratio = (data[day][minute].close - dividends[dividend_idx].amount) /
                                                 data[day][minute].close;
                            //do a sanity check (note that yahoo sometimes has dividends of 0, so we use <=1)
                            k_expects(price_ratio>0 && price_ratio<=1, "ticker =\"" + ticker + "\"");
                            adj_factor *= price_ratio;
                        }
                        dividend_idx--;
                    }
                    data[day][minute].open *= adj_factor;
                    data[day][minute].high *= adj_factor;
                    data[day][minute].low *= adj_factor;
                    data[day][minute].close *= adj_factor;
                    //I'm pretty sure we don't adjust volume for dividends
                }
            }
        }

        return data;
    }
    vector<vector<OHLCV_1min_Bar>> load_OHLCV_1min(const LoadDataInfo &info)
    {
        k_expects(info.adjust==0 || info.adjust==LoadDataInfo::ALL_ADJUST_BITS);
        //similarly, we only care about minutes for time cutoff points (finer values
        //like nanoseconds should be set to 0)
        k_expects(info.day_start_time.to_int64(Time::Length::nanosecond) % (60*(int64_t)1e9) == 0);
        k_expects(info.day_end_time.to_int64(Time::Length::nanosecond) % (60*(int64_t)1e9) == 0);

        k_expects(std::is_sorted(info.dates.begin(), info.dates.end()));

        if(!OHLCV_1min_folder_exists(info.ticker)) {
            log_error("OHLCV 1 min bars for ticker \"" + info.ticker + "\" have not been downloaded yet");
            //don't return here because that could cause a crash if the user expects
            //a non-empty data vector. We'll just return a bogus data set instead, and
            //the user should be aware it's bogus if they read the log and see the error.
        }

        vector<vector<OHLCV_1min_Bar>> data;

        {
            auto lg = ohlcv_1min_locks.get_lock_shared_guard(info.ticker);
            data = read_OHLCV_1min_binary(info);
        }

        //some checks
        auto day_start_ns = info.day_start_time.to_int64(Time::Length::ns);
        auto day_end_ns = info.day_end_time.to_int64(Time::Length::ns);
        k_expects(day_start_ns <= day_end_ns);
        k_expects(day_start_ns >= 0);
        k_expects(day_end_ns <= 1e9*60*(24*60-1));
        k_expects(day_start_ns % ((int64_t)1e9*60) == 0);
        k_expects(day_end_ns % ((int64_t)1e9*60) == 0);

        filter_by_time_of_day(&data, day_start_ns/(1e9*60), day_end_ns/(1e9*60));

        if(info.adjust) {
            //adjust for dividends first because Yahoo's dividends are split adjusted
            auto splits_opt = yahoo::load_splits(info.ticker);
            decltype(splits_opt)::value_type splits;

            if(!splits_opt.has_value())
                log_warning("failed to load Yahoo splits for ticker \"" + info.ticker + "\"");
            else
                splits = std::move(*splits_opt);

            data = adjust_for_splits(std::move(data), splits);
            data = adjust_for_dividends(std::move(data), splits, info.ticker);
        }

        //call plug_holes after adjusting because if, for example, today's an ex-div date, and
        //we fill some early morning values with yesterday's data, we could use unadjusted values.
        //TODO: are there downsides to plugging holes after adjusting? I think as long as every
        //date with a corporate action has at least one OHLCV datapoint, this should work.
        data = plug_holes(std::move(data),
                          info.ticker,
                          info.dates,
                          info.plug_hole_style,
                          info.day_start_time,
                          info.day_end_time);
        return data;
    }
}

}}}
