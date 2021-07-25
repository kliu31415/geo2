#include "geo2/game_render_scene_graph.h"

#include "kx/gfx/kwindow.h"
#include "kx/gfx/renderer.h"

namespace geo2 {

static const std::string PATH = "geo2_data/shaders/";

class ShadersLoader
{

    std::map<std::string, std::unique_ptr<IShader>, std::less<>> *shaders_map;
    kx::gfx::Renderer *rdr;

    void make_shader_vf_1ub(std::string_view name,
                            std::string_view path,
                            int max_instances,
                            kx::gfx::DrawMode draw_mode,
                            int count,
                            int ub_size)
    {
        k_assert(shaders_map->count(name) == 0);
        auto vert = rdr->make_vert_shader(PATH + (std::string)path + ".vert");
        auto frag = rdr->make_frag_shader(PATH + (std::string)path + ".frag");
        auto program = rdr->make_shader_program(*vert, *frag);
        auto shader = std::make_unique<IShader>(std::move(program), max_instances, draw_mode, count);
        shader->add_UB("block1", 0, ub_size * 4 * sizeof(float));
        shaders_map->emplace(name, std::move(shader));
    }
public:
    ShadersLoader(decltype(shaders_map) shaders_map_, decltype(rdr) rdr_):
        shaders_map(shaders_map_),
        rdr(rdr_)
    {}
    void load()
    {
        using DM = kx::gfx::DrawMode;

        make_shader_vf_1ub("test_terrain_1",
                           "map_objs/floor_type1/test_terrain1",
                           128, DM::TriangleStrip, 4,
                           2);

        make_shader_vf_1ub("outlined_tri",
                           "outlined_tri",
                           204, DM::Triangles, 3,
                           5);

        make_shader_vf_1ub("monoc_wall_1",
                           "map_objs/wall_type1/monochromatic_wall_1",
                           512, DM::TriangleStrip, 4,
                           2);

        make_shader_vf_1ub("monoc_floor_1",
                           "map_objs/floor_type1/monochromatic_floor_1",
                           512, DM::TriangleStrip, 4,
                           2);

        make_shader_vf_1ub("pig_1",
                           "map_objs/unit_type1/pig_1",
                           204, DM::TriangleStrip, 4,
                           5);

        make_shader_vf_1ub("laser_proj_1",
                           "map_objs/proj_type1/laser_proj_1",
                           256, DM::Triangles, 3,
                           4);

        make_shader_vf_1ub("laser_1",
                           "weapons/laser_1",
                           256, DM::TriangleStrip, 4,
                           4);

        make_shader_vf_1ub("cow_1",
                           "map_objs/unit_type1/cow_1",
                           128, DM::TriangleStrip, 4,
                           8);

        make_shader_vf_1ub("hexfly_1",
                           "map_objs/unit_type1/hexfly_1",
                           170, DM::Triangles, 3,
                           6);

        make_shader_vf_1ub("resource_bar",
                           "hud/resource_bar",
                           204, DM::TriangleStrip, 4,
                           5);

        make_shader_vf_1ub("line",
                           "line",
                           512, DM::Lines, 2,
                           2);
    }
};



void GameRenderSceneGraph::Shaders::init(kx::gfx::Renderer *rdr, kx::Passkey<GameRenderSceneGraph>)
{
    //note that if the renderer changes, we still bug out. Why? We reload all the shaders,
    //but map objects might hold pointers to old shaders. If we want to remedy this, we
    //should copy assign any non-null shaders instead of reassigning the pointer.
    //I don't anticipate the renderer changing, so this shouldn't be an issue.

    ShadersLoader loader(&shaders_map, rdr);
    loader.load();
}
const IShader* GameRenderSceneGraph::Shaders::get(std::string_view name) const
{
    auto find_result = shaders_map.find(name);
    k_assert(find_result != shaders_map.end());
    return find_result->second.get();
}
void GameRenderSceneGraph::Fonts::init(kx::gfx::FontLibrary *font_library,
                                   kx::gfx::Renderer *rdr,
                                   kx::Passkey<GameRenderSceneGraph>)
{
    using namespace kx::gfx;
    fonts_map.emplace("black_chancery",
                      std::make_unique<FontAtlas>(rdr, font_library->get_font(FontLibrary::FONT_BLACK_CHANCERY).get()));
}
const FontAtlas *GameRenderSceneGraph::Fonts::get(std::string_view name) const
{
    auto find_result = fonts_map.find(name);
    k_assert(find_result != fonts_map.end());
    return find_result->second.get();
}

GameRenderSceneGraph::GameRenderSceneGraph(kx::Passkey<MasterInstanceGfxImpl>,
                                           kx::gfx::FontLibrary *font_library,
                                           kx::gfx::Renderer *renderer):
    cur_renderer(renderer)
{
    shaders.init(cur_renderer, {});
    fonts.init(font_library, cur_renderer, {});
}
}
