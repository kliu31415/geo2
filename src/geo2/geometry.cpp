#include "geo2/geometry.h"

#include <immintrin.h>

namespace geo2 {

//not necessary, but good for performance and ensures we've checked every field
static_assert(sizeof(Polygon) == 20); //20 = sizeof(int) + sizeof(AABB)

uint32_t Polygon::get_d_len(uint32_t n)
{
    return 8 * ((n + 7) / 8) + 1;
}
void* Polygon::operator new(size_t bytes, uint32_t n)
{
    return ::operator new(bytes + sizeof(float) * 2 * get_d_len(n));
}
Polygon::Polygon(uint32_t num_sides):
    n(num_sides)
{}
template<class T> Polygon::Polygon(kx::kx_span<_MapCoord<T>> vertices):
    n(vertices.size())
{
    auto d_len = get_d_len(n);

    auto verts = get_verts();

    for(uint32_t i=0; i<n; i++) {
        verts[i] = vertices[i].x;
        verts[d_len + i] = vertices[i].y;
    }
    //we'll have some extra room, so just copy the last few sides
    //to fill up the space (we can't leave stuff uninitialized
    //because that could cause fake intersections). Note that we always
    //copy one of the vertices at least once; i.e. the vertices[0]
    //always appears more than once.
    for(uint32_t i=n; i<d_len; i++) {
        verts[i] = verts[i - n];
        verts[d_len + i] = verts[d_len + i - n];
    }

    calc_aabb();
}
uint32_t Polygon::get_num_vertices() const
{
    return n;
}
void Polygon::calc_aabb()
{
    auto verts = get_verts();

    auto d_len = get_d_len(n);
    aabb = AABB::make_maxbad_AABB();
    for(uint32_t i=0; i<n; i++) {
        _MapCoord<float> c(verts[i], verts[d_len + i]);
        aabb.combine(c);
    }
}
void Polygon::translate_internal(float dx, float dy)
{
    //use d_len, not n, as we have to move duplicated vertices too
    auto d_len = get_d_len(n);
    k_expects(d_len % 8 == 1);

    auto verts = get_verts();

    verts[0] += dx;
    verts[d_len] += dy;

    auto mm_dx = _mm256_set1_ps(dx);
    auto mm_dy = _mm256_set1_ps(dy);

    for(uint32_t i=1; i<d_len; i+=8) {
        auto mm_x = _mm256_loadu_ps(verts + i);
        auto mm_y = _mm256_loadu_ps(verts + d_len + i);

        mm_x += mm_dx;
        mm_y += mm_dy;

        _mm256_storeu_ps(verts + i, mm_x);
        _mm256_storeu_ps(verts + d_len + i, mm_y);
    }
}
void Polygon::rotate_about_origin_internal(float theta)
{
    auto d_len = get_d_len(n);
    k_expects(d_len % 8 == 1);

    auto verts = get_verts();

    //use d_len, not n, as we have to move duplicated vertices too
    auto cos_theta = std::cos(theta);
    auto sin_theta = std::sin(theta);

    {
        auto cur_x = verts[0];
        auto cur_y = verts[d_len];
        verts[0] = cur_x * cos_theta - cur_y * sin_theta;
        verts[d_len] = cur_x * sin_theta + cur_y * cos_theta;
    }

    auto mm_cos_theta = _mm256_set1_ps(cos_theta);
    auto mm_sin_theta = _mm256_set1_ps(sin_theta);

    for(uint32_t i=1; i<d_len; i+=8) {
        auto mm_x = _mm256_loadu_ps(verts + i);
        auto mm_y = _mm256_loadu_ps(verts + d_len + i);

        _mm256_storeu_ps(verts + i, mm_x * mm_cos_theta - mm_y * mm_sin_theta);
        _mm256_storeu_ps(verts + d_len + i, mm_x * mm_sin_theta + mm_y * mm_cos_theta);
    }
}
void Polygon::operator delete(void *ptr)
{
    ::operator delete(ptr);
}
std::unique_ptr<Polygon> Polygon::copy() const
{
    std::unique_ptr<Polygon> ret(new (n) Polygon(n));
    ret->aabb = aabb;
    auto d_len = get_d_len(n);
    std::copy(get_verts(), get_verts() + 2*d_len, ret->get_verts());
    return ret;
}
void Polygon::copy_from(const Polygon *other)
{
    k_expects(n == other->n);

    auto d_len = get_d_len(n);

    aabb = other->aabb;
    std::copy(other->get_verts(), other->get_verts() + 2*d_len, get_verts());
}
void Polygon::translate(float dx, float dy)
{
    translate_internal(dx, dy);

    //this shouldn't lead to precision issues even if done many times
    aabb.x1 += dx;
    aabb.y1 += dy;
    aabb.x2 += dx;
    aabb.y2 += dy;
}
template<class T> void Polygon::translate(const _MapVec<T> &v)
{
    translate(v.x, v.y);
}

template void Polygon::translate<float>(const _MapVec<float> &v);
template void Polygon::translate<double>(const _MapVec<double> &v);

void Polygon::rotate_about_origin(float theta)
{
    rotate_about_origin_internal(theta);
    calc_aabb();
}
template<class T> void Polygon::rotate_about_origin_and_translate(float theta, const _MapVec<T> &v)
{
    rotate_about_origin_internal(theta);
    translate_internal(v.x, v.y);
    calc_aabb();
}

template void Polygon::rotate_about_origin_and_translate<float>(float theta, const _MapVec<float> &v);
template void Polygon::rotate_about_origin_and_translate<double>(float theta, const _MapVec<double> &v);

template<class T> void Polygon::remake(kx::kx_span<_MapCoord<T>> vertices)
{
    auto new_n = vertices.size();
    k_expects(n == new_n);
    auto d_len = get_d_len(n);

    auto verts = get_verts();

    for(uint32_t i=0; i<n; i++) {
        verts[i] = vertices[i].x;
        verts[d_len + i] = vertices[i].y;
    }

    for(uint32_t i=n; i<d_len; i++) {
        verts[i] = verts[i - n];
        verts[d_len + i] = verts[d_len + i - n];
    }

    calc_aabb();
}

template void Polygon::remake<float>(kx::kx_span<_MapCoord<float>> vertices);
template void Polygon::remake<double>(kx::kx_span<_MapCoord<double>> vertices);

bool Polygon::has_collision(const Polygon &other) const
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

    const auto mm0 = _mm256_set1_ps(0);
    const auto mm1 = _mm256_set1_ps(1);

    auto verts = get_verts();

    for(uint32_t i=1; i<=this_n; i++) {

        const auto Px = _mm256_set1_ps(verts[i-1]);
        const auto Py = _mm256_set1_ps(verts[this_d_len + i-1]);
        const auto Rx = _mm256_set1_ps(verts[i] - verts[i-1]);
        const auto Ry = _mm256_set1_ps(verts[this_d_len + i] - verts[this_d_len + i-1]);

        for(uint32_t j=1; j<other_d_len; j+=8) {

            auto Qx = _mm256_loadu_ps(other.get_verts() + j-1);
            auto Qy = _mm256_loadu_ps(other.get_verts() + other_d_len + j-1);

            auto Sx = _mm256_loadu_ps(other.get_verts() + j) - Qx;
            auto Sy = _mm256_loadu_ps(other.get_verts() + other_d_len + j) - Qy;

            auto Ry_times_Sx = Ry * Sx;

            auto part1y = Qy - Py;
            auto part1x = Qx - Px;

            //multiplying by the reciprocal causes some edge cases to misbehave,
            //because the reciprocal function has precision issues,
            //so we have to use division (which is slower)
            auto part3 = _mm256_fmsub_ps(Rx, Sy, Ry_times_Sx);
            auto part3_inv = mm1 / part3;

            auto part1y_times_Sx = part1y * Sx;
            auto t_part1 = _mm256_fmsub_ps(part1x, Sy, part1y_times_Sx);
            auto t = t_part1 * part3_inv;

            auto part1y_times_Rx = part1y * Rx;
            auto u_part1 = _mm256_fmsub_ps(part1x, Ry, part1y_times_Rx);
            auto u = u_part1 * part3_inv;

            //use GE and LE to avoid having weird cases like when you're
            //walking along a wall in one direction, but suddenly stop
            //being able to due to a collision between the ends of lines
            auto t_ge_0 = _mm256_cmp_ps(t, mm0, _CMP_GT_OQ);
            auto u_ge_0 = _mm256_cmp_ps(u, mm0, _CMP_GT_OQ);
            auto t_le_1 = _mm256_cmp_ps(t, mm1, _CMP_LT_OQ);
            auto u_le_1 = _mm256_cmp_ps(u, mm1, _CMP_LT_OQ);

            auto t_good = _mm256_and_ps(t_ge_0, t_le_1);
            auto u_good = _mm256_and_ps(u_ge_0, u_le_1);

            auto good = _mm256_and_ps(t_good, u_good);
            if(_mm256_movemask_ps(good))
                return true;
        }
    }

    return false;
}
std::unique_ptr<Polygon> Polygon::make_with_num_sides(int num_sides)
{
    return std::unique_ptr<Polygon>(new (num_sides) Polygon(num_sides));
}
template<class T> std::unique_ptr<Polygon> Polygon::make(kx::kx_span<_MapCoord<T>> vertices)
{
    std::unique_ptr<Polygon> ret(new (vertices.size()) Polygon(vertices));
    return ret;
}

template std::unique_ptr<Polygon> Polygon::make<float>(kx::kx_span<_MapCoord<float>> vertices);
template std::unique_ptr<Polygon> Polygon::make<double>(kx::kx_span<_MapCoord<double>> vertices);

template<class T> std::unique_ptr<Polygon> Polygon::make(const std::vector<T> &coords)
{
    k_expects(coords.size() % 2 == 0);
    std::vector<_MapCoord<T>> mc;
    for(size_t i=0; i<coords.size(); i+=2) {
        mc.emplace_back(coords[i], coords[i+1]);
    }
    return make(kx::kx_span<_MapCoord<T>>(mc.begin(), mc.end()));
}

template std::unique_ptr<Polygon> Polygon::make<float>(const std::vector<float> &coords);
template std::unique_ptr<Polygon> Polygon::make<double>(const std::vector<double> &coords);

}
