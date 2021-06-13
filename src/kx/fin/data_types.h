#pragma once

#include "kx/time.h"

#include <cstdint>
#include <vector>
#include <string>

namespace kx { namespace fin {

std::vector<std::string> extract_tickers(const std::string &file_path);

struct OHLC_Bar
{
    Time time; ///the start of this interval
    double open;
    double high;
    double low;
    double close;

    OHLC_Bar() = default;
    OHLC_Bar(Time time_,
             double open_, double high_, double low_, double close_);
};

struct OHLCV_Intraday_Bar: public OHLC_Bar
{
    double volume;
    double cum_vol; ///cumulative volume since day start until the end of this interval

    OHLCV_Intraday_Bar() = default;
    OHLCV_Intraday_Bar(Time time_,
                       double open_, double high_, double low_, double close_,
                       double volume_, double cum_vol_);
};

struct OHLCV_1min_Bar: public OHLCV_Intraday_Bar
{
    OHLCV_1min_Bar() = default;
    OHLCV_1min_Bar(Time time_,
                   double open_, double high_, double low_, double close_,
                   double volume_, double cum_vol_);
};

struct OHLCV_5min_Bar: public OHLCV_Intraday_Bar
{
    OHLCV_5min_Bar() = default;
    OHLCV_5min_Bar(Time time_,
                   double open_, double high_, double low_, double close_,
                   double volume_, double cum_vol_);
};

struct OHLCV_1day_Bar: public OHLC_Bar
{
    double volume;

    OHLCV_1day_Bar() = default;
    OHLCV_1day_Bar(Time time_,
                   double open_, double high_, double low_, double close_,
                   double volume_);
};

}}
