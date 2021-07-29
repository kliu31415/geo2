#include <memory>
#include <functional>
#include <vector>

namespace geo2 { namespace map_obj {

struct MapTileCoord
{
    int x;
    int y;
    MapTileCoord() = default;
    MapTileCoord(int x_, int y_):
        x(x_),
        y(y_)
    {}
};

struct UnitPlacementInfo
{
    std::function<void(const std::function<void(std::shared_ptr<MapObject>&&)>&, double x, double y)> place;
    std::vector<MapTileCoord> occupied;
};

}}
