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
    std::map<kx::gfx::Texture*, PersistentTextureTarget> resolve_ms_func_texture;
    PersistentTextureTarget bloom_func_tex1;
    PersistentTextureTarget bloom_func_tex2;

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
        void add_ops(std::vector<std::shared_ptr<RenderOpGroup>> *op_groups, const GameGfxRenderArgs &args)
        {
            using kx::gfx::LinearColor;

            if(hp_op_group == nullptr) {
                auto rdr = args.kwin_r->rdr();

                float v0x = 0.05 * args.render_w;
                float v0y = 0.05 * args.render_h;
                float v3x = 0.30 * args.render_w;
                float v3y = 0.08 * args.render_h;
                constexpr float BORDER_Y = 0.12;

                auto hp_bar_op = std::make_unique<RenderOpShader>(*args.render_scene_graph->shaders.get("resource_bar"));
                auto iu_map = hp_bar_op->map_instance_uniform(0);
                hp_bar_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};
                hp_op_group = std::make_shared<RenderOpGroup>(HUD_RENDER_PRIORITY);
                hp_op_group->add_op(std::move(hp_bar_op));

                hp_text_op = std::make_shared<RenderOpText>();
                hp_text_op->set_font(args.render_scene_graph->fonts.get("black_chancery"));
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

            hp_bar_iu[4] = args.player->get_health() / args.player->get_max_health();

            std::string hp_text;
            hp_text += kx::to_str((int)std::ceil(args.player->get_health()));
            hp_text += " / ";
            hp_text += kx::to_str((int)args.player->get_max_health());
            hp_text_op->set_text(hp_text);
            op_groups->push_back(hp_op_group);
        }
    };
    _PlayerResourceBars player_resource_bars;

    void render_hitboxes(const GameGfxRenderArgs &args, const map_obj::MapObjRenderArgs &map_obj_args)
    {
        auto hitbox_op_group = std::make_shared<RenderOpGroup>(0);
        auto add_lines = [&hitbox_op_group, &args, &map_obj_args]
                         (const Polygon *polygon, [[maybe_unused]] int shape_id)
                        {
                            for(size_t i=1; i<=polygon->get_num_vertices(); i++) {
                                auto shader = args.render_scene_graph->shaders.get("line");
                                auto op = std::make_unique<RenderOpShader>(*shader);
                                auto iu_map = op->map_instance_uniform(0);
                                kx::kx_span<float> op_iu((float*)iu_map.begin(), (float*)iu_map.end());

                                auto v0 = polygon->get_vertex(i-1);
                                auto v1 = polygon->get_vertex(i);
                                op_iu[0] = map_obj_args.x_to_ndc(v0.x);
                                op_iu[1] = map_obj_args.y_to_ndc(v0.y);
                                op_iu[2] = map_obj_args.x_to_ndc(v1.x);
                                op_iu[3] = map_obj_args.y_to_ndc(v1.y);

                                if(map_obj_args.is_x_line_ndc_in_view(op_iu[0], op_iu[2]) &&
                                   map_obj_args.is_y_line_ndc_in_view(op_iu[1], op_iu[3]))
                                {
                                    op_iu[4] = 0.1;
                                    op_iu[5] = 2.0;
                                    op_iu[6] = 2.0;
                                    op_iu[7] = 0.95;

                                    hitbox_op_group->add_op(std::move(op));
                                }
                            }
                        };

        op_groups.push_back(hitbox_op_group);

        for(auto &d: *args.ceng_data) {
            if(d.get_move_intent() == MoveIntent::StayAtCurrentPos) {
                d.for_each_cur(add_lines);
            } else if(d.get_move_intent() == MoveIntent::GoToDesiredPos) {
                d.for_each_des(add_lines);
            }
        }

        args.render_scene_graph->render_and_clear_vec(&op_groups, args.kwin_r, args.render_w, args.render_h);
    }
public:
    PersistentTextureTarget render_func_return_texture;

    Impl(kx::gfx::Renderer *rdr)
    {
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
        auto no_ms_tex = resolve_ms_func_texture[tex].get(rdr,
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

        rdr->set_active_texture(0);

        //STEP 1: extract bright colors
        rdr->use_shader_program(*bloom1);
        bloom1->set_uniform1i(bloom1->get_uniform_loc("texture_in"), 0);
        rdr->bind_VAO(*bloom1_vao);
        rdr->bind_texture(*texture);
        rdr->set_target(tex1.get());
        rdr->clear(Color4f(0.0, 0.0, 0.0, 0.0));
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
        rdr->clear(Color4f(0.0, 0.0, 0.0, 0.0));
        rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

        rdr->bind_texture(*tex2);
        rdr->set_target(tex1.get());
        rdr->clear(Color4f(0.0, 0.0, 0.0, 0.0));
        bloom2->set_uniform1i(bloom2->get_uniform_loc("is_horizontal"), false);
        rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

        //STEP 3: sum the textures
        rdr->set_target(tex2.get());
        rdr->clear(Color4f(0.0, 0.0, 0.0, 0.0));
        rdr->set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::One);
        rdr->draw_texture_nc(*texture, Rect(0, 0, 1, 1));
        rdr->draw_texture_nc(*tex1, Rect(0, 0, 1, 1));

        //STEP 4: copy it back to the original texture
        rdr->set_blend_factors(BlendFactor::One, BlendFactor::Zero);
        rdr->set_target(texture);
        rdr->draw_texture_nc(*tex2, Rect(0, 0, 1, 1));
    }
    void render_map(const GameGfxRenderArgs &args)
    {
        using namespace kx::gfx;

        auto rdr = args.kwin_r->rdr();

        map_obj::MapObjRenderArgs map_obj_args;
        map_obj_args.set_renderer(rdr);
        map_obj_args.shaders = &args.render_scene_graph->shaders;
        map_obj_args.fonts = &args.render_scene_graph->fonts;

        kx::gfx::Rect camera;
        camera.w = args.render_w / args.tile_len;
        camera.h = args.render_h / args.tile_len;
        auto player_pos = args.player->get_position();
        camera.x = player_pos.x - 0.5f * camera.w;
        camera.y = player_pos.y - 0.5f * camera.h;
        map_obj_args.set_camera(camera);
        map_obj_args.pixels_per_tile_len = args.tile_len;
        map_obj_args.cur_level_time = args.cur_level_time;
        map_obj_args.set_rng(&(*args.rngs)[0]);
        map_obj_args.set_op_groups_vec(&op_groups);

        for(auto &map_obj: *args.map_objs) {
            map_obj->add_render_ops(map_obj_args);
        }
        for(auto &map_obj: *args.gfx_only_map_objs) {
            map_obj->add_render_ops(map_obj_args);
        }

        args.render_scene_graph->render_and_clear_vec(&op_groups, args.kwin_r, args.render_w, args.render_h);

        if(args.flags & GameGfxRenderArgs::FLAG_SHOW_HITBOXES)
            render_hitboxes(args, map_obj_args);
    }
    void render_HUD(const GameGfxRenderArgs &args)
    {
        using kx::gfx::LinearColor;

        player_resource_bars.add_ops(&op_groups, args);

        args.render_scene_graph->render_and_clear_vec(&op_groups, args.kwin_r, args.render_w, args.render_h);
    }
};


GameGfx::GameGfx(kx::Passkey<Game>)
{}
GameGfx::~GameGfx()
{}
std::shared_ptr<kx::gfx::Texture> GameGfx::render(const GameGfxRenderArgs &args)
{
    using namespace kx::gfx;

    //create the texture where the map will be rendered; note that this isn't the whole screen.
    auto rdr = args.kwin_r->rdr();
    if(impl==nullptr) {
        impl = std::make_unique<Impl>(rdr);
    }

    auto return_texture = impl->render_func_return_texture.get(rdr,
                                                               args.render_w,
                                                               args.render_h,
                                                               Texture::Format::RGB16F,
                                                               false,
                                                               1);
    rdr->set_target(return_texture.get());
    rdr->clear(kx::gfx::Color4f(0, 0, 0));

    impl->render_map(args);

    impl->render_HUD(args);

    if(return_texture->is_multisample()) {
        return_texture = impl->resolve_multisamples(rdr, return_texture.get());
    }

    impl->apply_bloom(rdr, return_texture.get(), 0.1*args.tile_len);

    return return_texture;
}

}
