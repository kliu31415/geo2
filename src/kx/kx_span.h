#pragma once

#include "kx/debug.h"

namespace kx {

template<class T> class kx_span
{
    T *data;
    size_t len;
public:
    kx_span():
        data(nullptr),
        len(0)
    {}

    kx_span(T *begin_, T *end_):
        data(begin_),
        len(end_ - begin_)
    {}

    template<class Iter, std::enable_if_t<std::is_same_v<class std::iterator_traits<Iter>::value_type, T>, int> = 0>
    kx_span(Iter begin_, Iter end_):
        data(&(*begin_)),
        len(end_ - begin_)
    {}

    T &operator [] (int idx)
    {
        k_expects(idx>=0 && (size_t)idx<len);
        return data[idx];
    }
    const T &operator [] (int idx) const
    {
        k_expects(idx>=0 && (size_t)idx<len);
        return data[idx];
    }
    T &operator [] (uint32_t idx)
    {
        k_expects(idx < len);
        return data[idx];
    }
    const T &operator [] (uint32_t idx) const
    {
        k_expects(idx < len);
        return data[idx];
    }
    T &operator [] (size_t idx)
    {
        k_expects(idx < len);
        return data[idx];
    }
    const T &operator [] (size_t idx) const
    {
        k_expects(idx < len);
        return data[idx];
    }
    T *begin()
    {
        return data;
    }
    T *end()
    {
        return data + len;
    }
    size_t size() const
    {
        return len;
    }
};

}
