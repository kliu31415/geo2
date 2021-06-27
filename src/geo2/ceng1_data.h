#pragma once

#include "geo2/geometry.h"

namespace geo2 {

namespace map_obj
{
    class CEng1DataReaderAttorney;
    class CEng1DataMutatorAttorney;
}

/** The vast majority of the time, there will be only one shape, so a linked
 *  list is used, which is efficient in the case of there being only 1 polygon.
 */
class CEng1Data final
{
    template<class T> struct LL_Node
    {
        T val;
        std::unique_ptr<LL_Node> next;

        LL_Node(const T &val_):
            val(val_),
            next(nullptr)
        {}

        LL_Node(T &&val_):
            val(std::move(val_)),
            next(nullptr)
        {}
    };

    using LL_Node_Polygon = std::unique_ptr<LL_Node<std::unique_ptr<Polygon>>>;

    LL_Node_Polygon cur;
    LL_Node_Polygon des;

    MoveIntent move_intent;

    inline static void push_back(LL_Node_Polygon *node, std::unique_ptr<Polygon> &&polygon)
    {
        while(*node != nullptr)
            node = &(*node)->next;
        *node = std::make_unique<LL_Node<std::unique_ptr<Polygon>>>(std::move(polygon));
    }
    template<class Func> inline static void for_each(LL_Node_Polygon *node, Func func)
    {
        int idx = 0;
        while(*node != nullptr) {
            func((*node)->val.get(), idx);
            node = &(*node)->next;
            idx++;
        }
    }
public:
    CEng1Data() = default;

    inline void add_current_pos(std::unique_ptr<Polygon> &&polygon)
    {
        push_back(&cur, std::move(polygon));
    }
    inline void add_desired_pos(std::unique_ptr<Polygon> &&polygon)
    {
        push_back(&des, std::move(polygon));
    }
    inline Polygon *get_current_pos_front()
    {
        k_expects(cur != nullptr);
        return cur->val.get();
    }
    inline const Polygon *get_current_pos_front() const
    {
        k_expects(cur != nullptr);
        return cur->val.get();
    }
    inline Polygon *get_desired_pos_front()
    {
        k_expects(des != nullptr);
        return des->val.get();
    }
    inline const Polygon *get_desired_pos_front() const
    {
        k_expects(des != nullptr);
        return des->val.get();
    }
    inline MoveIntent get_move_intent() const
    {
        return move_intent;
    }
    inline void set_move_intent(MoveIntent new_intent)
    {
        move_intent = new_intent;
    }
    template<class Func> inline void for_each_cur(Func func)
    {
        for_each(&cur, func);
    }
    template<class Func> inline void for_each_des(Func func)
    {
        for_each(&des, func);
    }
    template<class Func> inline void for_each(Func func)
    {
        for_each(&cur, func);
        for_each(&des, func);
    }

    ///TODO: add support for multiple shapes and deleting
    ///... more funcs here ...
};

}
