#include "geo2/map_obj/wall_type1/monochromatic_wall_1.h"
#include "geo2/map_obj/map_obj_args.h"

namespace geo2 { namespace map_obj {

MonochromaticWall_1::MonochromaticWall_1(const MapRect &position_,
                                         kx::gfx::LinearColor color_):
    Wall_Type1(position_),
    color(color_)
{}
void MonochromaticWall_1::add_render_objs(const MapObjRenderArgs &args)
{
    if(op == nullptr) {
        op = std::make_shared<RenderOpShader>(*args.shaders->monoc_wall_1);
        op_group = std::make_shared<RenderOpGroup>(1000.0);
        op_group->add_op(op);
        auto iu_map = op->map_instance_uniform(0);
        op_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};
    }

    op_iu[0] = args.x_to_ndc(position.x);
    op_iu[1] = args.y_to_ndc(position.y);
    op_iu[2] = args.x_to_ndc(position.x + position.w);
    op_iu[3] = args.y_to_ndc(position.y + position.h);

    op_iu[4] = color.r;
    op_iu[5] = color.g;
    op_iu[6] = color.b;
    op_iu[7] = color.a;

    if(args.is_x_line_ndc_in_view(op_iu[0], op_iu[2]) &&
       args.is_y_line_ndc_in_view(op_iu[1], op_iu[3]))
    {
        args.add_op_group(op_group);
    }
}

}}
