#pragma once

#include "kx/fin/data_types.h"
#include "kx/time.h"
#include "kx/net/client.h"

#include <vector>
#include <string>
#include <cstdint>
#include <deque>
#include <map>

namespace kx { namespace fin { namespace IQFeed {

/** Not all of the below functions that are marked thread-safe are safe if multiple copies
 *  of this program are running, i.e. we do interthread syncing but not interprocess
 *  syncing.
 */

struct EquityTickL1
{
    using flag_t = uint32_t;
    using market_t = uint8_t;
    using aggressor_t = uint8_t;
    using trade_cond_t = uint8_t;

    static constexpr flag_t SHORT_SALE_RESTRICTED_BIT = (1<<0);

    static constexpr flag_t LAST_QUALIFIED_BIT = (1<<1);
    static constexpr flag_t EXTENDED_TRADE_BIT = (1<<2);
    static constexpr flag_t OTHER_TRADE_BIT = (1<<3);

    static constexpr flag_t BID_UPDATE_BIT = (1<<4);
    static constexpr flag_t ASK_UPDATE_BIT = (1<<5);

    static constexpr flag_t SUMMARY_BIT = (1<<6);

    static constexpr flag_t TRADE_BITS = LAST_QUALIFIED_BIT | EXTENDED_TRADE_BIT | OTHER_TRADE_BIT;

    /** note that "last" here refers to the most recent trade. It does NOT refer to the
     *  most recent qualifying trade (i.e. non-qualifying trades can be "last" here, even
     *  though they can't in most data feeds. An example of a trade that does not qualify
     *  for last is an odd lot).
     */
    Time bid_time;
    Time ask_time;
    Time most_recent_time;

    double bid;
    double ask;
    double most_recent;

    double bid_size;
    double ask_size;
    double most_recent_size;

    double cum_vol;

    market_t bid_market;
    market_t ask_market;
    market_t most_recent_market;

    aggressor_t most_recent_aggressor;

    trade_cond_t most_recent_conditions[4];

    flag_t flags;

    uint8_t reserved[8 - sizeof(flag_t)]; ///explicitly reserve padding bytes for clarity

    bool is_out_of_sequence() const;
    bool operator < (const EquityTickL1 &other) const;
};
struct EquityTickL1_wTicker: public EquityTickL1
{
    std::string ticker;
};

/** We keep both the ticker and date for FundamentalData because IQFeed groups fundamental
 *  data by date rather than ticker, which makes it difficult to optimize out storing the
 *  ticker.
 *
 *  Note that fundamental data values at a time are likely values as they were last reported,
 *  i.e. if the date is 2020-03-15, but the EPS was last reported on 2020-02-20, then the
 *  EPS field would probably contain the EPS for 2020-02-20 (I believe most of these fields,
 *  such as EPS, are usually reported on a quarterly basis rather than every day).
 */
struct EquityFundamentalData
{
    static constexpr size_t MAX_TICKER_SIZE = 7;

    //if we have a ticker of length 7, we'll set ticker[7] = '\0', so we need to array to
    //be 1 longer than the maximum supported length
    char ticker[MAX_TICKER_SIZE+1];
    Time date;

    double EPS;
    double institutional_ownership;
    double beta;
    double assets;
    double liabilities;
    double long_term_debt;
    double common_shares_outstanding;
    double market_cap;
};

/** All three parse functions are thread-safe. However, they all use OpenMP internally, so
 *  there will be little speedup from using multiple threads.
 */
std::vector<EquityTickL1> parse_hist_equity_ticks_L1(const std::vector<std::string> &lines);
std::vector<OHLCV_1min_Bar> parse_OHLCV_1min(const std::vector<std::string> &lines);
std::vector<OHLCV_1day_Bar> parse_OHLCV_1day(const std::vector<std::string> &lines);

/** This loads lists containing mappings of market centers and trade conditions to strings.
 *  It's not thread-safe. Call this at the beginning of your program, before you spawn any
 *  additional threads.
 */
void load_ID_maps(); ///not thread-safe

/** The following four functions are thread-safe as long as load_ID_maps() was called
 *  before any additional threads were created.
 */
std::string get_exch_short_name(EquityTickL1::market_t ID); ///conditionally thread-safe
std::string get_exch_long_name(EquityTickL1::market_t ID); ///conditionally thread-safe

std::string get_trade_cond_short_name(EquityTickL1::trade_cond_t ID); ///conditionally thread-safe
std::string get_trade_cond_long_name(EquityTickL1::trade_cond_t ID); ///conditionally thread-safe

bool is_trade_cond_out_of_sequence(EquityTickL1::trade_cond_t ID); ///thread-safe

/**---------------------------------------------------------------------------------------
 *  You probably don't want to directly interact with any code below here. You should use
 *  an intermediate class that provides an abstraction and internally uses the API below.
 */

/** A User has full permission to use HistDb/Stream. There are some functions that you can
 *  call without a User, but anything involving network data transfer requires a User for
 *  safety purposes. A User itself contains very little state; it's just a safety check.
 *
 *  At most one client can be interacting with the stream at any given time because Stream
 *  is built that way. Stream::request_user() returns nullptr if the Stream is not
 *  connected or there is already another Stream::User (IQFeed's stream itself might allow
 *  multiple clients to connect, but this API does not in order to maintain simplicity).
 *
 *  Stream::disconnect() will block until there are no Users remaining.
 *
 *  HistDb::User is similar, except you are allowed to have multiple of them.
 *
 *  AbstractUser is a class that lets some code be reused between Stream::User and
 *  HistDb::User, since those two classes are quite similar. It's not actually inherited
 *  by User and is instead used as a member field because that resulted in cleaner code.
 *
 *  OTC Market quotes might not work properly right now because it seems that IQFeed may
 *  or may not want a .PK appended to the ticker depending on what type of data you're
 *  requesting. This seems to be a pretty bad design choice on IQFeed's part.
 *
 *  TODO: Right now, User works fine UNLESS we get disconnected from the IQFeed client, in
 *  which case previously valid Users are now invalid. How do we deal with this?
 *
 *  get_status() returns whether Stream/HistDb is connected to the socket. It'll return
 *  "Connected" if this program is connected to IQFeed's socket, but the IQFeed application
 *  is disconnected from the server, which is undesirable and should be changed.
 */

class AbstractUser;

namespace Stream
{
    void connect(); ///thread-safe
    void disconnect(); ///thread-safe
    net::Client::Status get_status(); ///thread-safe

    /** None of the Stream::User's public functions are thread-safe because there can only
     *  be one valid Stream::User anyway. The coder is responsible for making sure that
     *  this User is used in a thread-safe way (i.e. if there are two threads, each with
     *  access to the User, and they both call one of the below functions, then that would
     *  result in undefined behavior).
     */
    class User final
    {
        friend std::unique_ptr<User> request_user();

        std::unique_ptr<AbstractUser> abstract_user;

        User(std::unique_ptr<AbstractUser> abstract_user_);
    public:
        ///the dtor is probably empty but we need to declare it because we use PImpl
        ~User();

        User(const User&) = delete;
        User &operator = (const User&) = delete;

        ///It's easier to reason about things if User isn't movable
        User(User&&) = delete;
        User &operator = (User&&) = delete;

        void start_watching_ticker(const std::string &ticker); ///not thread-safe
        void stop_watching_ticker(const std::string &ticker); ///not thread-safe
        void update(); ///not thread-safe
        std::deque<EquityTickL1_wTicker> get_equity_ticks_L1(); ///not thread-safe
        Time::Delta get_T_message_ping(); ///not thread-safe
    };
    std::unique_ptr<User> request_user(); ///thread-safe. returns nullptr on failure
}
namespace HistDb
{
    /** 15 connections used to be the max amount allowed by IQFeed.
     *  However, now there is instead a rate limit of 50 requests per second.
     *  Sending a request that exceeds the limit results in an error message.
     *  The current value is arbitrary because there is no longer a connection
     *  limit.
     *
     *  The code has been updated to work with the new rate limit, but NUM_CONNECTIONS
     *  is going to be here until the code is refactored.
     */
    constexpr size_t NUM_CONNECTIONS = 12;

    /** connect() seems to bug out (usually an infinite loop) if only some connections
     *  succeed. I believe it's only possible for some connections to succeed if there's
     *  a bug in IQFeed/SDL_net/the compiler, so I guess it's not surprising that my
     *  program also bugs out (I don't know how much I can do about it). I
     */
    void connect(); ///thread-safe
    void disconnect(); ///thread-safe
    net::Client::Status get_status(); ///thread-safe

    /** More fields may be added to LoadDataInfo in the future, so don't assume it'll
     *  be a certain size in bytes (this only really matters for certain low-level
     *  stuff like writing binary data to a file).
     */
    struct LoadDataInfo
    {
        /** Note: if PlugHoleStyle==CopyPrevious and if there are insufficient
         *  data points at the beginning of the first day (e.g. you want 9:30-16:00
         *  but the first data point is at 9:40), it will take the NEXT data point
         *  (in this case 9:40) and backfill the previous ones. This allows some
         *  lookahead and is obviously not ideal, but it's probably the best solution.
         *
         *  CopyPrevious also only uses the data in the time and date range to fill holes,
         *  which is probably suboptimal. i.e. if your time range is 9:30-16:00 and
         *  there are no trades from 9:30-9:35 but there was a trade at 9:29, it'll
         *  use the data from the end of the previous day to fill 9:30-9:35 instead of
         *  using 9:29.

         *  Similarly, if the first trade on the first day in your range occurred at 9:33,
         *  and the last trade before that occurred at 15:50 the previous day, since the
         *  previous date's data isn't loaded, it wouldn't use the 15:50 trade to fill the
         *  data points from 9:30-9:32. Rather, it would use the future 9:33 data point to
         *  backfill the 9:30-9:32 data points (this lookahead is bad, but probably only
         *  slightly bad, because it generally only occurs in the first couple time points
         *  in the first day if the stock happens to be illiquid then).
         *
         *  day_start_time and day_end_time should be whole multiples of 1 minute for
         *  loading OHLCV 1 min data. Data in [day_start_time, day_end_time] will be
         *  returned. For loading tick data, the start and end time can be arbitrarily fine.
         *
         *  day_start_time and day_end_time are not used for OHLCV_1day (their values will
         *  be ignored).
         *
         *  plug_hole_style is ignored for loading ticks.
         */

        /** Note: split adjustments are applied completely. However, dividend
         *  adjustments are only applied for dates in the range. This means that all
         *  prices in a range will be correct relative to each other, but not
         *  necessarily correct relatively to other times, i.e. if a company pays
         *  a $1 dividend in 2018 but you query data from 2016, the 2016 data won't
         *  be adjusted for the $1 dividend in 2018.
         */

        /** Due to the aforementioned limitations with hole plugging and dividend
         *  adjustments, if you want adjusted and hole plugged data in a range [a, c], you
         *  HAVE to send ONE request to load it. You can't send a request for [a, b) and
         *  another for [b, c], then concatenate the data from those requests, because the
         *  hole plugging, and more importantly, dividend/split adjustments, might cause the
         *  concatenated data to look different than if you loaded it all at once.
         */

        /** Currently, adjustments must be done on all all or nothing basis. You either have
         *  to request unadjusted data or fully adjusted data. In the future, support may be
         *  added for requesting only some adjustments.
         */

        enum class PlugHoleStyle {Dont, CopyPrevious};

        using adjust_flag_t = uint32_t;

        static constexpr adjust_flag_t DIV_ADJUST_BIT = 1<<0;
        static constexpr adjust_flag_t SPLIT_ADJUST_BIT = 1<<1;
        static constexpr adjust_flag_t ALL_ADJUST_BITS = DIV_ADJUST_BIT | SPLIT_ADJUST_BIT;

        std::string ticker;
        std::vector<Time> dates; ///must be sorted
        Time::Delta day_start_time;
        Time::Delta day_end_time;
        PlugHoleStyle plug_hole_style;
        adjust_flag_t adjust;

    };

    ///old function that probably won't be used again
    [[deprecated]]
    void convert_raw_OHLCV_1min_to_binary(const std::vector<std::string> &tickers);

    /** Downloading L1 ticks has memory issues (it receives the whole response before writing,
     *  which is an issue if 12 threads are run in parallel download 10GB of ticks each)
     */
    [[deprecated]]
    void download_equity_ticks_L1(const std::vector<std::string> &tickers);

    class User final
    {
        friend std::unique_ptr<User> request_user();

        std::unique_ptr<AbstractUser> abstract_user;

        User(std::unique_ptr<AbstractUser> abstract_user_);
    public:
        ///the dtor is probably empty but we need to declare it because we use PImpl
        ~User();

        User(const User&) = delete;
        User &operator = (const User&) = delete;

        ///It's easier to reason about things if User isn't movable
        User(User&&) = delete;
        User &operator = (User&&) = delete;

        std::string send_request_and_get_response(const std::string &request); ///thread-safe

        /** Downloading is fairly robust to failures. OHLCV 1 minute data and tick data are stored
         *  as one file per day. The chronologically last file for OHLCV 1min and ticks may not be
         *  complete, but it will be redownloaded, so that's OK. Raw, human-readable data from
         *  IQFeed is in a ".raw.csv" file. The raw data file name has the start date and end date
         *  on it. These are the dates that we used for the IQFeed query that generated the file.
         *
         *  Note that it might not contain the start date or the end date. For example, if
         *  we download stuff at 1am EST on day X, no data for day X exists yet, so it's possible
         *  that the start date and end date could be [X-1, X]. It could also be [X-3, X] or
         *  [X-2, X] if X is a Sunday or Monday.
         *  The start date is just our lower date bound that we send to IQFeed, so if IQFeed
         *  doesn't have data around that time, it won't send us any back.
         *  Some older raw files (those created in Aug/Sep 2020) might have different naming
         *  conventions.
         */

        /** Note that downloading ticks has memory issues because it stores the entire response in
         *  a single buffer, and the response for a ticker like SPY is huge (can be >10GB). Note
         *  that downloading ticks isn't well tested, and while it usually works, it sometimes
         *  appears to not terminate. There appears to be a problem when using ofstream.write
         *  on 2GB+ (so it's probably a GCC issue) not finishing, even though it writes the contents
         *  to the disk (the file isn't empty and has most to all of the content).
         */
        void download_equity_ticks_L1(const std::string &ticker); ///thread-safe
        void download_equity_ticks_L1(const std::vector<std::string> &tickers); ///thread-safe

        void download_OHLCV_1min(const std::string &ticker); ///thread-safe
        void download_OHLCV_1min(const std::vector<std::string> &tickers); ///thread-safe
    };
    std::unique_ptr<User> request_user(); ///thread-safe. returns nullptr on failure

    ///the following functions don't need a connection so they don't need a User

    /** Loading ticks isn't well tested. Note that historical ticks and streaming ticks aren't the
     *  same. Streaming ticks have more data fields. In particular, historical ticks only create
     *  new ticks for new trades, while streaming ticks contain ticks for bid and ask updates.
     *  Historical ticks don't bid or ask information other than prices (e.g. no sizes).
     *  Historical ticks only go back 6 months, but that still takes a lot of space for liquid
     *  instruments (it can be >10GB for SPY).
     */
    std::vector<std::vector<EquityTickL1>> load_equity_ticks_L1(const LoadDataInfo &info);

    /** load_OHLCV_1min returns a 2D vector. The first index represents the day and it is
     *  always true that result[i] has all the data points for load_data_info.dates[i].
     */
    std::vector<std::vector<OHLCV_1min_Bar>> load_OHLCV_1min(const LoadDataInfo &info); ///thread-safe
}

}}}
