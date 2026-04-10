#include "spell-kind/blood-curse.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "view/display-messages.h"

void blood_curse_to_enemy(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &monster = creature.current_floor_ptr->get_monster(m_idx);
    const auto &grid = creature.current_floor_ptr->grid_array[monster.y][monster.x];
    BIT_FLAGS curse_flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);
    int count = 0;
    bool is_first_loop = true;
    while (is_first_loop || one_in_(5)) {
        is_first_loop = false;
        switch (randint1(28)) {
        case 1:
        case 2:
            if (!count) {
                msg_print(_("地面が揺れた...", "The ground trembles..."));
                earthquake(creature, monster.get_position(), 4 + randint0(4), 0); // 血の呪いによる地震なのでm_idxではなく0を渡す
                if (!one_in_(6)) {
                    break;
                }
            }
            [[fallthrough]];
        case 3:
        case 4:
        case 5:
        case 6:
            if (!count) {
                int extra_dam = Dice::roll(10, 10);
                msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));
                project(creature, 0, 8, monster.y, monster.x, extra_dam, AttributeType::MANA, curse_flg);
                if (!one_in_(6)) {
                    break;
                }
            }
            [[fallthrough]];
        case 7:
        case 8:
            if (!count) {
                msg_print(_("空間が歪んだ！", "Space warps about you!"));
                if (monster.is_valid()) {
                    teleport_away(creature, grid.m_idx, Dice::roll(10, 10), TELEPORT_PASSIVE);
                }
                if (one_in_(13)) {
                    count += activate_hi_summon(creature, monster.y, monster.x, true);
                }
                if (!one_in_(6)) {
                    break;
                }
            }
            [[fallthrough]];
        case 9:
        case 10:
        case 11:
            msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
            project(creature, 0, 7, monster.y, monster.x, 50, AttributeType::DISINTEGRATE, curse_flg);
            if (!one_in_(6)) {
                break;
            }
            [[fallthrough]];
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            aggravate_monsters(creature, 0);
            if (!one_in_(6)) {
                break;
            }
            [[fallthrough]];
        case 17:
        case 18:
            count += activate_hi_summon(creature, monster.y, monster.x, true);
            if (!one_in_(6)) {
                break;
            }
            [[fallthrough]];
        case 19:
        case 20:
        case 21:
        case 22: {
            bool pet = !one_in_(3);
            BIT_FLAGS mode = PM_ALLOW_GROUP;

            if (pet) {
                mode |= PM_FORCE_PET;
            } else {
                mode |= (PM_NO_PET | PM_FORCE_FRIENDLY);
            }

            const auto level = pet ? creature.level * 2 / 3 + randint1(creature.level / 2) : creature.current_floor_ptr->dun_level;
            count += summon_specific(creature, creature.y, creature.x, level, SUMMON_NONE, mode) ? 1 : 0;
            if (!one_in_(6)) {
                break;
            }
        }
            [[fallthrough]];
        case 23:
        case 24:
        case 25:
            if (creature.hold_exp && evaluate_percent(75)) {
                break;
            }

            msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));
            if (creature.hold_exp) {
                lose_exp(creature, creature.exp / 160);
            } else {
                lose_exp(creature, creature.exp / 16);
            }
            if (!one_in_(6)) {
                break;
            }
            [[fallthrough]];
        case 26:
        case 27:
        case 28:
        default: {
            if (one_in_(13)) {
                for (int i = 0; i < A_MAX; i++) {
                    do {
                        (void)do_dec_stat(creature, i);
                    } while (one_in_(2));
                }
            } else {
                (void)do_dec_stat(creature, randint0(6));
            }

            break;
        }
        }
    }
}
