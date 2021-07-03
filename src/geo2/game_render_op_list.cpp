#include "geo2/game_render_op_list.h"

#include "kx/gfx/kwindow.h"
#include "kx/gfx/renderer.h"

namespace geo2 {

const std::string PATH = "geo2_data/shaders/";

void make_shader_vf_1ub(std::unique_ptr<GShader> *shader,
                        kx::gfx::Renderer *rdr,
                        const std::string &path,
                        int max_instances,
                        kx::gfx::DrawMode draw_mode,
                        int count,
                        const std::string &ub_name,
                        int ub_offset,
                        int ub_size)
{
    auto vert = rdr->make_vert_shader(PATH + path + ".vert");
    auto frag = rdr->make_frag_shader(PATH + path + ".frag");
    auto program = rdr->make_shader_program(*vert, *frag);
    *shader = std::make_unique<GShader>(std::move(program), max_instances, draw_mode, count);
    (*shader)->add_UB(ub_name, ub_offset, ub_size);
}

void GameRenderOpList::Shaders::init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>)
{
    //note that if the renderer changes, we still bug out. Why? We reload all the shaders,
    //but map objects might hold pointers to old shaders. If we want to remedy this, we
    //should copy assign any non-null shaders instead of reassigning the pointer.
    //I don't anticipate the renderer changing, so this shouldn't be an issue.

    using DM = kx::gfx::DrawMode;

    make_shader_vf_1ub(&test_terrain_1, rdr, "map_objs/floor_type1/test_terrain1",
                       128, DM::TriangleStrip, 4,
                       "block1", 0, 8*sizeof(float));

    make_shader_vf_1ub(&outlined_tri, rdr, "outlined_tri",
                       204, DM::Triangles, 3,
                       "block1", 0, 20*sizeof(float));

    make_shader_vf_1ub(&monoc_wall_1, rdr, "map_objs/wall_type1/monochromatic_wall_1",
                       512, DM::TriangleStrip, 4,
                       "block1", 0, 8*sizeof(float));

    make_shader_vf_1ub(&monoc_floor_1, rdr, "map_objs/floor_type1/monochromatic_floor_1",
                       512, DM::TriangleStrip, 4,
                       "block1", 0, 8*sizeof(float));

    make_shader_vf_1ub(&pig_1, rdr, "map_objs/unit_type1/pig_1",
                       113, DM::TriangleStrip, 8,
                       "block1", 0, 9*4*sizeof(float));

    make_shader_vf_1ub(&laser_proj_1, rdr, "map_objs/proj_type1/laser_proj_1",
                       256, DM::Triangles, 3,
                       "block1", 0, 4*4*sizeof(float));

    make_shader_vf_1ub(&laser_1, rdr, "weapons/laser_1",
                       256, DM::TriangleStrip, 4,
                       "block1", 0, 4*4*sizeof(float));
}

GameRenderOpList::GameRenderOpList(kx::gfx::Renderer *renderer):
    cur_renderer(renderer)
{
    shaders.init(cur_renderer, {});
}
void GameRenderOpList::render([[maybe_unused]] const Game &game,
                              kx::gfx::KWindowRunning *kwin_r,
                              int render_w,
                              int render_h)
{
    //update shader global values


    //render
    auto rdr = kwin_r->rdr();
    if(cur_renderer != rdr) {
        cur_renderer = rdr;
        shaders.init(rdr, {});
    }
    render_internal(kwin_r, render_w, render_h);
}

}
