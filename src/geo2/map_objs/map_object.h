#pragma once

#include "geo2/collision_engine1.h"
#include "geo2/game_render_op_list.h"

#include "kx/gfx/renderer.h"
#include "kx/fixed_size_array.h"


namespace geo2 { namespace map_obj {

class MapObjRun1Args final
{
    std::vector<CollisionEng1Obj> *ceng_cur;
    std::vector<CollisionEng1Obj> *ceng_des;
    kx::FixedSizeArray<MoveIntent> *move_intent;
public:
    double tick_len;
    int idx;

    void set_ceng_cur_vec(std::vector<CollisionEng1Obj> *ceng_cur_)
    {
        ceng_cur = ceng_cur_;
    }
    void set_ceng_des_vec(std::vector<CollisionEng1Obj> *ceng_des_)
    {
        ceng_des = ceng_des_;
    }
    void set_move_intent_vec(kx::FixedSizeArray<MoveIntent> *move_intent_)
    {
        move_intent = move_intent_;
    }
    /** A raw pointer to shape is used for speed. Make sure that someone else
     *  retains ownership of shape to keep it alive while the collision engine
     *  runs, or else there may be a dangling pointer that could cause crashes!
     */
    inline void add_current_pos(const Shape *shape, uint16_t shape_id = 0) const
    {
        ceng_cur->emplace_back(shape, idx, shape_id, false);
    }
    inline void add_desired_pos(const Shape *shape, uint16_t shape_id = 0) const
    {
        ceng_des->emplace_back(shape, idx, shape_id, true);
    }
    inline void set_move_intent(MoveIntent intent) const
    {
        (*move_intent)[idx] = intent;
    }
};

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
public:
    const GameRenderOpList::Shaders *shaders;

    inline kx::gfx::Rect to_cam_nc(const kx::gfx::Rect &rect) const
    {
        kx::gfx::Rect ret;
        ret.x = (rect.x - camera.x) * cam_w_inv;
        ret.y = (rect.y - camera.y) * cam_h_inv;
        ret.w = rect.w * cam_w_inv;
        ret.h = rect.h * cam_h_inv;
        return ret;
    }
    inline kx::gfx::Rect to_cam_nc(const MapRect &rect) const
    {
        kx::gfx::Rect ret;
        ret.x = (rect.x - camera.x) * cam_w_inv;
        ret.y = (rect.y - camera.y) * cam_h_inv;
        ret.w = rect.w * cam_w_inv;
        ret.h = rect.h * cam_h_inv;
        return ret;
    }
    inline float x_to_cam_nc(float x) const
    {
        return (x - camera.x) * cam_w_inv;
    }
    inline float y_to_cam_nc(float y) const
    {
        return (y - camera.y) * cam_h_inv;
    }
    inline float x_nc_to_ndc(float x) const
    {
        return rdr->x_nc_to_ndc(x);
    }
    inline float y_nc_to_ndc(float y) const
    {
        return rdr->y_nc_to_ndc(y);
    }
    inline float x_to_ndc(float x) const
    {
        return x_nc_to_ndc(x_to_cam_nc(x));
    }
    inline float y_to_ndc(float y) const
    {
        return y_nc_to_ndc(y_to_cam_nc(y));
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

#define HANDLE_COLLISION_FUNC_DECLARATION(T) \
    MoveIntent handle_collision(const class T &other, const HandleCollisionArgs &args)

class MapObject: public Collidable
{
public:
    virtual ~MapObject() = default;

    ///default behavior for these functions = do nothing
    virtual void run1_mt(const MapObjRun1Args &args);
    virtual void run2_st(const MapObjRun2Args &args);
    virtual void add_render_objs(const MapObjRenderArgs &args);

    ///default return value = true
    virtual bool collision_could_matter(const MapObject &other) const;

    /** "other" handles the collision and reports its intent, so this function is
     *  const. Note that this function exists solely to perform the first level of
     *  double dispatch.
     */
    virtual MoveIntent handle_collision(MapObject *other, const HandleCollisionArgs &args) const = 0;
    ///for the below functions, "this" handles the collision and reports its intent
    virtual HANDLE_COLLISION_FUNC_DECLARATION(CosmeticMapObj);
    virtual HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) = 0;
    virtual HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) = 0;
    virtual HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) = 0;
};
/** Note that cosmetic map objects CAN collide with things; it's just that something
 *  collides with a cosmetic map object, the effect will be purely cosmetic; i.e.
 *  a floating blob might change the color of every unit it collides with.
 */
class CosmeticMapObj: public MapObject
{
    MoveIntent handle_collision(MapObject *other, const HandleCollisionArgs &args) const override;
    HANDLE_COLLISION_FUNC_DECLARATION(Wall_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Unit_Type1) override;
    HANDLE_COLLISION_FUNC_DECLARATION(Projectile_Type1) override;
};

}}
