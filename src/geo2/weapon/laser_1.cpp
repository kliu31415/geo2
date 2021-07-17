#include "geo2/weapon/laser_1.h"
#include "geo2/map_obj/projectile_type1/laser_proj_1.h"

#include <random>

namespace geo2 { namespace weapon {

TestLaser1::TestLaser1(const std::shared_ptr<map_obj::MapObject> &owner_):
    Weapon(owner_)
{

}

constexpr double FIRE_INTERVAL = 0.0008;
constexpr double DAMAGE = 0.1;
constexpr double LIFESPAN = 0.5;
constexpr double PROJ_SPEED = 150;

void TestLaser1::run(const WeaponRunArgs &args)
{
    if(reload_counter > 0)
        reload_counter -= args.get_tick_len();

    if(args.primary_attack()) {
        while(reload_counter <= 0) {
            auto mobj = owner.lock();
            k_expects(mobj != nullptr);
            auto owner_info = args.get_owner_info();

            auto dx = std::cos(args.get_angle());
            auto dy = std::sin(args.get_angle());

            auto proj = std::make_unique<map_obj::LaserProj_1>
                            (mobj,
                             DAMAGE,
                             LIFESPAN,
                             owner_info.pos + MapVec(dx, dy) * (owner_info.offset + 0.35),
                             MapVec(dx, dy) * PROJ_SPEED,
                             std::uniform_real_distribution<double>(0, 2*M_PI)(*args.get_rng()));

            args.add_map_obj(std::move(proj));
            reload_counter += FIRE_INTERVAL;
        }
    }
}
void TestLaser1::render(const WeaponRenderArgs &args)
{
    if(ops[0] == nullptr) {
        for(int i=0; i<2; i++) {
            ops[i] = std::make_shared<RenderOpShader>(*args.get_shaders()->laser_1);
            auto iu_map = ops[i]->map_instance_uniform(0);
            op_ius[i] = {(float*)iu_map.begin(), (float*)iu_map.end()};
        }

        //op 1
        //color 1
        op_ius[0][8] = 0.3f;
        op_ius[0][9] = 0.4f;
        op_ius[0][10] = 0.5f;
        op_ius[0][11] = 1.0f;

        //color 2
        op_ius[0][12] = 0.7f;
        op_ius[0][13] = 0.8f;
        op_ius[0][14] = 0.9f;
        op_ius[0][15] = 1.0f;


        //op 2
        //color 1
        op_ius[1][8] = 2.5f;
        op_ius[1][9] = 1.5f;
        op_ius[1][10] = 0.3f;
        op_ius[1][11] = 1.0f;

        //color 2
        op_ius[1][12] = 5.5f;
        op_ius[1][13] = 1.5f;
        op_ius[1][14] = 0.3f;
        op_ius[1][15] = 1.0f;
    }

    auto mobj = owner.lock();
    k_expects(mobj != nullptr);
    auto owner_info = args.get_owner_info();

    auto rot_mat = Matrix2::make_rotation_matrix(args.get_angle());
    constexpr MapVec v0[]{{-0.45f, -0.19f}, {-0.15f, -0.1f}};
    constexpr MapVec v1[]{{0.45f, -0.19f}, {0.15f, -0.1f}};
    constexpr MapVec v2[]{{-0.45f, 0.19f}, {-0.15f, 0.1f}};
    MapVec offset[]{{owner_info.offset, 0.0f}, {owner_info.offset + 0.35f, 0.0f}};

    for(int i=0; i<2; i++) {
        auto v0_rot = owner_info.pos + rot_mat * (v0[i] + offset[i]);
        auto v1_rot = owner_info.pos + rot_mat * (v1[i] + offset[i]);
        auto v2_rot = owner_info.pos + rot_mat * (v2[i] + offset[i]);

        op_ius[i][0] = args.x_to_ndc(v0_rot.x);
        op_ius[i][1] = args.y_to_ndc(v0_rot.y);
        op_ius[i][2] = args.x_to_ndc(v1_rot.x);
        op_ius[i][3] = args.y_to_ndc(v1_rot.y);
        op_ius[i][4] = args.x_to_ndc(v2_rot.x);
        op_ius[i][5] = args.y_to_ndc(v2_rot.y);

        args.add_render_op(ops[i]);
    }
}

}}
