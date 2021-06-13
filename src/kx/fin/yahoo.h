#pragma once

#include "kx/fin/data_types.h"

#include <vector>
#include <string>
#include <optional>

namespace kx { namespace fin { namespace yahoo {

struct Dividend
{
    Time ex_date;
    double amount;
    Dividend() = default;
    Dividend(Time ex_date_, double amount_);
};

struct Split
{
    Time date;
    double ratio;
    Split() = default;
    Split(Time date_, double ratio_);
};

struct Earnings
{
    Time date;
    double expected_EPS;
    double actual_EPS;
    Earnings() = default;
    Earnings(Time date_, double expected_EPS_, double actual_EPS_);
};

/** All functions SHOULD be thread safe, i.e. multiple threads can attempt to download
 *  the same ticker simultaneously and multiple tickers may be downloaded at the same time.
 *  These functions are not interprocess safe.
 *
 *  Downloading splits/divs work for US and Canadian tickers, and likely tickers from
 *  other areas of the world as well.
 *
 *  If the network breaks during a download operation, the operation is reported as
 *  successful even when it's not. This should be fixed sometime.
 *
 *  Yahoo has some errors.
 */

std::optional<std::vector<Split>> load_splits(const std::string &ticker); ///thread-safe

///Earnings/Dividends are generally split-adjusted but not dividend-adjusted
std::optional<std::vector<Dividend>> load_dividends(const std::string &ticker); ///thread-safe
/** Note that Earnings may contain lots of NaNs, which represent dates that Yahoo doesn't have
 *  data for. These are usually dates in the future, but may be dates that are just missing data
 *  or even dates before the IPO (???).
 */
std::optional<std::vector<Earnings>> load_earnings(const std::string &ticker); ///thread-safe

enum class Result
{
    Success, Failure
};

/** These download functions working depend on Yahoo not changing its website, which is
 *  probably not a great assumption.
 */

Result download_splits(const std::string &ticker); ///thread-safe
Result download_splits(const std::vector<std::string> &tickers); ///thread-safe

Result download_dividends(const std::string &ticker); ///thread-safe
Result download_dividends(const std::vector<std::string> &tickers); ///thread-safe

/** These functions return success even if Yahoo sends us a DDOS webpage.
 *  This should be fixed sometime. Some dates are also repeated, and it's very hard to
 *  download all earnings before getting a DDOS warning, so this function should probably
 *  be updated sometime. (Repeated dates will be removed if you re-run this; they only
 *  appeared in an older download_earnings version because I forgot to resize after
 *  calling std::unique, meaning that some dates at the end could be bogus if there
 *  are originally repeated dates).
 */
Result download_earnings(const std::string &ticker); ///thread-safe
Result download_earnings(const std::vector<std::string> &tickers); ///thread-safe

}}}
