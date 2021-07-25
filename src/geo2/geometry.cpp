#include "geo2/geometry.h"
#include "kx/multithread/spinlock.h"

#include <immintrin.h>
#include <cstring>
#include <cstdlib>

//#define USE_POLYGON_ALLOCATOR

namespace geo2 {

inline constexpr uint32_t get_polygon_d_len(uint32_t n)
{
    return 8 * ((n + 7) / 8) + 1;
}
inline constexpr size_t get_polygon_size(uint32_t n)
{
    return sizeof(Polygon) + sizeof(float) * 2 * get_polygon_d_len(n);
}

class PolygonAllocator
{
    static constexpr auto ALL_ONES = std::numeric_limits<uint64_t>::max();
    static_assert(__builtin_popcountll(ALL_ONES) == 64);

    static constexpr size_t POLYGONS_PER_LEVEL = 1<<16;

    static constexpr size_t LEVEL_POLYGON_SIZE_BYTES[] {
        get_polygon_size(0*8 + 1),
        get_polygon_size(1*8 + 1),
        get_polygon_size(2*8 + 1),
        get_polygon_size(3*8 + 1)
    };

    static constexpr size_t LEVEL_SIZE_BYTES[] {
        POLYGONS_PER_LEVEL * LEVEL_POLYGON_SIZE_BYTES[0],
        POLYGONS_PER_LEVEL * LEVEL_POLYGON_SIZE_BYTES[1],
        POLYGONS_PER_LEVEL * LEVEL_POLYGON_SIZE_BYTES[2],
        POLYGONS_PER_LEVEL * LEVEL_POLYGON_SIZE_BYTES[3]
    };

    static constexpr size_t CUM_LEVEL_SIZE_BYTES[] {
        0,
        LEVEL_SIZE_BYTES[0],
        LEVEL_SIZE_BYTES[0] + LEVEL_SIZE_BYTES[1],
        LEVEL_SIZE_BYTES[0] + LEVEL_SIZE_BYTES[1] + LEVEL_SIZE_BYTES[2]
    };

    void *base_addr;
    //0 bit = in use, 1 bit = free
    uint64_t L1;
    uint64_t L2[64];
    uint64_t L3[64][64];
    kx::Spinlock<> lock;
public:
    PolygonAllocator()
    {
        for(int i=0; i<4; i++) {
            for(int j=1; j<8; j++)
                k_assert(get_polygon_size(i*8 + j) == get_polygon_size(i*8 + j + 1));
        }

        size_t num_bytes = 0;

        for(int i=0; i<4; i++)
            num_bytes += LEVEL_SIZE_BYTES[i];

        base_addr = std::malloc(num_bytes);
        if((uintptr_t)base_addr % 16 != 0)
            kx::log_error("bad PolygonAllocator buffer alignment (needs to be a multiple of 16)");

        L1 = ALL_ONES;
        std::fill(std::begin(L2), std::end(L2), ALL_ONES);
        for(auto &level: L3)
            std::fill(std::begin(level), std::end(level), ALL_ONES);
    }
    ~PolygonAllocator()
    {
        /* //unfortunately, we can't rely on the order of static object destruction;
           //i.e. perhaps this destructor is called before some static Polygons are destroyed
        auto lg = lock.get_lock_guard();
        k_expects(L1 == ALL_ONES); //if this is the case, all polygons have been freed
        */
        std::free(base_addr);
    }
    void *allocate(uint32_t n)
    {
        if(__builtin_expect(n<=0 || n>32, 0)) {
            kx::log_error("PolygonAllocator got polygon with bad size (n = " + kx::to_str(n) + ")");
            return nullptr;
        }
        auto level = (n-1)/8;

        auto lg = lock.get_lock_guard();

        auto relevant_L1 = (L1 & (((1ULL<<16) - 1) << (level*16)));
        if(__builtin_expect(relevant_L1 == 0, 0)) {
            kx::log_error("PolygonAllocator ran out of memory (level = " + kx::to_str(level) + ")");
            return nullptr;
        }

        auto L1_bit = __builtin_ctzll(relevant_L1);
        auto L2_bit = __builtin_ctzll(L2[L1_bit]);
        auto L3_bit = __builtin_ctzll(L3[L1_bit][L2_bit]);

        L3[L1_bit][L2_bit] ^= (1ULL << L3_bit);
        L2[L1_bit] ^= (((uint64_t)(L3[L1_bit][L2_bit] == 0)) << L2_bit);
        L1 ^= (((uint64_t)(L2[L1_bit] == 0)) << L1_bit);

        auto ret_addr = (uintptr_t)base_addr;
        ret_addr += CUM_LEVEL_SIZE_BYTES[level];
        ret_addr += (64*64) * (L1_bit >> (level*16)) * LEVEL_POLYGON_SIZE_BYTES[level];
        ret_addr += 64 * L2_bit * LEVEL_POLYGON_SIZE_BYTES[level];
        ret_addr += L3_bit * LEVEL_POLYGON_SIZE_BYTES[level];
        return (void*)ret_addr;
    }
    void deallocate(void *ptr)
    {
        auto offset = (uintptr_t)ptr - (uintptr_t)base_addr;

        int L1_offset = 0;

        int level = 0;
        for(level=0; level<3; level++) {
            if(offset < LEVEL_SIZE_BYTES[level]) {
                break;
            }
            offset -= LEVEL_SIZE_BYTES[level];
            L1_offset += 16;
        }
        offset /= LEVEL_POLYGON_SIZE_BYTES[level];

        auto L1_bit = L1_offset + offset / (64*64);
        auto L2_bit = (offset / 64) % 64;
        auto L3_bit = offset % 64;

        auto lg = lock.get_lock_guard();

        L3[L1_bit][L2_bit] |= (1ULL << L3_bit);
        L2[L1_bit] |= (1ULL << L2_bit);
        L1 |= (1ULL << L1_bit);
    }
};

inline PolygonAllocator *get_polygon_allocator()
{
    static PolygonAllocator alloc;
    return &alloc;
}
//Declaring this ensures that the static field in the above function will be constructed
//by the time main starts, which is good because it prevents data races
auto dummy_polygon = Polygon::make_with_num_sides(1);

//not necessary, but good for performance and ensures we've checked every field
static_assert(sizeof(Polygon) == 20); //20 = sizeof(int) + sizeof(AABB)

uint32_t Polygon::get_d_len(uint32_t n)
{
    return get_polygon_d_len(n);
}
void *Polygon::operator new([[maybe_unused]] size_t bytes, uint32_t n)
{
    #ifdef USE_POLYGON_ALLOCATOR
    return get_polygon_allocator()->allocate(n);
    #else
    //n==0 shouldn't happen in general; there are also functions here that assume >0 vertices
    k_expects(n > 0);
    return ::operator new(get_polygon_size(n));
    #endif
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
void Polygon::calc_aabb()
{
    auto verts = get_verts();

    auto d_len = get_d_len(n);

    //not much benefit to using AVX2 here because you would need to do a
    //horizontal min or max on a 8-float vector at the end
    aabb.x1 = verts[0];
    aabb.y1 = verts[d_len];
    aabb.x2 = verts[0];
    aabb.y2 = verts[d_len];
    for(uint32_t i=1; i<n; i++) {
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
inline float *Polygon::get_verts() const
{
    return (float*)((uint8_t*)this + sizeof(*this));
}
uint32_t Polygon::get_num_vertices() const
{
    return n;
}
void Polygon::operator delete(void *ptr)
{
    #ifdef USE_POLYGON_ALLOCATOR
    get_polygon_allocator()->deallocate(ptr);
    #else
    ::operator delete(ptr);
    #endif
}
std::unique_ptr<Polygon> Polygon::copy() const
{
    std::unique_ptr<Polygon> ret(new (n) Polygon(n));
    ret->aabb = aabb;
    auto d_len = get_d_len(n);
    std::copy(get_verts(), get_verts() + 2*d_len, ret->get_verts());
    return ret;
}
void Polygon::copy_from(const Polygon &other)
{
    k_expects(n == other.n);

    auto d_len = get_d_len(n);

    aabb = other.aabb;
    std::copy(other.get_verts(), other.get_verts() + 2*d_len, get_verts());
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

///0 <= idx <= n (note the <= n instead of < n)
_MapCoord<float> Polygon::get_vertex(size_t idx) const
{
    auto d_len = get_d_len(n);
    auto verts = get_verts();
    return _MapCoord<float>(verts[idx], verts[d_len + idx]);
}
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

    if(!this->get_AABB().overlaps(other.get_AABB()))
        return false;

    auto this_n = this->get_num_vertices();
    auto other_n = other.get_num_vertices();
    auto this_d_len = get_d_len(this_n);
    auto other_d_len = get_d_len(other_n);
    float* this_verts;
    float* other_verts;

    //check if it is faster to do it the other way
    if(this_n * other_d_len > other_n * this_d_len) {
        other_n = this->get_num_vertices();
        this_n = other.get_num_vertices();
        this_d_len = get_d_len(this_n);
        other_d_len = get_d_len(other_n);
        this_verts = other.get_verts();
        other_verts = get_verts();
    } else {
        this_verts = get_verts();
        other_verts = other.get_verts();
    }

    const auto mm0 = _mm256_set1_ps(0);
    const auto mm1 = _mm256_set1_ps(1);

    for(uint32_t i=1; i<=this_n; i++) {

        const auto Px = _mm256_set1_ps(this_verts[i-1]);
        const auto Py = _mm256_set1_ps(this_verts[this_d_len + i-1]);
        const auto Rx = _mm256_set1_ps(this_verts[i] - this_verts[i-1]);
        const auto Ry = _mm256_set1_ps(this_verts[this_d_len + i] - this_verts[this_d_len + i-1]);

        for(uint32_t j=1; j<other_d_len; j+=8) {

            auto Qx = _mm256_loadu_ps(other_verts + j-1);
            auto Qy = _mm256_loadu_ps(other_verts + other_d_len + j-1);

            auto Sx = _mm256_loadu_ps(other_verts + j) - Qx;
            auto Sy = _mm256_loadu_ps(other_verts + other_d_len + j) - Qy;

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
std::unique_ptr<Polygon> Polygon::make_with_num_sides(uint32_t num_sides)
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
