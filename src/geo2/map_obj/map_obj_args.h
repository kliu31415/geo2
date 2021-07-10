#pragma once

#include "geo2/map_obj/map_object.h"
#include "geo2/render_args.h"
#include "geo2/ceng1_data.h"
#include "geo2/ceng1_obj.h"
#include "geo2/rng_args.h"

#include "kx/fixed_size_array.h"

namespace geo2 {class Game;}

namespace geo2 { namespace map_obj {

class MapObjInitArgs final
{
    CEng1Data *data;
public:
    MapObjInitArgs() = default;

    MapObjInitArgs(const MapObjInitArgs&) = delete;
    MapObjInitArgs & operator = (const MapObjInitArgs&) = delete;
    MapObjInitArgs(MapObjInitArgs&&) = delete;
    MapObjInitArgs & operator = (MapObjInitArgs&&) = delete;

    inline void add_current_pos(std::unique_ptr<Polygon> &&polygon) const
    {
        data->add_current_pos(std::move(polygon));
    }
    inline void add_desired_pos(std::unique_ptr<Polygon> &&polygon) const
    {
        data->add_desired_pos(std::move(polygon));
    }
    inline Polygon *get_sole_current_pos() const
    {
        return data->get_sole_current_pos();
    }
    inline Polygon *get_sole_desired_pos() const
    {
        return data->get_sole_desired_pos();
    }
    inline void set_move_intent(MoveIntent new_intent) const
    {
        data->set_move_intent(new_intent);
    }
    inline void set_ceng_data(CEng1Data *data_)
    {
        data = data_;
    }
};

class CEng1DataReaderAttorney
{
protected:
    std::vector<CEng1Data> *ceng_data;
    std::vector<std::shared_ptr<MapObject>> *map_objs_to_add;
    std::vector<int> *idx_to_delete;
    int idx;

    CEng1DataReaderAttorney() = default;
    CEng1DataReaderAttorney(const CEng1DataReaderAttorney&) = default;
public:
    double cur_level_time;

    CEng1DataReaderAttorney & operator = (const CEng1DataReaderAttorney&) = delete;
    CEng1DataReaderAttorney(CEng1DataReaderAttorney&&) = delete;
    CEng1DataReaderAttorney & operator = (CEng1DataReaderAttorney&&) = delete;

    inline void set_ceng_data(std::vector<CEng1Data> *ceng_data_)
    {
        ceng_data = ceng_data_;
    }
    inline void set_map_objs_to_add(std::vector<std::shared_ptr<MapObject>> *to_add)
    {
        map_objs_to_add = to_add;
    }
    inline void set_index(int idx_)
    {
        //make sure ceng_data and map_objs_to_add are null before calling this function
        //so the k_expects doesn't crash
        k_expects(idx_ < (int)ceng_data->size());
        idx = idx_;
    }
    inline void add_map_obj(std::shared_ptr<MapObject> obj) const
    {
        map_objs_to_add->push_back(std::move(obj));
    }

    inline const Polygon *get_sole_current_pos() const
    {
        return (*ceng_data)[idx].get_sole_current_pos();
    }
    inline const Polygon *get_sole_desired_pos() const
    {
        return (*ceng_data)[idx].get_sole_desired_pos();
    }
    inline MoveIntent get_move_intent() const
    {
        return (*ceng_data)[idx].get_move_intent();
    }
    inline void set_idx_to_delete(std::vector<int> *idx_to_delete_)
    {
        idx_to_delete = idx_to_delete_;
    }
};

class CEng1DataMutatorAttorney: public CEng1DataReaderAttorney
{
protected:
    CEng1DataMutatorAttorney() = default;
    CEng1DataMutatorAttorney(const CEng1DataMutatorAttorney&) = default;
public:
    CEng1DataMutatorAttorney & operator = (const CEng1DataMutatorAttorney&) = delete;
    CEng1DataMutatorAttorney(CEng1DataMutatorAttorney&&) = delete;
    CEng1DataMutatorAttorney & operator = (CEng1DataMutatorAttorney&&) = delete;

    inline void add_current_pos(std::unique_ptr<Polygon> &&polygon) const
    {
        (*ceng_data)[idx].add_current_pos(std::move(polygon));
    }
    inline void add_desired_pos(std::unique_ptr<Polygon> &&polygon)  const
    {
        (*ceng_data)[idx].add_desired_pos(std::move(polygon));
    }
    inline Polygon *get_sole_current_pos() const
    {
        return (*ceng_data)[idx].get_sole_current_pos();
    }
    inline Polygon *get_sole_desired_pos() const
    {
        return (*ceng_data)[idx].get_sole_desired_pos();
    }
    inline void set_move_intent(MoveIntent new_intent) const
    {
        //delete_me() already sets MoveIntent to RemoveShapes, so as of now
        //there's no reason to set both RemoveShapes and call delete_me();
        //in the future, if there is, we should be able to remove this statement
        //without issues
        k_expects(new_intent != MoveIntent::RemoveShapes);

        (*ceng_data)[idx].set_move_intent(new_intent);
    }
};

class MapObjRunArgs
{
public:
    double tick_len;
    MapObjRunArgs() = default;
};

/** Note that only MapObjRun1Args inherits CEngMutatorAttorney; you generally
 *  should not directly mutate collision engine data outside of run1. The
 *  exception to this is in run2, you can also mutate collision engine objects,
 *  albeit in a different way. However, 99.9% of the time you'll only want to
 *  interact with the collision engine in run1; it is rare that map objs require
 *  the more specialized (and more expensive) functions available in run2.
 *
 *  Example cases where interacting with the collision engine in run2 is necessary:
 *  -Simulating teleportation
 *  -Needing to move multiple times in a tick
 *  -Needing to insert dummy shapes to scan for collisions in an area
 */
class MapObjRun1Args final: public CEng1DataMutatorAttorney, public MapObjRunArgs, public RNG_Args
{
public:
    MapObjRun1Args() = default;

    MapObjRun1Args(const MapObjRun1Args&) = delete;
    MapObjRun1Args & operator = (const MapObjRun1Args&) = delete;
    MapObjRun1Args(MapObjRun1Args&&) = delete;
    MapObjRun1Args & operator = (MapObjRun1Args&&) = delete;

    inline void delete_me() const
    {
        //duplicates won't cause errors (yet, at least), but we really should
        //tried to avoid them because they're messy
        k_expects(idx_to_delete->empty() || idx_to_delete->back()!=idx);
        idx_to_delete->push_back(idx);
        (*ceng_data)[idx].set_move_intent(MoveIntent::NotSet);
    }
};

class HandleCollisionArgs: public CEng1DataReaderAttorney
{
    class MapObject *this_;
    class MapObject *other;
    CEng1Collision collision_info;
    int other_idx;

protected:
    HandleCollisionArgs(const HandleCollisionArgs &other) = default;
public:
    HandleCollisionArgs() = default;

    inline void set_this(MapObject *this__)
    {
        this_ = this__;
    }
    inline void set_other(MapObject *other_)
    {
        other = other_;
    }
    inline void set_other_idx(int other_idx_)
    {
        other_idx = other_idx_;
    }
    inline void set_collision_info(const CEng1Collision &collision_info_)
    {
        collision_info = collision_info_;
    }
    inline void set_move_intent(MoveIntent new_intent) const
    {
        (*ceng_data)[idx].set_move_intent(new_intent);
    }
    inline void delete_me() const
    {
        //duplicates won't cause errors (yet, at least), but we really should
        //tried to avoid them because they're messy
        k_expects(idx_to_delete->empty() || idx_to_delete->back()!=idx);
        idx_to_delete->push_back(idx);
        (*ceng_data)[idx].set_move_intent(MoveIntent::RemoveShapes);
    }

    void swap()
    {
        std::swap(this_, other);
        std::swap(idx, other_idx);
        collision_info.swap();
    }
};

class MapObjRun2Args final: public CEng1DataReaderAttorney, public MapObjRunArgs, public RNG_Args
{
public:
    MapObjRun2Args() = default;

    MapObjRun2Args(const MapObjRun2Args&) = delete;
    MapObjRun2Args & operator = (const MapObjRun2Args&) = delete;
    MapObjRun2Args(MapObjRun2Args&&) = delete;
    MapObjRun2Args & operator = (MapObjRun2Args&&) = delete;

    inline void delete_me() const
    {
        //duplicates won't cause errors (yet, at least), but we really should
        //tried to avoid them because they're messy
        k_expects(idx_to_delete->empty() || idx_to_delete->back()!=idx);
        idx_to_delete->push_back(idx);
        (*ceng_data)[idx].set_move_intent(MoveIntent::RemoveShapes);
    }
};

class EndHandleCollisionBlockArgs final
{
    EndHandleCollisionBlockArgs() = default;

    EndHandleCollisionBlockArgs(const EndHandleCollisionBlockArgs&) = delete;
    EndHandleCollisionBlockArgs & operator = (const EndHandleCollisionBlockArgs&) = delete;
    EndHandleCollisionBlockArgs(EndHandleCollisionBlockArgs&&) = delete;
    EndHandleCollisionBlockArgs & operator = (EndHandleCollisionBlockArgs&&) = delete;
};

class MapObjRenderArgs final: public CEng1DataMutatorAttorney, public RenderArgs
{
    std::vector<std::shared_ptr<RenderOpGroup>> *op_groups;

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
    MapObjRenderArgs() = default;

    MapObjRenderArgs(const MapObjRenderArgs&) = delete;
    MapObjRenderArgs & operator = (const MapObjRenderArgs&) = delete;
    MapObjRenderArgs(MapObjRenderArgs&&) = delete;
    MapObjRenderArgs & operator = (MapObjRenderArgs&&) = delete;

    inline void add_op_group(const std::shared_ptr<RenderOpGroup> &op_group) const
    {
        op_groups->push_back(op_group);
    }
    inline void set_op_groups_vec(std::vector<std::shared_ptr<RenderOpGroup>> *op_groups_)
    {
        op_groups = op_groups_;
    }
    inline float get_proj_render_priority() const
    {
        return 5000;
    }
    inline float get_player_render_priority() const
    {
        return 4000;
    }
    inline float get_NPC_render_priority() const
    {
        return 3000;
    }
    inline float get_wall_render_priority() const
    {
        return 2000;
    }
    inline float get_floor_render_priority() const
    {
        return 1000;
    }
};

}}
