#include "geo2/game_gfx.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/map_obj_args.h"
#include "geo2/map_obj/unit_type1/player_type1.h"
#include "geo2/texture_utils.h"

namespace geo2 {

class GameGfx::Impl
{
    static constexpr float HUD_RENDER_PRIORITY = 10000;

    std::vector<std::shared_ptr<RenderOpGroup>> op_groups;

    PersistentTextureTarget render_func_map_texture;
    PersistentTextureTarget resolve_ms_func_texture;
    PersistentTextureTarget bloom_func_tex1;
    PersistentTextureTarget bloom_func_tex2;

    std::unique_ptr<GameRenderOpList> render_op_list;

    std::unique_ptr<kx::gfx::ShaderProgram> bloom1;
    std::unique_ptr<kx::gfx::VAO> bloom1_vao;

    std::unique_ptr<kx::gfx::ShaderProgram> bloom2;
    std::unique_ptr<kx::gfx::VAO> bloom2_vao;

    std::shared_ptr<kx::gfx::VBO> full_target_vbo;
    std::array<float, 16> full_target;

    class _PlayerResourceBars
    {
        std::shared_ptr<RenderOpGroup> hp_op_group;
        std::shared_ptr<RenderOpText> hp_text_op;
        kx::kx_span<float> hp_bar_iu;
    public:
        void render(GameRenderOpList *render_op_list,
                    std::vector<std::shared_ptr<RenderOpGroup>> *op_groups,
                    kx::gfx::Renderer *rdr,
                    int render_w, int render_h,
                    map_obj::Player_Type1 *player)
        {
            using kx::gfx::LinearColor;

            if(hp_op_group == nullptr) {
                float v0x = 0.05 * render_w;
                float v0y = 0.05 * render_h;
                float v3x = 0.30 * render_w;
                float v3y = 0.08 * render_h;
                constexpr float BORDER_Y = 0.12;

                auto hp_bar_op = std::make_unique<RenderOpShader>(*render_op_list->shaders.get("resource_bar"));
                auto iu_map = hp_bar_op->map_instance_uniform(0);
                hp_bar_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};
                hp_op_group = std::make_shared<RenderOpGroup>(HUD_RENDER_PRIORITY);
                hp_op_group->add_op(std::move(hp_bar_op));

                hp_text_op = std::make_shared<RenderOpText>();
                hp_text_op->set_font(render_op_list->fonts.get("black_chancery"));
                hp_text_op->set_color(LinearColor(0, 1, 1, 0.98));
                hp_text_op->set_x(v0x + 2.0 * BORDER_Y * (v3y - v0y));
                hp_text_op->set_y(v0y + 1.4 * BORDER_Y * (v3y - v0y));
                hp_text_op->set_font_size(1.05 * (1 - 2.4*BORDER_Y) * (v3y - v0y));
                hp_op_group->add_op(hp_text_op);

                hp_bar_iu[0] = rdr->x_to_ndc(v0x);
                hp_bar_iu[1] = rdr->y_to_ndc(v0y);
                hp_bar_iu[2] = rdr->x_to_ndc(v3x);
                hp_bar_iu[3] = rdr->y_to_ndc(v3y);

                hp_bar_iu[6] = 1 - BORDER_Y;
                hp_bar_iu[5] = 1 - BORDER_Y * (v3y - v0y) / (v3x - v0x);

                *reinterpret_cast<LinearColor*>(&hp_bar_iu[8]) =  LinearColor(1, 0, 0, 0.95);
                *reinterpret_cast<LinearColor*>(&hp_bar_iu[12]) = LinearColor(0.1, 0, 0, 0.95);
                *reinterpret_cast<LinearColor*>(&hp_bar_iu[16]) = LinearColor(0.05, 0.05, 0.05, 0.95);
            }

            hp_bar_iu[4] = player->get_health() / player->get_max_health();

            std::string hp_text;
            hp_text += kx::to_str((int)std::ceil(player->get_health()));
            hp_text += " / ";
            hp_text += kx::to_str((int)player->get_max_health());
            hp_text_op->set_text(hp_text);
            op_groups->push_back(hp_op_group);
        }
    };
    _PlayerResourceBars player_resource_bars;
public:
    PersistentTextureTarget render_func_return_texture;

    Impl(kx::gfx::FontLibrary *font_library, kx::gfx::Renderer *rdr)
    {
        render_op_list = std::make_unique<GameRenderOpList>(font_library, rdr);

        auto bloom1_vert = rdr->make_vert_shader("geo2_data/shaders/bloom1_1.vert");
        auto bloom1_frag = rdr->make_frag_shader("geo2_data/shaders/bloom1_1.frag");
        bloom1 = rdr->make_shader_program(*bloom1_vert, *bloom1_frag);
        bloom1_vao = rdr->make_VAO();
        full_target_vbo = rdr->make_VBO();
        rdr->bind_VAO(*bloom1_vao);
        rdr->bind_VBO(*full_target_vbo);
        full_target_vbo->buffer_data(nullptr, full_target.size() * sizeof(full_target[0]));
        bloom1_vao->add_VBO(full_target_vbo);
        bloom1_vao->vertex_attrib_pointer_f(0, 2, 4*sizeof(float), 0*sizeof(float)); //dst loc
        bloom1_vao->vertex_attrib_pointer_f(1, 2, 4*sizeof(float), 2*sizeof(float)); //src loc
        bloom1_vao->enable_vertex_attrib_array(0);
        bloom1_vao->enable_vertex_attrib_array(1);

        auto bloom2_vert = rdr->make_vert_shader("geo2_data/shaders/bloom1_2.vert");
        auto bloom2_frag = rdr->make_frag_shader("geo2_data/shaders/bloom1_2.frag");
        bloom2 = rdr->make_shader_program(*bloom2_vert, *bloom2_frag);
        bloom2_vao = rdr->make_VAO();
        rdr->bind_VAO(*bloom2_vao);
        rdr->bind_VBO(*full_target_vbo);
        bloom2_vao->add_VBO(full_target_vbo);
        bloom2_vao->vertex_attrib_pointer_f(0, 2, 4*sizeof(float), 0*sizeof(float)); //dst loc
        bloom2_vao->vertex_attrib_pointer_f(1, 2, 4*sizeof(float), 2*sizeof(float)); //src loc
        bloom2_vao->enable_vertex_attrib_array(0);
        bloom2_vao->enable_vertex_attrib_array(1);
    }
    std::shared_ptr<kx::gfx::Texture> resolve_multisamples(kx::gfx::Renderer *rdr, kx::gfx::Texture *tex)
    {
        auto no_ms_tex = resolve_ms_func_texture.get(rdr,
                                                     tex->get_w(),
                                                     tex->get_h(),
                                                     tex->get_format(),
                                                     tex->is_srgb(),
                                                     1);
        rdr->set_target(no_ms_tex.get());
        rdr->draw_texture_ms(*tex, kx::gfx::Rect(0, 0, tex->get_w(), tex->get_h()), {});
        return no_ms_tex;
    }
    kx::FixedSizeArray<float> get_normal_pdf_sw(size_t num_iter, double bloom_radius_sd)
    {
        kx::FixedSizeArray<float> sw(num_iter*2 + 2);
        sw[0] = kx::stats::normal_cdf(0.5, 0.0, bloom_radius_sd) -
                kx::stats::normal_cdf(-0.5, 0.0, bloom_radius_sd);
        sw[1] = 0; //doesn't actually need to be set
        for(size_t i=1; i<=num_iter; i++) {
            float w1 = kx::stats::normal_cdf(i*2 - 0.5, 0.0, bloom_radius_sd) -
                       kx::stats::normal_cdf(i*2 - 1.5, 0.0, bloom_radius_sd);
            float w2 = kx::stats::normal_cdf(i*2 + 0.5, 0.0, bloom_radius_sd) -
                       kx::stats::normal_cdf(i*2 - 0.5, 0.0, bloom_radius_sd);
            sw[2*i] = w1 + w2;
            sw[2*i + 1] = i*2 - w1 / (w1 + w2);
        }
        return sw;
    }
    void apply_bloom(kx::gfx::Renderer *rdr, kx::gfx::Texture *texture, double bloom_radius_sd)
    {
        using namespace kx::gfx;

        k_expects(!texture->is_srgb());

        auto original_target = rdr->get_target();
        auto original_blend_factors = rdr->get_blend_factors();
        kx::ScopeGuard sg([=]() -> void {
                                            rdr->set_target(original_target);
                                            rdr->set_blend_factors(original_blend_factors);
                                        });

        int w = texture->get_w();
        int h = texture->get_h();

        set_to_full_target(&full_target, rdr, w, h);

        rdr->bind_VBO(*full_target_vbo);
        full_target_vbo->buffer_data(&full_target[0], full_target.size() * sizeof(full_target[0]));

        //the bloom shaders assume linear interpolation to speed things up,
        //so make sure linear interpolation is set. If it's not set, it'll be buggy.
        auto tex1 = bloom_func_tex1.get(rdr, w, h, Texture::Format::RGB16F, false);
        tex1->set_min_filter(Texture::FilterAlgo::Linear);
        tex1->set_mag_filter(Texture::FilterAlgo::Linear);
        auto tex2 = bloom_func_tex2.get(rdr, w, h, Texture::Format::RGB16F, false);
        tex2->set_min_filter(Texture::FilterAlgo::Linear);
        tex2->set_mag_filter(Texture::FilterAlgo::Linear);

        rdr->prepare_for_custom_shader();
        rdr->set_active_texture(0);

        //STEP 1: extract bright colors
        rdr->use_shader_program(*bloom1);
        bloom1->set_uniform1i(bloom1->get_uniform_loc("texture_in"), 0);
        rdr->bind_VAO(*bloom1_vao);
        rdr->bind_texture(*texture);
        rdr->set_target(tex1.get());
        rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
        rdr->clear();
        rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

        //STEP 2: blur (horizontal, then vertical)
        rdr->use_shader_program(*bloom2);
        constexpr int MAX_ITER = 49; //IMPORTANT: must be half - 1 of sw's size in the frag shader
        constexpr double RADIUS_SDs = 3.5;
        int num_iter = std::min(MAX_ITER, (int)std::ceil(bloom_radius_sd * (RADIUS_SDs / 2.0)));
        auto sw = get_normal_pdf_sw(num_iter, bloom_radius_sd);
        bloom2->set_uniform1i(bloom2->get_uniform_loc("texture_in"), 0);
        bloom2->set_uniform1fv(bloom2->get_uniform_loc("sw"), {sw.begin(), sw.end()});
        bloom2->set_uniform1i(bloom2->get_uniform_loc("num_iter"), num_iter);
        bloom2->set_uniform1i(bloom2->get_uniform_loc("is_horizontal"), true);

        rdr->bind_VAO(*bloom2_vao);
        rdr->bind_texture(*tex1);
        rdr->set_target(tex2.get());
        rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
        rdr->clear();
        rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

        rdr->bind_texture(*tex2);
        rdr->set_target(tex1.get());
        rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
        rdr->clear();
        bloom2->set_uniform1i(bloom2->get_uniform_loc("is_horizontal"), false);
        rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

        //STEP 3: sum the textures
        rdr->set_target(tex2.get());
        rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
        rdr->clear();
        rdr->set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::One);
        rdr->draw_texture_nc(*texture, Rect(0, 0, 1, 1));
        rdr->draw_texture_nc(*tex1, Rect(0, 0, 1, 1));

        //STEP 4: copy it back to the original texture
        rdr->set_blend_factors(BlendFactor::One, BlendFactor::Zero);
        rdr->set_target(texture);
        rdr->draw_texture_nc(*tex2, Rect(0, 0, 1, 1));
    }
    std::shared_ptr<kx::gfx::Texture> render_map(kx::gfx::KWindowRunning *kwin_r,
                                                 int map_render_w, int map_render_h,
                                                 float tile_len,
                                                 std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs,
                                                 map_obj::Player_Type1 *player,
                                                 std::vector<StandardRNG> *rngs,
                                                 double cur_level_time)
    {
        using namespace kx::gfx;
        auto rdr = kwin_r->rdr();

        auto map_texture = render_func_map_texture.get(rdr,
                                                       map_render_w,
                                                       map_render_h,
                                                       Texture::Format::RGB16F,
                                                       false,
                                                       1);

        rdr->set_target(map_texture.get());
        rdr->set_color(Color::BLACK);
        rdr->clear();

        //render the map and things on it
        map_obj::MapObjRenderArgs render_args;
        render_args.set_renderer(rdr);
        render_args.shaders = &render_op_list->shaders;
        render_args.fonts = &render_op_list->fonts;

        kx::gfx::Rect camera;
        camera.w = map_render_w / tile_len;
        camera.h = map_render_h / tile_len;
        auto player_pos = player->get_position();
        camera.x = player_pos.x - 0.5f * camera.w;
        camera.y = player_pos.y - 0.5f * camera.h;
        render_args.set_camera(camera);
        render_args.pixels_per_tile_len = tile_len;
        render_args.cur_level_time = cur_level_time;
        render_args.set_rng(&(*rngs)[0]);

        render_args.set_op_groups_vec(&op_groups);

        for(size_t i=0; i<map_objs->size(); i++) {
            (*map_objs)[i]->add_render_ops(render_args);
        }

        render_op_list->set_op_groups(std::move(op_groups));
        render_op_list->render(kwin_r, map_render_w, map_render_h);
        render_op_list->steal_op_groups_into(&op_groups);
        op_groups.clear();

        //if we have a multisample texture, resolve it into a 1-sample texture
        if(map_texture->is_multisample()) {
            map_texture = resolve_multisamples(rdr, map_texture.get());
        }
        return map_texture;
    }
    void render_HUD([[maybe_unused]] kx::gfx::Texture *texture,
                    kx::gfx::KWindowRunning *kwin_r,
                    int render_w, int render_h,
                    map_obj::Player_Type1 *player)
    {
        using kx::gfx::LinearColor;

        auto rdr = kwin_r->rdr();

        player_resource_bars.render(render_op_list.get(), &op_groups, rdr, render_w, render_h, player);

        render_op_list->set_op_groups(std::move(op_groups));
        render_op_list->render(kwin_r, render_w, render_h);
        render_op_list->steal_op_groups_into(&op_groups);
        op_groups.clear();
    }
};


GameGfx::GameGfx(kx::Passkey<Game>)
{}
GameGfx::~GameGfx()
{}
std::shared_ptr<kx::gfx::Texture> GameGfx::render(kx::gfx::FontLibrary *font_library,
                                                  kx::gfx::KWindowRunning *kwin_r,
                                                  int render_w, int render_h,
                                                  float tile_len,
                                                  std::vector<std::shared_ptr<map_obj::MapObject>> *map_objs,
                                                  map_obj::Player_Type1 *player,
                                                  std::vector<StandardRNG> *rngs,
                                                  double cur_level_time)
{
    using namespace kx::gfx;

    //create the texture where the map will be rendered; note that this isn't the whole screen.
    auto rdr = kwin_r->rdr();
    if(impl==nullptr) {
        impl = std::make_unique<Impl>(font_library, rdr);
    }

    float w = render_w;
    float h = render_h;
    int map_render_w = w;
    int map_render_h = h;
    auto map_texture = impl->render_map(kwin_r,
                                        map_render_w,
                                        map_render_h,
                                        tile_len,
                                        map_objs,
                                        player,
                                        rngs,
                                        cur_level_time);

    //combine the two textures
    auto return_texture = impl->render_func_return_texture.get(rdr,
                                                               (int)w,
                                                               (int)h,
                                                               Texture::Format::RGB16F,
                                                               false);

    map_texture->set_min_filter(Texture::FilterAlgo::Nearest);
    map_texture->set_mag_filter(Texture::FilterAlgo::Nearest);
    rdr->set_target(return_texture.get());
    rdr->draw_texture(*map_texture, Rect(0, 0, map_render_w, map_render_h));

    impl->render_HUD(return_texture.get(), kwin_r, render_w, render_h, player);

    impl->apply_bloom(rdr, return_texture.get(), 0.1*tile_len);

    return return_texture;
}

}
