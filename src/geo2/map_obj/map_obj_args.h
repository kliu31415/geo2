#pragma once

#include "geo2/map_obj/map_object.h"

#include "geo2/ceng1_obj.h"
#include "geo2/game_render_op_list.h"

#include "kx/gfx/renderer.h"
#include "kx/fixed_size_array.h"

namespace geo2 { namespace map_obj {

class MapObjRun1Args;
class MapObjRun2Args;
class MapObjCtorArgs;
class MapObjRenderArgs;

class CEng1DataReaderAttorney
{
    friend MapObjRun1Args;
    friend MapObjRun2Args;
    friend MapObjCtorArgs;
    friend MapObjRenderArgs;
    friend class CEng1DataMutatorAttorney;

    CEng1DataReaderAttorney() {}
public:
    inline const Polygon *cur_front(const MapObject *obj)
    {
        return obj->get_ceng_data_ref().cur_front();
    }
    inline const Polygon *des_front(const MapObject *obj)
    {
        return obj->get_ceng_data_ref().des_front();
    }
    inline MoveIntent get_move_intent(const MapObject *obj)
    {
        return obj->get_ceng_data_ref().get_move_intent();
    }
};

class CEng1DataMutatorAttorney: public CEng1DataReaderAttorney
{
    friend MapObjRun1Args;
    friend MapObjRun2Args;
    friend MapObjCtorArgs;
    friend MapObjRenderArgs;

    CEng1DataMutatorAttorney() {}
public:
    inline void push_back_cur(MapObject *obj, std::unique_ptr<Polygon> &&polygon)
    {
        obj->get_ceng_data_ref().push_back_cur(std::move(polygon));
    }
    inline void push_back_des(MapObject *obj, std::unique_ptr<Polygon> &&polygon)
    {
        obj->get_ceng_data_ref().push_back_des(std::move(polygon));
    }
    inline Polygon *cur_front(MapObject *obj)
    {
        return obj->get_ceng_data_ref().cur_front();
    }
    inline Polygon *des_front(MapObject *obj)
    {
        return obj->get_ceng_data_ref().des_front();
    }
    inline void set_move_intent(MapObject *obj, MoveIntent new_intent)
    {
        obj->get_ceng_data_ref().set_move_intent(new_intent);
    }
};


class MapObjRun1Args final
{
    std::vector<CEng1Obj> *ceng_cur;
    std::vector<CEng1Obj> *ceng_des;
    std::vector<MoveIntent> *move_intent;
    std::vector<std::shared_ptr<MapObject>> *map_objs;
public:
    double tick_len;
    size_t idx;

    void set_ceng_cur_vec(std::vector<CEng1Obj> *ceng_cur_)
    {
        ceng_cur = ceng_cur_;
    }
    void set_ceng_des_vec(std::vector<CEng1Obj> *ceng_des_)
    {
        ceng_des = ceng_des_;
    }
    void set_move_intent_vec(std::vector<MoveIntent> *move_intent_)
    {
        move_intent = move_intent_;
    }
    void set_map_objs_vec(std::vector<std::shared_ptr<MapObject>> *map_objs_)
    {
        map_objs = map_objs_;
    }
    /** A raw pointer to polygon is used for speed. Make sure that someone else
     *  retains ownership of shape to keep it alive while the collision engine
     *  runs, or else there may be a dangling pointer that could cause crashes!
     */
    inline void add_current_pos(const Polygon *polygon, uint16_t shape_id = 0) const
    {
        ceng_cur->emplace_back(polygon, idx, shape_id);
    }
    inline void add_desired_pos(const Polygon *polygon, uint16_t shape_id = 0) const
    {
        ceng_des->emplace_back(polygon, idx, shape_id);
    }
    inline void set_move_intent(MoveIntent intent) const
    {
        if(move_intent->size() <= idx)
            move_intent->resize(idx+1);
        (*move_intent)[idx] = intent;
    }
    inline void add_map_obj(std::shared_ptr<MapObject> obj) const
    {
        map_objs->push_back(std::move(obj));
    }
};

inline bool are_teams_enemies(int team1, int team2)
{
    return team1 != team2;
}

class MapObjRun2Args final
{
public:
    double tick_len;
};

class MapObjRenderArgs final
{
    std::vector<std::shared_ptr<RenderOpGroup>> *op_groups;
    const kx::gfx::Renderer *rdr;
    kx::gfx::Rect camera;
    float cam_w_inv;
    float cam_h_inv;

    /*inline kx::gfx::Rect to_cam_nc(const MapRect &rect) const
    {
        kx::gfx::Rect ret;
        ret.x = (rect.x - camera.x) * cam_w_inv;
        ret.y = (rect.y - camera.y) * cam_h_inv;
        ret.w = rect.w * cam_w_inv;
        ret.h = rect.h * cam_h_inv;
        return ret;
    }*/
public:
    const GameRenderOpList::Shaders *shaders;

    inline bool is_x_line_ndc_in_view(float x1, float x2) const
    {
        return rdr->is_x_line_ndc_in_view(x1, x2);
    }
    inline bool is_y_line_ndc_in_view(float y1, float y2) const
    {
        return rdr->is_y_line_ndc_in_view(y1, y2);
    }
    inline bool is_x_ndc_in_view(float x) const
    {
        return rdr->is_x_ndc_in_view(x);
    }
    inline bool is_y_ndc_in_view(float y) const
    {
        return rdr->is_y_ndc_in_view(y);
    }
    inline bool is_coord_ndc_in_view(float x, float y) const
    {
        return is_x_ndc_in_view(x) && is_y_ndc_in_view(y);
    }
    inline float x_to_ndc(float x) const
    {
        return rdr->x_nc_to_ndc((x - camera.x) * cam_w_inv);
    }
    inline float y_to_ndc(float y) const
    {
        return rdr->y_nc_to_ndc((y - camera.y) * cam_h_inv);
    }
    inline bool is_AABB_in_view(const AABB &aabb) const
    {
        return is_x_line_ndc_in_view(x_to_ndc(aabb.x1), x_to_ndc(aabb.x2)) &&
               is_y_line_ndc_in_view(y_to_ndc(aabb.y1), y_to_ndc(aabb.y2));
    }
    inline void add_op_group(const std::shared_ptr<RenderOpGroup> &op_group) const
    {
        op_groups->push_back(op_group);
    }
    inline void set_op_groups_vec(std::vector<std::shared_ptr<RenderOpGroup>> *op_groups_)
    {
        op_groups = op_groups_;
    }
    inline void set_renderer(kx::gfx::Renderer *renderer)
    {
        rdr = renderer;
    }
    inline void set_camera(kx::gfx::Rect camera_)
    {
        camera = camera_;
        cam_w_inv = 1.0f / camera.w;
        cam_h_inv = 1.0f / camera.h;
    }
};

struct HandleCollisionArgs final
{
    class MapObject *other;
    CEng1Collision collision_info;
    void swap()
    {
        collision_info.swap();
    }
};

}}
