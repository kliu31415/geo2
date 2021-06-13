#include "geo2/map_objs/wall_type1/wall_type1.h"

namespace geo2 { namespace map_obj {

Wall_Type1::Wall_Type1(const MapRect &position_):
    position(position_)
{
    MapCoord verts[]{{position.x, position.y},
                     {position.x + position.w, position.y},
                     {position.x + position.w, position.y + position.h},
                     {position.x, position.y + position.h}};
    auto s = nonstd::span<MapCoord>(std::begin(verts), std::end(verts));
    shape = Shape::make_polygon(s);
}
void Wall_Type1::run1_mt(const MapObjRun1Args &args)
{
    args.add_current_pos(shape.get());
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
MoveIntent Wall_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args) const
{
    return other->handle_collision(*this, args);
}
MoveIntent Wall_Type1::handle_collision([[maybe_unused]] const Wall_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Wall_Type1::handle_collision([[maybe_unused]] const Unit_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}
MoveIntent Wall_Type1::handle_collision([[maybe_unused]] const Projectile_Type1 &other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    return MoveIntent::StayAtCurrentPos;
}

}}
