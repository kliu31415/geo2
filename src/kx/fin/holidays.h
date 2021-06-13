#pragma once

#include "kx/time.h"

#include <vector>
#include <string>

namespace kx { namespace fin {

/** All functions are thread-safe given that no program is writing to the
 *  holidays files. Holidays are loaded automatically upon calling one of
 *  these functions, so there is no explicit init() function.
 */

std::vector<Time> get_exchange_days(const std::string &exchange_name,
                                    Time start_day, Time end_day);
std::vector<Time> get_exchange_days_XNYS(Time start_day, Time end_day);

bool is_XNYS_exchange_day(Time day); ///not well tested

Time get_next_exch_day_XNYS(Time day);

}}
