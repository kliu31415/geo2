#include "geo2/level_gen/v1/level_generator.h"

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

enum class LevelGeneratorResult {
    Success, Failure
};

struct LevelGenRoomInfo
{
    std::unique_ptr<RoomGenerator> generator;
    RoomGenGenerateResult generate_result;
    size_t x;
    size_t y;

    LevelGenRoomInfo(std::unique_ptr<RoomGenerator> generator_, RoomGenGenerateResult generate_result_):
        generator(std::move(generator_)),
        generate_result(std::move(generate_result_))
    {}
};

class LevelGenerator
{
    constexpr static size_t GRID_LEN = 512;

    const LevelGenerationRule* level_gen_rule;
    StandardRNG *rng;
    Level level;
    double tidi_used;
    double target_tidi;
    std::vector<LevelGenRoomInfo> rooms;
    kx::FixedSizeArray<int> num_occurrences;

    void init();
    void generate_rooms();
    LevelGeneratorResult place_rooms();
    void generate_room(size_t room_rule_idx);
    LevelGeneratorResult put_room_on_grid(BinaryGrid<GRID_LEN> *occupied_tiles, size_t room_idx);
public:
    Level operator () (const LevelGenerationRule&, StandardRNG* rng);
};

Level LevelGenerator::operator() (const LevelGenerationRule& level_gen_rule_, StandardRNG* rng_)
{
    level_gen_rule = &level_gen_rule_;
    rng = rng_;

    init();
    generate_rooms();
    [[maybe_unused]] auto place_rooms_result = place_rooms();
    k_assert(place_rooms_result != LevelGeneratorResult::Failure);

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
    std::shuffle(rooms.begin(), rooms.end(), *rng);

    BinaryGrid<GRID_LEN> occupied_tiles(GRID_LEN, GRID_LEN);
    for(size_t i=0; i<rooms.size(); i++) {
        if(put_room_on_grid(&occupied_tiles, i) == LevelGeneratorResult::Failure)
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
    auto result = gen->generate(args);
    rooms.emplace_back(std::move(gen), std::move(result));
    num_occurrences[room_rule_idx]++;
}

//-shifts all the tiles to right by hshift units
//-this requires 64 columns of buffer at the end
//-we just assume the last columns AND rows are empty to make the buffer length the same for rows and columns
template<size_t LEN>
BinaryGrid<LEN> make_shifted_tiles(const BinaryGrid<LEN>* tiles, uint64_t hshift)
{
    //special case that we can easily optimize for
    if(hshift == 0) {
        return *tiles;
    }

    BinaryGrid<LEN> shifted_tiles(LEN, LEN);

    auto room_tiles_data = (const uint64_t*)tiles->get_grid_data_ptr();
    auto shifted_tiles_data = (uint64_t*)shifted_tiles.get_grid_data_ptr();

    //c isn't the actual column; it's the uint64_t group, which represents up to 64 columns
    for(size_t c=0; c < LEN/64 - 1; c++) {
        for(size_t r=0; r < tiles->get_num_rows(); r++) {
            auto idx = r*LEN/64 + c;
            shifted_tiles_data[idx] |= (room_tiles_data[idx] >> hshift);
            shifted_tiles_data[idx+1] |= (room_tiles_data[idx] << (64 - hshift));
        }
    }
    return shifted_tiles;
}

template<size_t LEN_SRC, size_t LEN_DST>
BinaryGrid<LEN_DST> add_smeared_tiles(const BinaryGrid<LEN_DST>& dst_tiles,
                                      const BinaryGrid<LEN_SRC>& src_tiles,
                                      size_t dst_row,
                                      size_t dst_col,
                                      size_t num_iterations)
{
    //this requires 64 rows and columns of buffer at the ends
    k_expects(dst_row + LEN_SRC + 64 < LEN_DST);
    k_expects(dst_col + LEN_SRC + 64 < LEN_DST);

    //copy src to a blank grid
    BinaryGrid<LEN_DST> smeared_dst(LEN_DST, LEN_DST);
    auto smeared_dst_data = (uint64_t*)smeared_dst.get_grid_data_ptr();
    auto src_data = (uint64_t*)src_tiles.get_grid_data_ptr();
    for(size_t r=0; r<LEN_SRC; r++) {
        for(size_t c_idx=0; c_idx < LEN_SRC/64; c_idx++) {
            auto smeared_dst_idx = (dst_row + r) * LEN_DST/64 + c_idx + dst_col/64;
            auto src_idx = r * LEN_SRC/64 + c_idx;
            auto hshift = dst_col % 64;
            smeared_dst_data[smeared_dst_idx] |= (src_data[src_idx] >> hshift);
            smeared_dst_data[smeared_dst_idx + 1] |= (src_data[src_idx] << (64 - hshift));
        }
    }

    //smear src
    for(size_t iter=0; iter<num_iterations; iter++) {
        //vertical
        auto ROW_LEN = LEN_DST / 64;
        for(size_t idx = ROW_LEN; idx < ROW_LEN*LEN_DST; idx++) {
            smeared_dst_data[idx - ROW_LEN] |= smeared_dst_data[idx];
        }
        for(size_t idx = ROW_LEN*LEN_DST-1; idx >= ROW_LEN; idx--) {
            smeared_dst_data[idx] |= smeared_dst_data[idx - ROW_LEN];
        }

        //horizontal
        for(size_t r=0; r<LEN_DST; r++) {
            for(size_t c_idx=0; c_idx < ROW_LEN; c_idx++) {
                auto idx = r * ROW_LEN + c_idx;
                smeared_dst_data[idx] |= (smeared_dst_data[idx] << 1);
                if(c_idx + 1 != ROW_LEN) {
                    smeared_dst_data[idx] |= (smeared_dst_data[idx+1] >> 63);
                }
            }
        }

        for(size_t r=0; r<LEN_DST; r++) {
            for(size_t c_idx = ROW_LEN-1; c_idx != std::numeric_limits<size_t>::max(); c_idx--) {
                auto idx = r * ROW_LEN + c_idx;
                smeared_dst_data[idx] |= (smeared_dst_data[idx] >> 1);
                if(c_idx != 0) {
                    smeared_dst_data[idx] |= (smeared_dst_data[idx-1] << 63);
                }
            }
        }
    }

    //OR the smeared src with dst
    auto dst_data = (uint64_t*)dst_tiles.get_grid_data_ptr();
    for(size_t i=0; i < LEN_DST*LEN_DST/64; i++) {
        dst_data[i] |= smeared_dst_data[i];
    }
    return smeared_dst;
}
LevelGeneratorResult LevelGenerator::put_room_on_grid(BinaryGrid<GRID_LEN>* global_tiles, size_t room_idx)
{
    constexpr auto MIN_SPACE_BETWEEN_ROOMS = 5;

    auto& room_tiles = rooms[room_idx].generate_result.occupied_tiles;
    auto room_rows = room_tiles.get_num_rows();
    auto room_columns = room_tiles.get_num_columns();

    k_assert(room_rows <= MAX_ROOM_LEN - 64 && room_columns <= MAX_ROOM_LEN - 64);

    auto MAX_ROW = GRID_LEN - 64;
    auto MAX_COLUMN = GRID_LEN - 64;

    if(room_idx == 0) {
        auto r = (MAX_ROW - room_rows) / 2;
        auto c = (MAX_COLUMN - room_columns) / 2;
        add_smeared_tiles(*global_tiles, room_tiles, r, c, MIN_SPACE_BETWEEN_ROOMS);
        rooms[room_idx].x = c;
        rooms[room_idx].y = r;
        return LevelGeneratorResult::Success;
    }

    BinaryGrid<GRID_LEN> cant_place(GRID_LEN, GRID_LEN);

    for(size_t c=0; c + room_columns < MAX_COLUMN; c++) {
        auto hshift = c % 64;
        auto shifted_room_tiles = make_shifted_tiles(&room_tiles, hshift);
        auto shifted_global_times = make_shifted_tiles(global_tiles, hshift);
        auto shifted_room_tile_data = (uint64_t*)shifted_room_tiles.get_grid_data_ptr();
        auto shifted_global_tile_data = (uint64_t*)shifted_global_times.get_grid_data_ptr();

        auto max_column_idx = (room_columns + hshift - 1) / 64;
        auto global_column_offset = c / 64;
        for(size_t r=0; r + room_rows < MAX_ROW; r++) {
            for(size_t dr=0; dr<room_rows; dr++) {
                for(size_t dcol_idx = 0; dcol_idx < max_column_idx; dcol_idx++) {
                    auto room_tile_idx = dr * (MAX_ROOM_LEN/64) + dcol_idx;
                    auto global_tile_idx = (r + dr) * (GRID_LEN / 64) + global_column_offset + dcol_idx;
                    if((shifted_room_tile_data[room_tile_idx] & shifted_global_tile_data[global_tile_idx]) != 0) {
                        cant_place.set(r, c, true);
                        goto end_checking_this_tile;
                    }
                }
            }
            end_checking_this_tile: (void)0;
        }
    }

    size_t num_candidates = 0;
    for(size_t r=0; r<MAX_ROW; r++) {
        for(size_t c=0; c<MAX_COLUMN; c++) {
            num_candidates += (!cant_place.get(r, c));
        }
    }

    if(num_candidates == 0)
        return LevelGeneratorResult::Failure;

    auto candidate_select = std::uniform_int_distribution<size_t>(0, num_candidates - 1)(*rng);

    for(size_t r=0; r<MAX_ROW; r++) {
        for(size_t c=0; c<MAX_COLUMN; c++) {
            if(!cant_place.get(r, c)) {
                if(candidate_select == 0) {
                    add_smeared_tiles(*global_tiles, room_tiles, r, c, MIN_SPACE_BETWEEN_ROOMS);
                    rooms[room_idx].x = c;
                    rooms[room_idx].y = r;
                    return LevelGeneratorResult::Success;
                }
                candidate_select--;
            }
        }
    }

    /*
    RoomGenPlaceArgs place_args;
    place_args.x = //x;
    place_args.y = //y;
    place_args.add_map_obj_func = [this](std::shared_ptr<map_obj::MapObject>&& map_obj) -> void
                                    {
                                        level.map_objs.push_back(std::move(map_obj));
                                    };
    std::get<0>(rooms[room_idx])->place(place_args);
    return LevelGeneratorResult::Success;
    */
    return LevelGeneratorResult::Success;
}
Level LevelGenerationRule::generate(StandardRNG* rng) const
{
    return LevelGenerator()(*this, rng);
}

}}}
