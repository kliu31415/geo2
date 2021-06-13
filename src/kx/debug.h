#pragma once

#include "kx/util.h"
#include "kx/log.h"

#ifndef NDEBUG
#define KX_DEBUG ///KX_DEBUG defined = extra debugging checks for kx stuff
#endif

///all macros are thread-safe

#define _K_LOC_INFO "file \"" + kx::to_str(__FILE__) \
                    + "\", line " + kx::to_str(__LINE__) + ", function \"" + \
                    kx::to_str(__FUNCTION__) + "\""

#ifdef KX_DEBUG

/** The "a" parameter in k_check should have no side effects. That is, you SHOULD NOT do
 *  something like k_assert(my_set.erase(5) == 1) because all k_check statements are turned
 *  off in release mode, meaning that you won't be erasing anything from your set if KX_DEBUG
 *  is turned off. Instead, do {int num_erased = my_set.erase(5); k_assert(num_erased == 1);}.
 */

#define _k_check_nomsg(name, a) (a? void(0): kx::log_error(kx::to_str(name) + " failed: \"" + \
                                 kx::to_str(#a) + "\"," + _K_LOC_INFO))

#define _k_check_wmsg(name, a, msg) (a? void(0): kx::log_error(kx::to_str(name) + " failed: \"" + \
                                     kx::to_str(#a) + "\"," + _K_LOC_INFO + ", msg \"" + \
                                     kx::to_str(msg) + "\""))

#define _KX_GET_MACRO(_1, _2, _3, MACRO_NAME, ...) MACRO_NAME
///"unused" is to prevent a compiler warning about ISO C++ requiring "..." to expand into >0 args
#define _k_check(name, ...) _KX_GET_MACRO(__VA_ARGS__, unused1, _k_check_wmsg, _k_check_nomsg, \
                                          unused2) (name, __VA_ARGS__)
#define k_assert(...) _k_check("k_assert", __VA_ARGS__)
#define k_expects(...) _k_check("k_expects", __VA_ARGS__)
#define k_ensures(...) _k_check("k_ensures", __VA_ARGS__)

#define k_trace() kx::log_info("trace: " + kx::to_str(_K_LOC_INFO))

#else //KX_DEBUG
#define k_assert(...) ((void)0)
#define k_expects(...) ((void)0)
#define k_ensures(...) ((void)0)
#define k_trace() ((void)0)
#endif //KX_DEBUG

namespace kx { namespace debug {

}}
