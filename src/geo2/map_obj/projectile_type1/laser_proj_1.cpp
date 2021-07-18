#include "geo2/map_obj/projectile_type1/laser_proj_1.h"
#include "geo2/map_obj/map_obj_args.h"
#include "geo2/render_op.h"

#include <cmath>
#include <vector>

namespace geo2 { namespace map_obj {

constexpr auto SIDE_LEN = 0.3;

const std::unique_ptr<const Polygon> BASE_SHAPE = Polygon::make(
    std::vector<float>{-0.5 * SIDE_LEN  , SIDE_LEN * 1.0 / 3.0 * std::sqrt(3.0) / 2.0,
                       0.5 * SIDE_LEN   , SIDE_LEN * 1.0 / 3.0 * std::sqrt(3.0) / 2.0,
                       0.0              , -SIDE_LEN * 2.0 / 3.0 * std::sqrt(3.0) / 2.0});

LaserProj_1::LaserProj_1(const std::shared_ptr<MapObject> &owner_,
                         double damage_,
                         double lifespan_,
                         MapCoord pos_,
                         MapVec velocity_,
                         double rot):
    BasicProj_1(owner_, damage_, lifespan_, pos_, velocity_),
    base_shape([rot]()
                {
                    auto ret = BASE_SHAPE->copy();
                    ret->rotate_about_origin(rot);
                    return ret;
                }())
{}
void LaserProj_1::init(const MapObjInitArgs &args)
{
     args.add_current_pos(Polygon::make_with_num_sides(3));
     args.add_desired_pos(Polygon::make_with_num_sides(3));
}
void LaserProj_1::add_render_ops(const MapObjRenderArgs &args)
{
    if(op == nullptr) {
        op_group = std::make_shared<RenderOpGroup>(args.get_proj_render_priority());
        op = std::make_shared<RenderOpShader>(*args.shaders->laser_proj_1);
        op_group->add_op(op);

        auto iu_map = op->map_instance_uniform(0);
        op_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};

        //inner color
        op_iu[8] = 6.0f;
        op_iu[9] = 1.5f;
        op_iu[10] = 1.5f;
        op_iu[11] = 1.0f;

        //outer color
        op_iu[12] = 4.0f;
        op_iu[13] = 0.6f;
        op_iu[14] = 0.6f;
        op_iu[15] = 1.0f;
    }

    auto cur_shape = base_shape->copy();
    cur_shape->translate(current_position.x, current_position.y);
    if(args.is_AABB_in_view(cur_shape->get_AABB())) {
        //position
        for(size_t i=0; i<3; i++) {
            auto v = cur_shape->get_vertex(i);
            op_iu[i*2] = args.x_to_ndc(v.x);
            op_iu[i*2 + 1] = args.y_to_ndc(v.y);
        }
        args.add_op_group(op_group);
    }
}
void LaserProj_1::copy_base_shape_into(Polygon *p) const
{
    p->copy_from(base_shape.get());
}

}}
