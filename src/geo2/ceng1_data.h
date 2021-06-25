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
    friend map_obj::CEng1DataReaderAttorney;
    friend map_obj::CEng1DataMutatorAttorney;

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

    ///delete copy/now for now because it's safer that way and we prob won't use them anyway
    CEng1Data(const CEng1Data&) = delete;
    CEng1Data & operator = (const CEng1Data&) = delete;
    CEng1Data(CEng1Data&&) = delete;
    CEng1Data & operator = (CEng1Data&&) = delete;

    inline void push_back_cur(std::unique_ptr<Polygon> &&polygon)
    {
        push_back(&cur, std::move(polygon));
    }
    inline void push_back_des(std::unique_ptr<Polygon> &&polygon)
    {
        push_back(&des, std::move(polygon));
    }
    inline Polygon *cur_front()
    {
        k_expects(cur != nullptr);
        return cur->val.get();
    }
    inline const Polygon *cur_front() const
    {
        k_expects(cur != nullptr);
        return cur->val.get();
    }
    inline Polygon *des_front()
    {
        k_expects(des != nullptr);
        return des->val.get();
    }
    inline const Polygon *des_front() const
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
    ///TODO: add support for multiple shapes and deleting
public:
    CEng1Data() = default;
};

}
