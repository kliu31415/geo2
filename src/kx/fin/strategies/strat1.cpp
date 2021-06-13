#include "kx/fin/strategies/strat1.h"
#include "kx/debug.h"
#include "kx/fin/iqfeed.h"
#include "kx/fin/holidays.h"
#include "kx/stats.h"

#include <vector>
#include <cmath>
#include <limits>

namespace kx { namespace fin {

double Strat1_Results::get_pnl_t() const
{
    if(pnls.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::sum(pnls) / std::sqrt(stats::sumsq(pnls));
}
double Strat1_Results::get_pnl_one_sided_p() const
{
    if(pnls.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::t_to_one_sided_p((int)pnls.size(), get_pnl_t());
}
double Strat1_Results::get_pnl_mean() const
{
    if(pnls.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::mean(pnls);
}
double Strat1_Results::get_pnl_sd() const
{
    if(pnls.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::stdev(pnls);
}
double Strat1_Results::get_predictor_pnl_mean() const
{
    if(predictor_pnls.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::mean(predictor_pnls);
}
double Strat1_Results::get_responder_pnl_mean() const
{
    if(responder_pnls.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::mean(responder_pnls);
}
double Strat1_Results::get_hold_time_mean() const
{
    if(hold_times.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::mean(hold_times);
}
double Strat1_Results::get_hold_time_sd() const
{
    if(hold_times.size() == 0)
        return std::numeric_limits<double>::quiet_NaN();
    else
        return stats::stdev(hold_times);
}


namespace {
    std::vector<fin::OHLCV_1min_Bar> load_bars(const std::string &ticker,
                                               Time start_date,
                                               Time end_date,
                                               Time::Delta start_minute,
                                               Time::Delta end_minute,
                                               fin::IQFeed::HistDb::LoadDataInfo::adjust_flag_t adjust=
                                               fin::IQFeed::HistDb::LoadDataInfo::ALL_ADJUST_BITS)
    {
        fin::IQFeed::HistDb::LoadDataInfo load_data_info{};
        load_data_info.ticker = ticker;
        load_data_info.dates = fin::get_exchange_days("XNYS", start_date, end_date);
        load_data_info.day_start_time = start_minute;
        load_data_info.day_end_time = end_minute;
        load_data_info.plug_hole_style = fin::IQFeed::HistDb::LoadDataInfo::PlugHoleStyle::CopyPrevious;
        load_data_info.adjust = adjust;
        return flatten_vec2D(fin::IQFeed::HistDb::load_OHLCV_1min(load_data_info));
    }

    double get_log_ret(double v1, double v2)
    {
        return std::log1p(v2/v1 - 1);
    }
}

Strat1_Results Strat1_Args::run_with_args()
{
    k_expects(beta >= 0);
    k_expects(hedging_beta >= 0);

    k_expects(start_minute <= end_minute);
    k_expects(start_minute.is_multiple_of(Time::Delta(1, Time::Length::minute)));
    k_expects(end_minute.is_multiple_of(Time::Delta(1, Time::Length::minute)));

    k_expects(start_date <= end_date);
    k_expects(start_date.is_multiple_of(Time::Delta(1, Time::Length::day)));
    k_expects(end_date.is_multiple_of(Time::Delta(1, Time::Length::day)));

    auto predictor_bars = load_bars(predictor,
                                    start_date, end_date,
                                    start_minute, end_minute);
    auto responder_bars = load_bars(responder,
                                    start_date, end_date,
                                    start_minute, end_minute);

    size_t minutes_per_day = 1 + (end_minute - start_minute).
                                  to_int64(Time::Length::minute);

    size_t last_iter = predictor_bars.size() - execution_delay - 1;

    double rolling_predictor_sum = 0;
    double rolling_responder_sum = 0;
    double rolling_predictor_den = 0;
    double rolling_responder_den = 0;

    std::vector<double> pnls;
    std::vector<double> predictor_pnls;
    std::vector<double> responder_pnls;

    //equity curve
    std::vector<double> X_EC;
    std::vector<double> Y_EC;

    double cur_equity = 1;

    X_EC.push_back(0);
    Y_EC.push_back(0);

    std::vector<size_t> hold_times;

    Time open_position_time;
    double open_position_excess_ret;
    double cur_trade_predictor_pnl = 0;
    double cur_trade_responder_pnl = 0;
    size_t open_position_idx;

    int position = 0;

    size_t first_iter = minutes_per_day*5;

    std::vector<Strat1_Trade> trades;

    for(size_t i=first_iter; i<=last_iter; i++) {

        rolling_predictor_den *= decay_rate;
        rolling_predictor_sum *= decay_rate;
        rolling_predictor_den += 1;
        rolling_predictor_sum += predictor_bars[i].close * 1;

        rolling_responder_den *= decay_rate;
        rolling_responder_sum *= decay_rate;
        rolling_responder_den += 1;
        rolling_responder_sum += responder_bars[i].close * 1;

        double predictor_avg = rolling_predictor_sum / rolling_predictor_den;
        double responder_avg = rolling_responder_sum / rolling_responder_den;

        double predictor_ret = get_log_ret(predictor_avg, predictor_bars[i].close);
        double responder_ret = get_log_ret(responder_avg, responder_bars[i].close);

        double excess_ret = responder_ret - beta*predictor_ret;

        //if we have an open position, update our pnl
        if(position!=0) {

            cur_trade_predictor_pnl += position *
                                       (-get_log_ret(predictor_bars[i-1].close, predictor_bars[i].close));
            cur_trade_responder_pnl += position *
                                       get_log_ret(responder_bars[i-1].close, responder_bars[i].close);

            if(i==last_iter ||
               (i%minutes_per_day>=first_trade_minute && i%minutes_per_day<minutes_per_day-execution_delay &&
                predictor_bars[i+execution_delay].volume>0 && responder_bars[i+execution_delay].volume>0 &&
               (open_position_excess_ret*excess_ret<0 || std::fabs(excess_ret) < close_position_cutoff)))
            {
                //sell at next open
                cur_trade_predictor_pnl -= position *
                                           (-get_log_ret(predictor_bars[i].close,
                                                         predictor_bars[i+execution_delay].open));
                cur_trade_responder_pnl -= position *
                                           get_log_ret(responder_bars[i].close,
                                                       responder_bars[i+execution_delay].open);
                if(position == -1) {
                    predictor_pnls.push_back(std::exp(cur_trade_predictor_pnl) - 1);
                    responder_pnls.push_back(1 - std::exp(-cur_trade_responder_pnl));
                } else {
                    predictor_pnls.push_back(1 - std::exp(-cur_trade_predictor_pnl));
                    responder_pnls.push_back(std::exp(cur_trade_responder_pnl) - 1);
                }
                predictor_pnls.back() -= transaction_cost;
                responder_pnls.back() -= transaction_cost;
                responder_pnls.back() *= 1.0 / (1 + std::fabs(hedging_beta));
                predictor_pnls.back() *= hedging_beta / (1 + std::fabs(hedging_beta));

                pnls.push_back(responder_pnls.back() + predictor_pnls.back());

                position = 0;

                trades.emplace_back();
                trades.back().time1 = open_position_time;
                trades.back().time2 = responder_bars[i+execution_delay].time;
                trades.back().predictor_pnl = predictor_pnls.back();
                trades.back().responder_pnl = responder_pnls.back();
                if(open_position_excess_ret > 0)
                    trades.back().long_responder = false;
                else
                    trades.back().long_responder = true;

                hold_times.push_back(i - open_position_idx);

                cur_equity *= 1 + pnls.back();

                X_EC.push_back(i / (double)minutes_per_day);
                Y_EC.push_back(std::log(std::max(1e-9, cur_equity)));

                i += execution_delay;
                continue;
            }
        }

        if(i!=last_iter && i%minutes_per_day<minutes_per_day-execution_delay &&
           i%minutes_per_day>=first_trade_minute && position==0)
        {
            if(std::fabs(excess_ret) > open_position_cutoff && predictor_bars[i+execution_delay].volume>0 &&
               responder_bars[i+execution_delay].volume>0)
            {
                open_position_time = predictor_bars[i+execution_delay].time;
                open_position_excess_ret = excess_ret;
                //we want to buy at the next minute's open. If we initialized cur_trade_pnl=0, then we would end up
                //buying at this minute's close, which is bad. Initialize to an amount that simulates buying at the
                //next minute's open.
                position = std::copysign(1, -excess_ret);
                cur_trade_predictor_pnl = -position *
                                          (-get_log_ret(predictor_bars[i].close,
                                                        predictor_bars[i+execution_delay].open));
                cur_trade_responder_pnl = -position *
                                          get_log_ret(responder_bars[i].close,
                                                      responder_bars[i+execution_delay].open);
                open_position_idx = i;
            }
        }
    }

    Strat1_Results results;

    results.trades = trades;

    results.num_trades = pnls.size();
    results.pnls = std::move(pnls);
    results.predictor_pnls = std::move(predictor_pnls);
    results.responder_pnls = std::move(responder_pnls);

    std::vector<double> predictor_rets;
    std::vector<double> responder_rets;
    for(size_t i=2*minutes_per_day-1; i<predictor_bars.size(); i+=minutes_per_day) {
        predictor_rets.push_back(get_log_ret(predictor_bars[i-minutes_per_day].close, predictor_bars[i].close));
        responder_rets.push_back(get_log_ret(responder_bars[i-minutes_per_day].close, responder_bars[i].close));
    }
    results.daily_return_beta = stats::OLS_slope(predictor_rets, responder_rets);
    results.daily_return_corr = stats::corr(predictor_rets, responder_rets);

    results.hold_times = std::move(hold_times);

    results.equity_curve_x = std::move(X_EC);
    results.equity_curve_y = std::move(Y_EC);

    return results;
}

}}
