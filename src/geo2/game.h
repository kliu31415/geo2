#pragma once

#include "geo2/map_obj/unit/player_type1.h"
#include "geo2/ceng1_collision.h"
#include "geo2/ceng1_data.h"
#include "geo2/rng.h"
#include "geo2/library_pointers.h"
#include "geo2/level.h"

#include "kx/gfx/renderer.h"
#include "kx/gfx/kwindow.h"

#include <memory>
#include <vector>
#include <cstdint>

namespace geo2 {

class Game final
{
    std::unique_ptr<class GameGfx> gfx;

    std::shared_ptr<map_obj::Player_Type1> player;
    std::vector<std::shared_ptr<map_obj::MapObject>> gfx_only_map_objs;
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs;

    LevelName cur_level_name;
    int64_t cur_level_tick;
    double cur_level_time;
    double cur_level_time_left;

    int prev_mouse_x;
    int prev_mouse_y;

    std::shared_ptr<class ThreadPool> thread_pool;

    std::vector<StandardRNG> rngs;

    //these persistent across run() calls to save memory allocations
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs_to_add;
    std::vector<std::vector<std::shared_ptr<map_obj::MapObject>>> map_objs_to_add_lt;
    std::vector<int> idx_to_delete;
    std::vector<std::vector<int>> idx_to_delete_lt;
    std::vector<CEng1Data> ceng_data;
    std::unique_ptr<class CollisionEngine1> collision_engine;

    void generate_and_start_level(LevelName level_name);

    void run_player(double tick_len,
                    MapCoord cursor_pos,
                    kx::gfx::mouse_state_t mouse_state,
                    kx::gfx::keyboard_state_t keyboard_state);
    void run1(double tick_len);
    void run_collision_engine();
    void run3(double tick_len);
    void process_added_map_objs();
    void process_deleted_map_objs();

    void advance_one_tick(double tick_len,
                          MapCoord cursor_pos,
                          kx::gfx::mouse_state_t mouse_state,
                          kx::gfx::keyboard_state_t keyboard_state);
public:
    Game(kx::Passkey<class MasterInstance>);
    ~Game();

    ///noncopyable and nonmovable for safety
    Game(const Game&) = delete;
    Game &operator = (const Game&) = delete;
    Game(Game&&) = delete;
    Game &operator = (Game&&) = delete;

    std::shared_ptr<kx::gfx::Texture> run(const LibraryPointers &libraries,
                                          kx::gfx::KWindowRunning *kwin_r,
                                          GameRenderSceneGraph *render_scene_graph,
                                          int render_w, int render_h);
};

}
