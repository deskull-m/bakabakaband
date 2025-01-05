#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

#include <functional>

class MonraceDefinition;
SpoilerOutputResultType spoil_mon_desc_all(concptr fname);
SpoilerOutputResultType spoil_mon_desc(concptr fname, std::function<bool(const MonraceDefinition *)> filter_monster = nullptr);
SpoilerOutputResultType spoil_mon_info();
