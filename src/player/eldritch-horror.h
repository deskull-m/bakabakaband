#pragma once

#include <tl/optional.hpp>

class CreatureEntity;
void sanity_blast(CreatureEntity &creature, tl::optional<short> m_idx = tl::nullopt, bool necro = false);
