#pragma once

#include "geo2/level_gen/level_generator.h"

namespace geo2 { class Game;}

namespace geo2 { namespace level_gen {

template<> class LevelGenerator<LevelName::Test2>
{
public:
    Level generate(Game *game);
};

}}
