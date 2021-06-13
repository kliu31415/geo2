#include "kx/fin/holidays.h"
#include "kx/debug.h"
#include "kx/time.h"

#include <fstream>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

using std::vector;
using std::string;

namespace kx { namespace fin {

//Given the days a liquid ticker (e.g. SPY) trades, this returns what are probably
//holidays by finding weekdays where the ticker doesn't trade. If IQFeed is missing data
//for a day, then a day may falsely appear to be a holiday. Half-holidays aren't counted as
//holidays.
//This function probably won't be used much. Let's just put it here.
/*static void compute_holidays()
{
    fin::IQFeed::HistoricalDb::LoadDataInfo load_data_info{};
    load_data_info.ticker = "SPY";
    load_data_info.start_date = Time("2007-05-01", Time::Format::YYYY_mm_DD);
    load_data_info.end_date = Time("2020-08-20", Time::Format::YYYY_mm_DD);
    load_data_info.adjust = false;
    auto data = fin::IQFeed::HistoricalDb::load_OHLCV_1day(load_data_info);

    size_t data_idx = 0;
    vector<Time> holidays;
    for(Time day=load_data_info.start_date; day<=load_data_info.end_date; day+=Time::Delta(1, Time::Length::day)) {
        while(data_idx<data.size() && data[data_idx].time<day)
            data_idx++;
        std::tm tm(day);
        if(data_idx==data.size() || (data[data_idx].time!=day && !(tm.tm_wday==0) && !(tm.tm_wday==6))) {
            holidays.push_back(day);
        }
    }
    for(const auto &holiday: holidays)
        io::println(holiday.to_str(Time::Format::YYYY_mm_DD));
}
*/

vector<Time> get_exchange_days(const string &exchange_name, Time start_day, Time end_day)
{
    if(exchange_name == "XNYS")
        return get_exchange_days_XNYS(start_day, end_day);

    log_error("unknown exchange MIC code: \"" + exchange_name + "\"");
    return {};
}

[[maybe_unused]] static constexpr int64_t NS_PER_DAY = 1e9 * 86400;

static vector<Time> XNYS_holidays;
static std::once_flag XNYS_holidays_loaded;

static void load_XNYS_holidays()
{
    std::string file_name = "kx_data/holidays/XYNS_holidays.txt";
    std::ifstream fin(file_name); //we DON't want ios::binary
    if(fin.fail())
        log_error("failed to open \"" + file_name + "\"");
    string date;
    while(fin >> date)
        XNYS_holidays.emplace_back(date, Time::Format::YYYY_mm_DD);

    //there are definitely more than 0 holidays
    k_ensures(XNYS_holidays.size() > 0);
}

vector<Time> get_exchange_days_XNYS(Time start_day, Time end_day)
{
    k_expects(start_day <= end_day);
    k_expects(start_day.to_int64(Time::Length::ns) % NS_PER_DAY == 0);
    k_expects(end_day.to_int64(Time::Length::ns) % NS_PER_DAY == 0);

    if(std::atoi(start_day.to_str("%Y").c_str()) < 2007)
        log_warning("fin::holidays doesn't contain full info on years before 2007");
    if(std::atoi(end_day.to_str("%Y").c_str()) > 2021)
        log_warning("holidays haven't been updated for years beyond 2021");

    std::call_once(XNYS_holidays_loaded, load_XNYS_holidays);

    //holidays in [start_idx, end_idx) are relevant
    auto start_idx = std::lower_bound(XNYS_holidays.begin(), XNYS_holidays.end(), start_day);
    auto end_idx = std::upper_bound(XNYS_holidays.begin(), XNYS_holidays.end(), end_day);
    auto holiday_idx = start_idx;

    vector<Time> res;
    //wday represents days since Sunday. Let's make it days since Monday (that's more convenient)
    int day_of_week = (((std::tm)start_day).tm_wday + 6) % 7;

    for(Time cur_day=start_day; cur_day<=end_day; cur_day+=Time::Delta(1, Time::Length::day)) {

        while(holiday_idx<end_idx && *holiday_idx<cur_day)
            holiday_idx++;

        if(*holiday_idx != cur_day && day_of_week<5)
            res.push_back(cur_day);

        day_of_week++;
        if(day_of_week==7)
            day_of_week = 0;
    }
    return res;
}
bool is_XNYS_exchange_day(Time day)
{
    k_expects(day.to_int64(Time::Length::ns) % NS_PER_DAY == 0);

    auto cur_year = std::atoi(day.to_str("%Y").c_str());
    if(cur_year < 2007)
        log_warning("holidays don't contain full info on years before 2007");
    else if(cur_year > 2021)
        log_warning("holidays haven't been updated for years beyond 2021");

    std::call_once(XNYS_holidays_loaded, load_XNYS_holidays);

    int day_of_week = ((std::tm)day).tm_wday;
    if(day_of_week==0 || day_of_week==6)
        return false;
    if(std::binary_search(XNYS_holidays.begin(), XNYS_holidays.end(), day))
        return false;
    return true;
}
Time get_next_exch_day_XNYS(Time day)
{
    k_expects(day.to_int64(Time::Length::ns) % ((int64_t)1e9 * 86400) == 0);

    //assume there's at least one exchange day per week, which is probably accurate
    auto exch_days = get_exchange_days_XNYS(day + Time::Delta(1, Time::Length::day),
                                            day + Time::Delta(7, Time::Length::day));
    k_ensures(!exch_days.empty());

    return exch_days[0];
}

}}
