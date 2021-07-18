#include "geo2/map_obj/floor_type1/test_terrain1.h"
#include "geo2/game_render_op_list.h"
#include "geo2/map_obj/map_obj_args.h"
#include "geo2/render_op.h"

#include "kx/gfx/renderer.h"

#include <cmath>

namespace geo2 { namespace map_obj {

TestTerrain1::TestTerrain1(MapCoord pos_):
    Floor_Type1(MapRect(pos_.x, pos_.y, 1, 1))
{

}
void TestTerrain1::add_render_objs(const MapObjRenderArgs &args)
{
    using namespace kx::gfx;

    /*Rect dst(position.x, position.y, 1, 1);
    dst = args.to_cam_nc(dst);
    args.rdr->set_color(LinearColor(10 * position.x, 10 * position.y, 0.0, 1.0));
    args.rdr->fill_rect_nc(dst);*/

    if(op == nullptr) {
        op = std::make_shared<RenderOpShader>(*args.shaders->test_terrain_1);
        op_group = std::make_shared<RenderOpGroup>(args.get_floor_render_priority());
        op_group->add_op(op);
        auto iu_map = op->map_instance_uniform(0);
        op_iu = {(float*)iu_map.begin(), (float*)iu_map.end()};
    }

    Rect dst(position.x, position.y, 1.0f, 1.0f);

    op_iu[0] = args.x_to_ndc(dst.x);
    op_iu[1] = args.y_to_ndc(dst.y);
    op_iu[2] = args.x_to_ndc(dst.x + dst.w);
    op_iu[3] = args.y_to_ndc(dst.y + dst.h);
    op_iu[4] = 0.5f;
    op_iu[5] = 0.5f;
    op_iu[6] = 0.0f;
    op_iu[7] = 1.0f;

    args.add_op_group(op_group);
}

}}
