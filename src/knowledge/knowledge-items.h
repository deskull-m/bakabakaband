#pragma once

class CreatureEntity;
void do_cmd_knowledge_artifacts(CreatureEntity &creature);
void do_cmd_knowledge_objects(CreatureEntity &creature, bool *need_redraw, bool visual_only, short direct_k_idx);
