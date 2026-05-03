#include "object/warning.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/input-options.h"
#include "monster-attack/monster-attack-table.h"
#include "mspell/mspell-damage-calculator.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include <vector>

/*!
 * @brief 警告を放つアイテムを選択する
 * @return 装備品へのポインタ。警告を放つアイテムがない場合はnullptr
 */
std::shared_ptr<ItemEntity> choose_warning_item(CreatureEntity &creature)
{
    if (!creature.has_warning_flag()) {
        return nullptr;
    }

    std::vector<int> candidates;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto &item = creature.inventory[i];
        if (item->get_flags().has(TR_WARNING)) {
            candidates.push_back(i);
        }
    }

    return candidates.empty() ? nullptr : creature.inventory[rand_choice(candidates)];
}

/*!
 * @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する /
 * Calculate spell damages
 * @param m_ptr 魔法を行使するモンスターの構造体参照ポインタ
 * @param typ 効果属性のID
 * @param dam 基本ダメージ
 * @param max 算出した最大ダメージを返すポインタ
 */
static void spell_damcalc(CreatureEntity &creature, const CreatureEntity &monster, AttributeType typ, int dam, int *max)
{
    const auto &monrace = monster.get_monrace();
    int rlev = monrace.level;
    bool ignore_wraith_form = false;

    /* Vulnerability, resistance and immunity */
    switch (typ) {
    case AttributeType::ELEC:
        if (creature.has_immune_elec()) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_elec_damage_rate(creature) / 100;
        break;

    case AttributeType::POIS:
        dam = dam * calc_pois_damage_rate(creature) / 100;
        break;

    case AttributeType::ACID:
        if (creature.has_immune_acid()) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_acid_damage_rate(creature) / 100;
        break;

    case AttributeType::COLD:
    case AttributeType::ICE:
        if (creature.has_immune_cold()) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_cold_damage_rate(creature) / 100;
        break;

    case AttributeType::FIRE:
        if (creature.has_immune_fire()) {
            ignore_wraith_form = true;
        }
        dam = dam * calc_fire_damage_rate(creature) / 100;
        break;

    case AttributeType::PSY_SPEAR:
        ignore_wraith_form = true;
        break;

    case AttributeType::MONSTER_SHOOT:
        if (!creature.is_blind() && (has_invuln_arrow(creature))) {
            dam = 0;
            ignore_wraith_form = true;
        }
        break;

    case AttributeType::LITE:
        dam = dam * calc_lite_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::DARK:
        dam = dam * calc_dark_damage_rate(creature, CALC_MAX) / 100;
        if (creature.has_immune_dark() || creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM)) {
            ignore_wraith_form = true;
        }
        break;

    case AttributeType::SHARDS:
        dam = dam * calc_shards_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::SOUND:
        dam = dam * calc_sound_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::CONFUSION:
        dam = dam * calc_conf_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::CHAOS:
        dam = dam * calc_chaos_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::NETHER:
        dam = dam * calc_nether_damage_rate(creature, CALC_MAX) / 100;
        if (CreatureRace(&creature).equals(PlayerRaceType::SPECTRE)) {
            ignore_wraith_form = true;
            dam = 0;
        }
        break;

    case AttributeType::DISENCHANT:
        dam = dam * calc_disenchant_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::NEXUS:
        dam = dam * calc_nexus_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::TIME:
        dam = dam * calc_time_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::GRAVITY:
        dam = dam * calc_gravity_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::ROCKET:
        dam = dam * calc_rocket_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::NUKE:
        dam = dam * calc_nuke_damage_rate(creature) / 100;
        break;

    case AttributeType::DEATH_RAY:
        dam = dam * calc_deathray_damage_rate(creature, CALC_MAX) / 100;
        if (dam == 0) {
            ignore_wraith_form = true;
        }
        break;

    case AttributeType::HOLY_FIRE:
        dam = dam * calc_holy_fire_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::HELL_FIRE:
        dam = dam * calc_hell_fire_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::ABYSS:
        dam = dam * calc_abyss_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::VOID_MAGIC:
        dam = dam * calc_void_damage_rate(creature, CALC_MAX) / 100;
        break;

    case AttributeType::MIND_BLAST:
    case AttributeType::BRAIN_SMASH:
        if (100 + rlev / 2 <= std::max<short>(5, creature.get_skill_save())) {
            dam = 0;
            ignore_wraith_form = true;
        }

        break;

    case AttributeType::CAUSE_1:
    case AttributeType::CAUSE_2:
    case AttributeType::CAUSE_3:
    case AttributeType::HAND_DOOM:
        if (100 + rlev / 2 <= creature.get_skill_save()) {
            dam = 0;
            ignore_wraith_form = true;
        }

        break;

    case AttributeType::CAUSE_4:
        if ((100 + rlev / 2 <= creature.get_skill_save()) && (monster.r_idx != MonraceId::KENSHIROU)) {
            dam = 0;
            ignore_wraith_form = true;
        }

        break;
    default:
        break;
    }

    if (creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM) && !ignore_wraith_form) {
        dam /= 2;
        if (!dam) {
            dam = 1;
        }
    }

    if (dam > *max) {
        *max = dam;
    }
}

/*!
 * @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する。 /
 * Calculate spell damages
 * @param spell_num RF4ならRF_ABILITY::SPELL_STARTのように32区切りのベースとなる数値
 * @param typ 効果属性のID
 * @param m_idx 魔法を行使するモンスターのID
 * @param max 算出した最大ダメージを返すポインタ
 */
static void spell_damcalc_by_spellnum(CreatureEntity &creature, MonsterAbilityType ms_type, AttributeType typ, MONSTER_IDX m_idx, int *max)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    int dam = monspell_damage(creature, ms_type, m_idx, DAM_MAX);
    spell_damcalc(creature, monster, typ, dam, max);
}

/*!
 * @brief 警告基準を定めるためにモンスターの打撃最大ダメージを算出する /
 * Calculate blow damages
 * @param m_ptr 打撃を行使するモンスターの構造体参照ポインタ
 * @param blow モンスターの打撃能力の構造体参照
 * @return 算出された最大ダメージを返す。
 */
static int blow_damcalc(const CreatureEntity &monster, CreatureEntity &creature, const MonsterBlow &blow)
{
    int dam = blow.damage_dice.maxroll();
    int dummy_max = 0;

    if (blow.method == RaceBlowMethodType::EXPLODE) {
        dam = (dam + 1) / 2;
        spell_damcalc(creature, monster, mbe_info[enum2i(blow.effect)].explode_type, dam, &dummy_max);
        dam = dummy_max;
        return dam;
    }

    ARMOUR_CLASS ac = static_cast<ARMOUR_CLASS>(creature.get_ac());
    bool check_wraith_form = true;
    switch (blow.effect) {
    case RaceBlowEffectType::SUPERHURT: {
        int tmp_dam = dam - (dam * ((ac < 150) ? ac : 150) / 250);
        dam = std::max(dam, tmp_dam * 2);
        break;
    }

    case RaceBlowEffectType::HURT:
    case RaceBlowEffectType::SHATTER:
    case RaceBlowEffectType::DEFECATE:
        dam -= (dam * ((ac < 150) ? ac : 150) / 250);
        break;

    case RaceBlowEffectType::ACID:
        spell_damcalc(creature, monster, AttributeType::ACID, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::ELEC:
        spell_damcalc(creature, monster, AttributeType::ELEC, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::FIRE:
        spell_damcalc(creature, monster, AttributeType::FIRE, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::COLD:
        spell_damcalc(creature, monster, AttributeType::COLD, dam, &dummy_max);
        dam = dummy_max;
        check_wraith_form = false;
        break;

    case RaceBlowEffectType::DR_MANA:
        dam = 0;
        check_wraith_form = false;
        break;

    default:
        break;
    }

    if (check_wraith_form && creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM)) {
        dam /= 2;
        if (!dam) {
            dam = 1;
        }
    }

    return dam;
}

/*!
 * @brief プレイヤーが特定地点へ移動した場合に警告を発する処理
 * @param pos 危険性を調査する座標
 * @return 警告を無視して進むことを選択するかか問題が無ければTRUE、警告に従ったならFALSEを返す。
 */
bool process_warning(CreatureEntity &creature, const Pos2D &pos)
{
    constexpr auto warning_aware_range = 12;
    int dam_max = 0;
    static int old_damage = 0;

    auto &floor = *creature.get_floor();
    const auto &dungeon = floor.get_dungeon_definition();
    for (auto mx = pos.x - warning_aware_range; mx < pos.x + warning_aware_range + 1; mx++) {
        for (auto my = pos.y - warning_aware_range; my < pos.y + warning_aware_range + 1; my++) {
            const Pos2D pos_neighbor(my, mx);
            int dam_max0 = 0;
            if (!floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_EXCLUSIVE) || (Grid::calc_distance(pos_neighbor, pos) > warning_aware_range)) {
                continue;
            }

            const auto &grid = floor.grid_array[my][mx];

            if (!grid.has_monster()) {
                continue;
            }

            const auto &monster = floor.get_monster(grid.m_idx);

            if (monster.is_asleep()) {
                continue;
            }
            if (!monster.is_hostile()) {
                continue;
            }

            const auto &monrace = monster.get_monrace();

            /* Monster spells (only powerful ones)*/
            if (projectable(floor, pos_neighbor, pos)) {
                const auto flags = monrace.ability_flags;

                if (dungeon.flags.has_not(DungeonFeatureType::NO_MAGIC)) {
                    if (flags.has(MonsterAbilityType::BA_CHAO)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::BA_CHAO, AttributeType::CHAOS, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::BA_MANA)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::BA_MANA, AttributeType::MANA, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::BA_DARK)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::BA_DARK, AttributeType::DARK, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::BA_LITE)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::BA_LITE, AttributeType::LITE, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::HAND_DOOM)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::HAND_DOOM, AttributeType::HAND_DOOM, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::PSY_SPEAR)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::PSY_SPEAR, AttributeType::PSY_SPEAR, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::BA_VOID)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::BA_VOID, AttributeType::VOID_MAGIC, grid.m_idx, &dam_max0);
                    }
                    if (flags.has(MonsterAbilityType::BA_ABYSS)) {
                        spell_damcalc_by_spellnum(creature, MonsterAbilityType::BA_ABYSS, AttributeType::ABYSS, grid.m_idx, &dam_max0);
                    }
                }

                if (flags.has(MonsterAbilityType::ROCKET)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::ROCKET, AttributeType::ROCKET, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_ACID)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_ACID, AttributeType::ACID, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_ELEC)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_ELEC, AttributeType::ELEC, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_FIRE)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_FIRE, AttributeType::FIRE, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_COLD)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_COLD, AttributeType::COLD, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_POIS)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_POIS, AttributeType::POIS, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_NETH)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_NETH, AttributeType::NETHER, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_LITE)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_LITE, AttributeType::LITE, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_DARK)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_DARK, AttributeType::DARK, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_CONF)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_CONF, AttributeType::CONFUSION, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_SOUN)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_SOUN, AttributeType::SOUND, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_CHAO)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_CHAO, AttributeType::CHAOS, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_DISE)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_DISE, AttributeType::DISENCHANT, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_NEXU)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_NEXU, AttributeType::NEXUS, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_TIME)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_TIME, AttributeType::TIME, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_INER)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_INER, AttributeType::INERTIAL, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_GRAV)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_GRAV, AttributeType::GRAVITY, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_SHAR)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_SHAR, AttributeType::SHARDS, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_PLAS)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_PLAS, AttributeType::PLASMA, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_FORC)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_FORC, AttributeType::FORCE, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_MANA)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_MANA, AttributeType::MANA, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_NUKE)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_NUKE, AttributeType::NUKE, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_DISI)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_DISI, AttributeType::DISINTEGRATE, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_VOID)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_VOID, AttributeType::VOID_MAGIC, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_ABYSS)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_ABYSS, AttributeType::ABYSS, grid.m_idx, &dam_max0);
                }
                if (flags.has(MonsterAbilityType::BR_FECES)) {
                    spell_damcalc_by_spellnum(creature, MonsterAbilityType::BR_FECES, AttributeType::DIRT, grid.m_idx, &dam_max0);
                }
            }
            /* Monster melee attacks */
            if (monrace.behavior_flags.has(MonsterBehaviorType::NEVER_BLOW) || dungeon.flags.has(DungeonFeatureType::NO_MELEE)) {
                dam_max += dam_max0;
                continue;
            }

            if (!(mx <= pos.x + 1 && mx >= pos.x - 1 && my <= pos.y + 1 && my >= pos.y - 1)) {
                dam_max += dam_max0;
                continue;
            }

            int dam_melee = 0;
            for (const auto &blow : monrace.blows) {
                /* Skip non-attacks */
                if (blow.method == RaceBlowMethodType::NONE) {
                    continue;
                }

                /* Extract the attack info */
                dam_melee += blow_damcalc(monster, creature, blow);
                if (blow.method == RaceBlowMethodType::EXPLODE) {
                    break;
                }
            }

            if (dam_melee > dam_max0) {
                dam_max0 = dam_melee;
            }
            dam_max += dam_max0;
        }
    }

    /* Prevent excessive warning */
    if (dam_max > old_damage) {
        old_damage = dam_max * 3 / 2;

        if (dam_max > creature.hp / 2) {
            const auto &item = choose_warning_item(creature);
            std::string item_name;
            if (item) {
                item_name = describe_flavor(creature, *item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            } else {
                item_name = _("体", "body");
            }

            msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), item_name.data());

            disturb(creature, false, true);
            return input_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
        }
    } else {
        old_damage = old_damage / 2;
    }

    auto is_warning = (!easy_disarm || floor.get_grid(pos).mimic) && floor.has_trap_at(pos);
    is_warning &= !one_in_(13);
    if (!is_warning) {
        return true;
    }

    const auto &item = choose_warning_item(creature);
    std::string item_name;
    if (item) {
        item_name = describe_flavor(creature, *item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    } else {
        item_name = _("体", "body");
    }

    msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), item_name.data());
    disturb(creature, false, true);
    return input_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
}
