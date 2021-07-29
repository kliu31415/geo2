#pragma once

#include <vector>

#include "kx/fixed_size_array.h"

namespace geo2 { namespace level_gen { namespace v1 {

struct Grid
{
    static constexpr size_t MAX_DIM = 256;
    std::array<uint64_t, 256*4> grid;
    size_t rows;
    size_t columns;
public:
    Grid(size_t rows_, size_t columns_):
        rows(rows_),
        columns(columns_)
    {
        std::fill(grid.begin(), grid.end(), 0);
    }
    inline void set(size_t row, size_t column, bool val)
    {
        k_assert(row<rows && column<columns);
        grid[row*4 + column/64] |= 1 << (column % 64);
        grid[row*4 + column/64] &= val << (column % 64);
    }
    inline bool get(size_t row, size_t column) const
    {
        k_assert(row<rows && column<columns);
        return (grid[row*4 + column/64] >> (column % 64)) & 1;
    }
    inline size_t get_num_rows() const
    {
        return rows;
    }
    inline size_t get_num_columns() const
    {
        return columns;
    }
};

using room_gen_id_t = int;

class LevelGenerator;

class RoomGenerator
{
public:
    virtual ~RoomGenerator() = default;
    room_gen_id_t get_id() const;
    virtual Grid get_occupied_tiles() = 0;
};

struct RoomGenerator1 final: public RoomGenerator
{
    Grid get_occupied_tiles() override;
};

class LevelGenerator
{
public:
};

}}}
