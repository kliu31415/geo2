#include "geo2/level_gen/v1/level_gen1.h"

#include <numeric>
#include <random>

namespace geo2 { namespace level_gen { namespace v1 {

void RectangularRoomGenerator_1::set_target_tidi(double x)
{
    target_tidi = x;
}
void RectangularRoomGenerator_1::add_enemy_generation_rule(EnemyGenerationRule&& x)
{
    rules.emplace_back(std::move(x));
}
RoomGenGenerateResult RectangularRoomGenerator_1::generate(const RoomGenGenerateArgs&)
{
    RoomGenGenerateResult ret(125, 125);
    ret.tidi = 100;
    return ret;
}
RoomGenPlaceResult RectangularRoomGenerator_1::place(const RoomGenPlaceArgs&)
{
    return RoomGenPlaceResult();
}

LevelGenerator::LevelGenerator()
{}
Level LevelGenerator::generate(const LevelGenerationRule& level_gen_rule_, StandardRNG* rng_)
{
    level_gen_rule = &level_gen_rule_;
    rng = rng_;

    while(true) {
        init();
        generate_rooms();
        if(place_rooms() != LevelGeneratorResult::Failure)
            break;
        kx::log_warning("v1::LevelGenerator failed to place rooms. Starting over level generation.");
    }

    return level;
}
void LevelGenerator::init()
{
    tidi_used = 0;
    target_tidi = level_gen_rule->level_time_limit *
                  level_gen_rule->level_difficulty *
                  level_gen_rule->run_difficulty_setting;
    num_occurrences = kx::FixedSizeArray<int>(level_gen_rule->room_gen_rules.size());
    std::fill(num_occurrences.begin(), num_occurrences.end(), 0);

    level.is_timed = level_gen_rule->is_level_timed;
    level.time_limit = level_gen_rule->level_time_limit;
}
void LevelGenerator::generate_rooms()
{
    const auto &room_gen_rules = level_gen_rule->room_gen_rules;

    for(size_t i=0; i < room_gen_rules.size(); i++) {
        for(int j=0; j < room_gen_rules[i].min_occurences; j++) {
            generate_room(i);
        }
    }

    kx::FixedSizeArray<int> numbers_from_0_to_n(room_gen_rules.size() + 1);
    std::iota(numbers_from_0_to_n.begin(), numbers_from_0_to_n.end(), 0);
    kx::FixedSizeArray<double> selection_weights(room_gen_rules.size());
    while(tidi_used < target_tidi) {
        for(size_t i=0; i<room_gen_rules.size(); i++)
            selection_weights[i] = room_gen_rules[i].selection_weight_func(num_occurrences[i]);
        std::piecewise_constant_distribution<> distribution(numbers_from_0_to_n.begin(),
                                                            numbers_from_0_to_n.end(),
                                                            selection_weights.begin());
        generate_room((int)distribution(*rng));
        k_assert(rooms.size() != 10000,
                 "there is likely a bug that is causing too many rooms to be generated");
    }
}
LevelGeneratorResult LevelGenerator::place_rooms()
{
    BinaryGrid<GRID_LEN> occupied_tiles(GRID_LEN, GRID_LEN);
    for(size_t i=0; i<rooms.size(); i++) {
        if(place_room(&occupied_tiles, i) == LevelGeneratorResult::Failure)
            return LevelGeneratorResult::Failure;
    }
    return LevelGeneratorResult::Success;
}
void LevelGenerator::generate_room(size_t room_rule_idx)
{
    auto gen = level_gen_rule->room_gen_rules[room_rule_idx].make_room_generator();
    RoomGenGenerateArgs args;
    args.level_difficulty = level_gen_rule->level_difficulty;
    args.run_difficulty_setting = level_gen_rule->run_difficulty_setting;
    auto ret = gen->generate(args);
    rooms.emplace_back(std::move(gen), std::move(ret));
    num_occurrences[room_rule_idx]++;
}
LevelGeneratorResult LevelGenerator::place_room(BinaryGrid<GRID_LEN> *occupied_tiles, size_t room_idx)
{
    auto& room_tiles = std::get<1>(rooms[room_idx]).occupied_tiles;
    auto rows = room_tiles.get_num_rows();
    auto columns = room_tiles.get_num_columns();
    auto grid_data = (uint64_t*)room_tiles.get_grid_data_ptr();
    for(size_t r=0; r + rows < GRID_LEN; r++) {
        for(size_t c=0; c + columns < GRID_LEN; c++) {
            bool works = true;

            //right now, we're placing it in the first place that works... maybe we shouldn't

            if(works) {
                RoomGenPlaceArgs place_args;
                place_args.x = c;
                place_args.y = r;
                place_args.add_map_obj_func = [this](std::shared_ptr<map_obj::MapObject>&& map_obj) -> void
                                                {
                                                    level.map_objs.push_back(std::move(map_obj));
                                                };
                std::get<0>(rooms[room_idx])->place(place_args);
                return LevelGeneratorResult::Success;
            }
        }
    }
    return LevelGeneratorResult::Failure;
}

}}}
