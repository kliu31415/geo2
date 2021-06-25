#pragma once

#include "geo2/map_obj/projectile_type1/projectile_type1.h"

namespace geo2 { namespace map_obj {

class SimpleProj_1: public Projectile_Type1
{
protected:
    double life_left;
    MoveIntent move_intent;
    MapCoord cur_pos;
    MapCoord desired_pos;
    MapVec velocity;

    std::unique_ptr<Polygon> cur_shape;
    std::unique_ptr<Polygon> des_shape;

    SimpleProj_1(const std::shared_ptr<Unit_Type1> &owner_,
                 double lifespan,
                 MapCoord pos_,
                 MapVec velocity_,
                 std::unique_ptr<Polygon> &&shape);
public:
    virtual ~SimpleProj_1() = default;

    void run2_st(const MapObjRun2Args &args) override;
};

}}
