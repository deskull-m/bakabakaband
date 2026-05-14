/*!
 * @brief モンスターの特殊技能とターン経過処理 (移動等)/ Monster spells and movement for passaging a turn
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * This file has several additions to it by Keldon Jones (keldon@umr.edu)
 * to improve the general quality of the AI (version 0.1.1).
 */

#include "monster/monster-processor.h"
#include "avatar/avatar.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "melee/melee-postprocess.h"
#include "melee/melee-spell.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-floor/monster-direction.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-runaway.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-floor/quantum-effect.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "mspell/mspell-attack.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-util.h"
#include "object-enchant/trc-types.h"
#include "object/object-kind-hook.h"
#include "pet/pet-fall-off.h"
#include "player-base/player-class.h"
#include "player-info/ninja-data-type.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/spells-util.h"
#include "spell/summon-types.h"
#include "sv-definition/sv-junk-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-wand-types.h"
#include "system/angband-system.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/projection-path-calculator.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"
#include "world/world.h"

void decide_drop_from_monster(CreatureEntity &creature, MONSTER_IDX m_idx, bool is_riding_mon);
bool process_stealth(CreatureEntity &creature, MONSTER_IDX m_idx);
bool vanish_summoned_children(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m);
bool awake_monster(CreatureEntity &creature, MONSTER_IDX m_idx);
void process_angar(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m);
bool explode_grenade(CreatureEntity &creature, MONSTER_IDX m_idx);
bool decide_monster_multiplication(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION oy, POSITION ox);
void process_monster_change_feat(CreatureEntity &creature, MONSTER_IDX m_idx);
bool process_monster_spawn_monster(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION oy, POSITION ox);
void process_monster_spawn_item(CreatureEntity &creature, MONSTER_IDX m_idx);
void process_monster_spawn_zanki(CreatureEntity &creature, MONSTER_IDX m_idx);
void process_special(CreatureEntity &creature, MONSTER_IDX m_idx);
bool cast_spell(CreatureEntity &creature, MONSTER_IDX m_idx, bool aware);

bool process_monster_fear(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);

void sweep_monster_process(CreatureEntity &creature);
bool decide_process_continue(CreatureEntity &creature, CreatureEntity &monster);
bool process_stalking(CreatureEntity &creature, MONSTER_IDX m_idx);

constexpr auto STALKER_CHANCE_DENOMINATOR = 32; //!< モンスターが背後に忍び寄る確率分母
constexpr auto STALKER_DISTANCE_THRESHOLD = 20; //!< モンスターが背後に忍び寄る距離の閾値

/*!
 * @brief モンスター AI が「戦闘中」と見做せる状況かを判定 (フェーズ C-1 拡張)
 * @param creature 視点クリーチャー (プレイヤー)
 * @param monster 判定対象モンスター
 * @return 敵対モンスターはプレイヤー視認時、ペットは hostile monster 視認時に true
 * @details モンスターのバフ系/攻撃系アイテム使用判定で共有される判定。
 *          ペットでも hostile monster が近くにいれば fighting_context = true として
 *          バフ・攻撃用アイテムを発動できるようにする。
 */
static bool ai_is_in_fighting_context(const CreatureEntity &creature, const CreatureEntity &monster)
{
    (void)creature;
    if (!monster.is_visible_on_map()) {
        return false;
    }
    if (monster.is_hostile()) {
        return true;
    }
    if (monster.is_pet()) {
        return monster.has_visible_creature(
            [](const CreatureEntity &c) { return c.is_hostile() && c.is_visible_on_map(); });
    }
    return false;
}

/*!
 * @brief モンスターがポーションを飲んで状況に応じた効果を得る (フェーズ C-1 拡張)
 * @param creature 視点クリーチャー (メッセージ表示用)
 * @param monster ポーションを飲むモンスター
 * @return 飲んだら true
 * @details
 * 状況判定の優先度:
 *  1. HP 25%未満 で Star Healing/Life で大回復
 *  2. HP 50%未満 で Healing/Cure系 で中回復
 *  3. HP 25%未満 で Invulnerability で無敵化
 *  4. 強毒 (POISON > 100) で Cure Poison で解毒
 *  5. 恐怖時 で Boldness/Heroism/Berserk で恐怖解除 + バフ
 *  6. 戦闘想定で 加速なし & Speed なら加速
 *  7. 戦闘想定で 元素耐性なし & Resistance で 5 元素耐性
 *  8. 戦闘想定で バフなし & Heroism/Berserk でバフ
 *
 * quaff/zap effect path 全体のリファクタは大規模なため、各効果は
 * monster の HP 加算 / set_timed_effect() の直接呼出で簡易実装する。
 * プレイヤー固有の virtue 変動・gain_exp 等は適用しない。
 */
static bool monster_quaff_potion(CreatureEntity &creature, CreatureEntity &monster)
{
    const bool low_hp_severe = monster.hp < monster.maxhp / 4;
    const bool low_hp_mid = monster.hp < monster.maxhp / 2;
    const bool poisoned_heavy = monster.get_timed_effect(CreatureTimedEffect::POISON) > 100;
    const bool fearful = monster.is_fearful();
    const bool not_fast = monster.get_timed_effect(CreatureTimedEffect::ACCELERATION) == 0;
    const bool not_resistant = monster.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) == 0 && monster.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) == 0;
    const bool not_buffed = monster.get_timed_effect(CreatureTimedEffect::HERO) == 0 && monster.get_timed_effect(CreatureTimedEffect::BERSERK) == 0;
    const bool fighting_context = ai_is_in_fighting_context(creature, monster);

    INVENTORY_IDX potion_slot = -1;
    int priority = -1; // 0=最優先 ... 大=後回し
    auto select_if = [&](INVENTORY_IDX slot, int p) {
        if (priority < 0 || p < priority) {
            potion_slot = slot;
            priority = p;
        }
    };
    for (size_t i = 0; i < monster.inventory.size(); i++) {
        const auto &item = *monster.inventory[i];
        if (!item.is_valid() || item.bi_key.tval() != ItemKindType::POTION) {
            continue;
        }
        const auto sval = item.bi_key.sval().value_or(0);
        const auto idx = static_cast<INVENTORY_IDX>(i);
        switch (sval) {
        case SV_POTION_STAR_HEALING:
        case SV_POTION_LIFE:
            if (low_hp_severe) {
                select_if(idx, 0);
            }
            break;
        case SV_POTION_HEALING:
            if (low_hp_severe) {
                select_if(idx, 1);
            }
            break;
        case SV_POTION_INVULNERABILITY:
            if (low_hp_severe) {
                select_if(idx, 2);
            }
            break;
        case SV_POTION_CURE_CRITICAL:
            if (low_hp_mid) {
                select_if(idx, 3);
            }
            break;
        case SV_POTION_CURE_SERIOUS:
            if (low_hp_mid) {
                select_if(idx, 4);
            }
            break;
        case SV_POTION_CURE_LIGHT:
            if (low_hp_mid) {
                select_if(idx, 5);
            }
            break;
        case SV_POTION_CURE_POISON:
            if (poisoned_heavy) {
                select_if(idx, 6);
            }
            break;
        case SV_POTION_BOLDNESS:
            if (fearful) {
                select_if(idx, 7);
            }
            break;
        case SV_POTION_BESERK_STRENGTH:
            if (fearful || (fighting_context && not_buffed)) {
                select_if(idx, 8);
            }
            break;
        case SV_POTION_HEROISM:
            if (fearful || (fighting_context && not_buffed)) {
                select_if(idx, 9);
            }
            break;
        case SV_POTION_SPEED:
            if (fighting_context && not_fast) {
                select_if(idx, 10);
            }
            break;
        case SV_POTION_RESISTANCE:
            if (fighting_context && not_resistant) {
                select_if(idx, 11);
            }
            break;
        default:
            break;
        }
    }
    if (potion_slot < 0) {
        return false;
    }

    auto &potion = *monster.inventory[potion_slot];
    const auto sval = potion.bi_key.sval().value_or(0);
    const auto potion_name = describe_flavor(creature, potion, OD_OMIT_PREFIX);
    const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
    if (is_seen(creature, monster)) {
        msg_format(_("%s^は%sを飲んだ。", "%s^ quaffs %s."), m_name.data(), potion_name.data());
    }

    auto heal = [&](int amount) {
        monster.hp = std::min<int>(monster.maxhp, monster.hp + amount);
    };
    auto add_timed = [&](CreatureTimedEffect effect, int duration) {
        const auto current = monster.get_timed_effect(effect);
        monster.set_timed_effect(effect, static_cast<short>(std::min<int>(MAX_SHORT, current + duration)));
    };

    switch (sval) {
    case SV_POTION_CURE_LIGHT:
        heal(Dice::roll(2, 8));
        monster.set_timed_effect(CreatureTimedEffect::CUT, static_cast<short>(std::max<int>(0, monster.get_timed_effect(CreatureTimedEffect::CUT) - 10)));
        break;
    case SV_POTION_CURE_SERIOUS:
        heal(Dice::roll(4, 8));
        monster.set_timed_effect(CreatureTimedEffect::CUT, static_cast<short>(std::max<int>(0, monster.get_timed_effect(CreatureTimedEffect::CUT) / 2 - 50)));
        break;
    case SV_POTION_CURE_CRITICAL:
        heal(Dice::roll(6, 8));
        monster.set_timed_effect(CreatureTimedEffect::CUT, 0);
        monster.set_timed_effect(CreatureTimedEffect::STUN, 0);
        monster.set_timed_effect(CreatureTimedEffect::POISON, 0);
        break;
    case SV_POTION_HEALING:
        heal(300);
        monster.set_timed_effect(CreatureTimedEffect::CUT, 0);
        monster.set_timed_effect(CreatureTimedEffect::STUN, 0);
        monster.set_timed_effect(CreatureTimedEffect::POISON, 0);
        break;
    case SV_POTION_STAR_HEALING:
    case SV_POTION_LIFE:
        heal(1200);
        monster.set_timed_effect(CreatureTimedEffect::CUT, 0);
        monster.set_timed_effect(CreatureTimedEffect::STUN, 0);
        monster.set_timed_effect(CreatureTimedEffect::POISON, 0);
        monster.set_timed_effect(CreatureTimedEffect::FEAR, 0);
        break;
    case SV_POTION_INVULNERABILITY:
        add_timed(CreatureTimedEffect::INVULNERABILITY, randint1(8) + 8);
        break;
    case SV_POTION_CURE_POISON:
        monster.set_timed_effect(CreatureTimedEffect::POISON, 0);
        break;
    case SV_POTION_BOLDNESS:
        monster.set_timed_effect(CreatureTimedEffect::FEAR, 0);
        break;
    case SV_POTION_HEROISM:
        monster.set_timed_effect(CreatureTimedEffect::FEAR, 0);
        add_timed(CreatureTimedEffect::HERO, randint1(25) + 25);
        heal(10);
        break;
    case SV_POTION_BESERK_STRENGTH:
        monster.set_timed_effect(CreatureTimedEffect::FEAR, 0);
        monster.set_timed_effect(CreatureTimedEffect::STUN, 0);
        add_timed(CreatureTimedEffect::BERSERK, randint1(25) + 25);
        heal(30);
        break;
    case SV_POTION_SPEED:
        if (monster.get_timed_effect(CreatureTimedEffect::ACCELERATION) == 0) {
            monster.set_timed_effect(CreatureTimedEffect::ACCELERATION, randint1(25) + 15);
        } else {
            add_timed(CreatureTimedEffect::ACCELERATION, 5);
        }
        break;
    case SV_POTION_RESISTANCE: {
        const int dur = randint1(20) + 20;
        add_timed(CreatureTimedEffect::OPPOSE_ACID, dur);
        add_timed(CreatureTimedEffect::OPPOSE_ELEC, dur);
        add_timed(CreatureTimedEffect::OPPOSE_FIRE, dur);
        add_timed(CreatureTimedEffect::OPPOSE_COLD, dur);
        add_timed(CreatureTimedEffect::OPPOSE_POIS, dur);
        break;
    }
    default:
        break;
    }

    // ポーションを 1 個消費
    if (potion.number > 1) {
        potion.number--;
    } else {
        potion.wipe();
        if (monster.get_inven_cnt() > 0) {
        }
    }
    return true;
}

/*!
 * @brief モンスターが巻物を読んで状況に応じた効果を得る (フェーズ C-1 拡張)
 * @param creature 視点クリーチャー (メッセージ表示用)
 * @param monster 巻物を読むモンスター
 * @param m_idx モンスターの floor インデックス
 * @return 読んだら true
 * @details
 * 状況判定:
 *  - HP 25%未満 で SCROLL_TELEPORT/PHASE_DOOR で離脱
 *  - HP 25%未満 で SCROLL_HOLY_PRAYER (即時 HP 回復用) — 未対応 (player only)
 */
static bool monster_read_scroll(CreatureEntity &creature, CreatureEntity &monster, MONSTER_IDX m_idx)
{
    const bool low_hp_severe = monster.hp < monster.maxhp / 4;
    const bool fighting_context = monster.is_hostile() && monster.is_visible_on_map();

    enum class ScrollAction { TELEPORT,
        TELEPORT_LEVEL,
        PHASE_DOOR,
        SUMMON,
        DESTRUCTION };
    INVENTORY_IDX scroll_slot = -1;
    int priority = -1; // 小=優先
    int teleport_dist = 0;
    ScrollAction action = ScrollAction::TELEPORT;
    summon_type summon_kind = SUMMON_NONE;
    int destruction_radius = 0;
    auto select_if = [&](INVENTORY_IDX slot, int p, ScrollAction act, int dist = 0, summon_type st = SUMMON_NONE, int radius = 0) {
        if (priority < 0 || p < priority) {
            scroll_slot = slot;
            priority = p;
            action = act;
            teleport_dist = dist;
            summon_kind = st;
            destruction_radius = radius;
        }
    };
    for (size_t i = 0; i < monster.inventory.size(); i++) {
        const auto &item = *monster.inventory[i];
        if (!item.is_valid() || item.bi_key.tval() != ItemKindType::SCROLL) {
            continue;
        }
        const auto sval = item.bi_key.sval().value_or(0);
        const auto idx = static_cast<INVENTORY_IDX>(i);
        switch (sval) {
        // 脱出系 (HP 危機時のみ)
        case SV_SCROLL_TELEPORT_LEVEL:
            if (low_hp_severe) {
                select_if(idx, 0, ScrollAction::TELEPORT_LEVEL);
            }
            break;
        case SV_SCROLL_TELEPORT:
            if (low_hp_severe) {
                select_if(idx, 1, ScrollAction::TELEPORT, 200);
            }
            break;
        case SV_SCROLL_PHASE_DOOR:
            if (low_hp_severe) {
                select_if(idx, 2, ScrollAction::PHASE_DOOR, 10);
            }
            break;
        // 召喚系 (戦闘中なら使用)
        case SV_SCROLL_SUMMON_MONSTER:
            if (fighting_context) {
                select_if(idx, 8, ScrollAction::SUMMON, 0, SUMMON_NONE);
            }
            break;
        case SV_SCROLL_SUMMON_UNDEAD:
            if (fighting_context) {
                select_if(idx, 8, ScrollAction::SUMMON, 0, SUMMON_UNDEAD);
            }
            break;
        case SV_SCROLL_SUMMON_KIN:
            if (fighting_context) {
                select_if(idx, 7, ScrollAction::SUMMON, 0, SUMMON_KIN);
            }
            break;
        // 破壊系 (HP 危機 + 戦闘中で発動、自身の周囲を破壊して脱出)
        case SV_SCROLL_STAR_DESTRUCTION:
            if (low_hp_severe && fighting_context) {
                select_if(idx, 4, ScrollAction::DESTRUCTION, 0, SUMMON_NONE, 24);
            }
            break;
        default:
            break;
        }
    }
    if (scroll_slot < 0) {
        return false;
    }

    auto &scroll = *monster.inventory[scroll_slot];
    const auto scroll_name = describe_flavor(creature, scroll, OD_OMIT_PREFIX);
    const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
    if (is_seen(creature, monster)) {
        msg_format(_("%s^は%sを読んだ。", "%s^ reads %s."), m_name.data(), scroll_name.data());
    }

    switch (action) {
    case ScrollAction::TELEPORT_LEVEL:
        teleport_level(creature, m_idx);
        break;
    case ScrollAction::TELEPORT:
    case ScrollAction::PHASE_DOOR:
        teleport_away(creature, m_idx, teleport_dist, TELEPORT_SPONTANEOUS);
        break;
    case ScrollAction::SUMMON:
        // モンスター位置から自身のレベル相当で召喚 (3 体まで試行)
        for (int n = 0; n < 3; n++) {
            (void)summon_specific(creature, monster.y, monster.x, monster.get_monrace().level, summon_kind, PM_NONE, m_idx);
        }
        break;
    case ScrollAction::DESTRUCTION:
        // 周囲一帯を破壊 (壁・モンスター・アイテム消滅)
        // 自分自身も巻き込まれるため、HP 危機時の最終手段として使用
        (void)destroy_area(creature, monster.y, monster.x, destruction_radius, false);
        break;
    }

    // 巻物を 1 個消費
    if (scroll.number > 1) {
        scroll.number--;
    } else {
        scroll.wipe();
        if (monster.get_inven_cnt() > 0) {
        }
    }
    return true;
}

/*!
 * @brief モンスターが杖/ロッドを起動して状況に応じた効果を得る (フェーズ C-1 拡張)
 * @param creature 視点クリーチャー
 * @param monster 起動するモンスター
 * @return 起動したら true
 * @details
 *  対応 wand (sval / 発動条件 / 効果):
 *   - HEAL_MONSTER : HP < 50% / HP+(20) クランプ
 *   - HASTE_MONSTER: 戦闘&未加速 / ACCELERATION 加算
 *  対応 rod:
 *   - HEALING : HP < 50% / HP+500
 *   - SPEED   : 戦闘&未加速 / ACCELERATION 加算
 *   - CURING  : 状態異常時 / FEAR/CONFUSION/STUN/POISON リセット
 *  消費:
 *   - 杖: pval-- (charges 1 消費)
 *   - ロッド: timeout += base_pval (再充填時間設定)
 */
static bool monster_use_wand_or_rod(CreatureEntity &creature, CreatureEntity &monster, MONSTER_IDX m_idx)
{
    const bool low_hp_mid = monster.hp < monster.maxhp / 2;
    const bool not_fast = monster.get_timed_effect(CreatureTimedEffect::ACCELERATION) == 0;
    const bool has_status = monster.is_fearful() || monster.is_confused() || monster.is_stunned() || monster.get_timed_effect(CreatureTimedEffect::POISON) > 0;

    // [ペット AI 拡張] モンスターの場合: target = プレイヤー
    //                  ペットの場合: target = 最寄りの hostile monster (projectable)
    // [提案 14] AI ターゲット選定共通化
    auto &floor = *creature.get_floor();
    POSITION attack_target_y = creature.y;
    POSITION attack_target_x = creature.x;
    bool can_attack_target = false;
    const bool fighting_context = ai_is_in_fighting_context(creature, monster);
    if (monster.is_pet()) {
        const auto target_idx = monster.find_nearest_creature(
            [](const CreatureEntity &c) { return c.is_hostile() && c.is_visible_on_map(); }, true);
        if (target_idx > 0) {
            const auto &target_mon = floor.get_monster(target_idx);
            attack_target_y = target_mon.y;
            attack_target_x = target_mon.x;
            can_attack_target = true;
        }
    } else {
        can_attack_target = fighting_context && projectable(floor, monster.get_position(), creature.get_position());
    }
    // 旧来名 can_target_player を残してコード互換性を保つ (実際には pet の場合 hostile monster)
    const bool can_target_player = can_attack_target;

    INVENTORY_IDX device_slot = -1;
    int priority = -1;
    auto select_if = [&](INVENTORY_IDX slot, int p) {
        if (priority < 0 || p < priority) {
            device_slot = slot;
            priority = p;
        }
    };
    for (size_t i = 0; i < monster.inventory.size(); i++) {
        const auto &item = *monster.inventory[i];
        if (!item.is_valid()) {
            continue;
        }
        const auto idx = static_cast<INVENTORY_IDX>(i);
        const auto sval = item.bi_key.sval().value_or(0);
        if (item.bi_key.tval() == ItemKindType::WAND) {
            // 杖: pval > 0 で残充填あり
            if (item.pval <= 0) {
                continue;
            }
            switch (sval) {
            case SV_WAND_HEAL_MONSTER:
                if (low_hp_mid) {
                    select_if(idx, 1);
                }
                break;
            case SV_WAND_HASTE_MONSTER:
                if (fighting_context && not_fast) {
                    select_if(idx, 5);
                }
                break;
            // [攻撃用 wand] プレイヤー標的、可視 & 直射可能な敵対モンスターのみ
            case SV_WAND_MAGIC_MISSILE:
            case SV_WAND_ACID_BOLT:
            case SV_WAND_FIRE_BOLT:
            case SV_WAND_COLD_BOLT:
            case SV_WAND_HYPODYNAMIA:
            case SV_WAND_STINKING_CLOUD:
            case SV_WAND_ACID_BALL:
            case SV_WAND_ELEC_BALL:
            case SV_WAND_FIRE_BALL:
            case SV_WAND_COLD_BALL:
            case SV_WAND_DRAGON_FIRE:
            case SV_WAND_DRAGON_COLD:
            case SV_WAND_DRAGON_BREATH:
                if (can_target_player) {
                    select_if(idx, 10);
                }
                break;
            // [防衛用 wand] プレイヤーをテレポートで離す (HP 危機時)
            case SV_WAND_TELEPORT_AWAY:
                if (can_target_player && low_hp_mid) {
                    select_if(idx, 3);
                }
                break;
            // [自己分裂 wand] 自分の分身を作成 (戦闘中、ニッチだが強力)
            case SV_WAND_CLONE_MONSTER:
                if (fighting_context) {
                    select_if(idx, 9);
                }
                break;
            default:
                break;
            }
        } else if (item.bi_key.tval() == ItemKindType::ROD) {
            // ロッド: timeout が base_pval * (number-1) 以下で 1 個以上使用可能
            const auto base_pval = item.get_baseitem_pval();
            if (item.number == 1 && item.timeout > 0) {
                continue;
            }
            if (item.number > 1 && item.timeout > base_pval * (item.number - 1)) {
                continue;
            }
            switch (sval) {
            case SV_ROD_HEALING:
                if (low_hp_mid) {
                    select_if(idx, 0);
                }
                break;
            case SV_ROD_SPEED:
                if (fighting_context && not_fast) {
                    select_if(idx, 4);
                }
                break;
            case SV_ROD_CURING:
                if (has_status) {
                    select_if(idx, 6);
                }
                break;
            // [攻撃用 rod] プレイヤー標的、可視 & 直射可能な敵対モンスターのみ
            case SV_ROD_ACID_BOLT:
            case SV_ROD_ELEC_BOLT:
            case SV_ROD_FIRE_BOLT:
            case SV_ROD_COLD_BOLT:
            case SV_ROD_HYPODYNAMIA:
            case SV_ROD_ACID_BALL:
            case SV_ROD_ELEC_BALL:
            case SV_ROD_FIRE_BALL:
            case SV_ROD_COLD_BALL:
                if (can_target_player) {
                    select_if(idx, 11);
                }
                break;
            default:
                break;
            }
        }
    }
    if (device_slot < 0) {
        return false;
    }

    auto &device = *monster.inventory[device_slot];
    const auto sval = device.bi_key.sval().value_or(0);
    const auto device_name = describe_flavor(creature, device, OD_OMIT_PREFIX);
    const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
    const bool is_wand = device.bi_key.tval() == ItemKindType::WAND;
    if (is_seen(creature, monster)) {
        if (is_wand) {
            msg_format(_("%s^は%sを振った。", "%s^ aims %s."), m_name.data(), device_name.data());
        } else {
            msg_format(_("%s^は%sを振った。", "%s^ zaps %s."), m_name.data(), device_name.data());
        }
    }

    auto heal = [&](int amount) {
        monster.hp = std::min<int>(monster.maxhp, monster.hp + amount);
    };
    auto add_timed = [&](CreatureTimedEffect effect, int duration) {
        const auto current = monster.get_timed_effect(effect);
        monster.set_timed_effect(effect, static_cast<short>(std::min<int>(MAX_SHORT, current + duration)));
    };

    // 攻撃系の標的座標 (敵対モンスターはプレイヤー、ペットは hostile monster)
    const auto target_y = attack_target_y;
    const auto target_x = attack_target_x;
    constexpr BIT_FLAGS flg_bolt = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    constexpr BIT_FLAGS flg_ball = PROJECT_STOP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM;
    auto fire_bolt_at_player = [&](AttributeType typ, int dam) {
        project(creature, m_idx, 0, target_y, target_x, dam, typ, flg_bolt);
    };
    auto fire_ball_at_player = [&](AttributeType typ, int dam, int rad) {
        project(creature, m_idx, rad, target_y, target_x, dam, typ, flg_ball);
    };

    if (is_wand) {
        switch (sval) {
        case SV_WAND_HEAL_MONSTER:
            heal(20);
            break;
        case SV_WAND_HASTE_MONSTER:
            add_timed(CreatureTimedEffect::ACCELERATION, 100 + randint1(100));
            break;
        case SV_WAND_MAGIC_MISSILE:
            fire_bolt_at_player(AttributeType::MISSILE, Dice::roll(3, 4));
            break;
        case SV_WAND_ACID_BOLT:
            fire_bolt_at_player(AttributeType::ACID, Dice::roll(10, 8));
            break;
        case SV_WAND_FIRE_BOLT:
            fire_bolt_at_player(AttributeType::FIRE, Dice::roll(9, 8));
            break;
        case SV_WAND_COLD_BOLT:
            fire_bolt_at_player(AttributeType::COLD, Dice::roll(6, 8));
            break;
        case SV_WAND_HYPODYNAMIA:
            fire_bolt_at_player(AttributeType::HYPODYNAMIA, 80);
            break;
        case SV_WAND_STINKING_CLOUD:
            fire_ball_at_player(AttributeType::POIS, 12, 2);
            break;
        case SV_WAND_ACID_BALL:
            fire_ball_at_player(AttributeType::ACID, 60, 2);
            break;
        case SV_WAND_ELEC_BALL:
            fire_ball_at_player(AttributeType::ELEC, 32, 2);
            break;
        case SV_WAND_FIRE_BALL:
            fire_ball_at_player(AttributeType::FIRE, 72, 2);
            break;
        case SV_WAND_COLD_BALL:
            fire_ball_at_player(AttributeType::COLD, 48, 2);
            break;
        case SV_WAND_DRAGON_FIRE:
            fire_ball_at_player(AttributeType::FIRE, 200, 3);
            break;
        case SV_WAND_DRAGON_COLD:
            fire_ball_at_player(AttributeType::COLD, 180, 3);
            break;
        case SV_WAND_DRAGON_BREATH: {
            // 5 属性からランダム選択 (player 版 cmd-zapwand.cpp と整合)
            constexpr std::array<std::pair<int, AttributeType>, 5> candidates = { {
                { 240, AttributeType::ACID },
                { 210, AttributeType::ELEC },
                { 240, AttributeType::FIRE },
                { 210, AttributeType::COLD },
                { 180, AttributeType::POIS },
            } };
            const auto [dam, type] = rand_choice(candidates);
            fire_ball_at_player(type, dam, 3);
            break;
        }
        case SV_WAND_TELEPORT_AWAY:
            fire_bolt_at_player(AttributeType::AWAY_ALL, 100);
            break;
        case SV_WAND_CLONE_MONSTER:
            // モンスター自身の分身を生成 (PM_NO_PET で勝手に味方にならないよう)
            (void)multiply_monster(creature, m_idx, monster.r_idx, true, PM_NO_PET);
            break;
        }
        device.pval--;
    } else {
        // ロッド
        switch (sval) {
        case SV_ROD_HEALING:
            heal(500);
            break;
        case SV_ROD_SPEED:
            add_timed(CreatureTimedEffect::ACCELERATION, 15 + randint1(25));
            break;
        case SV_ROD_CURING:
            monster.set_timed_effect(CreatureTimedEffect::FEAR, 0);
            monster.set_timed_effect(CreatureTimedEffect::CONFUSION, 0);
            monster.set_timed_effect(CreatureTimedEffect::STUN, 0);
            monster.set_timed_effect(CreatureTimedEffect::POISON, 0);
            break;
        case SV_ROD_ACID_BOLT:
            fire_bolt_at_player(AttributeType::ACID, Dice::roll(12, 8));
            break;
        case SV_ROD_ELEC_BOLT:
            fire_bolt_at_player(AttributeType::ELEC, Dice::roll(6, 6));
            break;
        case SV_ROD_FIRE_BOLT:
            fire_bolt_at_player(AttributeType::FIRE, Dice::roll(16, 8));
            break;
        case SV_ROD_COLD_BOLT:
            fire_bolt_at_player(AttributeType::COLD, Dice::roll(10, 8));
            break;
        case SV_ROD_HYPODYNAMIA:
            fire_bolt_at_player(AttributeType::HYPODYNAMIA, 75);
            break;
        case SV_ROD_ACID_BALL:
            fire_ball_at_player(AttributeType::ACID, 60, 2);
            break;
        case SV_ROD_ELEC_BALL:
            fire_ball_at_player(AttributeType::ELEC, 32, 2);
            break;
        case SV_ROD_FIRE_BALL:
            fire_ball_at_player(AttributeType::FIRE, 72, 2);
            break;
        case SV_ROD_COLD_BALL:
            fire_ball_at_player(AttributeType::COLD, 48, 2);
            break;
        }
        device.timeout += device.get_baseitem_pval();
    }
    return true;
}

/*!
 * @brief モンスター単体の1ターン行動処理メインルーチン /
 * Process a monster
 * @param creature クリーチャーへの参照
 * @param m_idx 行動モンスターの参照ID
 * @details
 * The monster is known to be within 100 grids of the creature\n
 *\n
 * In several cases, we directly update the monster lore\n
 *\n
 * Note that a monster is only allowed to "reproduce" if there\n
 * are a limited number of "reproducing" monsters on the current\n
 * level.  This should prevent the level from being "swamped" by\n
 * reproducing monsters.  It also allows a large mass of mice to\n
 * prevent a louse from multiplying, but this is a small price to\n
 * pay for a simple ENERGY_MULTIPLICATION method.\n
 *\n
 * XXX Monster fear is slightly odd, in particular, monsters will\n
 * fixate on opening a door even if they cannot open it.  Actually,\n
 * the same thing happens to normal monsters when they hit a door\n
 *\n
 * In addition, monsters which *cannot* open or bash\n
 * down a door will still stand there trying to open it...\n
 *\n
 * XXX Technically, need to check for monster in the way\n
 * combined with that monster being in a wall (or door?)\n
 *\n
 * A "direction" of "5" means "pick a random direction".\n
 */
void process_monster(CreatureEntity &creature, MONSTER_IDX m_idx)
{

    auto &monster = creature.get_floor()->get_monster(m_idx);
    turn_flags tmp_flags;
    turn_flags *turn_flags_ptr = init_turn_flags(monster.is_riding(), &tmp_flags);
    turn_flags_ptr->see_m = is_seen(creature, monster);

    decide_drop_from_monster(creature, m_idx, turn_flags_ptr->is_riding_mon);
    if (monster.is_chameleon() && one_in_(13) && !monster.is_asleep()) {
        const auto &floor = *creature.get_floor();
        const auto old_m_name = monster_desc(creature, monster, 0);
        const auto &monrace = monster.get_monrace();
        const auto m_pos = monster.get_position();
        const auto &grid = floor.get_grid(m_pos);
        choose_chameleon_polymorph(creature, m_idx, grid.get_terrain_id());
        update_monster(creature, m_idx, false);
        lite_spot(creature, m_pos);
        const auto &new_monrace = monster.get_monrace();

        if (new_monrace.brightness_flags.has_any_of(ld_mask) || monrace.brightness_flags.has_any_of(ld_mask)) {
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
        }

        if (turn_flags_ptr->is_riding_mon) {
            msg_format(_("突然%sが変身した。", "Suddenly, %s transforms!"), old_m_name.data());
            if (new_monrace.misc_flags.has_not(MonsterMiscType::RIDING)) {
                if (process_fall_off_horse(creature, 0, true)) {
                    const auto m_name = monster_desc(creature, monster, 0);
                    msg_print(_("地面に落とされた。", format("You have fallen from %s.", m_name.data())));
                }
            }
        }

        monster.set_individual_speed(floor.inside_arena);

        const auto old_maxhp = monster.max_maxhp;
        if (new_monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
            monster.max_maxhp = new_monrace.hit_dice.maxroll();
        } else {
            monster.max_maxhp = new_monrace.hit_dice.roll();
        }

        if (ironman_nightmare) {
            const auto hp = monster.max_maxhp * 2;
            monster.max_maxhp = std::min(MONSTER_MAXHP, hp);
        }

        monster.maxhp = monster.maxhp * monster.max_maxhp / old_maxhp;
        if (monster.maxhp < 1) {
            monster.maxhp = 1;
        }
        monster.hp = monster.hp * monster.max_maxhp / old_maxhp;
        monster.dealt_damage = 0;
    }

    auto &monrace = monster.get_monrace();

    mark_monsters_present(creature);

    turn_flags_ptr->aware = process_stealth(creature, m_idx);
    if (monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY_STANDBY) && monster.is_friendly()) {
        return;
    }

    if (vanish_summoned_children(creature, m_idx, turn_flags_ptr->see_m)) {
        return;
    }

    if (process_quantum_effect(creature, m_idx, turn_flags_ptr->see_m)) {
        return;
    }

    if (explode_grenade(creature, m_idx)) {
        return;
    }

    if (runaway_monster(creature, turn_flags_ptr, m_idx)) {
        return;
    }

    if (!awake_monster(creature, m_idx)) {
        return;
    }

    if (monster.is_stunned() && one_in_(2)) {
        return;
    }

    // [フェーズ C-1] ポーション自己使用 (回復/解毒/バフ等)
    if (monster_quaff_potion(creature, monster)) {
        // ポーション使用でターンを消費。次のターン処理は通常通り続行する
        // (move energy は別途 turn 管理で扱う)
    }

    // [フェーズ C-1 拡張] 巻物使用 (テレポートで離脱)
    if (monster_read_scroll(creature, monster, m_idx)) {
        // テレポートが成功すると monster はもう近くにいない可能性があるが、
        // 後続のチェックは monster へのポインタ経由なので継続可能
    }

    // [フェーズ C-1 拡張] 杖/ロッド使用 (自己回復/自己加速/状態回復/プレイヤー攻撃)
    if (monster_use_wand_or_rod(creature, monster, m_idx)) {
        // 同上、ターン消費とは別に副次行動として処理
    }

    if (process_stalking(creature, m_idx)) {
        return;
    }

    if (turn_flags_ptr->is_riding_mon) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    process_angar(creature, m_idx, turn_flags_ptr->see_m);

    POSITION oy = monster.y;
    POSITION ox = monster.x;
    if (decide_monster_multiplication(creature, m_idx, oy, ox)) {
        return;
    }

    if (process_monster_spawn_monster(creature, m_idx, oy, ox)) {
        return;
    }

    process_monster_spawn_item(creature, m_idx);
    process_monster_spawn_zanki(creature, m_idx);
    process_monster_change_feat(creature, m_idx);
    process_special(creature, m_idx);
    process_sound(creature, m_idx);
    process_speak(creature, m_idx, oy, ox, turn_flags_ptr->aware);

    // 狂乱状態のモンスターは魔法を使わず、プレイヤーに隣接していれば攻撃のみを行う
    if (monster.is_frenzied()) {
        const auto dy = std::abs(monster.y - creature.y);
        const auto dx = std::abs(monster.x - creature.x);
        if (dy <= 1 && dx <= 1 && turn_flags_ptr->aware) {
            MonsterAttackPlayer(creature, m_idx).make_attack_normal();
        }
        return;
    }

    if (cast_spell(creature, m_idx, turn_flags_ptr->aware)) {
        return;
    }

    const auto mmdl = decide_monster_movement_direction(creature, m_idx, turn_flags_ptr->aware);
    if (!mmdl) {
        return;
    }

    int count = 0;
    if (!process_monster_movement(creature, turn_flags_ptr, *mmdl, { oy, ox }, &count)) {
        return;
    }

    /*
     *  Forward movements failed, but now received LOS attack!
     *  Try to flow by smell.
     */
    if (creature.no_flowed && count > 2 && monster.get_target_position().y) {
        monster.reset_constant_flag(MonsterConstantFlagType::NOFLOW);
    }

    if (!turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && !monster.is_fearful() && !turn_flags_ptr->is_riding_mon && turn_flags_ptr->aware) {
        if (monrace.freq_spell && randint1(100) <= monrace.freq_spell) {
            if (make_attack_spell(creature, m_idx)) {
                return;
            }
        }
    }

    update_map_flags(turn_flags_ptr);
    update_lite_flags(turn_flags_ptr, monrace);
    update_monster_race_flags(creature, turn_flags_ptr, monster);

    if (!process_monster_fear(creature, turn_flags_ptr, m_idx)) {
        return;
    }

    if (monster.is_visible_on_map()) {
        chg_virtue(creature, Virtue::COMPASSION, -1);
    }
}

/*!
 * @brief 超隠密処理
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @return モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 */
bool process_stealth(CreatureEntity &creature, MONSTER_IDX m_idx)
{

    auto ninja_data = CreatureClass(creature).get_specific_data<ninja_data_type>();
    if (!ninja_data || !ninja_data->s_stealth) {
        return true;
    }

    const auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    int tmp = creature.level * 6 + (creature.get_skill_stealth() + 10) * 4;
    if (creature.monlite) {
        tmp /= 3;
    }

    if (has_aggravate(creature)) {
        tmp /= 2;
    }

    if (monrace.level > (creature.level * creature.level / 20 + 10)) {
        tmp /= 3;
    }

    return randint0(tmp) <= (monrace.level + 20);
}

/*!
 * @brief 死亡したモンスターが乗馬中のモンスターだった場合に落馬処理を行う
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param is_riding_mon 騎乗中であればTRUE
 */
void decide_drop_from_monster(CreatureEntity &creature, MONSTER_IDX m_idx, bool is_riding_mon)
{

    const auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    if (!is_riding_mon || monrace.misc_flags.has(MonsterMiscType::RIDING)) {
        return;
    }

    if (process_fall_off_horse(creature, 0, true)) {
#ifdef JP
        msg_print("地面に落とされた。");
#else
        const auto m_name = monster_desc(creature, creature.get_floor()->get_monster(creature.riding), 0);
        msg_format("You have fallen from %s.", m_name.data());
#endif
    }
}

/*!
 * @brief 召喚の親元が消滅した時、子供も消滅させる
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @return 召喚モンスターが消滅したらTRUE
 */
bool vanish_summoned_children(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m)
{

    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();

    if (!monster.has_parent()) {
        return false;
    }

    // parent_m_idxが自分自身を指している場合は召喚主は消滅している
    if (monster.get_parent_m_idx() != m_idx && floor.get_monster(monster.get_parent_m_idx()).is_valid()) {
        return false;
    }

    const auto m_name = monster_desc(creature, monster, 0);
    if (monster.is_quylthlug_born()) {
        if (see_m) {
            msg_format(_("%sは崩壊して朽ち果てた。", "%s^ crumbles into dust."), m_name.data());
        }
    } else if (monrace.misc_flags.has(MonsterMiscType::BREAK_DOWN)) {
        if (see_m) {
            msg_format(_("%sは主を失い消え去った。", "%s^ disappears without a master."), m_name.data());
        }
    } else if (monster.is_pet()) {
        if (see_m) {
            msg_format(_("%sは消え去った。", "%s^ disappears."), m_name.data());
        }
    } else {
        return false;
    }

    if (record_named_pet && monster.is_named_pet()) {
        const auto pet_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_LOSE_PARENT, pet_name);
    }

    delete_monster_idx(creature, m_idx);
    return true;
}

/*!
 * @brief 寝ているモンスターの起床を判定する
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @return 寝たままならFALSE、起きているor起きたらTRUE
 * @note 馬鹿馬鹿独自仕様あり
 */
bool awake_monster(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    auto &monrace = monster.get_monrace();
    if (!monster.is_asleep()) {
        return true;
    }

    bool awaken = false;
    awaken |= has_aggravate(creature);
    awaken |= has_aggravate_nasty(creature) && monrace.kind_flags.has(MonsterKindType::NASTY);

    if (!awaken) {
        return false;
    }

    (void)set_monster_csleep(*creature.get_floor(), m_idx, 0);
    if (monster.is_visible_on_map()) {
        const auto m_name = monster_desc(creature, monster, 0);
        msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
    }

    if (is_original_ap_and_seen(creature, monster) && (monrace.r_wake < MAX_UCHAR)) {
        monrace.r_wake++;
    }

    return true;
}

/*!
 * @brief モンスターの怒り状態を判定する (怒っていたら敵に回す)
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 */
void process_angar(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m)
{

    auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    auto gets_angry = monster.is_friendly() && has_aggravate(creature);
    const auto should_aggravate = monster.is_pet();
    auto has_hostile = monrace.kind_flags.has(MonsterKindType::UNIQUE) || (monrace.population_flags.has(MonsterPopulationType::NAZGUL));
    has_hostile &= monster_has_hostile_to_player(creature, 10, -10, monrace);
    const auto has_resist_all = monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL);
    if (should_aggravate && (has_hostile || has_resist_all)) {
        gets_angry = true;
    }

    if (AngbandSystem::get_instance().is_phase_out() || !gets_angry) {
        return;
    }

    const auto m_name = monster_desc(creature, monster, monster.is_pet() ? MD_ASSUME_VISIBLE : 0);

    /* When riding a hostile alignment pet */
    if (monster.is_riding()) {
        if (abs(creature.alignment / 10) < randint0(creature.get_skill_exp(PlayerSkillKindType::RIDING))) {
            return;
        }

        msg_format(_("%s^が突然暴れだした！", "%s^ suddenly begins unruly!"), m_name.data());
        if (!process_fall_off_horse(creature, 1, true)) {
            return;
        }

        msg_format(_("あなたは振り落とされた。", "You have fallen."));
    }

    if (monster.is_pet() || see_m) {
        msg_format(_("%s^は突然敵にまわった！", "%s^ suddenly becomes hostile!"), m_name.data());
    }

    monster.set_hostile();
}

/*!
 * @brief 手榴弾の爆発処理
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @return 爆死したらTRUE
 */
bool explode_grenade(CreatureEntity &creature, MONSTER_IDX m_idx)
{

    const auto &monster = creature.get_floor()->get_monster(m_idx);
    if (monster.r_idx != MonraceId::GRENADE) {
        return false;
    }

    bool fear, dead;
    mon_take_hit_mon(creature, m_idx, 1, &dead, &fear, _("は爆発して粉々になった。", " explodes into tiny shreds."), m_idx);
    return dead;
}

/*!
 * @brief モンスター依存の特別な行動を取らせる
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 */
void process_special(CreatureEntity &creature, MONSTER_IDX m_idx)
{

    const auto &monster = creature.get_floor()->get_monster(m_idx);
    auto &monrace = monster.get_monrace();
    auto can_do_special = monrace.ability_flags.has(MonsterAbilityType::SPECIAL);
    can_do_special &= monster.r_idx == MonraceId::OHMU;
    can_do_special &= !creature.get_floor()->inside_arena;
    can_do_special &= !AngbandSystem::get_instance().is_phase_out();
    can_do_special &= monrace.freq_spell != 0;
    can_do_special &= randint1(100) <= monrace.freq_spell;

    // 違法改造モンスターの自滅処理
    if (monster.is_illegal_modified() && one_in_(50)) {
        const auto m_name = monster_desc(creature, monster, 0);
        const auto pos = monster.get_position();
        msg_format(_("%sが突然機能停止し、爆発した！", "%s suddenly malfunctions and explodes!"), m_name.data());

        // 破片のボール攻撃
        project(creature, m_idx, 2, pos.y, pos.x, monrace.level + randint1(monrace.level), AttributeType::SHARDS,
            PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);

        // モンスターを削除
        delete_monster_idx(creature, m_idx);
        return;
    }

    if (!can_do_special) {
        return;
    }

    int count = 0;
    DEPTH rlev = ((monrace.level >= 1) ? monrace.level : 1);
    BIT_FLAGS p_mode = monster.is_pet() ? PM_FORCE_PET : PM_NONE;

    for (int k = 0; k < A_MAX; k++) {
        if (auto summoned_m_idx = summon_specific(creature, monster.y, monster.x, rlev, SUMMON_MOLD, (PM_ALLOW_GROUP | p_mode), m_idx)) {
            if (creature.get_floor()->get_monster(*summoned_m_idx).is_visible_on_map()) {
                count++;
            }
        }
    }

    if (count && is_original_ap_and_seen(creature, monster)) {
        monrace.r_ability_flags.set(MonsterAbilityType::SPECIAL);
    }
}

/*!
 * @brief モンスターを分裂させるかどうかを決定する (分裂もさせる)
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param oy 分裂元モンスターのY座標
 * @param ox 分裂元モンスターのX座標
 * @return 実際に分裂したらTRUEを返す
 */
bool decide_monster_multiplication(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION oy, POSITION ox)
{

    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    auto &monrace = monster.get_monrace();
    if (monrace.misc_flags.has_not(MonsterMiscType::MULTIPLY) || (floor.num_repro >= MAX_REPRODUCTION)) {
        return false;
    }

    auto k = 0;
    for (auto y = oy - 1; y <= oy + 1; y++) {
        for (auto x = ox - 1; x <= ox + 1; x++) {
            const Pos2D pos(y, x);
            if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                continue;
            }

            if (floor.get_grid(pos).has_monster()) {
                k++;
            }
        }
    }

    if (SpellHex(creature).check_hex_barrier(m_idx, HEX_ANTI_MULTI)) {
        k = 8;
    }

    constexpr auto chance_reproduction = 8;
    if ((k < 4) && (!k || !randint0(k * chance_reproduction))) {
        if (auto multiplied_m_idx = multiply_monster(creature, m_idx, monrace.idx, false, (monster.is_pet() ? PM_FORCE_PET : 0))) {
            if (creature.get_floor()->get_monster(*multiplied_m_idx).is_visible_on_map() && is_original_ap_and_seen(creature, monster)) {
                monrace.r_misc_flags.set(MonsterMiscType::MULTIPLY);
            }
            if (floor.get_monster(*multiplied_m_idx).is_visible_on_map() && is_original_ap_and_seen(creature, monster)) {
                monrace.r_misc_flags.set(MonsterMiscType::MULTIPLY);
            }
            return true;
        }
    }
    return false;
}

/*!
 * @brief モンスターのアイテム自然生成処理
 */
void process_monster_spawn_item(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    CreatureEntity *m_ptr = &creature.get_floor()->get_monster(m_idx);
    MonraceDefinition &monrace = MonraceList::get_instance().get_monrace(m_ptr->r_idx);
    for (const auto &spawn_info : monrace.spawn_items) {
        auto num = std::get<0>(spawn_info);
        auto deno = std::get<1>(spawn_info);
        auto kind = std::get<2>(spawn_info);
        if (randint1(deno) <= num) {
            ItemEntity item;
            item.generate(kind);
            item.number = 1;
            (void)drop_near(creature, item, m_ptr->get_position());
        }
    }
}

/*!
 * @brief モンスターの残気自然生成処理
 * @note 馬鹿馬鹿の固有実装
 */
void process_monster_spawn_zanki(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    CreatureEntity *m_ptr = &creature.get_floor()->get_monster(m_idx);
    MonraceDefinition *r_ptr = &MonraceList::get_instance().get_monrace(m_ptr->r_idx);
    if (r_ptr->level < 30 || !r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->r_misc_flags.has(MonsterMiscType::EMPTY_MIND)) {
        return;
    }

    // 猿空間フラグ持ちは530/10000、通常は53/10000の確率
    const auto threshold = r_ptr->kind_flags.has(MonsterKindType::MONKEY_SPACE) ? 530 : 53;
    if (randint1(10000) > threshold) {
        return;
    }
    ItemEntity item;
    item.generate(684);
    item.number = 1;
    item.pval = enum2i(m_ptr->ap_r_idx);
    (void)drop_near(creature, item, m_ptr->get_position());
}

/*!
 * @brief モンスターによる地形変化処理
 * @note 馬鹿馬鹿の固有実装
 */
void process_monster_change_feat(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    auto *m_ptr = &creature.get_floor()->get_monster(m_idx);
    auto *r_ptr = &MonraceList::get_instance().get_monrace(m_ptr->r_idx);
    for (const auto &spawn_info : r_ptr->change_feats) {
        auto num = std::get<0>(spawn_info);
        auto deno = std::get<1>(spawn_info);
        auto feat = std::get<2>(spawn_info);
        if (randint1(deno) <= num && feat) {
            set_terrain_id_to_grid(creature, m_ptr->get_position(), feat);
        }
    }
}

/*!
 * @brief モンスターの落とし子生成処理
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param oy 分裂元モンスターのY座標
 * @param ox 分裂元モンスターのX座標
 * @return 実際に分裂したらTRUEを返す
 */
bool process_monster_spawn_monster(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION oy, POSITION ox)
{

    CreatureEntity *m_ptr = &creature.get_floor()->get_monster(m_idx);
    MonraceDefinition *r_ptr = &MonraceList::get_instance().get_monrace(m_ptr->r_idx);
    if ((r_ptr->spawn_monsters.size() == 0) || (creature.get_floor()->num_repro >= MAX_REPRODUCTION)) {
        return false;
    }

    int k = 0;

    if (SpellHex(creature).check_hex_barrier(m_idx, HEX_ANTI_MULTI)) {
        return false;
    }

    for (const auto &spawn_info : r_ptr->spawn_monsters) {

        for (POSITION y = oy - 1; y <= oy + 1; y++) {
            for (POSITION x = ox - 1; x <= ox + 1; x++) {
                if (!creature.get_floor()->contains(Pos2D(y, x), FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                    continue;
                }

                if (creature.get_floor()->grid_array[y][x].m_idx) {
                    k++;
                }
            }
        }

        if (k >= 8) {
            return false;
        }

        auto num = std::get<0>(spawn_info);
        auto deno = std::get<1>(spawn_info);
        auto idx = std::get<2>(spawn_info);
        if (randint1(deno) <= num) {
            if (multiply_monster(creature, m_idx, idx, false, (m_ptr->is_pet() ? PM_FORCE_PET : 0))) {
                auto &monster = creature.get_floor()->get_monster(m_idx);
                if (monster.is_visible_on_map() && is_original_ap_and_seen(creature, *m_ptr)) {
                    r_ptr->misc_flags.set(MonsterMiscType::MULTIPLY);
                }
                return true;
            }
        }
    }

    return false;
}

/*!
 * @brief モンスターに魔法を試行させる
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return 魔法を唱えられなければ強制的にFALSE、その後モンスターが実際に魔法を唱えればTRUE
 */
bool cast_spell(CreatureEntity &creature, MONSTER_IDX m_idx, bool aware)
{

    const auto &floor = *creature.get_floor();
    const auto &monster_from = floor.get_monster(m_idx);
    const auto &monrace = monster_from.get_monrace();

    // 狂乱状態のモンスターは魔法を使わない
    if (monster_from.is_frenzied()) {
        return false;
    }

    if ((monrace.freq_spell == 0) || (randint1(100) > monrace.freq_spell)) {
        return false;
    }

    auto counter_attack = false;
    if (monster_from.get_target_position().y) {
        const auto pos_to = monster_from.get_target_position();
        const auto t_m_idx = floor.get_grid(pos_to).m_idx;
        const auto &monster_to = floor.get_monster(t_m_idx);
        const auto pos_from = monster_from.get_position();
        const auto is_projectable = projectable(floor, pos_from, pos_to);
        if (t_m_idx && monster_from.is_hostile_to_melee(monster_to) && is_projectable) {
            counter_attack = true;
        }
    }

    if (counter_attack) {
        if (monst_spell_monst(creature, m_idx) || (aware && make_attack_spell(creature, m_idx))) {
            return true;
        }
    } else {
        if ((aware && make_attack_spell(creature, m_idx)) || monst_spell_monst(creature, m_idx)) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief モンスターの恐怖状態を処理する
 * @param creature クリーチャーへの参照
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return モンスターが戦いを決意したらTRUE
 */
bool process_monster_fear(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx)
{
    const auto &baseitems = BaseitemList::get_instance();
    auto *m_ptr = &creature.get_floor()->get_monster(m_idx);
    const auto m_name = monster_desc(creature, *m_ptr, 0);
    const auto &monrace = m_ptr->get_monrace();

    if (monrace.resistance_flags.has_not(MonsterResistanceType::NO_DEFECATE) && !m_ptr->is_defecated() && m_ptr->is_fearful() && one_in_(20)) {
        msg_format(_("%s^は恐怖のあまり脱糞した！", "%s^ was defecated because of fear!"), m_name.data());
        ItemEntity item;
        item.generate(baseitems.lookup_baseitem_id({ ItemKindType::JUNK, SV_JUNK_FECES }));
        (void)drop_near(creature, item, m_ptr->get_position());
        m_ptr->set_constant_flag(MonsterConstantFlagType::DEFECATED);
    }

    if (monrace.resistance_flags.has_not(MonsterResistanceType::NO_VOMIT) && !m_ptr->is_vomited() && m_ptr->is_fearful() && one_in_(20)) {
        msg_format(_("%s^は恐怖のあまり嘔吐した！", "%s^ vomited in fear!"), m_name.data());
        ItemEntity item;
        item.generate(baseitems.lookup_baseitem_id({ ItemKindType::JUNK, SV_JUNK_VOMITTING }));
        (void)drop_near(creature, item, m_ptr->get_position());
        m_ptr->set_constant_flag(MonsterConstantFlagType::VOMITED);
    }

    const auto &monster = creature.get_floor()->get_monster(m_idx);
    bool is_battle_determined = !turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && monster.is_fearful() && turn_flags_ptr->aware;
    if (!is_battle_determined) {
        return false;
    }

    (void)set_monster_monfear(*creature.get_floor(), m_idx, 0);
    if (!turn_flags_ptr->see_m) {
        return true;
    }

    msg_format(_("%s^は戦いを決意した！", "%s^ turns to fight!"), m_name.data());
    return true;
}

/*!
 * @brief 全モンスターのターン管理メインルーチン /
 * Process all the "live" monsters, once per game turn.
 * @details
 * During each game current game turn, we scan through the list of all the "live" monsters,\n
 * (backwards, so we can excise any "freshly dead" monsters), energizing each\n
 * monster, and allowing fully energized monsters to move, attack, pass, etc.\n
 *\n
 * Note that monsters can never move in the monster array (except when the\n
 * "compact_monsters()" function is called by "dungeon()" or "save_player()").\n
 *\n
 * This function is responsible for at least half of the processor time\n
 * on a normal system with a "normal" amount of monsters and a creature doing\n
 * normal things.\n
 *\n
 * When the creature is resting, virtually 90% of the processor time is spent\n
 * in this function, and its children, "process_monster()" and "make_move()".\n
 *\n
 * Most of the rest of the time is spent in "update_view()" and "lite_spot()",\n
 * especially when the creature is running.\n
 *\n
 * Note the special "PRESENT_AT_TURN_START" flag.  Monsters without that flag\n
 * are "fresh" and are still being "born".  A monster is "fresh" only\n
 * during the game turn in which it is created, and we use the "hack_m_idx" to\n
 * determine if the monster is yet to be processed during the game turn.\n
 *\n
 * Note the special "PREVENT_MAGIC" flag, which allows the creature to get one\n
 * move before any "nasty" monsters get to use their spell attacks.\n
 *\n
 * Note that when the "knowledge" about the currently tracked monster\n
 * changes (flags, attacks, spells), we induce a redraw of the monster\n
 * recall window.\n
 */
void process_monsters(CreatureEntity &creature)
{
    const auto &tracker = LoreTracker::get_instance();
    const auto old_monrace_id = tracker.get_trackee();
    OldRaceFlags flags(old_monrace_id);
    creature.get_floor()->monster_noise = false;
    sweep_monster_process(creature);
    if (!tracker.is_tracking() || !tracker.is_tracking(old_monrace_id)) {
        return;
    }

    flags.update_lore_window_flag(tracker.get_tracking_monrace());
}

/*!
 * @brief フロア内のモンスターについてターン終了時の処理を繰り返す
 * @param creature クリーチャーへの参照
 */
void sweep_monster_process(CreatureEntity &creature)
{

    auto &floor = *creature.get_floor();

    // 処理中の召喚などで生成されたモンスターが即座に行動しないようにするため、
    // 先に現在存在するモンスターをリストアップしておく
    std::vector<MONSTER_IDX> valid_m_idx_list;
    for (MONSTER_IDX m_idx = floor.m_max - 1; m_idx >= 1; m_idx--) {
        if (floor.get_monster(m_idx).is_valid()) {
            valid_m_idx_list.push_back(m_idx);
        }
    }

    for (const auto m_idx : valid_m_idx_list) {
        auto &monster = floor.get_monster(m_idx);

        if (creature.leaving) {
            return;
        }

        if (!monster.is_valid() || AngbandWorld::get_instance().is_wild_mode()) {
            continue;
        }

        if ((Grid::calc_distance(creature.get_position(), monster.get_position()) >= MAX_MONSTER_SENSING) || !decide_process_continue(creature, monster)) {
            continue;
        }

        byte speed = monster.is_riding() ? creature.get_speed() : monster.get_temporary_speed();
        monster.energy_need -= speed_to_energy(speed);
        if (monster.energy_need > 0) {
            continue;
        }

        monster.energy_need += ENERGY_NEED();
        auto m_name = monster_desc(creature, monster, 0);

        if (monster.get_death_count() > 0) {
            if (monster.decrement_death_count() == 0) {
                bool fear;
                monster.max_maxhp = monster.maxhp = monster.hp = -1;
                MonsterDamageProcessor mdp(creature, m_idx, 0, &fear, AttributeType::ATTACK);
                mdp.mon_take_hit(_("は爆発した。", " explodes."));
            }
        }

        auto g_ptr = &creature.get_floor()->grid_array[monster.y][monster.x];
        auto &f_ptr = TerrainList::get_instance().get_terrain(g_ptr->feat);
        if (f_ptr.flags.has(TerrainCharacteristics::TENTACLE)) {
            int pow = 30;
            const auto &r_ptr = monster.get_monrace();
            if (r_ptr.kind_flags.has(MonsterKindType::HENTAI)) {
                pow += 50;
            }
            if (r_ptr.feature_flags.has(MonsterFeatureType::CAN_FLY)) {
                pow /= 2;
            }
            if (r_ptr.behavior_flags.has(MonsterBehaviorType::STUPID)) {
                pow *= 3;
            }
            if (r_ptr.level < randint0(pow)) {
                if (monster.is_visible_on_map()) {
                    msg_format(_("%sは%sに絡めとられて動けなかった！", "%s wes entangled in the %s and could not move!"), m_name.data(), f_ptr.name.data());
                }
                if (r_ptr.kind_flags.has(MonsterKindType::HENTAI)) {
                    switch (randint1(3)) {
                    case 1:
                        msg_format(_("%s「んほぉ！」", "%s 'Nnhor!'"), m_name.data());
                        (void)set_monster_stunned(*creature.get_floor(), 0, monster.get_remaining_stun() + 10 + randint0(creature.level) / 5);
                        break;
                    case 2:
                        msg_format(_("%s「アへぇ！」", "%s 'Aherr!'"), m_name.data());
                        (void)set_monster_slow(*creature.get_floor(), 0, monster.get_remaining_deceleration() + 10 + randint0(creature.level) / 5);
                        break;
                    case 3: {
                        bool fear = false;
                        msg_format(_("%s「イグゥ！」", "%s 'Igur!'"), m_name.data());
                        MonsterDamageProcessor mdp(creature, m_idx, Dice::roll(1, 4), &fear, AttributeType::ATTACK);

                        if (fear) {
                            msg_format(_("%s「イッジャイましゅうう！」", "%s'I’m commingrrr!'"), m_name.data());
                        }
                        mdp.mon_take_hit(_("は絶頂であの世に逝った。", " was passed to the other side with intense acme."));
                        break;
                    }
                    }
                }
                return;
            }
        }

        process_monster(creature, m_idx);
        monster.reset_target();
        if (creature.no_flowed && one_in_(3)) {
            monster.set_constant_flag(MonsterConstantFlagType::NOFLOW);
        }

        if (!creature.playing || creature.is_dead() || creature.leaving) {
            return;
        }
    }
}

/*!
 * @brief 後続のモンスター処理が必要かどうか判定する (要調査)
 * @param creature クリーチャーへの参照
 * @param m_ptr モンスターへの参照ポインタ
 * @return 後続処理が必要ならTRUE
 */
bool decide_process_continue(CreatureEntity &creature, CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();

    if (!creature.no_flowed) {
        monster.reset_constant_flag(MonsterConstantFlagType::NOFLOW);
    }

    const auto cdis = Grid::calc_distance(creature.get_position(), monster.get_position());
    if (cdis <= (monster.is_pet() ? (monrace.aaf > MAX_PLAYER_SIGHT ? MAX_PLAYER_SIGHT : monrace.aaf) : monrace.aaf)) {
        return true;
    }

    auto should_continue = (cdis <= MAX_PLAYER_SIGHT) || AngbandSystem::get_instance().is_phase_out();
    should_continue &= creature.get_floor()->has_los_at({ monster.y, monster.x }) || has_aggravate(creature);
    if (should_continue) {
        return true;
    }

    if (monster.get_target_position().y) {
        return true;
    }

    return false;
}

bool process_stalking(CreatureEntity &creature, MONSTER_IDX m_idx)
{

    auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();

    // モンスターが背後に忍び寄るフラグを持っていないなら何もしない
    if (monrace.misc_flags.has_not(MonsterMiscType::STALKER)) {
        return false;
    }

    // モンスターからプレイヤーの距離が空いているなら何もしない
    if (Grid::calc_distance(creature.get_position(), monster.get_position()) > STALKER_DISTANCE_THRESHOLD) {
        return false;
    }

    // モンスターが恐怖しているならば寄ってこない
    if (monster.is_fearful()) {
        return false;
    }

    // 確率で寄ってくる
    if (!one_in_(STALKER_CHANCE_DENOMINATOR)) {
        return false;
    }

    const auto m_name = monster_name(creature, m_idx);

    // 呪術魔法によりテレポートが阻害されているならば寄ってこない
    if (SpellHex(creature).check_hex_barrier(m_idx, HEX_ANTI_TELE)) {
        if (see_monster(creature, m_idx)) {
            msg_format(_("魔法のバリアが%s^のテレポートを邪魔した。", "Magic barrier obstructs teleporting of %s^."), m_name.data());
        }
        return false;
    }

    teleport_monster_to(creature, m_idx, creature.y, creature.x, 100, TELEPORT_SPONTANEOUS);

    disturb(creature, true, true);

    if (see_monster(creature, m_idx)) {
        const auto message_stalker = monrace.get_message(m_name, MonsterMessageType::MESSAGE_STALKER);
        if (message_stalker) {
            msg_print(*message_stalker);
        }
    }

    return true;
}
