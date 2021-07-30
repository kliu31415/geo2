#include "geo2/map_obj/floor_type1/test_terrain1.h"
#include "geo2/game_render_scene_graph.h"
#include "geo2/game.h"
#include "geo2/game_gfx.h"
#include "geo2/map_obj/map_object.h"
#include "geo2/collision_engine1.h"
#include "geo2/texture_utils.h"

#include "geo2/level_gen/test2.h"
#include "geo2/level_gen/test3.h"

#include "geo2/multithread/thread_pool.h"
#include "geo2/timer.h"

#include "kx/gfx/renderer.h"
#include "kx/log.h"
#include "kx/io.h"
#include "kx/stats.h"

#include <SDL2/SDL_scancode.h>

namespace geo2 {

//3600 corresponds to 16x16 tiles on a 1280x720 screen
//it's somewhat important to make tiles have roughly integer pixel dimensions to
//minimize artifacting around the edges
constexpr float TILES_PER_SCREEN = 3600;
constexpr int PREV_MOUSE_X_NOT_SET = -123456;

void Game::generate_and_start_level(LevelName level_name)
{
    using namespace level_gen;

    Level level;

    switch(level_name) {
    case LevelName::NotSet:
        kx::log_error("attempting to generate level NotSet, which is invalid");
        break;
    case LevelName::Test1:
        for(int i=0; i<200; i++) {
            for(int j=0; j<200; j++) {
                MapCoord pos(0.1*i, 0.1*j);
                level.map_objs.push_back(std::make_shared<map_obj::TestTerrain1>(pos));
            }
        }
        level.is_timed = true;
        level.time_limit = 120;
        level.player_start_x = 10;
        level.player_start_y = 10;
        break;
    case LevelName::Test2:
        level = NamedLevelGenerator<LevelName::Test2>().generate(this);
        break;
    case LevelName::Test3:
        level = NamedLevelGenerator<LevelName::Test3>().generate(this);
        break;
    default:
        kx::log_error("Game attempted to generate unknown level");
    }

    prev_mouse_x = PREV_MOUSE_X_NOT_SET;

    map_objs.clear();
    map_objs_to_add = std::move(level.map_objs);
    map_objs_to_add.push_back(player);
    process_added_map_objs();
    cur_level_time_left = level.time_limit;
    cur_level_time = 0;
    cur_level_tick = 0;
    cur_level_name = level_name;
    player->start_new_level({level.player_start_x, level.player_start_y}, {});
}

void Game::run_player(double tick_len,
                      MapCoord cursor_pos,
                      kx::gfx::mouse_state_t mouse_state,
                      kx::gfx::keyboard_state_t keyboard_state)
{
    map_obj::PlayerRunSpecialArgs player_args;
    player_args.tick_len = tick_len;

    auto keystate = keyboard_state;
    player_args.left_pressed = keystate[SDL_SCANCODE_A];
    player_args.right_pressed = keystate[SDL_SCANCODE_D];
    player_args.up_pressed = keystate[SDL_SCANCODE_W];
    player_args.down_pressed = keystate[SDL_SCANCODE_S];

    weapon::WeaponRunArgs weapon_args;
    weapon_args.set_map_objs_to_add(&map_objs_to_add);
    auto player_pos = player->get_position();
    weapon_args.cursor_pos = cursor_pos;
    weapon_args.tick_len = tick_len;
    weapon_args.cur_level_time = cur_level_time;
    weapon_args.mouse_state = mouse_state;
    weapon_args.angle = std::atan2(cursor_pos.y - player_pos.y, cursor_pos.x - player_pos.x);
    weapon_args.set_rng(&rngs[0]);
    player_args.weapon_run_args = weapon_args;

    player->run_special(player_args, {});
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
    run1_args.set_rng(&rngs[0]);

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
    idx_to_delete_lt.resize(num_threads);

    for(int t=num_threads-1; t>=0; t--) {
        int idx1 = (num_map_objs * (uint64_t)t) / num_threads;
        int idx2 = (num_map_objs * (uint64_t)(t+1)) / num_threads;
        auto task = [this, t, tick_len, idx1, idx2]
        {
            map_obj::MapObjRun1Args run1_args;
            run1_args.tick_len = tick_len;
            run1_args.set_ceng_data(&ceng_data);
            run1_args.set_map_objs_to_add(&map_objs_to_add_lt[t]);
            run1_args.set_rng(&rngs[t]);
            run1_args.set_idx_to_delete(&idx_to_delete_lt[t]);
            run1_args.cur_level_time = cur_level_time;

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
    for(auto &i: idx_to_delete_lt) {
        idx_to_delete.insert(idx_to_delete.end(), i.begin(), i.end());
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
        args.set_idx_to_delete(&idx_to_delete);
        args.set_ceng_data(&ceng_data);
        args.set_collision_info(collisions[i]);
        args.set_this(map_objs[idx2].get());
        args.set_other(map_objs[idx1].get());
        args.cur_level_time = cur_level_time;
        args.set_index(idx2);
        args.set_other_idx(idx1);
        //the order is SWAPPED; A->handle_collision(B) results in
        //B processing the collision and reporting its new intent
        auto prev_intent1 = ceng_data[idx1].get_move_intent();
        auto prev_intent2 = ceng_data[idx2].get_move_intent();

        map_objs[idx1]->handle_collision(map_objs[idx2].get(), args);
        args.swap();
        map_objs[idx2]->handle_collision(map_objs[idx1].get(), args);

        //these can't be run in parallel because they might depend on each other,
        //in particular, if MoveIntent::GoToDesiredPosIfOtherDoesntCollide is used.
        collision_engine->update_intent_after_collision(idx1, prev_intent1,
                                                        idx2, prev_intent2,
                                                        &collisions);
        collision_engine->update_intent_after_collision(idx2, prev_intent2,
                                                        idx1, prev_intent1,
                                                        &collisions);

        map_objs[idx1]->end_handle_collision_block({});
        map_objs[idx2]->end_handle_collision_block({});
    }
}
void Game::run3(double tick_len)
{
    /*
    map_obj::MapObjRun3Args run3_args;
    run3_args.tick_len = tick_len;
    run3_args.set_ceng_data(&ceng_data);
    run3_args.set_map_objs_to_add(&map_objs_to_add);
    run3_args.set_idx_to_delete(&idx_to_delete);
    run3_args.cur_level_time = cur_level_time;
    run3_args.set_rng(&rngs[0]);

    for(size_t i=0; i<map_objs.size(); i++) {
        run3_args.set_index(i);
        map_objs[i]->run3_mt(run3_args);
    }
    */
    int num_threads = thread_pool->size() + 1;
    std::vector<std::future<void>> is_done_futures(num_threads);

    int num_map_objs = map_objs.size();

    map_objs_to_add_lt.resize(num_threads);
    idx_to_delete_lt.resize(num_threads);

    for(int t=num_threads-1; t>=0; t--) {
        int idx1 = (num_map_objs * (uint64_t)t) / num_threads;
        int idx2 = (num_map_objs * (uint64_t)(t+1)) / num_threads;
        auto task = [this, t, tick_len, idx1, idx2]
        {
            map_obj::MapObjRun3Args run3_args;
            run3_args.tick_len = tick_len;
            run3_args.set_ceng_data(&ceng_data);
            run3_args.set_map_objs_to_add(&map_objs_to_add_lt[t]);
            run3_args.set_idx_to_delete(&idx_to_delete_lt[t]);
            run3_args.cur_level_time = cur_level_time;
            run3_args.set_rng(&rngs[t]);

            for(int i=idx1; i<idx2; i++) {
                run3_args.set_index(i);
                map_objs[i]->run3_mt(run3_args);
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
    for(auto &i: idx_to_delete_lt) {
        idx_to_delete.insert(idx_to_delete.end(), i.begin(), i.end());
        i.clear();
    }
}
void Game::process_added_map_objs()
{
    using namespace map_obj;

    #ifdef __GNUC__
    //optimization: if a map object doesn't override any of a certain set of functions,
    //then we can label it noncollidable and purely cosmetic and put it in a separate
    //vector that will only be used during rendering. This hack is only supported by some
    //compilers.
    size_t new_size = 0;
    for(size_t i=0; i<map_objs_to_add.size(); i++) {
        const auto &this_obj = *map_objs_to_add[i];
        //If we add run2_st and sfx as functions, then we should add those into the condition here too.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        if((void*)(&MapObject::init) == (void*)(this_obj.*(&MapObject::init)) &&
           (void*)(&MapObject::run1_mt) == (void*)(this_obj.*(&MapObject::run1_mt)) &&
           (void*)(&MapObject::run3_mt) == (void*)(this_obj.*(&MapObject::run3_mt)))
        #pragma GCC diagnostic pop
        {
            gfx_only_map_objs.push_back(std::move(map_objs_to_add[i]));
        } else {
            map_objs_to_add[new_size] = map_objs_to_add[i];
            new_size++;
        }
    }
    map_objs_to_add.resize(new_size);
    #endif

    //everything in map_objs_to_add is moved to map_objs
    ceng_data.resize(map_objs.size() + map_objs_to_add.size());
    MapObjInitArgs args;
    args.set_rng(&rngs[0]);
    size_t ceng_data_idx = map_objs.size();
    for(auto &mobj: map_objs_to_add) {
        args.set_ceng_data(&ceng_data[ceng_data_idx]);
        mobj->init(args);
        ceng_data_idx++;
    }
    map_objs.insert(map_objs.end(), map_objs_to_add.begin(), map_objs_to_add.end());
    map_objs_to_add.clear();
}
void Game::process_deleted_map_objs()
{
    //-remove all map objects that want to be removed
    //-note that we should preserve the order to prevent rendering glitches
    // (if two things have the same priority, then their order in map_objs
    // determines which one is rendered first, so should keep all relative
    // orders, (this is a similar concept to stable sort))
    if(!idx_to_delete.empty()) {
        int first_idx = std::numeric_limits<decltype(first_idx)>::max();
        //note that duplicate indices won't cause bugs (yet), but they're messy
        //so they're not recommended
        for(const auto &idx: idx_to_delete) {
            k_assert(idx < (int)map_objs.size());
            map_objs[idx] = nullptr;
            first_idx = std::min(first_idx, idx);
        }
        size_t after_idx = first_idx;
        for(size_t i=first_idx+1; i<map_objs.size(); i++) {
            if(map_objs[i] != nullptr) {
                ceng_data[after_idx] = std::move(ceng_data[i]);
                map_objs[after_idx] = std::move(map_objs[i]);
                after_idx++;
            }
        }
        ceng_data.resize(after_idx);
        map_objs.resize(after_idx);
        idx_to_delete.clear();
    }
}
void Game::advance_one_tick(double tick_len,
                            MapCoord cursor_pos,
                            kx::gfx::mouse_state_t mouse_state,
                            kx::gfx::keyboard_state_t keyboard_state)
{
    //move forward a tick
    cur_level_time += tick_len;
    cur_level_time_left -= tick_len;
    cur_level_tick++;

    /** Standard sequence:
     *  -Call run1_mt() on everything in parallel. All objects insert shapes representing
     *   their desired positions. These shapes will be fed into the collision engine. Not all
     *   objects will try to move at this stage; some objects can only calculate their
     *   movements/actions when they have exclusive access to data (i.e. things aren't running
     *   in parallel). Some objects (Noncollidables) won't insert desired positions because
     *   they don't interact with the collision engine.
     *  -Run the collision engine and find all collisions. Objects process all collisions,
     *   possibly moving back to their original place.
     *  -run2_st(); plan to add in the future
     *  -Call run3_mt() on everything. Objects will update their internal positions here.
     *   Objects that tried to move in run1_mt() will likely perform a simple update of
     *   their position. Objects that require sole access to data to update will likely
     *   perform a more complicated operation.
     */

    run_player(tick_len, cursor_pos, mouse_state, keyboard_state);

    //~100us on Test2(40, 40)
    run1(tick_len);

    //~400us on Test2(40, 40)
    run_collision_engine();

    run3(tick_len);

    process_deleted_map_objs();

    process_added_map_objs();
}
//the thread pool size is the number of threads we have - 1 because we should
//make use of the current thread too to reduce overhead
Game::Game(kx::Passkey<MasterInstance>):
    gfx(new GameGfx({})),
    player(std::make_unique<map_obj::Player_Type1>()),
    thread_pool(std::make_shared<ThreadPool>(std::thread::hardware_concurrency() - 1)),
    rngs(thread_pool->size() + 1),
    collision_engine(std::make_unique<CollisionEngine1>(thread_pool))
{
    /** Note:
     *  In order to make the program deterministic even if it multi-threaded, we have
     *  to ensure that no matter what thread in a thread pool executes a function,
     *  the result will appear the same; we might naively assume that if a thread pool
     *  has X threads and we give the pool X tasks in order, then thread i will execute
     *  task i, but this isn't true; perhaps a thread that is fast to wake up executes
     *  more than 1 task, and a slow thread to wake up executes none, as the task queue
     *  is empty by the time it wakes up.
     */

    generate_and_start_level(LevelName::Test3);
}
Game::~Game()
{}

inline float lerp(double a, double b, double t)
{
    return a*(1-t) + b*t;
}
std::shared_ptr<kx::gfx::Texture> Game::run(const LibraryPointers &libraries,
                                            kx::gfx::KWindowRunning *kwin_r,
                                            GameRenderSceneGraph *render_scene_graph,
                                            int render_w, int render_h)
{
    constexpr int TICKS_PER_FRAME = 10;

    auto gfx_library = libraries.gfx_library;

    //using lerped mouse positions allows for smoother laser beams when rapidly moving the mouse
    int mouse_x = gfx_library->get_mouse_x();
    int mouse_y = gfx_library->get_mouse_y();
    if(prev_mouse_x == PREV_MOUSE_X_NOT_SET) {
        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;
    }

    float tile_len = std::sqrt(render_w * render_h / TILES_PER_SCREEN);

    for(int i=0; i<TICKS_PER_FRAME; i++) {
        //~1300us on Test2(40, 40)
        float simulated_mouse_x = lerp(prev_mouse_x, mouse_x, (i+1)/((double)TICKS_PER_FRAME));
        float simulated_mouse_y = lerp(prev_mouse_y, mouse_y, (i+1)/((double)TICKS_PER_FRAME));

        auto offset = MapVec(simulated_mouse_x - 0.5f*render_w,
                             simulated_mouse_y - 0.5f*render_h)
                             / tile_len;

        advance_one_tick(1.0 / 1440.0,
                         player->get_position() + offset,
                         gfx_library->get_mouse_state(),
                         gfx_library->get_keyboard_state());
    }

    prev_mouse_x = mouse_x;
    prev_mouse_y = mouse_y;

    //a few hundred ms (integrated GPU, 1920x1080, Test3)
    GameGfxRenderArgs render_args;
    render_args.flags = 0;
    //render_args.flags |= GameGfxRenderArgs::FLAG_SHOW_HITBOXES;
    render_args.kwin_r = kwin_r;
    render_args.render_scene_graph = render_scene_graph;
    render_args.render_w = render_w;
    render_args.render_h = render_h;
    render_args.tile_len = tile_len;
    render_args.map_objs = &map_objs;
    render_args.gfx_only_map_objs = &gfx_only_map_objs;
    render_args.ceng_data = &ceng_data;
    render_args.player = player.get();
    render_args.rngs = &rngs;
    render_args.cur_level_time = cur_level_time;

    auto ret = gfx->render(render_args);
    return ret;
}
}
