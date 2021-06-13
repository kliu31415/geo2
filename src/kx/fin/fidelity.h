#include "kx/time.h"

#include <vector>
#include <string>

namespace kx { namespace fin { namespace fidelity {

/** This returns a sorted vector of all tickers that have an ex-div on "date".
 *  This works for US stocks but not Canadian ones.
 *
 *  This is thread-safe but not interprocess safe.
 *
 *  This isn't well tested.
 *
 *  This working depends on Fidelity not changing their website layout, so it's
 *  not very reliable. It wouldn't be bad to occasionally manually check the tickers
 *  to ensure that they're right.
 */
std::vector<std::string> get_ex_div_tickers(Time date);

}}}
