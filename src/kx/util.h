#pragma once

#include "kx/debug.h"

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>

namespace kx {

template<class T> [[gnu::pure]]
std::string to_str(T &&x)
{
    std::stringstream ss;
    ss << std::forward<T>(x);
    return ss.str();
}

/** container shouldn't be nullptr. However, I can't use k_expects due to circular header
 *  dependencies, so it's commented out.
 */
template<class Container, class T>
std::enable_if_t<!std::is_same<T, std::nullptr_t>::value,
void> erase_remove(Container *container, T &&value)
{
    //k_expects(container != nullptr);
    auto new_end = std::remove(container->begin(), container->end(), std::forward<T>(value));
    container->erase(new_end, container->end());
}

template<class Container>
void erase_remove(Container *container, std::nullptr_t)
{
    //k_expects(container != nullptr);
    auto new_end = std::remove(container->begin(), container->end(), nullptr);
    container->erase(new_end, container->end());
}

template<class Container, class Func>
void erase_remove_if(Container *container, Func &&func)
{
    //k_expects(container != nullptr);
    auto new_end = std::remove_if(container->begin(), container->end(), std::forward<Func>(func));
    container->erase(new_end, container->end());
}

/** _PasskeyDummy has no definition, which is intended, as it's just a placeholder
 *  and isn't meant to be constructible.
 */
class _PasskeyDummy;

///Passkey<f_1 ... f_i> accepts any of f_1 ... f_i as keys
template<class f1, class f2=_PasskeyDummy, class f3=_PasskeyDummy, class f4=_PasskeyDummy>
class Passkey
{
    friend f1;
    friend f2;
    friend f3;
    friend f4;

    Passkey() {} /** can't use =delete because then Passkey would be an aggregate struct,
                   * meaning uniform initialization (aka "{}") would still work
                   */

    Passkey(const Passkey&) = delete;
    Passkey &operator = (const Passkey&) = delete;

    Passkey(Passkey&&) = delete;
    Passkey &operator = (Passkey&&) = delete;

public:
    ///Passkey<T>, where T!=_PasskeyDummy, can be promoted to any Passkey that accepts T as a key
    template<class T>
    Passkey(const Passkey<T>&)
    {
        static_assert(!std::is_same<T, _PasskeyDummy>::value);
        static_assert(std::is_same<T, f1>::value || std::is_same<T, f2>::value ||
                      std::is_same<T, f3>::value || std::is_same<T, f4>::value);
    }
};

///ScopeGuard executes func() during the dtor
class ScopeGuard
{
    std::function<void()> func;
public:
    /** It's important to mark the ctor as noexcept because this class is often used to
     *  simulate scoped locks (lock() is called and then a ScopeGuard that unlocks is
     *  constructed). To ensure that the lock is eventually unlocked, this ctor can't throw,
     *  or else the lock would remain locked as the stack unwound from the exception.
     */
    ScopeGuard(std::function<void()> func_) noexcept;
    ~ScopeGuard();

    ///copying a ScopeGuard doesn't make sense
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard &operator = (const ScopeGuard&) = delete;

    ///ban moving ScopeGuards for simplicity
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard &operator = (ScopeGuard&&) = delete;
};

template<class T> [[gnu::pure]]
std::vector<T> flatten_vec2D(const std::vector<std::vector<T>> &input)
{
    std::vector<T> res;
    size_t total_size = 0;

    for(const auto &vec: input)
        total_size += vec.size();
    res.resize(total_size);

    size_t cur_size = 0;
    for(const auto &vec: input) {
        if(vec.size() > 0) {
            std::copy(vec.begin(), vec.end(), res.begin() + cur_size);
            cur_size += vec.size();
        }
    }
    return res;
}

[[gnu::pure]]
std::vector<std::string> split_str(std::string_view str, char by);

/** I haven't rigorously checked leading_digit for bugs
  * (idk if it might bug out occasionally due to finite precision floats)
  */
[[gnu::const]]
int leading_digit(double val, int base = 10);

}
