#include "geo2/game_render_op_list.h"

#include "kx/gfx/kwindow.h"
#include "kx/gfx/renderer.h"

namespace geo2 {

const std::string PATH = "geo2_data/shaders/map_objs/";

std::unique_ptr<GShader> make_shader_vf_1ub(kx::gfx::Renderer *rdr,
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
    auto ret = std::make_unique<GShader>(std::move(program), max_instances, draw_mode, count);
    ret->add_UB(ub_name, ub_offset, ub_size);
    return ret;
}

void GameRenderOpList::Shaders::init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>)
{
    using DM = kx::gfx::DrawMode;

    test_terrain1 = make_shader_vf_1ub(rdr, "floor_type1/test_terrain1",
                                       128, DM::TriangleStrip, 4,
                                       "block1", 0, 8*sizeof(float));

    outlined_tri = make_shader_vf_1ub(rdr, "outlined_tri",
                                      204, DM::Triangles, 3,
                                      "block1", 0, 20*sizeof(float));

    monoc_wall_1 = make_shader_vf_1ub(rdr, "wall_type1/monochromatic_wall_1",
                                      512, DM::TriangleStrip, 4,
                                      "block1", 0, 8*sizeof(float));

    monoc_floor_1 = make_shader_vf_1ub(rdr, "floor_type1/monochromatic_floor_1",
                                       512, DM::TriangleStrip, 4,
                                       "block1", 0, 8*sizeof(float));

    pig_1 = make_shader_vf_1ub(rdr, "unit_type1/pig_1",
                               113, DM::TriangleStrip, 8,
                               "block1", 0, 9*4*sizeof(float));
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
