#pragma once

struct text_body_type;
class CreatureEntity;
void update_autopick_expression_states(CreatureEntity &creature, text_body_type *tb);
void draw_text_editor(CreatureEntity &creature, text_body_type *tb);
