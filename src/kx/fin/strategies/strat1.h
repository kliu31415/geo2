#pragma once

#include "kx/time.h"

#include <string>
#include <optional>
#include <functional>
#include <memory>
#include <vector>

///forward declare this to reduce compile times
namespace kx { namespace plot {class LineGraph; }}

namespace kx { namespace fin {

struct Strat1_Trade
{
    bool long_responder;

    Time time1;
    Time time2;

    double predictor_pnl;
    double responder_pnl;
};

struct Strat1_Results
{
    size_t num_trades;

    std::vector<Strat1_Trade> trades;

    ///these vectors will all be of size num_trades
    std::vector<double> pnls;
    std::vector<double> predictor_pnls;
    std::vector<double> responder_pnls;

    double daily_return_beta;
    double daily_return_corr;

    std::vector<size_t> hold_times;

    /** X is the number of trading days that have elapsed
     *  Y is ln(max(equity, 1e-9)) (the max is to prevent undefined values)
     */
    std::vector<double> equity_curve_x;
    std::vector<double> equity_curve_y;

    double get_pnl_t() const;
    double get_pnl_one_sided_p() const;
    double get_pnl_mean() const;
    double get_pnl_sd() const;
    double get_predictor_pnl_mean() const;
    double get_responder_pnl_mean() const;

    double get_hold_time_mean() const;
    double get_hold_time_sd() const;
};

struct Strat1_Args
{
    std::string predictor;
    std::string responder;

    Time start_date;
    Time end_date;

    Time::Delta start_minute;
    Time::Delta end_minute;

    double beta;
    double hedging_beta;

    double open_position_cutoff;
    double close_position_cutoff;

    double decay_rate;

    size_t execution_delay;
    size_t first_trade_minute;

    double transaction_cost;

    Strat1_Results run_with_args();
};

}}
