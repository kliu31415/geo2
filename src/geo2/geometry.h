#pragma once

#include "kx/fixed_size_array.h"

#include "span_lite.h"
#include <type_traits>
#include <algorithm>
#include <memory>
#include <optional>
#include <cmath>

namespace geo2 {

class Collidable
{
public:
    virtual ~Collidable() = default;
};

template<class T> struct _MapVec
{
    ///T should not be integral. It could be fixed, but fixeds don't really exist.
    static_assert(std::is_floating_point<T>::value);

    T x;
    T y;

    _MapVec() {}
    _MapVec(T x_, T y_):
        x(x_),
        y(y_)
    {}

    T cross_prod(_MapVec other) const
    {
        return x * other.y - other.x * y;
    }
    T dot_prod(_MapVec other) const
    {
        return x * other.x + y * other.y;
    }
    T norm() const
    {
        return std::hypot(x, y);
    }
    _MapVec& operator + (const _MapVec& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    template<class S> _MapVec& operator * (S val)
    {
        x *= val;
        y *= val;
        return *this;
    }
    template<class S> _MapVec& operator / (S val)
    {
        x /= val;
        y /= val;
        return *this;
    }
};
using MapVec = _MapVec<double>;

template<class T> struct _MapCoord
{
    ///T should not be integral. It could be fixed, but fixeds don't really exist.
    static_assert(std::is_floating_point<T>::value);

    T x;
    T y;

    _MapCoord() {}
    _MapCoord(T x_, T y_):
        x(x_),
        y(y_)
    {}

    template<class T2> _MapCoord operator = (const _MapCoord<T2> &other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    _MapVec<T> operator - (const _MapCoord &other) const
    {
        return _MapVec<T>(x - other.x, y - other.y);
    }
};
using MapCoord = _MapCoord<double>;

template<class T> struct _MapRect
{
    ///T should not be integral. It could be fixed, but fixeds don't really exist.
    static_assert(std::is_floating_point<T>::value);

    T x;
    T y;
    T w;
    T h;

    _MapRect(T x_, T y_, T w_, T h_):
        x(x_),
        y(y_),
        w(w_),
        h(h_)
    {}
};
using MapRect = _MapRect<double>;

template<class T> struct _AABB
{
    ///invariant: x1 <= x2 and y1 <= y2 for all valid AABBs
    T x1;
    T x2;
    T y1;
    T y2;

    ///initialize to invalid AABB
    _AABB():
        x1(std::numeric_limits<T>::max()),
        x2(std::numeric_limits<T>::min()),
        y1(std::numeric_limits<T>::max()),
        y2(std::numeric_limits<T>::min())
    {}

    _AABB(T x1_, T x2_, T y1_, T y2_):
         x1(x1_),
         x2(x2_),
         y1(y1_),
         y2(y2_)
    {}
    inline void combine(const _AABB &other)
    {
        x1 = std::min(x1, other.x1);
        x2 = std::max(x2, other.x2);
        y1 = std::min(y1, other.y1);
        y2 = std::max(y2, other.y2);
    }
    template<class T2> inline void combine(const _MapCoord<T2> &coord)
    {
        x1 = std::min(x1, coord.x);
        x2 = std::max(x2, coord.x);
        y1 = std::min(y1, coord.y);
        y2 = std::max(y2, coord.y);
    }
    inline bool overlaps(const _AABB &other) const
    {
        bool x_overlap = (x1>=other.x1 && x1<=other.x2) || (other.x1>=x1 && other.x1<=x2);
        bool y_overlap = (y2>=other.y1 && y2<=other.y2) || (other.y2>=y1 && other.y2<=y2);
        return x_overlap && y_overlap;
    }
    inline bool contains(const _AABB &other) const
    {
        return x1<=other.x1 && x2>=other.x2 && y1<=other.y1 && y2>=other.y2;
    }
};
using AABB = _AABB<float>;

enum class MoveIntent: uint8_t {
    NotSet, Delete, StayAtCurrentPos, GoToDesiredPos,
};

class Shape
{
public:
    virtual ~Shape() = default;
    virtual void translate(float dx, float dy) = 0;
    virtual void rotate_about_origin(float theta) = 0;
    virtual AABB get_AABB() const = 0;
    virtual bool has_collision(const Shape &other) const = 0;
    virtual bool has_collision(const class Polygon &other) const = 0;

    template<class T> static std::unique_ptr<Shape> make_polygon(nonstd::span<_MapCoord<T>> vertices);
};

class Polygon: public Shape
{
    //note that one vertex is stored twice for efficiency purposes (i.e. begin()==rbegin())
    kx::FixedSizeArray<_MapCoord<float>> vertices;

public:
    template<class T> Polygon(nonstd::span<_MapCoord<T>> vertices_, kx::Passkey<Shape>):
        vertices(vertices_.size()+1)
    {
        for(size_t i=0; i<vertices_.size(); i++)
            vertices[i] = vertices_[i];
        vertices[vertices_.size()] = vertices[0];
    }

    size_t get_num_vertices() const
    {
        return vertices.size() - 1;
    }
    void translate(float dx, float dy) override
    {
        for(auto &coord: vertices) {
            coord.x += dx;
            coord.y += dy;
        }
    }
    void rotate_about_origin([[maybe_unused]] float theta) override
    {
        k_assert(false);
    }
    AABB get_AABB() const override
    {
        AABB ret;
        std::for_each(vertices.begin() + 1,
                      vertices.end(),
                      [&](_MapCoord<float> &x) -> void {ret.combine(x);});
        return ret;
    }
    bool has_collision(const Shape &other) const override
    {
        return other.has_collision(*this);
    }
    bool has_collision(const Polygon &other) const override
    {
        //This only checks for edge intersections; it doesn't check if one is inside the other.

        //https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
        //Note that this DOESN'T handle parallel lines properly, but that's usually OK.
        auto n = get_num_vertices();
        auto m = other.get_num_vertices();
        for(size_t i=1; i<n; i++) {
            for(size_t j=1; j<m; j++) {
                _MapCoord<float> P = vertices[i-1];
                _MapCoord<float> Q = other.vertices[j-1];
                _MapVec<float> R = vertices[i] - vertices[i-1];
                _MapVec<float> S = other.vertices[j] - other.vertices[j-1];

                auto part1 = Q - P;
                auto part3 = 1.0 / R.cross_prod(S);

                auto t = part1.cross_prod(S) * part3;
                auto u = part1.cross_prod(R) * part3;

                if(t>0 && t<1 && u>0 && u<1)
                    return true;
            }
        }

        return false;
    }
};

template<class T> std::unique_ptr<Shape> Shape::make_polygon(nonstd::span<_MapCoord<T>> vertices)
{
    ///I have to construct it like this or I get errors due to the Passkey
    std::unique_ptr<Shape> ret(new Polygon(vertices, kx::Passkey<Shape>()));
    return ret;
}

}
