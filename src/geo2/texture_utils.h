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

inline float int_bits_to_float(int x)
{
    static_assert(sizeof(int) == sizeof(float));
    float ret;
    std::memcpy(&ret, &x, sizeof(ret));
    return ret;
}

}
