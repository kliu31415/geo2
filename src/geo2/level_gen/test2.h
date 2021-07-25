#pragma once

#include "geo2/level_gen/named_level_generator.h"

namespace geo2 { class Game;}

namespace geo2 { namespace level_gen {

template<> class NamedLevelGenerator<LevelName::Test2>
{
public:
    Level generate(Game *game);
};

}}
