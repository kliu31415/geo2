#pragma once

#include "geo2/map_obj/map_object.h"

namespace geo2 { namespace map_obj {

/** Generic rectangular floor
 */
class Floor_Type1: public CosmeticMapObj
{
protected:
    MapRect position;

    Floor_Type1(const MapRect &position_);
public:
    virtual ~Floor_Type1() = default;
};

}}
