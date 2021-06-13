#include "geo2/map_objs/wall_type1/monochromatic_wall1.h"

namespace geo2 { namespace map_obj {

MonochromaticWall_1::MonochromaticWall_1(const MapRect &position_,
                                         kx::gfx::LinearColor color_):
    Wall_Type1(position_),
    color(color_)
{}
void MonochromaticWall_1::add_render_objs(const MapObjRenderArgs &info)
{
    if(op == nullptr) {
        op = std::make_shared<RenderOpShader>(*info.shaders->test_terrain1);
        op_group = std::make_shared<RenderOpGroup>(1000.0);
        op_group->add_op(op);
        auto iu_map = op->map_instance_uniform(0);
        op_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};
    }

    auto dst = info.to_cam_nc(position);

    op_iu[0] = info.x_nc_to_ndc(dst.x);
    op_iu[1] = info.y_nc_to_ndc(dst.y);
    op_iu[2] = info.x_nc_to_ndc(dst.x + dst.w);
    op_iu[3] = info.y_nc_to_ndc(dst.y + dst.h);
    op_iu[4] = color.r;
    op_iu[5] = color.g;
    op_iu[6] = color.b;
    op_iu[7] = color.a;

    info.add_op_group(op_group);
}

}}
