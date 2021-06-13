#pragma once

#include "kx/util.h"

#include <stack>
#include <mutex>
#include <utility>

namespace kx {

///moving probably isn't thread safe until I write move functions, so I'm deleting it for now
template<class T> class AtomicStack final
{
    std::stack<T> st;
    mutable std::mutex mtx;
public:
    AtomicStack() = default;

    AtomicStack(const AtomicStack&) = delete;
    AtomicStack &operator = (const AtomicStack&) = delete;
    AtomicStack(const AtomicStack&&) = delete;
    AtomicStack &operator = (const AtomicStack&&) = delete;

    void push(const T &val)
    {
        std::lock_guard<std::mutex> lg(mtx);
        st.push(val);
    }
    void push(T &&val)
    {
        std::lock_guard<std::mutex> lg(mtx);
        st.push(std::forward<T>(val));
    }
    void pop()
    {
        std::lock_guard<std::mutex> lg(mtx);
        k_expects(!st.empty());
        st.pop();
    }
    T &top()
    {
        std::lock_guard<std::mutex> lg(mtx);
        k_expects(!st.empty());
        return st.top();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lg(mtx);
        return st.empty();
    }
    size_t size() const
    {
        std::lock_guard<std::mutex> lg(mtx);
        return st.size();
    }
};

}
