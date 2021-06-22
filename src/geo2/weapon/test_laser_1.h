#pragma once

#include "geo2/weapon/weapon.h"

namespace geo2 { namespace weapon {

class TestLaser1 final: public Weapon
{
    int ammo;
    double reload_counter;
public:
    void run(const WeaponRunArgs &args) override;
};

}}
