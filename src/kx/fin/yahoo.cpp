#include "kx/fin/yahoo.h"
#include "kx/time.h"
#include "kx/log.h"
#include "kx/util.h"
#include "kx/io.h"
#include "kx/debug.h"
#include "kx/net/kcurl.h"
#include "kx/multithread/shared_lock_set.h"

#include <curl/curl.h>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>
#include <optional>

namespace kx { namespace fin { namespace yahoo {

Dividend::Dividend(Time ex_date_, double amount_):
    ex_date(ex_date_), amount(amount_)
{}

Split::Split(Time date_, double ratio_):
    date(date_), ratio(ratio_)
{}

Earnings::Earnings(Time date_, double expected_EPS_, double actual_EPS_):
    date(date_), expected_EPS(expected_EPS_), actual_EPS(actual_EPS_)
{}

/*Known Yahoo issues:
  -AKR has no splits, but Yahoo says it does. I don't download AKR splits.
  -Yahoo sometimes has "double adjustments." For example, on 9/19/2016, something
   weird happens in XLF. Either there's a 4.44 dividend or a 1231:1000 split.
   They both result in a similar adjustment factor. However, Yahoo finance includes
   both of these, causing a double-adjustment, which is bad. I'm just going to
   assume the split is correct in these instances. I take of these by, when
   loading adjusted OHLCV data, ignoring split dates that coincide with dividend
   dates.
  -Yahoo thinks we're DDOSing it if we send too many requests at once. If we use
   the same IP to download splits/data for all tickers, then later runs might bug
   out (a VPN can be used to circumvent this). If curl_multi_perform is used to
   download hundreds of tickers simultaneously, then we don't get a response (I'm
   not sure if this is because I'm using curl's multi functionality incorrectly or
   because Yahoo is rejecting me, but I think it's the latter).
*/

template<class T>
static std::optional<std::vector<T>> read_binary_file_to_vector(const std::string &file_path)
{
    std::vector<T> res;

    std::ifstream file(file_path, std::ios::binary);
    if(file.fail()) {
        return {};
    }
    file.seekg(0, std::ios::end);
    size_t file_len = file.tellg();
    file.seekg(0, std::ios::beg);

    k_expects(file_len % sizeof(T) == 0);
    res.resize(file_len / sizeof(T));
    file.read((char*)res.data(), file_len);
    return res;
}

static constexpr const char* splits_path = "kx_data/yahoo/splits/";
static constexpr const char* dividends_path = "kx_data/yahoo/dividends/";
static constexpr const char* earnings_path = "kx_data/yahoo/earnings/";

static SharedLock_Set<std::string> splits_locks;
static SharedLock_Set<std::string> dividends_locks;
static SharedLock_Set<std::string> earnings_locks;

std::optional<std::vector<Split>> load_splits(const std::string &ticker)
{
    static_assert(sizeof(Split) == 16);

    auto lock_guard = splits_locks.get_lock_shared_guard(ticker);
    std::string file_path = splits_path + ticker + ".csv.binary";
    return read_binary_file_to_vector<Split>(file_path);
}
std::optional<std::vector<Dividend>> load_dividends(const std::string &ticker)
{
    static_assert(sizeof(Dividend) == 16);

    auto lock_guard = dividends_locks.get_lock_shared_guard(ticker);
    std::string file_path = dividends_path + ticker + ".csv.binary";
    return read_binary_file_to_vector<Dividend>(file_path);
}
std::optional<std::vector<Earnings>> load_earnings(const std::string &ticker)
{
    static_assert(sizeof(Earnings) == 24);
    auto lock_guard = earnings_locks.get_lock_shared_guard(ticker);
    std::string file_path = earnings_path + ticker + ".csv.binary";
    return read_binary_file_to_vector<Earnings>(file_path);
}
static std::string get_yahoo_ticker(std::string ticker)
{
    if(ticker.size()>=3 && ticker.substr(ticker.size()-3)==".TO")
        std::replace(ticker.begin(), ticker.end()-3, '.', '-');
    else
        std::replace(ticker.begin(), ticker.end(), '.', '-');
    return ticker;
}
static std::unique_ptr<net::KCurl> make_kcurl(const std::string &ticker, const std::string &events_field)
{
    if(std::atoi(Time::now_ET().to_str("%Y").c_str()) >= 2038)
        log_warning("You might need to change the end time (the period2 POST field) to "
                    "include the recent date. Note that Yahoo had a Y2038 bug, so make sure "
                    "that it doesn't affect results.");

    auto curl = std::make_unique<net::KCurl>();
    std::string URL = "https://query1.finance.yahoo.com/v7/finance/download/";
    URL += get_yahoo_ticker(ticker);
    URL += "?period1=1580857132&period2=2147483647&interval=1d&events=" + events_field;
    curl->set_URL(URL);
    return curl;
}
static Result make_splits_file(const std::string &ticker, const net::KCurl &curl)
{
    if(ticker == "AKR") //bad data (AKR doesn't have any splits, but Yahoo says it does)
        return Result::Success;

    auto lock_guard = splits_locks.get_lock_guard(ticker);

    std::string reply = curl.get_last_response();
    if(reply.find("404 Not Found") != std::string::npos) {
        //it's OK if some tickers don't have data for them, because IQFeed symbols
        //and Yahoo symbols don't match up exactly.
        log_warning("Yahoo returned data no split data for ticker \"" + ticker + "\"");
        return Result::Success; //it's a success because 404 Not Found is a valid message
    }
    io::make_folder(splits_path);
    std::string raw_file_path = splits_path + ticker + ".csv";
    std::string binary_file_path = splits_path + ticker + ".csv.binary";
    std::ofstream output_raw(raw_file_path, std::ios::binary | std::ios::trunc);
    std::ofstream output_binary(binary_file_path, std::ios::binary | std::ios::trunc);

    if(output_raw.fail()) {
        log_error("failed to create file \"" + raw_file_path + "\"");
        return Result::Failure;
    }
    if(output_binary.fail()) {
        log_error("failed to create file \"" + binary_file_path + "\"");
        return Result::Failure;
    }
    output_raw.write(reply.data(), reply.size());
    auto lines = split_str(reply, '\n');
    if(lines.size() > 0) {
        std::vector<Split> splits;
        for(size_t j=1; j<lines.size(); j++) { //ignore the 0th line, which is just titles
            auto parts = split_str(lines[j], ',');
            if(parts.size() != 2) { //This happens if Yahoo thinks we're DDOSing it
                log_info("bad Yahoo reply for split query=" + reply);
                return Result::Failure;
            }
            Time time(parts[0], Time::Format::YYYY_mm_DD);
            auto ratio_parts = split_str(parts[1], ':');
            double ratio_numerator = std::atof(ratio_parts[0].c_str());
            double ratio_denominator = std::atof(ratio_parts[1].c_str());

            //yahoo has some weird seemingly bs info like 99:98 splits, but at least
            //some of those are real.
            //There are some other weird things that are almost certainly wrong that
            //we will just ignore (like 0:1 splits) by ignoring nans/infs/negatives
            double ratio = ratio_numerator / ratio_denominator;
            if(!std::isnan(ratio) && !std::isinf(ratio) && ratio>0)
                splits.emplace_back(time, ratio);
        }
        //The dates are sometimes out of order, so sort stuff. Note that this means
        //the binary file won't perfectly correspond to the raw file.
        std::sort(splits.begin(), splits.end(),
                  [](const Split &a, const Split &b) -> bool
                  {
                      return a.date < b.date;
                  });
        output_binary.write((char*)splits.data(), splits.size() * sizeof(splits[0]));
    }
    return Result::Success;
}
Result download_splits(const std::string &ticker)
{
    auto curl = make_kcurl(ticker, "split");
    if(curl->send_request() == net::Result::Failure)
        return Result::Failure;
    return make_splits_file(ticker, *curl);
}
Result download_splits(const std::vector<std::string> &tickers)
{
    /*std::vector<net::KCurl> curls;
    for(const auto &ticker: tickers)
        curls.push_back(make_kcurl(ticker, "split"));
    net::KCurl::multi_perform_block_until_done(&curls);

    bool failed = false;
    #pragma omp parallel for
    for(size_t i=0; i<tickers.size(); i++) {
        bool failed_local = (make_splits_file(tickers[i], curls[i]) == Result::Failure);
        #pragma omp atomic
        failed |= failed_local;
    }
    return failed? Result::Failure: Result::Success;*/

    bool failed = false;
    #pragma omp parallel for
    for(size_t i=0; i<tickers.size(); i++)
        failed |= (download_splits(tickers[i]) == Result::Failure);
    return failed? Result::Failure: Result::Success;
}

static Result make_dividends_file(const std::string &ticker, const net::KCurl &curl)
{
    auto lock_guard = dividends_locks.get_lock_guard(ticker);

    std::string reply = curl.get_last_response();

    if(reply.find("404 Not Found") != std::string::npos) {
        //it's OK if some tickers don't have data for them, because IQFeed symbols
        //and Yahoo symbols don't match up exactly.
        log_warning("Yahoo returned no dividend data for ticker \"" + ticker + "\"");
        return Result::Success; //404 Not Found is a valid message, so it's a success
    }
    io::make_folder(dividends_path);
    std::string raw_file_path = dividends_path + ticker + ".csv";
    std::string binary_file_path = dividends_path + ticker + ".csv.binary";
    std::ofstream output_raw(raw_file_path, std::ios::binary | std::ios::trunc);
    std::ofstream output_binary(binary_file_path, std::ios::binary | std::ios::trunc);

    if(output_raw.fail()) {
        log_error("failed to create file \"" + raw_file_path + "\"");
        return Result::Failure;
    }
    if(output_binary.fail()) {
        log_error("failed to create file \"" + binary_file_path + "\"");
        return Result::Failure;
    }
    output_raw.write(reply.data(), reply.size());
    auto lines = split_str(reply, '\n');
    if(lines.size() > 0) {
        std::vector<Dividend> dividends;
        for(size_t j=1; j<lines.size(); j++) { //ignore the 0th line, which is just titles
            auto parts = split_str(lines[j], ',');
            if(parts.size() != 2) { //This happens if Yahoo thinks we're DDOSing it
                log_info("bad Yahoo reply for dividend query=" + reply);
                return Result::Failure;
            }
            Time time(parts[0], Time::Format::YYYY_mm_DD);
            double amount = std::atof(parts[1].c_str());
            dividends.emplace_back(time, amount);
        }
        //The dates are sometimes out of order, so sort stuff. Note that this means
        //the binary file won't perfectly correspond to the raw file.
        std::sort(dividends.begin(), dividends.end(),
                  [](const Dividend &a, const Dividend &b) -> bool
                  {
                      return a.ex_date < b.ex_date;
                  });
        output_binary.write((char*)dividends.data(), dividends.size() * sizeof(dividends[0]));
    }
    return Result::Success;
}
Result download_dividends(const std::string &ticker)
{
    auto curl = make_kcurl(ticker, "div");
    if(curl->send_request() == net::Result::Failure)
        return Result::Failure;
    return make_dividends_file(ticker, *curl);
}
Result download_dividends(const std::vector<std::string> &tickers)
{
    /*std::vector<net::KCurl> curls;
    for(const auto &ticker: tickers)
        curls.push_back(make_kcurl(ticker, "div"));
    net::KCurl::multi_perform_block_until_done(&curls);

    bool failed = false;
    #pragma omp parallel for
    for(size_t i=0; i<tickers.size(); i++) {
        bool failed_local = (make_dividends_file(tickers[i], curls[i]) == Result::Failure);
        #pragma omp atomic
        failed |= failed_local;
    }
    return failed? Result::Failure: Result::Success;*/

    bool failed = false;
    #pragma omp parallel for
    for(size_t i=0; i<tickers.size(); i++)
        failed |= (download_dividends(tickers[i]) == Result::Failure);
    return failed? Result::Failure: Result::Success;
}

static const std::string month_str[] {
    "",
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};
static Time parse_date(const std::string &s)
{
    k_expects(s.size() == 12);

    std::string month_s = s.substr(0, 3);
    int month_int = -1;
    for(int i=1; i<=12; i++) {
        if(month_str[i] == month_s) {
            month_int = i;
            break;
        }
    }
    if(month_int == -1) {
        log_error("Unknown month in yahoo::parse_date=\"" + month_s + "\"");
        return Time::NA();
    }
    int day_int = std::atoi(s.substr(4, 2).c_str());
    int year_int = std::atoi(s.substr(8, 4).c_str());
    return Time::from_YmD(year_int, month_int, day_int);
}

static const std::string earnings_pat1 = "\"Earnings Date\" data-reactid=";
static const std::string earnings_pat2 = "-->";

static std::optional<std::vector<Earnings>> parse_earnings_from_HTML(const std::string &html)
{
    std::vector<Earnings> earnings;
    size_t pos = 0;
    while(pos < html.size()) {
        pos = html.find(earnings_pat1, pos);
        if(pos == std::string::npos)
            break;
        pos += earnings_pat1.size();
        int num_g_signs = 0;
        while(pos < html.size()) {
            if(html[pos] == '>') {
                num_g_signs++;
                if(num_g_signs == 2) {
                    pos++;
                    break;
                }
            }
            pos++;
        }
        if(pos==html.size() || num_g_signs!=2)
            break;
        if(pos + 12 > html.size()) {
            log_error("malformed Yahoo earnings HTML: expected at least 12 chars for date");
            break;
        }
        std::string date_str = html.substr(pos, 12);
        earnings.emplace_back();
        earnings.back().date = parse_date(date_str);

        pos += 12;
        if(pos == html.size()) {
            log_error("malformed Yahoo earnings HTML: no more text after earnings date");
            break;
        }

        pos = html.find(earnings_pat2, pos);
        if(pos == std::string::npos || pos+earnings_pat2.size()>=html.size()) {
            log_error("malformed Yahoo earnings HTML: no estimated earnings after earnings date");
            break;
        }
        pos += earnings_pat2.size();

        size_t last = pos+1;
        while(last < html.size()) {
            if(html[last] == '<')
                break;
            last++;
        }
        if(last==html.size()) {
            log_error("malformed Yahoo earnings HTML: unterminated estimated earnings");
            break;
        }
        auto expected_EPS_str = html.substr(pos, last-pos);
        if(expected_EPS_str == "-") //"-" usually indicates a date in the future
            earnings.back().expected_EPS = std::numeric_limits<double>::quiet_NaN();
        else
            earnings.back().expected_EPS = std::atof(expected_EPS_str.c_str());
        pos = last;

        pos = html.find(earnings_pat2, pos);
        if(pos == std::string::npos || pos+earnings_pat2.size()>=html.size()) {
            log_error("malformed Yahoo earnings HTML: no actual earnings after estimated earnings");
            break;
        }
        pos += earnings_pat2.size();

        pos = html.find(earnings_pat2, pos);
        if(pos == std::string::npos || pos+earnings_pat2.size()>=html.size()) {
            log_error("malformed Yahoo earnings HTML: no actual earnings after estimated earnings");
            break;
        }
        pos += earnings_pat2.size();

        last = pos+1;
        while(last < html.size()) {
            if(html[last] == '<')
                break;
            last++;
        }
        if(last==html.size()) {
            log_error("malformed Yahoo earnings HTML: unterminated estimated earnings");
            break;
        }
        auto actual_EPS_str = html.substr(pos, last-pos);
        if(actual_EPS_str == "-") //"-" usually indicates a date in the future
            earnings.back().actual_EPS = std::numeric_limits<double>::quiet_NaN();
        else
            earnings.back().actual_EPS = std::atof(actual_EPS_str.c_str());
        pos = last;
    }
    auto same_date = [](const Earnings &a, const Earnings &b) -> bool
                    {
                        return a.date == b.date;
                    };
    auto date_cmp = [](const Earnings &a, const Earnings &b) -> bool
                    {
                        return a.date < b.date;
                    };
    std::reverse(earnings.begin(), earnings.end());
    if(!std::is_sorted(earnings.begin(), earnings.end(), date_cmp)) {
        log_warning("Yahoo earnings data is not sorted. Sorting.");
        std::sort(earnings.begin(), earnings.end(), date_cmp);
    }
    //Yahoo sometimes repeats earnings dates
    earnings.resize(std::unique(earnings.begin(), earnings.end(), same_date) - earnings.begin());

    return earnings;
}
Result download_earnings(const std::string &ticker)
{
    auto lock_guard = earnings_locks.get_lock_guard(ticker);

    /*
    //don't redownload stuff even tho it may be updated cuz Yahoo query limits
    std::ifstream input_file(earnings_path + ticker + ".csv.binary");
    if(!input_file.fail()) {
        return Result::Success;
    }
    */

    net::KCurl curl;
    std::string URL = "https://finance.yahoo.com/calendar/earnings?symbol=";
    URL += get_yahoo_ticker(ticker);
    curl.set_URL(URL);
    curl.send_request();

    //We should receive an empty optional if the query fails, but right now that's
    //not working, so the optional always has a value (even if the query values).
    //We should fix it so that it fails when we get a bad response.
    auto earnings_opt = parse_earnings_from_HTML(curl.get_last_response());

    if(!earnings_opt.has_value())
        return Result::Failure;

    const auto &earnings = *earnings_opt;

    io::make_folder(earnings_path);
    auto output_file_path = earnings_path + ticker + ".csv.binary";
    std::ofstream output_file(output_file_path, std::ios::binary | std::ios::trunc);
    if(output_file.fail()) {
        log_error("failed to create outpput file " + output_file_path);
        return Result::Failure;
    }
    output_file.write((char*)earnings.data(), earnings.size() * sizeof(earnings[0]));
    return Result::Success;
}
Result download_earnings(const std::vector<std::string> &tickers)
{
    bool failed = false;
    #pragma omp parallel for
    for(size_t i=0; i<tickers.size(); i++)
        failed |= (download_earnings(tickers[i]) == Result::Failure);
    return failed? Result::Failure: Result::Success;
}

}}}
