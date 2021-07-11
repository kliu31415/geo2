#pragma once

#include "geo2/map_obj/projectile_type1/projectile_type1.h"

namespace geo2 { namespace map_obj {

class BasicProj_1: public Projectile_Type1
{
protected:
    double damage;
    double life_left;
    MapCoord current_position;
    MapCoord desired_position;
    MapVec velocity;

    BasicProj_1(const std::shared_ptr<MapObject> &owner_,
                double damage_,
                double lifespan_,
                MapCoord pos_,
                MapVec velocity_);
public:
    virtual ~BasicProj_1() = default;

    void run1_mt(const MapObjRun1Args &args) override;
    void run3_mt(const MapObjRun3Args &args) override;

    double get_damage() const override;

    virtual void copy_base_shape_into(Polygon *p) const = 0;
};

}}
