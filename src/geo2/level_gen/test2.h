#pragma once

#include "geo2/game.h"

namespace geo2 {

template<> class LevelGenerator<Level::Name::Test2>
{
public:
    Level generate(Game *game);
};

}
