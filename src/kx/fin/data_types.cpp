#include "kx/fin/data_types.h"
#include "kx/util.h"
#include "kx/debug.h"

#include <algorithm>
#include <vector>
#include <string>
#include <fstream>

using std::vector;

namespace kx { namespace fin {

//because we process these in binary form sometimes, their layouts must stay constant
static_assert(sizeof(OHLC_Bar) == 40);
static_assert(sizeof(OHLCV_Intraday_Bar) == 56);
static_assert(sizeof(OHLCV_1day_Bar) == 48);

std::vector<std::string> extract_tickers(const std::string &file_path)
{
    using namespace kx;

    std::ifstream input_file(file_path, std::ios::binary);
    if(input_file.fail()) {
        log_error("failed to open file \"" + file_path + "\"");
        return {};
    }
    input_file.seekg(0, std::ios::end);
    auto len = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(len);
    input_file.read((char*)buffer.data(), len);

    auto lines = split_str(buffer, '\n');
    if(lines.size() == 0) {
        log_warning("fin::extract_tickers found 0 lines in file");
        return {};
    }
    lines.erase(lines.begin());

    std::vector<std::string> tickers;

    for(const auto &line: lines)
        tickers.push_back(split_str(line, ',')[0]);

    //there might be newlines at the end
    while(!tickers.empty() && tickers.back().empty())
        tickers.pop_back();

    return tickers;
}

OHLC_Bar::OHLC_Bar(Time time_,
                   double open_, double high_,
                   double low_, double close_):
    time(time_),
    open(open_), high(high_), low(low_), close(close_)
{}

OHLCV_Intraday_Bar::OHLCV_Intraday_Bar(Time time_,
                                       double open_, double high_,
                                       double low_, double close_,
                                       double volume_, double cum_vol_):
    OHLC_Bar(time_, open_, high_, low_, close_),
    volume(volume_), cum_vol(cum_vol_)
{}

OHLCV_1min_Bar::OHLCV_1min_Bar(Time time_,
                               double open_, double high_,
                               double low_, double close_,
                               double volume_, double cum_vol_):
    OHLCV_Intraday_Bar(time_, open_, high_, low_, close_, volume_, cum_vol_)
{}

OHLCV_5min_Bar::OHLCV_5min_Bar(Time time_,
                               double open_, double high_,
                               double low_, double close_,
                               double volume_, double cum_vol_):
    OHLCV_Intraday_Bar(time_, open_, high_, low_, close_, volume_, cum_vol_)
{}

OHLCV_1day_Bar::OHLCV_1day_Bar(Time time_,
                               double open_, double high_, double low_, double close_,
                               double volume_):
    OHLC_Bar(time_, open_, high_, low_, close_),
    volume(volume_)
{}

}}
