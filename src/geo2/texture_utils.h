#pragma once

#include "kx/gfx/renderer.h"

#include <memory>
#include <cstring>

namespace geo2 {

class PersistentTextureTarget
{
    std::shared_ptr<kx::gfx::Texture> texture;
    kx::gfx::Renderer *owner;
public:
    std::shared_ptr<kx::gfx::Texture> get(kx::gfx::Renderer *rdr,
                                          int w,
                                          int h,
                                          kx::gfx::Texture::Format format,
                                          bool is_srgb,
                                          int num_samples = 1)
    {
        if(texture == nullptr ||
           rdr != owner ||
           texture->get_w()!=w ||
           texture->get_h()!=h ||
           texture->get_format()!=format ||
           texture->is_srgb() != is_srgb ||
           texture->get_num_samples() != num_samples)
        {
            owner = rdr;
            texture = rdr->make_texture_target(w,
                                               h,
                                               format,
                                               is_srgb,
                                               num_samples);
        }
        return texture;
    }
};

inline void set_to_full_target(std::array<float, 16> *full_target_, kx::gfx::Renderer *rdr, int w, int h)
{
    auto &full_target = *full_target_;

    full_target[0] = rdr->x_nc_to_ndc(0.0);
    full_target[1] = rdr->y_nc_to_ndc(0.0);
    full_target[2] = rdr->x_nc_to_tex_coord(0.0, w);
    full_target[3] = rdr->y_nc_to_tex_coord(0.0, h);

    full_target[4] = rdr->x_nc_to_ndc(1.0);
    full_target[5] = rdr->y_nc_to_ndc(0.0);
    full_target[6] = rdr->x_nc_to_tex_coord(1.0, w);
    full_target[7] = rdr->y_nc_to_tex_coord(0.0, h);

    full_target[8] = rdr->x_nc_to_ndc(0.0);
    full_target[9] = rdr->y_nc_to_ndc(1.0);
    full_target[10] = rdr->x_nc_to_tex_coord(0.0, w);
    full_target[11] = rdr->y_nc_to_tex_coord(1.0, h);

    full_target[12] = rdr->x_nc_to_ndc(1.0);
    full_target[13] = rdr->y_nc_to_ndc(1.0);
    full_target[14] = rdr->x_nc_to_tex_coord(1.0, w);
    full_target[15] = rdr->y_nc_to_tex_coord(1.0, h);
}

inline float int_bits_to_float(int32_t x)
{
    float ret;
    std::memcpy(&ret, &x, sizeof(ret));
    return ret;
}
inline float uint_bits_to_float(uint32_t x)
{
    float ret;
    std::memcpy(&ret, &x, sizeof(ret));
    return ret;
}
inline int get_f32_exponent(float arg)
{
    int exponent;
    std::frexp(arg, &exponent);
    return exponent;
}
inline float pack_as_two_f16(float x_, float y_)
{
    //dealing with nans and infs is possible, but it's slow and we
    //usually shouldn't be sending nans and infs to the GPU anyway
    k_assert(!std::isnan(x_) && !std::isinf(x_) && std::abs(get_f32_exponent(x_)) <= 14);
    k_assert(!std::isnan(y_) && !std::isinf(y_) && std::abs(get_f32_exponent(y_)) <= 14);
    //IEEE f16: 1 bit sign, 7 bit exponent, 24 bit mantissa
    uint32_t x;
    uint32_t y;
    std::memcpy(&x, &x_, sizeof(x));
    std::memcpy(&y, &y_, sizeof(y));
    uint32_t v = 0;
    constexpr auto SIGN_BIT = 1u<<31;
    v |= (x & SIGN_BIT);
    v |= (y & SIGN_BIT) >> 16;
    //right now, we truncate the mantissa instead of rounding it; this will cause
    //a greater loss of precision than necessary.
    constexpr auto EXPONENT_AND_MANTISSA = 0x1fffc000u;
    v |= (x & EXPONENT_AND_MANTISSA) << 2;
    v |= (y & EXPONENT_AND_MANTISSA) >> 14;
    return uint_bits_to_float(v);
}

}
