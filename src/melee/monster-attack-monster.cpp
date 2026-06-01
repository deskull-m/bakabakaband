/*!
 * @brief モンスター同士が乱闘する処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "melee/monster-attack-monster.h"
#include "combat/attack-accuracy.h"
#include "combat/combat-options-type.h"
#include "combat/hallucination-attacks-table.h"
#include "combat/slaying.h"
#include "core/disturbance.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "melee/melee-switcher.h"
#include "melee/melee-util.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/health-bar-tracker.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

static void heal_monster_by_melee(mam_type *mam_ptr)
{
    if (!mam_ptr->t_ptr->has_living_flag() || (mam_ptr->damage <= 2)) {
        return;
    }

    bool did_heal = mam_ptr->m_ptr->hp < mam_ptr->m_ptr->maxhp;
    mam_ptr->m_ptr->hp += Dice::roll(4, mam_ptr->damage / 6);
    if (mam_ptr->m_ptr->hp > mam_ptr->m_ptr->maxhp) {
        mam_ptr->m_ptr->hp = mam_ptr->m_ptr->maxhp;
    }

    HealthBarTracker::get_instance().set_flag_if_tracking(mam_ptr->m_idx);
    if (mam_ptr->m_ptr->is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
    }

    if (mam_ptr->see_m && did_heal) {
        msg_format(_("%sは体力を回復したようだ。", "%s^ appears healthier."), mam_ptr->m_name);
    }
}

static void process_blow_effect(CreatureEntity &creature, mam_type *mam_ptr)
{
    const auto &monrace = mam_ptr->m_ptr->get_monrace();
    switch (mam_ptr->attribute) {
    case BlowEffectType::FEAR:
        project(creature, mam_ptr->m_idx, 0, mam_ptr->t_ptr->y, mam_ptr->t_ptr->x, mam_ptr->damage,
            AttributeType::TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
        break;
    case BlowEffectType::SLEEP:
        project(creature, mam_ptr->m_idx, 0, mam_ptr->t_ptr->y, mam_ptr->t_ptr->x, monrace.level,
            AttributeType::OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
        break;
    case BlowEffectType::HEAL:
        heal_monster_by_melee(mam_ptr);
        break;
    default:
        break;
    }
}

static void aura_fire_by_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    auto &monrace = mam_ptr->m_ptr->get_monrace();
    auto &monrace_target = mam_ptr->t_ptr->get_monrace();
    if (monrace_target.aura_flags.has_not(MonsterAuraType::FIRE) || !mam_ptr->m_ptr->is_valid()) {
        return;
    }

    if (monrace.resistance_flags.has_any_of(RFR_EFF_IM_FIRE_MASK) && is_original_ap_and_seen(creature, *mam_ptr->m_ptr)) {
        monrace.r_resistance_flags.set(monrace.resistance_flags & RFR_EFF_IM_FIRE_MASK);
        return;
    }

    if (mam_ptr->see_either) {
        msg_format(_("%s^は突然熱くなった！", "%s^ is suddenly very hot!"), mam_ptr->m_name);
    }

    if (mam_ptr->m_ptr->is_visible_on_map() && is_original_ap_and_seen(creature, *mam_ptr->t_ptr)) {
        monrace_target.aura_flags.set(MonsterAuraType::FIRE);
    }

    const auto dam = Dice::roll(1 + ((monrace_target.level) / 26), 1 + ((monrace_target.level) / 17));
    constexpr auto flags = PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED;
    project(creature, mam_ptr->t_idx, 0, mam_ptr->m_ptr->y, mam_ptr->m_ptr->x, dam, AttributeType::FIRE, flags);
}

static void aura_cold_by_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    const auto &monster = *mam_ptr->m_ptr;
    auto &monrace = monster.get_monrace();
    auto &monrace_target = mam_ptr->t_ptr->get_monrace();
    if (monrace_target.aura_flags.has_not(MonsterAuraType::COLD) || !monster.is_valid()) {
        return;
    }

    if (monrace.resistance_flags.has_any_of(RFR_EFF_IM_COLD_MASK) && is_original_ap_and_seen(creature, monster)) {
        monrace.r_resistance_flags.set(monrace.resistance_flags & RFR_EFF_IM_COLD_MASK);
        return;
    }

    if (mam_ptr->see_either) {
        msg_format(_("%s^は突然寒くなった！", "%s^ is suddenly very cold!"), mam_ptr->m_name);
    }

    if (monster.is_visible_on_map() && is_original_ap_and_seen(creature, *mam_ptr->t_ptr)) {
        monrace_target.aura_flags.set(MonsterAuraType::COLD);
    }

    const auto dam = Dice::roll(1 + ((monrace_target.level) / 26), 1 + ((monrace_target.level) / 17));
    constexpr auto flags = PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED;
    project(creature, mam_ptr->t_idx, 0, monster.y, monster.x, dam, AttributeType::COLD, flags);
}

static void aura_elec_by_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    const auto &monster = *mam_ptr->m_ptr;
    auto &monrace = monster.get_monrace();
    auto &monrace_target = mam_ptr->t_ptr->get_monrace();
    if (monrace_target.aura_flags.has_not(MonsterAuraType::ELEC) || !monster.is_valid()) {
        return;
    }

    if (monrace.resistance_flags.has_any_of(RFR_EFF_IM_ELEC_MASK) && is_original_ap_and_seen(creature, monster)) {
        monrace.r_resistance_flags.set(monrace.resistance_flags & RFR_EFF_IM_ELEC_MASK);
        return;
    }

    if (mam_ptr->see_either) {
        msg_format(_("%s^は電撃を食らった！", "%s^ gets zapped!"), mam_ptr->m_name);
    }

    if (monster.is_visible_on_map() && is_original_ap_and_seen(creature, *mam_ptr->t_ptr)) {
        monrace_target.aura_flags.set(MonsterAuraType::ELEC);
    }

    const auto dam = Dice::roll(1 + ((monrace_target.level) / 26), 1 + ((monrace_target.level) / 17));
    constexpr auto flags = PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED;
    project(creature, mam_ptr->t_idx, 0, monster.y, monster.x, dam, AttributeType::ELEC, flags);
}

static bool check_same_monster(CreatureEntity &creature, mam_type *mam_ptr)
{
    if (mam_ptr->m_idx == mam_ptr->t_idx) {
        return false;
    }

    const auto &monrace = mam_ptr->m_ptr->get_monrace();
    if (monrace.behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (creature.get_floor()->get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        return false;
    }

    return true;
}

static void redraw_health_bar(mam_type *mam_ptr)
{
    if (!mam_ptr->t_ptr->is_visible_on_map()) {
        return;
    }

    HealthBarTracker::get_instance().set_flag_if_tracking(mam_ptr->t_idx);
    if (mam_ptr->t_ptr->is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
    }
}

static void describe_silly_melee(mam_type *mam_ptr)
{
    if ((mam_ptr->act == nullptr) || !mam_ptr->see_either) {
        return;
    }

#ifdef JP
    if (mam_ptr->do_silly_attack) {
        mam_ptr->act = rand_choice(silly_attacks2);
    }

    const auto temp = format(mam_ptr->act, mam_ptr->t_name);
    msg_format("%s^は%s", mam_ptr->m_name, temp.data());
#else
    std::string temp;
    if (mam_ptr->do_silly_attack) {
        mam_ptr->act = rand_choice(silly_attacks);
        temp = format("%s %s.", mam_ptr->act, mam_ptr->t_name);
    } else {
        temp = format(mam_ptr->act, mam_ptr->t_name);
    }

    msg_format("%s^ %s", mam_ptr->m_name, temp.data());
#endif
}

/*!
 * @brief 武器を装備したモンスターの打撃に「〜で攻撃した」という追加メッセージを表示する
 * @details weapon_slot_for_blow が有効 (物理打撃かつ武器装備時) かつ視認可能な場合のみ表示する。
 */
static void describe_weapon_melee(mam_type *mam_ptr)
{
    if ((mam_ptr->weapon_slot_for_blow < 0) || !mam_ptr->see_either) {
        return;
    }

    const auto &weapon = *mam_ptr->m_ptr->inventory[mam_ptr->weapon_slot_for_blow];
    if (!weapon.is_valid()) {
        return;
    }

    const auto weapon_name = describe_flavor(*mam_ptr->m_ptr, weapon, OD_NAME_ONLY | OD_OMIT_PREFIX | OD_NO_PLURAL);
    msg_format(_("%s^は%sで%sを攻撃した。", "%s^ attacks %s with %s."),
#ifdef JP
        mam_ptr->m_name, weapon_name.data(), mam_ptr->t_name);
#else
        mam_ptr->m_name, mam_ptr->t_name, weapon_name.data());
#endif
}

static void process_monster_attack_effect(CreatureEntity &creature, mam_type *mam_ptr)
{
    if (mam_ptr->pt == AttributeType::NONE) {
        return;
    }

    if (!mam_ptr->explode) {
        project(creature, mam_ptr->m_idx, 0, mam_ptr->t_ptr->y, mam_ptr->t_ptr->x, mam_ptr->damage, mam_ptr->pt,
            PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
    }

    process_blow_effect(creature, mam_ptr);
    if (!mam_ptr->touched) {
        return;
    }

    aura_fire_by_melee(creature, mam_ptr);
    aura_cold_by_melee(creature, mam_ptr);
    aura_elec_by_melee(creature, mam_ptr);
}

static void process_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    const auto remaining_stun = mam_ptr->m_ptr->get_remaining_stun();
    if (mam_ptr->effect != RaceBlowEffectType::NONE && !check_hit_from_monster_to_monster(mam_ptr->power, mam_ptr->rlev, mam_ptr->ac, remaining_stun)) {
        describe_monster_missed_monster(*creature.get_floor(), mam_ptr);
        return;
    }

    (void)set_monster_csleep(*creature.get_floor(), mam_ptr->t_idx, 0);
    redraw_health_bar(mam_ptr);
    describe_melee_method(mam_ptr);
    describe_silly_melee(mam_ptr);
    describe_weapon_melee(mam_ptr);
    mam_ptr->obvious = true;
    mam_ptr->damage = mam_ptr->damage_dice.roll();

    // 武器を装備している場合、プレイヤーと共通の calc_attack_damage_with_slay() で
    // スレイ・ブランド効果を反映したダメージを加算する。
    if (!mam_ptr->explode && (mam_ptr->weapon_slot_for_blow >= 0)) {
        auto &weapon = *mam_ptr->m_ptr->inventory[mam_ptr->weapon_slot_for_blow];
        const auto base_dam = weapon.damage_dice.roll();
        mam_ptr->damage += calc_attack_damage_with_slay(*mam_ptr->m_ptr, &weapon, base_dam, *mam_ptr->t_ptr, HISSATSU_NONE, false) + weapon.to_d;
    }

    mam_ptr->attribute = BlowEffectType::NONE;
    mam_ptr->pt = AttributeType::MONSTER_MELEE;
    decide_monster_attack_effect(creature, mam_ptr);
    process_monster_attack_effect(creature, mam_ptr);
}

static void thief_runaway_by_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    if (creature.is_player()) {
        if (SpellHex(creature).check_hex_barrier(mam_ptr->m_idx, HEX_ANTI_TELE)) {
            if (mam_ptr->see_m) {
                msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
            } else if (mam_ptr->known) {
                creature.get_floor()->monster_noise = true;
            }
            return;
        }
    }
    if (mam_ptr->see_m) {
        msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
    } else if (mam_ptr->known) {
        creature.get_floor()->monster_noise = true;
    }

    teleport_away(creature, mam_ptr->m_idx, MAX_PLAYER_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
}

static void explode_monster_by_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    if (!mam_ptr->explode) {
        return;
    }

    if (!creature.is_player()) {
        return;
    }

    sound(SoundKind::EXPLODE);
    (void)set_monster_invulner(*creature.get_floor(), mam_ptr->m_idx, 0, false);
    mon_take_hit_mon(creature, mam_ptr->m_idx, mam_ptr->m_ptr->hp + 1, &mam_ptr->dead, &mam_ptr->fear,
        _("は爆発して粉々になった。", " explodes into tiny shreds."), mam_ptr->m_idx);
    mam_ptr->blinked = false;
}

/*!
 * @brief MonsterRaceDefinitionで定義した攻撃回数の分だけ、モンスターからモンスターへの直接攻撃処理を繰り返す
 * @param creature クリーチャーへの参照
 * @param mam_ptr モンスター乱闘構造体への参照ポインタ
 */
static void repeat_melee(CreatureEntity &creature, mam_type *mam_ptr)
{
    const auto &monster = *mam_ptr->m_ptr;
    auto &monrace = monster.get_monrace();
    const auto blow_count = static_cast<int>(monrace.blows.size());
    for (int ap_cnt = 0; ap_cnt < blow_count; ap_cnt++) {
        mam_ptr->effect = monrace.blows[ap_cnt].effect;
        mam_ptr->method = monrace.blows[ap_cnt].method;
        mam_ptr->damage_dice = monrace.blows[ap_cnt].damage_dice;

        // 物理打撃 (HIT/PUNCH/SLASH/STING) かつ武器装備時は当該打撃で使う武器スロットを決める。
        // 二刀流なら blow index で交互、片手のみならそちら、武器なしなら -1。
        // (monster-attack-player.cpp の処理と揃える)
        mam_ptr->weapon_slot_for_blow = -1;
        switch (mam_ptr->method) {
        case RaceBlowMethodType::HIT:
        case RaceBlowMethodType::PUNCH:
        case RaceBlowMethodType::SLASH:
        case RaceBlowMethodType::STING: {
            const bool main_valid = monster.inventory[INVEN_MAIN_HAND]->is_valid() && monster.inventory[INVEN_MAIN_HAND]->is_melee_weapon();
            const bool sub_valid = monster.inventory[INVEN_SUB_HAND]->is_valid() && monster.inventory[INVEN_SUB_HAND]->is_melee_weapon();
            if (main_valid && sub_valid) {
                mam_ptr->weapon_slot_for_blow = (ap_cnt % 2 == 0) ? INVEN_MAIN_HAND : INVEN_SUB_HAND;
            } else if (main_valid) {
                mam_ptr->weapon_slot_for_blow = INVEN_MAIN_HAND;
            } else if (sub_valid) {
                mam_ptr->weapon_slot_for_blow = INVEN_SUB_HAND;
            }
            break;
        }
        default:
            break;
        }

        if (!monster.is_valid()) {
            break;
        }

        const auto x_saver = mam_ptr->t_ptr->x != mam_ptr->x_saver;
        const auto y_saver = mam_ptr->t_ptr->y != mam_ptr->y_saver;
        if (x_saver || y_saver || mam_ptr->method == RaceBlowMethodType::NONE) {
            break;
        }

        mam_ptr->power = mbe_info[enum2i(mam_ptr->effect)].power;
        process_melee(creature, mam_ptr);
        if (!creature.is_player() || mam_ptr->do_silly_attack) {
            continue;
        }
        if (!is_original_ap_and_seen(creature, *mam_ptr->m_ptr)) {
            continue;
        }

        if (ap_cnt >= static_cast<int>(monrace.r_blows.size())) {
            monrace.r_blows.resize(ap_cnt + 1, 0);
        }

        if (!mam_ptr->obvious && !mam_ptr->damage && (monrace.r_blows[ap_cnt] <= 10)) {
            continue;
        }

        if (monrace.r_blows[ap_cnt] < MAX_UCHAR) {
            monrace.r_blows[ap_cnt]++;
        }
    }
}

/*!
 * @brief モンスターから敵モンスターへの打撃攻撃処理
 * @param m_idx 攻撃側モンスターの参照ID
 * @param t_idx 目標側モンスターの参照ID
 * @return 実際に打撃処理が行われた場合TRUEを返す
 */
bool monst_attack_monst(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    mam_type tmp_mam;
    mam_type *mam_ptr = initialize_mam_type(creature, &tmp_mam, m_idx, t_idx);

    if (!check_same_monster(creature, mam_ptr)) {
        return false;
    }

    angband_strcpy(mam_ptr->m_name, monster_desc(creature, *mam_ptr->m_ptr, 0), sizeof(mam_ptr->m_name));
    angband_strcpy(mam_ptr->t_name, monster_desc(creature, *mam_ptr->t_ptr, 0), sizeof(mam_ptr->t_name));
    if (!mam_ptr->see_either && mam_ptr->known) {
        creature.get_floor()->monster_noise = true;
    }

    // disturb() はプレイヤー以外で no-op のため、is_player() ガードは不要
    if (mam_ptr->m_ptr->is_riding()) {
        disturb(creature, true, true);
    }

    repeat_melee(creature, mam_ptr);
    explode_monster_by_melee(creature, mam_ptr);
    if (!mam_ptr->blinked || !mam_ptr->m_ptr->is_valid()) {
        return true;
    }

    thief_runaway_by_melee(creature, mam_ptr);
    return true;
}
