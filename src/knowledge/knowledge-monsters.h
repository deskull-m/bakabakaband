#pragma once

#include <string_view>
#include <tl/optional.hpp>

enum class MonraceId : short;
class CreatureEntity;
void do_cmd_knowledge_monsters(CreatureEntity &creature, bool *need_redraw, bool visual_only, tl::optional<MonraceId> direct_r_idx = tl::nullopt);
void do_cmd_knowledge_pets(CreatureEntity &creature);
void do_cmd_knowledge_kill_count(CreatureEntity &creature);
void do_cmd_knowledge_bounty(std::string_view player_name);
