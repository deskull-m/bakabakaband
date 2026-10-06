/*!
 * @brief 荒野マップの生成とルール管理定義
 * @date 2025/02/01
 * @author
 * Robert A. Koeneke, 1983
 * James E. Wilson, 1989
 * Deskull, 2013
 * Hourier, 2025
 */

#pragma once

extern bool reinit_wilderness;

class CreatureEntity;
void wilderness_gen(CreatureEntity &creature);
void wilderness_gen_small(CreatureEntity &creature);
void init_wilderness_encounter();
void init_wilderness_terrains();
bool change_wild_mode(CreatureEntity &creature, bool encount);
