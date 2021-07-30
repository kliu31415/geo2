#pragma once

#include "geo2/level.h"
#include "geo2/map_obj/unit/unit_rect_placement_info.h"
#include "geo2/rng.h"

#include "kx/fixed_size_array.h"

#include <vector>
#include <limits>

namespace geo2 { namespace level_gen { namespace v1 {

/** BinaryGrid contains any binary grid with side lengths of up to
 *  side_len. Why not just have a dynamically sized grid that contains
 *  exactly the desired number of tiles? It would take less memory, but
 *  it would be less CPU efficient, and we really want high CPU efficiency.
 *
 *  The data is guaranteed to be tightly packed bits in a 1D array.
 */
template<size_t side_len> struct BinaryGrid
{
    static_assert(side_len % 64 == 0);
    static constexpr auto ROW_MULT = side_len / 64;

    std::array<uint64_t, side_len*side_len/64> grid;
    size_t rows;
    size_t columns;
public:
    BinaryGrid(size_t rows_, size_t columns_):
        rows(rows_),
        columns(columns_)
    {
        k_assert(rows <= side_len);
        k_assert(columns <= side_len);
        std::fill(grid.begin(), grid.end(), 0);
    }
    inline void set(size_t row, size_t column, bool val)
    {
        k_assert(row<rows && column<columns);
        grid[row*ROW_MULT + column/64] |= 1 << (column % 64);
        grid[row*ROW_MULT + column/64] &= val << (column % 64);
    }
    inline bool get(size_t row, size_t column) const
    {
        k_assert(row<rows && column<columns);
        return (grid[row*ROW_MULT + column/64] >> (column % 64)) & 1;
    }
    inline size_t get_num_rows() const
    {
        return rows;
    }
    inline size_t get_num_columns() const
    {
        return columns;
    }
    inline const void* get_grid_data_ptr() const
    {
        return grid.data();
    }
};

struct EnemyGenerationRule
{
    int min_occurences = 0;
    std::function<double(int)> selection_weight_func;
    map_obj::UnitRectPlacementInfo placement_info;
};

struct RoomGenGenerateArgs
{
    double run_difficulty_setting; //e.g. a difficulty of 1.0 is normal
    double level_difficulty; //e.g. later levels have higher difficulties
};

struct RoomGenGenerateResult
{
    BinaryGrid<256> occupied_tiles;
    double tidi;

    RoomGenGenerateResult(size_t rows, size_t columns):
        occupied_tiles(rows, columns)
    {}
};

struct RoomGenPlaceArgs
{
    int x;
    int y;
    std::function<void(std::shared_ptr<map_obj::MapObject>&&)> add_map_obj_func;
};

struct RoomGenPlaceResult
{

};

class RoomGenerator
{
public:
    virtual ~RoomGenerator() = default;
    virtual RoomGenGenerateResult generate(const RoomGenGenerateArgs&) = 0;
    virtual RoomGenPlaceResult place(const RoomGenPlaceArgs&) = 0;
};

class RectangularRoomGenerator_1 final: public RoomGenerator
{
    double target_tidi;
    std::vector<EnemyGenerationRule> rules;
public:
    void set_target_tidi(double);
    void add_enemy_generation_rule(EnemyGenerationRule&&);
    RoomGenGenerateResult generate(const RoomGenGenerateArgs&) override;
    RoomGenPlaceResult place(const RoomGenPlaceArgs&) override;
};

struct RoomGenerationRule
{
    std::function<double(int)> selection_weight_func;
    std::function<std::unique_ptr<RoomGenerator>()> make_room_generator;
    int min_occurences = 0;
};

struct LevelGenerationRule
{
    std::vector<RoomGenerationRule> room_gen_rules;
    int num_placement_attempts = 10;
    bool is_level_timed;
    double level_time_limit;
    double level_difficulty;
    double run_difficulty_setting;
};

enum class LevelGeneratorResult {
    Success, Failure
};
class LevelGenerator
{
    constexpr static size_t GRID_LEN = 512;

    const LevelGenerationRule* level_gen_rule;
    StandardRNG *rng;
    Level level;
    double tidi_used;
    double target_tidi;
    std::vector<std::tuple<std::unique_ptr<RoomGenerator>, RoomGenGenerateResult>> rooms;
    kx::FixedSizeArray<int> num_occurrences;

    void init();
    void generate_rooms();
    LevelGeneratorResult place_rooms();
    void generate_room(size_t room_rule_idx);
    LevelGeneratorResult place_room(BinaryGrid<GRID_LEN> *occupied_tiles, size_t room_idx);
public:
    LevelGenerator();
    Level generate(const LevelGenerationRule&, StandardRNG*);
};

}}}
