#pragma once

#include "kx/debug.h"

#include <memory>
#include <algorithm>

namespace kx {

template<class T> class FixedSizeArray
{
    std::unique_ptr<T[]> vals;
    size_t len;
public:
    FixedSizeArray():
        vals(nullptr),
        len(0)
    {}
    FixedSizeArray(size_t len_):
        vals(std::make_unique<T[]>(len_)),
        len(len_)
    {}
    FixedSizeArray(std::initializer_list<T> elements):
        vals(std::make_unique<T[]>(elements.size())),
        len(elements.size())
    {
        std::copy(elements.begin(), elements.end(), vals.get());
    }

    FixedSizeArray(const FixedSizeArray &other):
        vals(std::make_unique<T[]>(other.len)),
        len(other.len)
    {
        std::copy(other.vals.get(),other.vals.get() + other.len, vals.get());
    }
    FixedSizeArray &operator = (const FixedSizeArray &other)
    {
        vals = std::make_unique<T[]>(other.len);
        len = other.len;
        std::copy(other.vals.get(), other.vals.get() + other.len, vals.get());
        return *this;
    }
    FixedSizeArray(FixedSizeArray &&other):
        vals(std::move(other.vals)),
        len(other.len)
    {
        other.len = 0;
    }
    FixedSizeArray &operator = (FixedSizeArray &&other)
    {
        vals = std::move(other.vals);
        len = other.len;
        other.len = 0;
        return *this;
    }

    inline decltype(vals.get()) begin() const
    {
        return vals.get();
    }
    inline decltype(vals.get()) end() const
    {
        return vals.get() + len;
    }
    inline size_t size() const
    {
        return len;
    }
    inline T &operator[](size_t idx)
    {
        k_expects(idx <= len);
        return vals[idx];
    }
    inline const T &operator[](size_t idx) const
    {
        k_expects(idx <= len);
        return vals[idx];
    }
};

/*
template<class T> class FixedSizeArray2D
{
    std::unique_ptr<T[]> vals;
    std::unique_ptr<size_t> lens;
    size_t total_len;
    size_t num_rows;
public:
    FixedSizeArray2D():
        vals(nullptr),
        lens(nullptr),
        num_rows(0)
    {}
    FixedSizeArray2D(size_t total_len):
        vals(std::make_unique<T[]>(total_len)),
        lens(nullptr),
        num_rows(0)
    {}

    FixedSizeArray2D(const FixedSizeArray2D &other):
        vals(std::make_unique<T[]>(other.len)),
        lens(other.lens)
    {
        std::copy(other.vals.get(),other.vals.get() + other.len, vals.get());
    }
    FixedSizeArray &operator = (const FixedSizeArray &other)
    {
        vals = std::make_unique<T[]>(other.len);
        len = other.len;
        std::copy(other.vals.get(), other.vals.get() + other.len, vals.get());
        return *this;
    }
    FixedSizeArray(FixedSizeArray &&other):
        vals(std::move(other.vals)),
        len(other.len)
    {
        other.len = 0;
    }
    FixedSizeArray &operator = (FixedSizeArray &&other)
    {
        vals = std::move(other.vals);
        len = other.len;
        other.len = 0;
        return *this;
    }

    decltype(vals.get()) begin() const
    {
        return vals.get();
    }
    decltype(vals.get()) end() const
    {
        return vals.get() + len;
    }
    size_t size() const
    {
        return len;
    }
    T &operator[](size_t idx)
    {
        k_expects(idx <= len);
        return vals[idx];
    }
    const T &operator[](size_t idx) const
    {
        k_expects(idx <= len);
        return vals[idx];
    }
};
*/

}
