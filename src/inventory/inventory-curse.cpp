#include "inventory/inventory-curse.h"
#include "artifact/fixed-art-types.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-mode-changer.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/write-diary.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "perception/object-perception.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <string>
#include <tl/optional.hpp>

// clang-format off
namespace {
const EnumClassFlagGroup<CurseTraitType> TRC_P_FLAG_MASK({
    CurseTraitType::TY_CURSE,
    CurseTraitType::DRAIN_EXP,
    CurseTraitType::ADD_L_CURSE,
    CurseTraitType::ADD_H_CURSE,
    CurseTraitType::CALL_ANIMAL,
    CurseTraitType::CALL_DEMON,
    CurseTraitType::CALL_DRAGON,
    CurseTraitType::COWARDICE,
    CurseTraitType::TELEPORT,
    CurseTraitType::DRAIN_HP,
    CurseTraitType::DRAIN_MANA,
    CurseTraitType::CALL_UNDEAD,
    CurseTraitType::BERS_RAGE,
    CurseTraitType::PERSISTENT_CURSE });
const EnumClassFlagGroup<CurseSpecialTraitType> TRCS_P_FLAG_MASK({
    CurseSpecialTraitType::TELEPORT_SELF,
    CurseSpecialTraitType::CHAINSWORD });
}
// clang-format on

static bool is_specific_curse(CurseTraitType flag)
{
    switch (flag) {
    case CurseTraitType::ADD_L_CURSE:
    case CurseTraitType::ADD_H_CURSE:
    case CurseTraitType::DRAIN_HP:
    case CurseTraitType::DRAIN_MANA:
    case CurseTraitType::CALL_ANIMAL:
    case CurseTraitType::CALL_DEMON:
    case CurseTraitType::CALL_DRAGON:
    case CurseTraitType::CALL_UNDEAD:
    case CurseTraitType::COWARDICE:
    case CurseTraitType::LOW_MELEE:
    case CurseTraitType::LOW_AC:
    case CurseTraitType::HARD_SPELL:
    case CurseTraitType::FAST_DIGEST:
    case CurseTraitType::SLOW_REGEN:
    case CurseTraitType::BERS_RAGE:
    case CurseTraitType::PERSISTENT_CURSE:
        return true;
    default:
        return false;
    }
}

static void choise_cursed_item(CurseTraitType flag, ItemEntity *o_ptr, int *choices, int *number, int item_num)
{
    if (!is_specific_curse(flag)) {
        return;
    }

    tr_type cf = TR_STR;
    const auto flags = o_ptr->get_flags();
    switch (flag) {
    case CurseTraitType::ADD_L_CURSE:
        cf = TR_ADD_L_CURSE;
        break;
    case CurseTraitType::ADD_H_CURSE:
        cf = TR_ADD_H_CURSE;
        break;
    case CurseTraitType::DRAIN_HP:
        cf = TR_DRAIN_HP;
        break;
    case CurseTraitType::DRAIN_MANA:
        cf = TR_DRAIN_MANA;
        break;
    case CurseTraitType::CALL_ANIMAL:
        cf = TR_CALL_ANIMAL;
        break;
    case CurseTraitType::CALL_DEMON:
        cf = TR_CALL_DEMON;
        break;
    case CurseTraitType::CALL_DRAGON:
        cf = TR_CALL_DRAGON;
        break;
    case CurseTraitType::CALL_UNDEAD:
        cf = TR_CALL_UNDEAD;
        break;
    case CurseTraitType::COWARDICE:
        cf = TR_COWARDICE;
        break;
    case CurseTraitType::LOW_MELEE:
        cf = TR_LOW_MELEE;
        break;
    case CurseTraitType::LOW_AC:
        cf = TR_LOW_AC;
        break;
    case CurseTraitType::HARD_SPELL:
        cf = TR_HARD_SPELL;
        break;
    case CurseTraitType::FAST_DIGEST:
        cf = TR_FAST_DIGEST;
        break;
    case CurseTraitType::SLOW_REGEN:
        cf = TR_SLOW_REGEN;
        break;
    case CurseTraitType::BERS_RAGE:
        cf = TR_BERS_RAGE;
        break;
    case CurseTraitType::PERSISTENT_CURSE:
        cf = TR_PERSISTENT_CURSE;
        break;
    case CurseTraitType::VUL_CURSE:
        cf = TR_VUL_CURSE;
        break;
    default:
        break;
    }

    if (flags.has_not(cf)) {
        return;
    }

    choices[*number] = item_num;
    (*number)++;
}

/*!
 * @brief 現在呪いを保持している装備品を一つランダムに探し出す / Choose one of items that have cursed flag
 * @param flag 探し出したい呪いフラグ配列
 * @return 該当の呪いが一つでもあった場合にランダムに選ばれた装備品のオブジェクト構造体参照ポインタを返す。\n
 * 呪いがない場合nullptrを返す。
 */
ItemEntity *choose_cursed_obj_name(CreatureEntity &creature, CurseTraitType flag)
{
    int choices[INVEN_TOTAL - INVEN_MAIN_HAND];
    int number = 0;
    if (creature.get_cursed_flags().has_not(flag)) {
        return nullptr;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = creature.inventory[i].get();
        if (o_ptr->curse_flags.has(flag)) {
            choices[number] = i;
            number++;
            continue;
        }

        choise_cursed_item(flag, o_ptr, choices, &number, i);
    }

    return creature.inventory[choices[randint0(number)]].get();
}

/*!
 * @brief 呪われている、トランプエゴ等による装備品由来のテレポートを実行する
 * @param creature クリーチャーへの参照
 */
static void curse_teleport(CreatureEntity &creature)
{
    if ((creature.get_cursed_special_flags().has_not(CurseSpecialTraitType::TELEPORT_SELF)) || !one_in_(200)) {
        return;
    }

    int i_keep = 0, count = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto &item = *creature.inventory[i];
        if (!item.is_valid()) {
            continue;
        }

        const auto flags = item.get_flags();

        if (flags.has_not(TR_TELEPORT)) {
            continue;
        }

        if (item.is_inscribed() && angband_strchr(item.inscription->data(), '.')) {
            continue;
        }

        count++;
        if (one_in_(count)) {
            i_keep = i;
        }
    }

    const auto &item = *creature.inventory[i_keep];
    const auto item_name = describe_flavor(creature, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s tries to teleport you."), item_name.data());
    if (input_check_strict(creature, _("テレポートしますか？", "Teleport? "), UserCheck::OKAY_CANCEL)) {
        disturb(creature, false, true);
        teleport_player(creature, 50, TELEPORT_SPONTANEOUS);
    } else {
        msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。", "You can inscribe {.} on your %s to disable random teleportation. "), item_name.data());
        disturb(creature, true, true);
    }
}

/*!
 * @details 元々呪い効果の発揮ルーチン中にいたので、整合性保持のためここに置いておく
 */
static void occur_chainsword_effect(CreatureEntity &creature)
{
    constexpr auto chance_noise = 100;
    if ((creature.get_cursed_special_flags().has_not(CurseSpecialTraitType::CHAINSWORD)) || !one_in_(chance_noise)) {
        return;
    }

    const auto noise = get_random_line(_("chainswd_j.txt", "chainswd.txt"), 0);
    if (noise.has_value()) {
        msg_print(noise.value());
    }

    disturb(creature, false, false);
}

static void curse_drain_exp(CreatureEntity &creature)
{
    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID) || (creature.get_cursed_flags().has_not(CurseTraitType::DRAIN_EXP)) || !one_in_(4)) {
        return;
    }

    creature.exp -= (creature.level + 1) / 2;
    if (creature.exp < 0) {
        creature.set_exp(0);
    }

    creature.max_exp -= (creature.level + 1) / 2;
    if (creature.max_exp < 0) {
        creature.set_max_exp(0);
    }

    check_experience(creature);
}

static void multiply_low_curse(CreatureEntity &creature)
{
    if ((creature.get_cursed_flags().has_not(CurseTraitType::ADD_L_CURSE)) || !one_in_(2000)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(creature, CurseTraitType::ADD_L_CURSE);
    auto new_curse = get_curse(0, o_ptr);
    if (o_ptr->curse_flags.has(new_curse)) {
        return;
    }

    const auto item_name = describe_flavor(creature, *o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(new_curse);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), item_name.data());
    o_ptr->feeling = FEEL_NONE;
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
}

static void multiply_high_curse(CreatureEntity &creature)
{
    if ((creature.get_cursed_flags().has_not(CurseTraitType::ADD_H_CURSE)) || !one_in_(2000)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(creature, CurseTraitType::ADD_H_CURSE);
    auto new_curse = get_curse(1, o_ptr);
    if (o_ptr->curse_flags.has(new_curse)) {
        return;
    }

    const auto item_name = describe_flavor(creature, *o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(new_curse);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), item_name.data());
    o_ptr->feeling = FEEL_NONE;
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
}

static void persist_curse(CreatureEntity &creature)
{
    if ((creature.get_cursed_flags().has_not(CurseTraitType::PERSISTENT_CURSE)) || !one_in_(500)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(creature, CurseTraitType::PERSISTENT_CURSE);
    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        return;
    }

    const auto item_name = describe_flavor(creature, *o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), item_name.data());
    o_ptr->feeling = FEEL_NONE;
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
}

static void curse_call_monster(CreatureEntity &creature)
{
    const int call_type = PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET;
    const int obj_desc_type = OD_OMIT_PREFIX | OD_NAME_ONLY;
    const auto &floor = *creature.get_floor();
    if (creature.get_cursed_flags().has(CurseTraitType::CALL_ANIMAL) && one_in_(2500)) {
        if (summon_specific(creature, creature.y, creature.x, floor.dun_level, SUMMON_ANIMAL, call_type)) {
            const auto item_name = describe_flavor(creature, *choose_cursed_obj_name(creature, CurseTraitType::CALL_ANIMAL), obj_desc_type);
            msg_format(_("%sが動物を引き寄せた！", "Your %s has attracted an animal!"), item_name.data());
            disturb(creature, false, true);
        }
    }

    if (creature.get_cursed_flags().has(CurseTraitType::CALL_DEMON) && one_in_(1111)) {
        if (summon_specific(creature, creature.y, creature.x, floor.dun_level, SUMMON_DEMON, call_type)) {
            const auto item_name = describe_flavor(creature, *choose_cursed_obj_name(creature, CurseTraitType::CALL_DEMON), obj_desc_type);
            msg_format(_("%sが悪魔を引き寄せた！", "Your %s has attracted a demon!"), item_name.data());
            disturb(creature, false, true);
        }
    }

    if (creature.get_cursed_flags().has(CurseTraitType::CALL_DRAGON) && one_in_(800)) {
        if (summon_specific(creature, creature.y, creature.x, floor.dun_level, SUMMON_DRAGON, call_type)) {
            const auto item_name = describe_flavor(creature, *choose_cursed_obj_name(creature, CurseTraitType::CALL_DRAGON), obj_desc_type);
            msg_format(_("%sがドラゴンを引き寄せた！", "Your %s has attracted a dragon!"), item_name.data());
            disturb(creature, false, true);
        }
    }

    if (creature.get_cursed_flags().has(CurseTraitType::CALL_UNDEAD) && one_in_(1111)) {
        if (summon_specific(creature, creature.y, creature.x, floor.dun_level, SUMMON_UNDEAD, call_type)) {
            const auto item_name = describe_flavor(creature, *choose_cursed_obj_name(creature, CurseTraitType::CALL_UNDEAD), obj_desc_type);
            msg_format(_("%sが死霊を引き寄せた！", "Your %s has attracted an undead!"), item_name.data());
            disturb(creature, false, true);
        }
    }
}

static void curse_cowardice(CreatureEntity &creature)
{
    if (creature.get_cursed_flags().has_not(CurseTraitType::COWARDICE)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(creature, CurseTraitType::COWARDICE);
    auto chance = 1500;
    short duration = 13 + randint1(26);
    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        chance = 150;
        duration *= 2;
    }

    if (!one_in_(chance) || (creature.has_resist_fear() != 0)) {
        return;
    }

    disturb(creature, false, true);
    msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
    (void)BadStatusSetter(creature).mod_fear(duration);
}

/*!
 * @brief 装備による狂戦士化の発作を引き起こす
 * @param creature クリーチャーへの参照
 */
static void curse_berserk_rage(CreatureEntity &creature)
{
    if (creature.get_cursed_flags().has_not(CurseTraitType::BERS_RAGE)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(creature, CurseTraitType::BERS_RAGE);
    auto chance = 1500;
    short duration = 10 + randint1(creature.level);

    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        chance = 150;
        duration *= 2;
    }

    if (!one_in_(chance)) {
        return;
    }

    disturb(creature, false, true);
    msg_print(_("ウガァァア！", "RAAAAGHH!"));
    msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
    (void)set_berserk(creature, duration, false);
    (void)BadStatusSetter(creature).set_fear(0);
}

static void curse_drain_hp(CreatureEntity &creature)
{
    if ((creature.get_cursed_flags().has_not(CurseTraitType::DRAIN_HP)) || !one_in_(666)) {
        return;
    }

    const auto *item_ptr = choose_cursed_obj_name(creature, CurseTraitType::DRAIN_HP);
    const auto item_name = describe_flavor(creature, *item_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), item_name.data());
    take_hit(creature, DAMAGE_LOSELIFE, std::min(creature.level * 2, 100), item_name);
}

static void curse_drain_mp(CreatureEntity &creature)
{
    if ((creature.get_cursed_flags().has_not(CurseTraitType::DRAIN_MANA)) || (creature.csp == 0) || !one_in_(666)) {
        return;
    }

    const auto *item_ptr = choose_cursed_obj_name(creature, CurseTraitType::DRAIN_MANA);
    const auto item_name = describe_flavor(creature, *item_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの魔力を吸収した！", "Your %s drains mana from you!"), item_name.data());
    creature.sub_csp(std::min<short>(creature.level, 50));
    if (creature.csp < 0) {
        creature.set_csp(0);
        creature.csp_frac = 0;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
}

static void curse_megaton_coin(CreatureEntity &creature)
{
    if (!get_player_flags(creature, TR_MEGATON_COIN)) {
        return;
    }
    const auto &dungeon = creature.get_floor()->get_dungeon_definition();
    if (!one_in_(364) || creature.get_floor()->dun_level == 0 || creature.get_floor()->dun_level == dungeon.maxdepth ||
        inside_quest(creature.get_floor()->quest_number) || creature.get_floor()->inside_arena) {
        return;
    }

    msg_print(_("メガトンコインで床が抜けた！ンアアアアアアァァァ！", "The floor came off with the Megaton Coin!AAAAaaaaaa!"));

    auto dam = Dice::roll(2, 8);
    take_hit(creature, DAMAGE_NOESCAPE, dam, _("メガトンコイン", "the Megaton Coin"));

    if (autosave_l && (creature.hp >= 0)) {
        do_cmd_save_game(creature, true);
    }

    exe_write_diary(*(creature.get_floor()), DiaryKind::DESCRIPTION, 0, _("メガトンコインで落ちた!", "fell through the Megaton Coin"));
    auto &fcms = FloorChangeModesStore::get_instace();
    fcms->set({ FloorChangeMode::SAVE_FLOORS,
        FloorChangeMode::DOWN,
        FloorChangeMode::RANDOM_PLACE,
        FloorChangeMode::RANDOM_CONNECT });

    creature.leaving = true;
}

static void occur_curse_effects(CreatureEntity &creature)
{
    auto is_cursed = creature.get_cursed_flags().has_any_of(TRC_P_FLAG_MASK);
    is_cursed |= creature.get_cursed_special_flags().has_any_of(TRCS_P_FLAG_MASK);
    if (!is_cursed || AngbandSystem::get_instance().is_phase_out() || AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    curse_teleport(creature);
    occur_chainsword_effect(creature);
    constexpr auto chance_ty_curse = 200;
    if (creature.get_cursed_flags().has(CurseTraitType::TY_CURSE) && one_in_(chance_ty_curse)) {
        int count = 0;
        (void)activate_ty_curse(creature, false, &count);
    }

    curse_drain_exp(creature);
    multiply_low_curse(creature);
    multiply_high_curse(creature);
    persist_curse(creature);
    curse_call_monster(creature);
    curse_cowardice(creature);
    curse_berserk_rage(creature);
    if (creature.get_cursed_flags().has(CurseTraitType::TELEPORT) && one_in_(200) && !creature.has_anti_tele()) {
        disturb(creature, false, true);
        teleport_player(creature, 40, TELEPORT_PASSIVE);
    }

    curse_drain_hp(creature);
    curse_drain_mp(creature);
    curse_megaton_coin(creature);
}

/*!
 * @brief 10ゲームターンが進行するごとに装備効果の発動判定を行う処理
 * / Handle curse effects once every 10 game turns
 * @param creature クリーチャーへの参照
 */
void execute_cursed_items_effect(CreatureEntity &creature)
{
    occur_curse_effects(creature);
    if (!one_in_(999) || creature.has_anti_magic() || (one_in_(2) && creature.has_resist_curse())) {
        return;
    }

    auto *o_ptr = creature.inventory[INVEN_LITE].get();
    if (!o_ptr->is_specific_artifact(FixedArtifactId::JUDGE)) {
        return;
    }

    if (o_ptr->is_known()) {
        msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
    } else {
        msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));
    }

    take_hit(creature, DAMAGE_LOSELIFE, std::min<short>(creature.level, 50), _("審判の宝石", "the Jewel of Judgement"));
}
