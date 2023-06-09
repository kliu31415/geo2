#include "geo2/map_obj/wall_type1/wall_type1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

Wall_Type1::Wall_Type1(const MapRect &position_):
    position(position_)
{}
void Wall_Type1::init(const MapObjInitArgs &args)
{
    MapCoord verts[]{{position.x, position.y},
                     {position.x + position.w, position.y},
                     {position.x + position.w, position.y + position.h},
                     {position.x, position.y + position.h}};
    auto polygon_this = Polygon::make(kx::kx_span<MapCoord>(std::begin(verts), std::end(verts)));
    args.add_current_pos_polygon_with_num_sides(4);
    args.get_sole_current_pos()->copy_from(*polygon_this);
}
void Wall_Type1::run1_mt(const MapObjRun1Args &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void Wall_Type1::handle_collision(MapObject *other, const HandleCollisionArgs &args)
{
    other->handle_collision(this, args);
}
void Wall_Type1::handle_collision([[maybe_unused]] Wall_Type1 *other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void Wall_Type1::handle_collision([[maybe_unused]] Unit *other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}
void Wall_Type1::handle_collision([[maybe_unused]] Projectile_Type1 *other,
                                        [[maybe_unused]] const HandleCollisionArgs &args)
{
    args.set_move_intent(MoveIntent::StayAtCurrentPos);
}

}}
