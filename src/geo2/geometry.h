#pragma once

#include "kx/kx_span.h"

#include <memory>
#include <type_traits>
#include <algorithm>
#include <cmath>

namespace geo2 {

template<class T> struct _MapVec
{
    ///T should not be integral. It could be fixed, but fixeds don't really exist.
    static_assert(std::is_floating_point<T>::value);

    T x;
    T y;

    constexpr _MapVec() {}
    constexpr _MapVec(T x_, T y_):
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
    _MapVec operator - () const
    {
        return _MapVec(-x, -y);
    }
    _MapVec operator + (const _MapVec& other) const
    {
        return _MapVec(x + other.x, y + other.y);
    }
    _MapVec operator - (const _MapVec& other) const
    {
        return _MapVec(x - other.x, y - other.y);
    }
    template<class U> _MapVec operator * (U val) const
    {
        return _MapVec(x * val, y * val);
    }
    template<class U> _MapVec& operator *= (U val)
    {
        x *= val;
        y *= val;
        return *this;
    }
    template<class U> _MapVec& operator /= (U val)
    {
        x /= val;
        y /= val;
        return *this;
    }
    template<class U> _MapVec& operator += (const _MapVec<U> &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    template<class U> _MapVec& operator -= (const _MapVec<U> &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    template<class U> _MapVec operator / (U val) const
    {
        return _MapVec(x / val, y / val);
    }
    bool operator == (const _MapVec &other) const
    {
        return x==other.x && y==other.y;
    }
};
using MapVec = _MapVec<double>;

template<class T, class U, std::enable_if_t<std::is_fundamental_v<U>, int> = 0>
_MapVec<T> operator * (U c, _MapVec<T> vec)
{
    return _MapVec<T>(c*vec.x, c*vec.y);
}

template<class T> struct _MapCoord
{
    ///T should not be integral. It could be fixed, but fixeds don't really exist.
    static_assert(std::is_floating_point<T>::value);

    static constexpr _MapCoord ORIGIN = _MapCoord(0, 0);

    T x;
    T y;

    constexpr _MapCoord() {}
    constexpr _MapCoord(T x_, T y_):
        x(x_),
        y(y_)
    {}

    template<class U> _MapCoord operator = (const _MapCoord<U> &other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }
    _MapVec<T> operator - (const _MapCoord &other) const
    {
        return _MapVec<T>(x - other.x, y - other.y);
    }
    template<class U> _MapCoord operator + (const _MapVec<U> &other) const
    {
        return _MapCoord(x + other.x, y + other.y);
    }
    bool operator == (const _MapCoord &other) const
    {
        return x==other.x && y==other.y;
    }
    template<class U> T dot_prod(U v1, U v2) const
    {
        return x*v1 + y*v2;
    }
};
using MapCoord = _MapCoord<double>;

template<class T> struct _Matrix2
{
    static_assert(std::is_floating_point<T>::value);

    T a00;
    T a01;
    T a10;
    T a11;

    template<class U> _MapVec<U> operator * (const _MapVec<U> &vec) const
    {
        U x = a00*vec.x + a01*vec.y;
        U y = a10*vec.x + a11*vec.y;
        return _MapVec<U>(x, y);
    }
    inline static _Matrix2 make_rotation_matrix(T theta)
    {
        _Matrix2 ret;
        ret.a00 = std::cos(theta);
        ret.a10 = std::sin(theta);
        ret.a01 = -ret.a10;
        ret.a11 = ret.a00;
        return ret;
    }
};
using Matrix2 = _Matrix2<float>;

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
    ///invariant: x1 <= x2 and y1 <= y2 for all valid AABBs.
    ///The AABB covers [x1, x2) x [y1, y2). This means X and Y don't
    ///overlap if they touch on an edge, but they do if X==Y.
    T x1;
    T y1;
    T x2;
    T y2;

    _AABB()
    {}
    _AABB(T x1_, T y1_, T x2_, T y2_):
         x1(x1_),
         y1(y1_),
         x2(x2_),
         y2(y2_)
    {}
    inline void combine(const _AABB &other)
    {
        x1 = std::min(x1, other.x1);
        y1 = std::min(y1, other.y1);
        x2 = std::max(x2, other.x2);
        y2 = std::max(y2, other.y2);
    }
    template<class T2> inline void combine(const _MapCoord<T2> &coord)
    {
        x1 = std::min(x1, coord.x);
        y1 = std::min(y1, coord.y);
        x2 = std::max(x2, coord.x);
        y2 = std::max(y2, coord.y);
    }
    inline bool overlaps(const _AABB &other) const
    {
        bool x_overlap = (x1>=other.x1 && x1<other.x2) || (other.x1>=x1 && other.x1<x2);
        bool y_overlap = (y1>=other.y1 && y1<other.y2) || (other.y1>=y1 && other.y1<y2);
        return x_overlap && y_overlap;
    }
    inline bool contains(const _AABB &other) const
    {
        return x1<=other.x1 && x2>=other.x2 && y1<=other.y1 && y2>=other.y2;
    }
    static _AABB make_maxbad_AABB()
    {
        _AABB ret;
        ret.x1 = std::numeric_limits<T>::max();
        ret.y1 = std::numeric_limits<T>::max();
        ret.x2 = std::numeric_limits<T>::min();
        ret.y2 = std::numeric_limits<T>::min();
        return ret;
    }
};
using AABB = _AABB<float>;

enum class MoveIntent: uint8_t {
    NotSet,
    RemoveShapes,
    StayAtCurrentPos,
    GoToDesiredPos,
    GoToDesiredPosIfOtherDoesntCollide
};

/** This is a polygon class that uses AVX2
 *  The first coordinate will be repeated (this saves a mod instruction
 *  when looping over all edges). It's possible, but not guaranteed,
 *  that other coordinates will too. This means that, in a polygon P with
 *  n vertices, P[i] is valid for 0 <= i <= n (note that <= n instead of < n).
 */
class Polygon final
{
    AABB aabb;
    uint32_t n;

    static uint32_t get_d_len(uint32_t n);

    static void *operator new(size_t bytes, uint32_t n);
    static void *operator new[](size_t bytes) = delete;

    Polygon(uint32_t num_sides);
    template<class T> Polygon(kx::kx_span<_MapCoord<T>> vertices);
    uint32_t get_num_vertices() const;
    void calc_aabb();
    void translate_internal(float dx, float dy);
    void rotate_about_origin_internal(float theta);

    inline float *get_verts() const;
public:
    static constexpr size_t offset_of_aabb()
    {
        return offsetof(Polygon, aabb);
    }

    static void operator delete(void *ptr);
    std::unique_ptr<Polygon> copy() const;
    void copy_from(const Polygon *other);

    /** Polygons are only heap-allocatable; note that vertices are stored
     *  just past the end of the polygon, so sizeof(Polygon) is inaccurate,
     *  meaning they can't be allocated on the stack.
     */
    Polygon(const Polygon &other) = delete;
    Polygon &operator = (const Polygon &other);
    Polygon(Polygon &&other) = delete;
    Polygon & operator = (Polygon &&other) = delete;

    void translate(float dx, float dy);
    template<class T> void translate(const _MapVec<T> &v);

    void rotate_about_origin(float theta);

    template<class T> void rotate_about_origin_and_translate(float theta, const _MapVec<T> &v);

    template<class T> void remake(kx::kx_span<_MapCoord<T>> vertices);

    ///0 <= idx <= n (note the <= n instead of < n)
    _MapCoord<float> get_vertex(size_t idx) const;
    bool has_collision(const Polygon &other) const;

    inline const AABB &get_AABB() const
    {
        return aabb;
    }

    static std::unique_ptr<Polygon> make_with_num_sides(int num_sides);
    template<class T> static std::unique_ptr<Polygon> make(kx::kx_span<_MapCoord<T>> vertices);
    template<class T> static std::unique_ptr<Polygon> make(const std::vector<T> &coords);
};

}
