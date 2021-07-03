#pragma once

#include "geo2/weapon/weapon.h"

#include <array>

namespace geo2 { namespace weapon {

class TestLaser1 final: public Weapon
{
    double reload_counter;
    int ammo;
    std::array<std::shared_ptr<RenderOpShader>, 2> ops;
    std::array<nonstd::span<float>, 2> op_ius;
public:
    TestLaser1(const std::shared_ptr<map_obj::MapObject> &owner_);
    void run(const WeaponRunArgs &args) override;
    void render(const WeaponRenderArgs &args) override;
};

}}
