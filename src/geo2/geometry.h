#pragma once

#include <immintrin.h>

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

    _AABB()
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
    static _AABB make_maxbad_AABB()
    {
        _AABB ret;
        ret.x1 = std::numeric_limits<T>::max();
        ret.x2 = std::numeric_limits<T>::min();
        ret.y1 = std::numeric_limits<T>::max();
        ret.y2 = std::numeric_limits<T>::min();
        return ret;
    }
};
using AABB = _AABB<float>;

enum class MoveIntent: uint8_t {
    NotSet, Delete, StayAtCurrentPos, GoToDesiredPos,
};

/*
class Polygon final
{
    //note that one vertex is stored twice for efficiency purposes (i.e. begin()==rbegin())
    kx::FixedSizeArray<_MapCoord<float>> vertices;

    template<class T> Polygon(nonstd::span<_MapCoord<T>> vertices_):
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
public:
    void translate(float dx, float dy)
    {
        for(auto &coord: vertices) {
            coord.x += dx;
            coord.y += dy;
        }
    }
    void rotate_about_origin([[maybe_unused]] float theta)
    {
        k_assert(false);
    }
    AABB get_AABB() const
    {
        AABB ret = AABB::make_maxbad_AABB();
        std::for_each(vertices.begin() + 1,
                      vertices.end(),
                      [&](_MapCoord<float> &x) -> void {ret.combine(x);});
        return ret;
    }
    bool has_collision(const Polygon &other) const
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
                auto part3_rcp = 1.0 / R.cross_prod(S);

                auto t = part1.cross_prod(S) * part3_rcp;
                auto u = part1.cross_prod(R) * part3_rcp;

                if(t>0 && t<1 && u>0 && u<1)
                    return true;
            }
        }

        return false;
    }
    template<class T> static std::unique_ptr<Polygon> make(nonstd::span<_MapCoord<T>> vertices)
    {
        std::unique_ptr<Polygon> ret(new Polygon(vertices));
        return ret;
    }
};
*/

//weird speed bug in Level::Name::Test1
//This is a polygon class optimized for AVX2
class Polygon final
{
    AABB aabb;
    int n;
    float *vals;

    inline static int get_d_len(int n)
    {
        return 7 * ((n + 6) / 7) + 2;
    }

    template<class T> Polygon(nonstd::span<_MapCoord<T>> vertices):
        n(vertices.size())
    {
        auto d_len = get_d_len(n);
        vals = (float*)std::malloc(sizeof(float) * 2 * d_len);
        for(int i=0; i<n; i++) {
            vals[i] = vertices[i].x;
            vals[d_len + i] = vertices[i].y;
        }
        //we'll have some extra room, so just copy the last few sides
        //to fill up the space (we can't leave stuff uninitialized
        //because that could cause fake intersections). Note that we always
        //copy one of the vertices at least once; i.e. the vertices[0]
        //always appears more than once.
        for(int i=n; i<d_len; i++) {
            vals[i] = vals[i - n];
            vals[d_len + i] = vals[d_len + i - n];
        }

        calc_aabb();
    }
    int get_num_vertices() const
    {
        return n;
    }
    void calc_aabb()
    {
        auto d_len = get_d_len(n);
        aabb = AABB::make_maxbad_AABB();
        for(int i=0; i<n; i++) {
            _MapCoord<float> c(vals[i], vals[d_len + i]);
            aabb.combine(c);
        }
    }
public:
    ///too lazy to add support for copy/move rn, but there's no reason copy/move can't work
    Polygon(const Polygon &other) = delete;
    Polygon & operator = (const Polygon &other) = delete;
    Polygon(Polygon &&other) = delete;
    Polygon & operator = (Polygon &&other) = delete;

    ~Polygon()
    {
        std::free(vals);
    }

    void translate([[maybe_unused]] float dx, [[maybe_unused]] float dy)
    {
        calc_aabb();
        k_assert(false);
    }
    void rotate_about_origin([[maybe_unused]] float theta)
    {
        calc_aabb();
        k_assert(false);
    }
    inline const AABB &get_AABB() const
    {
        return aabb;
    }
    bool has_collision(const Polygon &other) const
    {
        //This only checks for edge intersections; it doesn't check if one is inside the other.

        //https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
        //Note that this DOESN'T handle parallel lines properly, but that's usually OK.

        auto this_n = get_num_vertices();
        auto other_n = other.get_num_vertices();
        auto this_d_len = get_d_len(this_n);
        auto other_d_len = get_d_len(other_n);

        const __m256 mm0 = _mm256_set1_ps(0);
        const __m256 mm1 = _mm256_set1_ps(1);

        std::array<uint64_t, 4> good_vals;

        for(int i=1; i<this_n; i++) {

            __m256 Px = _mm256_set1_ps(vals[i-1]);
            __m256 Py = _mm256_set1_ps(vals[this_d_len + i-1]);
            __m256 Rx = _mm256_set1_ps(vals[i] - vals[i-1]);
            __m256 Ry = _mm256_set1_ps(vals[this_d_len + i] - vals[this_d_len + i-1]);

            for(int j=1; j<other_d_len-1; j+=7) {

                __m256 Qx = _mm256_loadu_ps(other.vals + j-1);
                __m256 Qy = _mm256_loadu_ps(other.vals + other_d_len + j-1);

                __m256 p1_Sx = _mm256_loadu_ps(other.vals + j);
                __m256 p2_Sx = _mm256_loadu_ps(other.vals + j-1);
                __m256 Sx = p1_Sx - p2_Sx;

                __m256 p1_Sy = _mm256_loadu_ps(other.vals + other_d_len + j);
                __m256 p2_Sy = _mm256_loadu_ps(other.vals + other_d_len + j-1);
                __m256 Sy = p1_Sy - p2_Sy;

                __m256 Ry_times_Sx = Ry * Sx;
                __m256 part3 = _mm256_fmsub_ps(Rx, Sy, Ry_times_Sx);
                __m256 part3_rcp = _mm256_rcp_ps(part3);

                __m256 part1y = Qy - Py;
                __m256 part1x = Qx - Px;

                __m256 part1y_times_Sx = part1y * Sx;
                __m256 t_part1 = _mm256_fmsub_ps(part1x, Sy, part1y_times_Sx);
                __m256 t = t_part1 * part3_rcp;

                __m256 part1y_times_Rx = part1y * Rx;
                __m256 u_part1 = _mm256_fmsub_ps(part1x, Ry, part1y_times_Rx);
                __m256 u = u_part1 * part3_rcp;

                __m256 t_ge_0 = _mm256_cmp_ps(t, mm0, _CMP_GT_OQ);
                __m256 t_le_1 = _mm256_cmp_ps(t, mm1, _CMP_LT_OQ);
                __m256 t_good = _mm256_and_ps(t_ge_0, t_le_1);

                __m256 u_ge_0 = _mm256_cmp_ps(u, mm0, _CMP_GT_OQ);
                __m256 u_le_1 = _mm256_cmp_ps(u, mm1, _CMP_LT_OQ);
                __m256 u_good = _mm256_and_ps(u_ge_0, u_le_1);

                __m256 good = _mm256_and_ps(t_good, u_good);

                _mm256_storeu_ps((float*)good_vals.data(), good);

                //remember that the last value is bogus; we only operate on 7 things
                good_vals[0] |= good_vals[1];
                good_vals[2] |= *(uint32_t*)&good_vals[3]; //ignore the last 4 bytes

                if(good_vals[0] || good_vals[2])
                    return true;
            }
        }

        return false;
    }
    template<class T> static std::unique_ptr<Polygon> make(nonstd::span<_MapCoord<T>> vertices)
    {
        std::unique_ptr<Polygon> ret(new Polygon(vertices));
        return ret;
    }
};

}
