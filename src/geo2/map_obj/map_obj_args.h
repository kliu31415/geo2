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
    size_t idx;
public:
    CEng1DataReaderAttorney() = default;

    CEng1DataReaderAttorney(const CEng1DataReaderAttorney&) = delete;
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
    inline void set_index(size_t idx_)
    {
        //make sure ceng_data and map_objs_to_add are null before calling this function
        //so the k_expects doesn't crash
        k_expects(idx_ < ceng_data->size());
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
};

class CEng1DataMutatorAttorney: public CEng1DataReaderAttorney
{
public:
    CEng1DataMutatorAttorney() = default;

    CEng1DataMutatorAttorney(const CEng1DataMutatorAttorney&) = delete;
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
        (*ceng_data)[idx].set_move_intent(new_intent);
    }
};

class MapObjRunArgs
{
    double tick_len;
public:
    inline void set_tick_len(double tick_len_)
    {
        tick_len = tick_len_;
    }
    inline double get_tick_len() const
    {
        return tick_len;
    }
};

class MapObjRun1Args final: public CEng1DataMutatorAttorney, public MapObjRunArgs, public RNG_Args
{
public:
    MapObjRun1Args() = default;

    MapObjRun1Args(const MapObjRun1Args&) = delete;
    MapObjRun1Args & operator = (const MapObjRun1Args&) = delete;
    MapObjRun1Args(MapObjRun1Args&&) = delete;
    MapObjRun1Args & operator = (MapObjRun1Args&&) = delete;
};

struct HandleCollisionArgs final: public CEng1DataReaderAttorney
{
    class MapObject *other;
    CEng1Collision collision_info;

    HandleCollisionArgs() = default;

    void swap()
    {
        collision_info.swap();
    }
    inline void set_move_intent(MoveIntent new_intent) const
    {
        (*ceng_data)[idx].set_move_intent(new_intent);
    }
};

class MapObjRun2Args final: public CEng1DataReaderAttorney, public MapObjRunArgs
{
public:
    MapObjRun2Args() = default;

    MapObjRun2Args(const MapObjRun2Args&) = delete;
    MapObjRun2Args & operator = (const MapObjRun2Args&) = delete;
    MapObjRun2Args(MapObjRun2Args&&) = delete;
    MapObjRun2Args & operator = (MapObjRun2Args&&) = delete;
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
