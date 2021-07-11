#pragma once

#include "geo2/geometry.h"

#include <memory>
#include <vector>
#include <cstring>

namespace geo2 {

namespace map_obj
{
    class CEng1DataReaderAttorney;
    class CEng1DataMutatorAttorney;
}

/** The vast majority of the time, there will be only one shape, so this case
 *  is optimized for
 */
class CEng1Data final
{
    struct PolygonData
    {
        int num_polygons;

        union
        {
            std::unique_ptr<Polygon> ptr;
            //use a pointer to save memory because we won't use this 99% of the time
            std::unique_ptr<std::vector<std::unique_ptr<Polygon>>> vec;
        };

        PolygonData():
            num_polygons(0)
        {}
        PolygonData(PolygonData &&other)
        {
            //we copy the biggest member of the union; here we assume it's vec
            static_assert(sizeof(vec) >= sizeof(ptr));

            num_polygons = other.num_polygons;
            other.num_polygons = 0;
            std::memcpy((void*)&vec, (void*)&other.vec, sizeof(vec));
        }
        PolygonData &operator = (PolygonData &&other)
        {
            //we copy the biggest member of the union; here we assume it's vec
            static_assert(sizeof(vec) >= sizeof(ptr));

            num_polygons = other.num_polygons;
            other.num_polygons = 0;
            std::memcpy((void*)&vec, (void*)&other.vec, sizeof(vec));
            return *this;
        }
        ~PolygonData()
        {
            if(num_polygons == 1)
                ptr.~unique_ptr();
            else if(num_polygons > 1)
                vec.~unique_ptr();
        }
        void push_back(std::unique_ptr<Polygon> &&polygon)
        {
            if(num_polygons == 0) {
                new (&ptr) decltype(ptr)(std::move(polygon));
            } else if(num_polygons >= 1) {
                if(num_polygons == 1) {
                    auto tmp = std::move(ptr);
                    new (&vec) decltype(vec)();
                    vec->push_back(std::move(tmp));
                }
                vec->push_back(std::move(polygon));
            }
            num_polygons++;
        }
    };

    PolygonData cur;
    PolygonData des;

    MoveIntent move_intent;

    template<class Func> inline static void for_each(const PolygonData &data, Func &&func)
    {
        if(data.num_polygons == 1) {
            func(data.ptr.get(), 0);
        } else if(data.num_polygons > 1) {
            for(size_t i=0; i<data.vec->size(); i++) {
                func((*data.vec)[i].get(), i);
            }
        }
    }
public:
    CEng1Data() = default;

    inline void add_current_pos(std::unique_ptr<Polygon> &&polygon)
    {
        cur.push_back(std::move(polygon));
    }
    inline void add_desired_pos(std::unique_ptr<Polygon> &&polygon)
    {
        des.push_back(std::move(polygon));
    }
    inline Polygon *get_sole_current_pos()
    {
        k_expects(cur.num_polygons == 1);
        return cur.ptr.get();
    }
    inline const Polygon *get_sole_current_pos() const
    {
        k_expects(cur.num_polygons == 1);
        return cur.ptr.get();
    }
    inline Polygon *get_sole_desired_pos()
    {
        k_expects(des.num_polygons == 1);
        return des.ptr.get();
    }
    inline const Polygon *get_sole_desired_pos() const
    {
        k_expects(des.num_polygons == 1);
        return des.ptr.get();
    }
    inline MoveIntent get_move_intent() const
    {
        return move_intent;
    }
    inline void set_move_intent(MoveIntent new_intent)
    {
        move_intent = new_intent;
    }
    template<class Func> inline void for_each_cur(Func &&func)
    {
        for_each(cur, std::forward<Func>(func));
    }
    template<class Func> inline void for_each_des(Func &&func)
    {
        for_each(des, std::forward<Func>(func));
    }
    template<class Func> inline void for_each(Func &&func)
    {
        for_each(cur, std::forward<Func>(func));
        for_each(des, std::forward<Func>(func));
    }

    ///TODO: add support for multiple shapes and deleting
    ///... more funcs here ...
};

}
