#pragma once

#include "geo2/map_obj/unit_type1/player_type1.h"
#include "geo2/ceng1_obj.h"
#include "geo2/ceng1_data.h"
#include "geo2/rng.h"

#include "kx/gfx/renderer.h"
#include "kx/gfx/kwindow.h"

#include <memory>
#include <vector>
#include <cstdint>

namespace geo2 {

struct Level
{
    enum class Name {
        NotSet,
        Test1,
        Test2,
        Test3,
    };
    std::vector<std::shared_ptr<map_obj::MapObject>> to_add;
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs;
    std::vector<CEng1Data> ceng_data;
    double time_to_complete;
    double player_start_x;
    double player_start_y;
};

template<enum Level::Name> class LevelGenerator;

class Game final
{
    std::unique_ptr<struct _gfx> gfx; ///PImpl

    std::shared_ptr<map_obj::Player_Type1> player;
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs;

    Level::Name cur_level;
    int64_t cur_level_tick;
    double cur_level_time;
    double cur_level_time_left;

    std::shared_ptr<class ThreadPool> thread_pool;

    std::vector<StandardRNG> rngs;

    //these persistent across run() calls to save memory allocations
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs_to_add;
    std::vector<std::vector<std::shared_ptr<map_obj::MapObject>>> map_objs_to_add_lt;
    std::vector<int> idx_to_delete;
    std::vector<std::vector<int>> idx_to_delete_lt;
    std::vector<CEng1Data> ceng_data;
    std::unique_ptr<class CollisionEngine1> collision_engine;

    void generate_and_start_level(Level::Name level_name);

    void process_added_map_objs();
    void run1(double tick_len);
    void run_collision_engine();
    void advance_one_tick(double tick_len, int render_w, int render_h);

    std::shared_ptr<kx::gfx::Texture> render(kx::gfx::KWindowRunning *kwin_r,
                                             int render_w, int render_h);
public:
    Game();
    ~Game();

    ///noncopyable and nonmovable for safety
    Game(const Game&) = delete;
    Game &operator = (const Game&) = delete;
    Game(Game&&) = delete;
    Game &operator = (Game&&) = delete;

    std::shared_ptr<kx::gfx::Texture> run(kx::gfx::KWindowRunning *kwin_r,
                                          int render_w, int render_h);
};

}
