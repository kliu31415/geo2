#include <memory>
#include <functional>
#include <vector>

namespace geo2 { namespace map_obj {

struct UnitRectPlacementInfo
{
    std::function<void(const std::function<void(std::shared_ptr<MapObject>&&)>&, MapCoord)> place_func;
    double tidi = std::numeric_limits<double>::signaling_NaN();
    int w;
    int h;
};

}}
