#include "geo2/game_render_op_list.h"

#include "kx/gfx/kwindow.h"
#include "kx/gfx/renderer.h"

namespace geo2 {

void GameRenderOpList::Shaders::init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderOpList>)
{
    using DM = kx::gfx::DrawMode;
    std::string path = "geo2_data/shaders/map_objs/";

    auto test_terrain1_v = rdr->make_vert_shader(path + "floor_type1/test_terrain1.vert");
    auto test_terrain1_f = rdr->make_frag_shader(path + "floor_type1/test_terrain1.frag");
    auto test_terrain1_p = rdr->make_shader_program(*test_terrain1_v, *test_terrain1_f);
    test_terrain1 = std::make_unique<GShader>(std::move(test_terrain1_p), DM::TriangleStrip, 4);
    test_terrain1->add_UB("block1", 0, 8*sizeof(float));

    auto outlined_tri_v = rdr->make_vert_shader(path + "outlined_tri.vert");
    auto outlined_tri_f = rdr->make_frag_shader(path + "outlined_tri.frag");
    auto outlined_tri_p = rdr->make_shader_program(*outlined_tri_v, *outlined_tri_f);
    outlined_tri = std::make_unique<GShader>(std::move(outlined_tri_p), DM::Triangles, 3);
    outlined_tri->add_UB("block1", 0, 20*sizeof(float));
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
