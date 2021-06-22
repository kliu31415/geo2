#pragma once

#include "geo2/map_obj/unit_type1/player_type1.h"

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
    std::vector<std::shared_ptr<map_obj::MapObject>> map_objs;
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
    double cur_level_time_left;

    std::shared_ptr<class ThreadPool> thread_pool;

    //these persistent across run() calls to save memory allocations
    std::vector<MoveIntent> move_intent;
    std::vector<CollisionEng1Obj> ceng_cur;
    std::vector<CollisionEng1Obj> ceng_des;
    std::unique_ptr<class CollisionEngine1> collision_engine;

    void generate_and_start_level(Level::Name level_name);

    void run1(double tick_len);
    void run_collision_engine();
    void advance_one_tick(double tick_len);

    void apply_bloom_and_hdr(kx::gfx::KWindowRunning *kwin_r,
                             kx::gfx::Texture *texture,
                             double bloom_radius_sd);
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
