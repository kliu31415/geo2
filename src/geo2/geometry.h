#pragma once

#include <immintrin.h>

#include "kx/fixed_size_array.h"

#include "kx/kx_span.h"
#include <type_traits>
#include <algorithm>
#include <memory>
#include <optional>
#include <cmath>
#include <vector>
#include <array>

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
        ret.x2 = std::numeric_limits<T>::min();
        ret.y1 = std::numeric_limits<T>::max();
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

/*
class Polygon final
{
    AABB aabb;
    //note that one vertex is stored twice for efficiency purposes (i.e. begin()==rbegin())
    kx::FixedSizeArray<_MapCoord<float>> vertices;

    template<class T> Polygon(kx::kx_span<_MapCoord<T>> vertices_):
        vertices(vertices_.size()+1)
    {
        for(size_t i=0; i<vertices_.size(); i++)
            vertices[i] = vertices_[i];
        vertices[vertices_.size()] = vertices[0];
        calc_aabb();
    }
    void calc_aabb()
    {
        aabb = AABB::make_maxbad_AABB();
        std::for_each(vertices.begin() + 1,
                      vertices.end(),
                      [&](_MapCoord<float> &x) -> void {aabb.combine(x);});
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
        calc_aabb();
    }
    void rotate_about_origin([[maybe_unused]] float theta)
    {
        calc_aabb();
        k_assert(false);
    }
    const AABB& get_AABB() const
    {
        return aabb;
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
    template<class T> static std::unique_ptr<Polygon> make(kx::kx_span<_MapCoord<T>> vertices)
    {
        std::unique_ptr<Polygon> ret(new Polygon(vertices));
        return ret;
    }
};
*/

/** This is a polygon class that uses AVX2
 *  The first coordinate will be repeated (this saves a mod instruction
 *  when looping over all edges). It's possible, but not guaranteed,
 *  that other coordinates will too. This means that, in a polygon P with
 *  n vertices, P[i] is valid for 0 <= i <= n (note that <= n instead of < n).
 */
class Polygon final
{
    AABB aabb;
    float *vals;
    int n;

    inline static int get_d_len(int n)
    {
        return 8 * ((n + 7) / 8) + 1;
    }

    Polygon()
    {}

    Polygon(int num_sides):
        n(num_sides)
    {
        auto d_len = get_d_len(n);
        vals = (float*)std::malloc(sizeof(float) * 2 * d_len);
    }

    template<class T> Polygon(kx::kx_span<_MapCoord<T>> vertices):
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
    void translate_internal(float dx, float dy)
    {
        //use d_len, not n, as we have to move duplicated vertices too
        auto d_len = get_d_len(n);
        k_expects(d_len % 8 == 1);

        vals[0] += dx;
        vals[d_len] += dy;

        auto mm_dx = _mm256_set1_ps(dx);
        auto mm_dy = _mm256_set1_ps(dy);

        for(int i=1; i<d_len; i+=8) {
            auto mm_x = _mm256_loadu_ps(vals + i);
            auto mm_y = _mm256_loadu_ps(vals + d_len + i);

            mm_x += mm_dx;
            mm_y += mm_dy;

            _mm256_storeu_ps(vals + i, mm_x);
            _mm256_storeu_ps(vals + d_len + i, mm_y);
        }
    }
    void rotate_about_origin_internal(float theta)
    {
        auto d_len = get_d_len(n);
        k_expects(d_len % 8 == 1);

        //use d_len, not n, as we have to move duplicated vertices too
        float cos_theta = std::cos(theta);
        float sin_theta = std::sin(theta);

        {
            auto cur_x = vals[0];
            auto cur_y = vals[d_len];
            vals[0] = cur_x * cos_theta - cur_y * sin_theta;
            vals[d_len] = cur_x * sin_theta + cur_y * cos_theta;
        }

        auto mm_cos_theta = _mm256_set1_ps(cos_theta);
        auto mm_sin_theta = _mm256_set1_ps(sin_theta);

        for(int i=1; i<d_len; i+=8) {
            auto mm_x = _mm256_loadu_ps(vals + i);
            auto mm_y = _mm256_loadu_ps(vals + d_len + i);

            _mm256_storeu_ps(vals + i, mm_x * mm_cos_theta - mm_y * mm_sin_theta);
            _mm256_storeu_ps(vals + d_len + i, mm_x * mm_sin_theta + mm_y * mm_cos_theta);
        }
    }
public:

    std::unique_ptr<Polygon> copy() const
    {
        std::unique_ptr<Polygon> ret(new Polygon());
        ret->aabb = aabb;
        ret->n = n;
        auto d_len = get_d_len(n);
        ret->vals = (float*)std::malloc(sizeof(float) * 2 * d_len);
        std::copy(vals, vals + 2*d_len, ret->vals);
        return ret;
    }
    void copy_from(const Polygon *other)
    {
        k_expects(n == other->n);

        auto d_len = get_d_len(n);

        aabb = other->aabb;
        std::copy(other->vals, other->vals + 2*d_len, vals);
    }
    /** Polygons are only heap-allocatable for future proofing reasons
     */
    Polygon(const Polygon &other) = delete;
    Polygon &operator = (const Polygon &other);
    Polygon(Polygon &&other) = delete;
    Polygon & operator = (Polygon &&other) = delete;

    ~Polygon()
    {
        std::free(vals);
    }

    void translate(float dx, float dy)
    {
        translate_internal(dx, dy);

        //this shouldn't lead to precision issues even if done many times
        aabb.x1 += dx;
        aabb.y1 += dy;
        aabb.x2 += dx;
        aabb.y2 += dy;
    }
    template<class T> void translate(const _MapVec<T> &v)
    {
        translate(v.x, v.y);
    }
    void rotate_about_origin(float theta)
    {
        rotate_about_origin_internal(theta);
        calc_aabb();
    }
    template<class T> void rotate_about_origin_and_translate(float theta, const _MapVec<T> &v)
    {
        rotate_about_origin_internal(theta);
        translate_internal(v.x, v.y);
        calc_aabb();
    }
    inline const AABB &get_AABB() const
    {
        return aabb;
    }
    bool has_collision(const Polygon &other) const
    {
        //TODO: ensure reflexivity, e.g. ensure a.has_collision(b) == b.has_collision(a).
        //Perhaps this might not always hold due to floating point inaccuracies; the
        //algorithm should be analyzed in more detail to make sure.
        //Reflexivity is important because the collision engine could bug if it doesn't hold,
        //i.e. two units move and the collision engine doesn't think they overlap, but then
        //the order in has_collision changes and the collision engine now thinks they do, so
        //the units are overlapping, which is an illegal state.

        //This only checks for edge intersections; it doesn't check if one is inside the other.

        //this probably bugs for parallel objects moving into each other

        //https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
        //Note that this DOESN'T handle parallel lines properly, but that's usually OK.

        auto this_n = get_num_vertices();
        auto other_n = other.get_num_vertices();
        auto this_d_len = get_d_len(this_n);
        auto other_d_len = get_d_len(other_n);

        //check if it is faster to do it the other way
        if(this_n * other_d_len > other_n * this_d_len)
            return other.has_collision(*this);

        if(!get_AABB().overlaps(other.get_AABB()))
            return false;

        const __m256 mm0 = _mm256_set1_ps(0);
        const __m256 mm1 = _mm256_set1_ps(1);

        for(int i=1; i<=this_n; i++) {

            __m256 Px = _mm256_set1_ps(vals[i-1]);
            __m256 Py = _mm256_set1_ps(vals[this_d_len + i-1]);
            __m256 Rx = _mm256_set1_ps(vals[i] - vals[i-1]);
            __m256 Ry = _mm256_set1_ps(vals[this_d_len + i] - vals[this_d_len + i-1]);

            for(int j=1; j<other_d_len; j+=8) {

                __m256 Qx = _mm256_loadu_ps(other.vals + j-1);
                __m256 Qy = _mm256_loadu_ps(other.vals + other_d_len + j-1);

                __m256 Sx = _mm256_loadu_ps(other.vals + j) - Qx;
                __m256 Sy = _mm256_loadu_ps(other.vals + other_d_len + j) - Qy;

                __m256 Ry_times_Sx = Ry * Sx;

                __m256 part1y = Qy - Py;
                __m256 part1x = Qx - Px;

                //multiplying by the reciprocal causes some edge cases to misbehave,
                //because the reciprocal function has precision issues,
                //so we have to use division (which is slower)
                __m256 part3 = _mm256_fmsub_ps(Rx, Sy, Ry_times_Sx);
                __m256 part3_inv = mm1 / part3;

                __m256 part1y_times_Sx = part1y * Sx;
                __m256 t_part1 = _mm256_fmsub_ps(part1x, Sy, part1y_times_Sx);
                __m256 t = t_part1 * part3_inv;

                __m256 part1y_times_Rx = part1y * Rx;
                __m256 u_part1 = _mm256_fmsub_ps(part1x, Ry, part1y_times_Rx);
                __m256 u = u_part1 * part3_inv;

                //use GE and LE to avoid having weird cases like when you're
                //walking along a wall in one direction, but suddenly stop
                //being able to due to a collision between the ends of lines
                __m256 t_ge_0 = _mm256_cmp_ps(t, mm0, _CMP_GT_OQ);
                __m256 u_ge_0 = _mm256_cmp_ps(u, mm0, _CMP_GT_OQ);
                __m256 t_le_1 = _mm256_cmp_ps(t, mm1, _CMP_LT_OQ);
                __m256 u_le_1 = _mm256_cmp_ps(u, mm1, _CMP_LT_OQ);

                __m256 t_good = _mm256_and_ps(t_ge_0, t_le_1);
                __m256 u_good = _mm256_and_ps(u_ge_0, u_le_1);

                __m256 good = _mm256_and_ps(t_good, u_good);
                if(_mm256_movemask_ps(good))
                    return true;
            }
        }

        return false;
    }
    template<class T> void remake(kx::kx_span<_MapCoord<T>> vertices)
    {
        int new_n = vertices.size();
        auto d_len = get_d_len(new_n);

        if(get_d_len(n) != get_d_len(new_n)) {
            std::free(vals);
            vals = (float*)std::malloc(sizeof(float) * 2 * d_len);
        }

        n = new_n;

        for(int i=0; i<n; i++) {
            vals[i] = vertices[i].x;
            vals[d_len + i] = vertices[i].y;
        }

        for(int i=n; i<d_len; i++) {
            vals[i] = vals[i - n];
            vals[d_len + i] = vals[d_len + i - n];
        }

        calc_aabb();
    }
    ///0 <= idx <= n (note the <= n instead of < n)
    inline _MapCoord<float> get_vertex(size_t idx) const
    {
        auto d_len = get_d_len(n);
        return _MapCoord<float>(vals[idx], vals[d_len + idx]);
    }
    template<class T> static std::unique_ptr<Polygon> make(kx::kx_span<_MapCoord<T>> vertices)
    {
        std::unique_ptr<Polygon> ret(new Polygon(vertices));
        return ret;
    }
    template<class T> static std::unique_ptr<Polygon> make(const std::vector<T> &coords)
    {
        k_expects(coords.size() % 2 == 0);
        std::vector<_MapCoord<T>> mc;
        for(size_t i=0; i<coords.size(); i+=2) {
            mc.emplace_back(coords[i], coords[i+1]);
        }
        return make(kx::kx_span<_MapCoord<T>>(mc.begin(), mc.end()));
    }
    static std::unique_ptr<Polygon> make_with_num_sides(int num_sides)
    {
        return std::unique_ptr<Polygon>(new Polygon(num_sides));
    }
};

}
