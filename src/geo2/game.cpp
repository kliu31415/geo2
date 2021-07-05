#include "geo2/map_obj/floor_type1/test_terrain1.h"
#include "geo2/game_render_op_list.h"
#include "geo2/game.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/map_obj/map_obj_args.h"
#include "geo2/collision_engine1.h"

#include "geo2/level_gen/test2.h"
#include "geo2/level_gen/test3.h"

#include "geo2/multithread/thread_pool.h"
#include "geo2/timer.h"

#include "kx/gfx/renderer.h"
#include "kx/log.h"
#include "kx/io.h"
#include "kx/stats.h"

#include <thread>
#include <cmath>

namespace geo2 {

struct _gfx
{
    kx::gfx::Renderer *cur_renderer;

    std::unique_ptr<GameRenderOpList> render_op_list;

    std::unique_ptr<kx::gfx::ShaderProgram> bloom1;
    std::unique_ptr<kx::gfx::VAO> bloom1_vao;

    std::unique_ptr<kx::gfx::ShaderProgram> bloom2;
    std::unique_ptr<kx::gfx::VAO> bloom2_vao;

    std::unique_ptr<kx::gfx::ShaderProgram> hdr;
    std::unique_ptr<kx::gfx::VAO> hdr_vao;

    std::shared_ptr<kx::gfx::VBO> full_target_vbo;
    kx::FixedSizeArray<float> full_target;

    void init(kx::gfx::Renderer *rdr)
    {
        cur_renderer = rdr;

        render_op_list = std::make_unique<GameRenderOpList>(rdr);

        full_target = kx::FixedSizeArray<float>(16);

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

        auto hdr_vert = rdr->make_vert_shader("geo2_data/shaders/hdr1.vert");
        auto hdr_frag = rdr->make_frag_shader("geo2_data/shaders/hdr1.frag");
        hdr = rdr->make_shader_program(*hdr_vert, *hdr_frag);
        hdr_vao = rdr->make_VAO();
        rdr->bind_VAO(*hdr_vao);
        rdr->bind_VBO(*full_target_vbo);
        hdr_vao->add_VBO(full_target_vbo);
        hdr_vao->vertex_attrib_pointer_f(0, 2, 4*sizeof(float), 0*sizeof(float)); //dst loc
        hdr_vao->vertex_attrib_pointer_f(1, 2, 4*sizeof(float), 2*sizeof(float)); //src loc
        hdr_vao->enable_vertex_attrib_array(0);
        hdr_vao->enable_vertex_attrib_array(1);
    }
    void resolve_multisamples(kx::gfx::Renderer *rdr, std::unique_ptr<kx::gfx::Texture> *tex)
    {
        auto no_ms_tex = rdr->make_texture_target((*tex)->get_w(),
                                                  (*tex)->get_h(),
                                                  (*tex)->get_format(),
                                                  (*tex)->is_srgb(),
                                                  1);
        rdr->set_target(no_ms_tex.get());
        rdr->draw_texture_ms(**tex, kx::gfx::Rect(0, 0, (*tex)->get_w(), (*tex)->get_h()), {});
        *tex = std::move(no_ms_tex);
    }
};

//3060 corresponds to 16x16 tiles on a 1280x720 screen
//it's somewhat important to make tiles have roughly integer pixel dimensions to
//minimize artifacting around the edges
constexpr float TILES_PER_SCREEN = 3060;
constexpr double MENU_OFFSET = 0.85;

void Game::generate_and_start_level(Level::Name level_name)
{
    using namespace kx;
    using namespace map_obj;

    Level level;

    switch(level_name) {
    case Level::Name::NotSet:
        log_error("attempting to generate level NotSet, which is invalid");
        break;
    case Level::Name::Test1: {
        for(int i=0; i<200; i++) {
            for(int j=0; j<200; j++) {
                MapCoord pos(i*0.2 + 1, j*0.2 + 1);
                level.map_objs.push_back(std::make_shared<TestTerrain1>(pos));
            }
        }
        level.time_to_complete = 120;
        level.player_start_x = 10;
        level.player_start_y = 10;
        break;}
    case Level::Name::Test2:
        level = LevelGenerator<Level::Name::Test2>().generate(this);
        break;
    case Level::Name::Test3:
        level = LevelGenerator<Level::Name::Test3>().generate(this);
        break;
    default:
        log_error("Game attempted to generate unknown level");
    }

    map_objs.clear();
    map_objs_to_add = std::move(level.map_objs);
    map_objs_to_add.push_back(player);
    process_added_map_objs();
    cur_level_time_left = level.time_to_complete;
    cur_level_time = 0;
    cur_level_tick = 0;
    cur_level = level_name;
    player->set_position({level.player_start_x, level.player_start_y}, {});
}
void Game::process_added_map_objs()
{
    ceng_data.resize(map_objs.size() + map_objs_to_add.size());
    map_obj::MapObjInitArgs args;
    size_t ceng_data_idx = map_objs.size();
    for(auto &mobj: map_objs_to_add) {
        args.set_ceng_data(&ceng_data[ceng_data_idx]);
        mobj->init(args);
        ceng_data_idx++;
    }
    map_objs.insert(map_objs.end(), map_objs_to_add.begin(), map_objs_to_add.end());
    map_objs_to_add.clear();
}
void Game::run1(double tick_len)
{
    for(auto &cdata: ceng_data)
        cdata.set_move_intent(MoveIntent::NotSet);

    //single threaded version takes ~300us
    /*map_obj::MapObjRun1Args run1_args({});
    run1_args.set_tick_len(tick_len);
    run1_args.set_ceng_data(&ceng_data);
    run1_args.set_map_objs_to_add(&map_objs_to_add);

    //don't use an enhanced for loop here because we might modify map_objs
    for(size_t i=0; i<map_objs.size(); i++) {
        run1_args.set_index(i);
        map_objs[i]->run1_mt(run1_args);
    }
    */

    //multithreaded version takes ~150us

    int num_threads = thread_pool->size() + 1;
    std::vector<std::future<void>> is_done_futures(num_threads);

    int num_map_objs = map_objs.size();

    map_objs_to_add_lt.resize(num_threads);

    for(int t=num_threads-1; t>=0; t--) {
        int idx1 = (num_map_objs * (uint64_t)t) / num_threads;
        int idx2 = (num_map_objs * (uint64_t)(t+1)) / num_threads;
        auto task = [this, t, tick_len, idx1, idx2]
        {
            map_obj::MapObjRun1Args run1_args;
            run1_args.set_tick_len(tick_len);
            run1_args.set_ceng_data(&ceng_data);
            run1_args.set_map_objs_to_add(&map_objs_to_add_lt[t]);

            for(int i=idx1; i<idx2; i++) {
                run1_args.set_index(i);
                map_objs[i]->run1_mt(run1_args);
            }
        };

        if(t == 0)
            task();
        else
            is_done_futures[t] = thread_pool->add_task(task);
    }

    for(int t=1; t<num_threads; t++)
        is_done_futures[t].get();

    for(auto &i: map_objs_to_add_lt) {
        map_objs_to_add.insert(map_objs_to_add.end(), i.begin(), i.end());
        i.clear();
    }
}
void Game::run_collision_engine()
{
    using map_obj::MapObject;
    auto collision_could_matter = [](const MapObject &a, const MapObject &b) -> bool
                                    {
                                        auto a_map_obj = (const MapObject*)&a;
                                        auto b_map_obj = (const MapObject*)&b;
                                        return a_map_obj->collision_could_matter(*b_map_obj);
                                    };

    //all reset/set combined are ~50-100us on Test2(40, 40)
    collision_engine->reset();
    collision_engine->set_ceng_data(&ceng_data);
    collision_engine->set2(&map_objs, std::move(collision_could_matter));

    //~500-550us on Test2(40, 40)
    auto collisions = collision_engine->find_collisions();

    //don't use a range-based loop, because collisions may be modified by
    //update_intent, which would invalidate iterators to it
    //~0us on Test2(40, 40)
    for(size_t i=0; i<collisions.size(); i++) {
        auto idx1 = collisions[i].idx1;
        auto idx2 = collisions[i].idx2;

        map_obj::HandleCollisionArgs args;
        args.set_ceng_data(&ceng_data);
        args.collision_info = collisions[i];
        args.other = map_objs[idx1].get();
        //the order has to be SWAPPED; A->handle_collision(B) results in
        //B processing the collision and reporting its new intent
        auto prev_intent1 = ceng_data[idx1].get_move_intent();
        auto prev_intent2 = ceng_data[idx2].get_move_intent();
        args.set_index(idx2);
        map_objs[idx1]->handle_collision(map_objs[idx2].get(), args);
        args.swap();
        args.other = map_objs[idx2].get();
        args.set_index(idx1);
        map_objs[idx2]->handle_collision(map_objs[idx1].get(), args);
        collision_engine->update_intent(idx1, prev_intent1, &collisions);
        collision_engine->update_intent(idx2, prev_intent2, &collisions);
    }
}
void Game::advance_one_tick(double tick_len, int render_w, int render_h)
{
    /** Standard sequence:
     *  -Call run1_mt() on everything in parallel. All objects insert shapes representing
     *   their desired positions. These shapes will be fed into the collision engine. Not all
     *   objects will try to move at this stage; some objects can only calculate their
     *   movements/actions when they have exclusive access to data (i.e. things aren't running
     *   in parallel). Some objects (Noncollidables) won't insert desired positions because
     *   they don't interact with the collision engine.
     *  -Run the collision engine and find all collisions. Objects process all collisions,
     *   possibly moving back to their original place.
     *  -Call run2_st() on everything. Objects will update their internal positions here.
     *   Objects that tried to move in run1_mt() will likely perform a simple update of
     *   their position. Objects that require sole access to data to update will likely
     *   perform a more complicated operation.
     */
    //process input
    map_obj::PlayerRunSpecialArgs player_args;
    player_args.tick_len = tick_len;
    auto mouse_x = kx::gfx::get_mouse_x();
    auto mouse_y = kx::gfx::get_mouse_y();
    player_args.mouse_x = mouse_x;
    player_args.mouse_y = mouse_y;

    auto keystate = kx::gfx::get_keyboard_state();
    player_args.left_pressed = keystate[SDL_SCANCODE_A];
    player_args.right_pressed = keystate[SDL_SCANCODE_D];
    player_args.up_pressed = keystate[SDL_SCANCODE_W];
    player_args.down_pressed = keystate[SDL_SCANCODE_S];

    weapon::WeaponRunArgs weapon_args;
    weapon_args.set_map_objs_to_add(&map_objs_to_add);
    float tile_len = std::sqrt(render_w * MENU_OFFSET * render_h / TILES_PER_SCREEN);
    auto offset = MapVec(mouse_x - 0.5f*MENU_OFFSET*render_w,
                         mouse_y - 0.5f*render_h)
                         / tile_len;
    auto player_pos = player->get_pos();
    weapon_args.set_owner_info(weapon::WeaponOwnerInfo(player_pos, map_obj::Player_Type1::WEAPON_OFFSET));
    auto cursor_pos = player_pos + offset;
    weapon_args.set_cursor_pos(cursor_pos);
    weapon_args.set_tick_len(tick_len);
    weapon_args.set_cur_level_time(cur_level_time);
    weapon_args.set_mouse_state(kx::gfx::get_mouse_state());
    weapon_args.set_angle(std::atan2(cursor_pos.y - player_pos.y, cursor_pos.x - player_pos.x));
    player_args.weapon_run_args = weapon_args;

    player->run_special(player_args, {});

    //~400us on Test2(40, 40)
    run1(tick_len);

    //~750us on Test2(40, 40)
    run_collision_engine();

    //second run
    map_obj::MapObjRun2Args run2_args;
    run2_args.set_tick_len(tick_len);
    run2_args.set_ceng_data(&ceng_data);
    run2_args.set_map_objs_to_add(&map_objs_to_add);
    //~150us on Test2(40, 40)
    for(size_t i=0; i<map_objs.size(); i++) {
        run2_args.set_index(i);
        map_objs[i]->run2_st(run2_args);
    }

    //remove all map objects that want to be removed
    size_t after_idx = 0;
    for(size_t i=0; i<map_objs.size(); i++) {
        if(ceng_data[i].get_move_intent() != MoveIntent::Delete) {
            if(i != after_idx) {
                ceng_data[after_idx] = std::move(ceng_data[i]);
                map_objs[after_idx] = std::move(map_objs[i]);
            }
            after_idx++;
        }
    }
    ceng_data.resize(after_idx);
    map_objs.resize(after_idx);

    //everything in map_objs_to_add is moved to map_objs
    process_added_map_objs();

    //move forward a tick
    cur_level_time += tick_len;
    cur_level_time_left -= tick_len;
    cur_level_tick++;
}
void Game::apply_bloom_and_hdr(kx::gfx::KWindowRunning *kwin_r,
                               kx::gfx::Texture *texture,
                               double bloom_radius_sd)
{
    using namespace kx::gfx;

    auto rdr = kwin_r->rdr();
    k_expects(!texture->is_srgb());
    auto cur_target = rdr->get_target();
    kx::ScopeGuard sg([=]() -> void {rdr->set_target(cur_target);});

    int w = texture->get_w();
    int h = texture->get_h();

    gfx->full_target[0] = rdr->x_nc_to_ndc(0.0);
    gfx->full_target[1] = rdr->y_nc_to_ndc(0.0);
    gfx->full_target[2] = rdr->x_nc_to_tex_coord(0.0, w);
    gfx->full_target[3] = rdr->y_nc_to_tex_coord(0.0, h);

    gfx->full_target[4] = rdr->x_nc_to_ndc(1.0);
    gfx->full_target[5] = rdr->y_nc_to_ndc(0.0);
    gfx->full_target[6] = rdr->x_nc_to_tex_coord(1.0, w);
    gfx->full_target[7] = rdr->y_nc_to_tex_coord(0.0, h);

    gfx->full_target[8] = rdr->x_nc_to_ndc(0.0);
    gfx->full_target[9] = rdr->y_nc_to_ndc(1.0);
    gfx->full_target[10] = rdr->x_nc_to_tex_coord(0.0, w);
    gfx->full_target[11] = rdr->y_nc_to_tex_coord(1.0, h);

    gfx->full_target[12] = rdr->x_nc_to_ndc(1.0);
    gfx->full_target[13] = rdr->y_nc_to_ndc(1.0);
    gfx->full_target[14] = rdr->x_nc_to_tex_coord(1.0, w);
    gfx->full_target[15] = rdr->y_nc_to_tex_coord(1.0, h);

    rdr->bind_VBO(*gfx->full_target_vbo);
    gfx->full_target_vbo->buffer_data(&gfx->full_target[0],
                                      gfx->full_target.size() * sizeof(gfx->full_target[0]));

    //the bloom shaders assume linear interpolation to speed things up,
    //so make sure linear interpolation is set. If it's not set, it'll be buggy.
    auto tex1 = rdr->make_texture_target(w, h, Texture::Format::RGB16F, false);
    tex1->set_min_filter(Texture::FilterAlgo::Linear);
    tex1->set_mag_filter(Texture::FilterAlgo::Linear);
    auto tex2 = rdr->make_texture_target(w, h, Texture::Format::RGB16F, false);
    tex2->set_min_filter(Texture::FilterAlgo::Linear);
    tex2->set_mag_filter(Texture::FilterAlgo::Linear);

    rdr->prepare_for_custom_shader();
    rdr->set_active_texture(0);

    //extract bright colors
    rdr->use_shader_program(*gfx->bloom1);
    gfx->bloom1->set_uniform1i(gfx->bloom1->get_uniform_loc("texture_in"), 0);
    rdr->bind_VAO(*gfx->bloom1_vao);
    rdr->bind_texture(*texture);
    rdr->set_target(tex1.get());
    rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
    rdr->clear();
    rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

    //blur (horizontal, then vertical)
    rdr->use_shader_program(*gfx->bloom2);
    constexpr int MAX_ITER = 49; //IMPORTANT: must be half - 1 of sw's size in the frag shader
    constexpr double RADIUS_SDs = 3.5;
    int num_iter = std::min(MAX_ITER, (int)std::ceil(bloom_radius_sd * (RADIUS_SDs / 2.0)));
    kx::FixedSizeArray<float> sw(num_iter*2 + 2);
    sw[0] = kx::stats::normal_cdf(0.5, 0.0, bloom_radius_sd) -
            kx::stats::normal_cdf(-0.5, 0.0, bloom_radius_sd);
    sw[1] = 0; //doesn't actually need to be set
    for(int i=1; i<=num_iter; i++) {
        float w1 = kx::stats::normal_cdf(i*2 - 0.5, 0.0, bloom_radius_sd) -
                   kx::stats::normal_cdf(i*2 - 1.5, 0.0, bloom_radius_sd);
        float w2 = kx::stats::normal_cdf(i*2 + 0.5, 0.0, bloom_radius_sd) -
                   kx::stats::normal_cdf(i*2 - 0.5, 0.0, bloom_radius_sd);
        sw[2*i] = w1 + w2;
        sw[2*i + 1] = i*2 - w1 / (w1 + w2);
    }
    gfx->bloom2->set_uniform1i(gfx->bloom2->get_uniform_loc("texture_in"), 0);
    gfx->bloom2->set_uniform1fv(gfx->bloom2->get_uniform_loc("sw"), {sw.begin(), sw.end()});
    gfx->bloom2->set_uniform1i(gfx->bloom2->get_uniform_loc("num_iter"), num_iter);
    gfx->bloom2->set_uniform1i(gfx->bloom2->get_uniform_loc("is_horizontal"), true);

    rdr->bind_VAO(*gfx->bloom2_vao);
    rdr->bind_texture(*tex1);
    rdr->set_target(tex2.get());
    rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
    rdr->clear();
    rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

    rdr->bind_texture(*tex2);
    rdr->set_target(tex1.get());
    rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
    rdr->clear();
    gfx->bloom2->set_uniform1i(gfx->bloom2->get_uniform_loc("is_horizontal"), false);
    rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);

    //sum the textures
    rdr->set_target(tex2.get());
    rdr->set_color(SRGB_Color(0.0, 0.0, 0.0, 0.0));
    rdr->clear();
    rdr->set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::One);
    rdr->draw_texture_nc(*texture, Rect(0.0, 0.0, 1.0, 1.0));
    rdr->draw_texture_nc(*tex1, Rect(0.0, 0.0, 1.0, 1.0));
    rdr->set_blend_factors(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    //apply HDR
    rdr->prepare_for_custom_shader();
    rdr->set_target(texture);
    rdr->bind_texture(*tex2);
    rdr->use_shader_program(*gfx->hdr);
    gfx->hdr->set_uniform1i(gfx->hdr->get_uniform_loc("texture_in"), 0);
    rdr->bind_VAO(*gfx->hdr_vao);
    rdr->draw_arrays(DrawMode::TriangleStrip, 0, 4);
}
std::shared_ptr<kx::gfx::Texture> Game::render(kx::gfx::KWindowRunning *kwin_r,
                                               int render_w, int render_h)
{
    using namespace kx::gfx;

    //create the texture where the map will be rendered; note that this isn't the whole screen.
    auto rdr = kwin_r->rdr();
    if(rdr != gfx->cur_renderer) {
        gfx->init(rdr);
    }
    float w = render_w;
    float h = render_h;
    int map_render_w = w * MENU_OFFSET;
    int map_render_h = h;
    auto map_texture = rdr->make_texture_target(map_render_w,
                                                map_render_h,
                                                Texture::Format::RGB16F,
                                                false,
                                                1);
    rdr->set_target(map_texture.get());
    rdr->set_color(Color::BLACK);
    rdr->clear();

    //render the map and things on it
    map_obj::MapObjRenderArgs render_args;
    render_args.set_renderer(kwin_r->rdr());
    render_args.set_shaders(&gfx->render_op_list->shaders);

    float tile_len = std::sqrt(map_render_w * map_render_h / TILES_PER_SCREEN);

    kx::gfx::Rect camera;
    camera.w = map_render_w / tile_len;
    camera.h = map_render_h / tile_len;
    auto player_pos = player->get_pos();
    camera.x = player_pos.x - 0.5f * camera.w;
    camera.y = player_pos.y - 0.5f * camera.h;
    render_args.set_camera(camera);
    render_args.set_pixels_per_tile_len(tile_len);

    std::vector<std::shared_ptr<RenderOpGroup>> op_groups;
    render_args.set_op_groups_vec(&op_groups);

    for(size_t i=0; i<map_objs.size(); i++) {
        map_objs[i]->add_render_objs(render_args);
    }

    gfx->render_op_list->add_op_groups(op_groups);
    gfx->render_op_list->render(*this, kwin_r, map_render_w, map_render_h);
    gfx->render_op_list->clear();


    //if we have a multisample texture, resolve it into a 1-sample texture
    if(map_texture->is_multisample()) {
        gfx->resolve_multisamples(rdr, &map_texture);
    }

    apply_bloom_and_hdr(kwin_r, map_texture.get(), 0.1*tile_len);

    //add the menu bar and other stuff here
    //(code goes here)

    //combine the two textures
    auto return_texture = rdr->make_texture_target((int)w,
                                                   (int)h,
                                                   Texture::Format::RGB16F,
                                                   false);
    map_texture->set_min_filter(Texture::FilterAlgo::Nearest);
    map_texture->set_mag_filter(Texture::FilterAlgo::Nearest);
    rdr->set_target(return_texture.get());
    rdr->draw_texture(*map_texture, Rect(0, 0, map_render_w, map_render_h));

    return return_texture;
}
//the thread pool size is the number of threads we have - 1 because we should
//make use of the current thread too to reduce overhead
Game::Game():
    gfx(std::make_unique<_gfx>()),
    player(std::make_unique<map_obj::Player_Type1>()),
    thread_pool(std::make_shared<ThreadPool>(std::thread::hardware_concurrency() - 1)),
    collision_engine(std::make_unique<CollisionEngine1>(thread_pool))
{
    generate_and_start_level(Level::Name::Test3);
}
Game::~Game()
{}
std::shared_ptr<kx::gfx::Texture> Game::run(kx::gfx::KWindowRunning *kwin_r,
                                            int render_w, int render_h)
{
    constexpr int TICKS_PER_FRAME = 10;
    for(int i=0; i<TICKS_PER_FRAME; i++) {
        //~1300us on Test2(40, 40)
        advance_one_tick(1.0 / 1440.0, render_w, render_h);
    }

    //~500us (integrated GPU, no MSAA, 1600x900, TILES_PER_SCREEN=2125) on Test2(40, 40)
    return render(kwin_r, render_w, render_h);
}
}
