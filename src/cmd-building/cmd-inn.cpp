#include "cmd-building/cmd-inn.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/turn-compensator.h"
#include "game-option/birth-options.h"
#include "io/write-diary.h"
#include "market/bounty.h"
#include "market/building-actions-table.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "rumor/rumor-rarity.h"
#include "rumor/rumor-service.h"
#include "status/bad-status-setter.h"
#include "store/rumor.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/inner-game-data.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"

/*!
 * @brief 宿屋で食事を摂る
 * @param creature クリーチャーへの参照
 * @return 満腹ならFALSE、そうでないならTRUE
 */
static bool buy_food(CreatureEntity &creature)
{
    if (creature.get_food() >= PY_FOOD_FULL) {
        msg_print(_("今は満腹だ。", "You are full now."));
        return false;
    }

    msg_print(_("バーテンはいくらかの食べ物とビールをくれた。", "The barkeep gives you some gruel and a beer."));
    (void)set_food(creature, PY_FOOD_MAX - 1);
    return true;
}

/*!
 * @brief 健康体しか宿屋に泊めない処理
 * @param creature クリーチャーへの参照
 * @return 毒でも切り傷でもないならTRUE、そうでないならFALSE
 */
static bool is_healthy_stay(CreatureEntity &creature)
{
    if (!creature.is_poisoned() && !creature.is_cut()) {
        return true;
    }

    msg_print(_("あなたに必要なのは部屋ではなく、治療者です。", "You need a healer, not a room."));
    msg_erase();
    msg_print(_("すみません、でもうちで誰かに死なれちゃ困りますんで。", "Sorry, but I don't want anyone dying in here."));
    return false;
}

static bool is_player_undead(CreatureEntity &creature)
{
    return CreatureRace(&creature, true).life() == PlayerRaceLifeType::UNDEAD;
}

/*!
 * @brief 宿屋に泊まったことを日記に残す
 * @param creature クリーチャーへの参照
 * @param prev_hour 宿屋に入った直後のゲーム内時刻
 */
static void write_diary_stay_inn(CreatureEntity &creature, int prev_hour)
{
    const auto &floor = *creature.get_floor();
    if ((prev_hour >= 6) && (prev_hour < 18)) {
        const auto stay_message = _(is_player_undead(creature) ? "宿屋に泊まった。" : "日が暮れるまで宿屋で過ごした。", "stayed during the day at the inn.");
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, stay_message);
        return;
    }

    const auto stay_message = _(is_player_undead(creature) ? "夜が明けるまで宿屋で過ごした。" : "宿屋に泊まった。", "stayed overnight at the inn.");
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, stay_message);
}

/*!
 * @brief 悪夢モードなら悪夢を見せる
 * @param creature クリーチャーへの参照
 * @return 悪夢モードならばTRUE
 */
static bool has_a_nightmare(CreatureEntity &creature)
{
    if (!ironman_nightmare) {
        return false;
    }

    msg_print(_("眠りに就くと恐ろしい光景が心をよぎった。", "Horrible visions flit through your mind as you sleep."));

    while (true) {
        sanity_blast(creature);
        if (!one_in_(3)) {
            break;
        }
    }

    msg_print(_("あなたは絶叫して目を覚ました。", "You awake screaming."));
    exe_write_diary(*creature.get_floor(), DiaryKind::DESCRIPTION, 0, _("悪夢にうなされてよく眠れなかった。", "had a nightmare."));
    return true;
}

/*!
 * @brief 体調を元に戻す
 * @param creature クリーチャーへの参照
 */
static void back_to_health(CreatureEntity &creature)
{
    BadStatusSetter bss(creature);
    (void)bss.set_blindness(0);
    (void)bss.set_confusion(0);
    creature.set_timed_effect(CreatureTimedEffect::STUN, 0);
    creature.hp = creature.maxhp;
    creature.set_csp(creature.get_msp());
}

/*!
 * @brief 魔道具術師の取り込んだ魔法をすべて完全に回復した状態にする
 * @param creature クリーチャーへの参照
 */
static void charge_magic_eating_energy(CreatureEntity &creature)
{
    auto magic_eater_data = CreatureClass(creature).get_specific_data<MagicEaterDataList>();
    if (!magic_eater_data) {
        return;
    }

    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND }) {
        for (auto &item : magic_eater_data->get_item_group(tval)) {
            item.charge = item.count * EATER_CHARGE;
        }
    }
    for (auto &item : magic_eater_data->get_item_group(ItemKindType::ROD)) {
        item.charge = 0;
    }
}

/*!
 * @brief リフレッシュ結果を画面に表示する
 * @param creature クリーチャーへの参照
 * @param prev_hour 宿屋に入った直後のゲーム内時刻
 */
static void display_stay_result(CreatureEntity &creature, int prev_hour)
{
    const auto &floor = *creature.get_floor();
    if ((prev_hour >= 6) && (prev_hour < 18)) {
#if JP
        char refresh_message_jp[50];
        sprintf(refresh_message_jp, "%s%s%s", "あなたはリフレッシュして目覚め、", is_player_undead(creature) ? "夜" : "夕方", "を迎えた。");
        msg_print(refresh_message_jp);
#else
        msg_format("You awake refreshed for the %s.", is_player_undead(creature) ? "evening" : "twilight");
#endif
        const auto awake_message = _(is_player_undead(creature) ? "すがすがしい夜を迎えた。" : "夕方を迎えた。", "awoke refreshed.");
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, awake_message);
        return;
    }

    msg_print(_("あなたはリフレッシュして目覚め、新たな日を迎えた。", "You awake refreshed for the new day."));
    const auto awake_message = _(is_player_undead(creature) ? "すがすがしい朝を迎えた。" : "朝を迎えた。", "awoke refreshed.");
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, awake_message);
}

/*!
 * @brief 宿屋への宿泊実行処理
 * @param creature クリーチャーへの参照
 * @return 泊まれたらTRUE
 */
static bool stay_inn(CreatureEntity &creature)
{
    if (!is_healthy_stay(creature)) {
        return false;
    }

    auto &world = AngbandWorld::get_instance();
    const auto &[prev_day, prev_hour, prev_min] = world.extract_date_time(InnerGameData::get_instance().get_start_race());
    write_diary_stay_inn(creature, prev_hour);
    world.pass_game_turn_by_stay();
    wc_ptr->plus_timed_world_collapsion(&world, creature, 25000);
    prevent_turn_overflow(creature);
    if ((prev_hour >= 18) && (prev_hour <= 23)) {
        determine_daily_bounty(creature);
        exe_write_diary(*creature.get_floor(), DiaryKind::DIALY, 0);
    }

    creature.hp = creature.maxhp;
    if (has_a_nightmare(creature)) {
        return true;
    }

    back_to_health(creature);
    charge_magic_eating_energy(creature);

    display_stay_result(creature, prev_hour);

    creature.plus_incident_tree("STAY_INN", 1);

    return true;
}

/*!
 * @brief 宿屋を利用する
 * @param creature クリーチャーへの参照
 * @param cmd 宿屋の利用施設ID
 * @param cost 噂を聞くのに払った金額
 * @return 施設の利用が実際に行われたらTRUE
 * @todo 悪夢を見る前後に全回復しているが、何か意図がある？
 */
bool inn_comm(CreatureEntity &creature, int cmd, int cost)
{
    switch (cmd) {
    case BACT_FOOD:
        return buy_food(creature);
    case BACT_REST:
        return stay_inn(creature);
    case BACT_RUMORS: {
        constexpr auto high_rarity_rumor_threshold = 100;
        if (cost >= high_rarity_rumor_threshold) {
            const auto &rumor = RumorService::pick_rumor(RumorRarity::HIGH);
            display_selected_rumor(rumor);
            return true;
        }

        constexpr auto medium_rarity_rumor_threshold = 10;
        if (cost >= medium_rarity_rumor_threshold) {
            const auto &rumor = RumorService::pick_rumor(RumorRarity::MEDIUM);
            display_selected_rumor(rumor);
            return true;
        }

        // V3.2.0.4時点で1～9$を取られる宿屋の噂はなく、デッドコード.
        const auto &rumor = RumorService::pick_rumor(RumorRarity::LOW);
        display_selected_rumor(rumor);
        return true;
    }
    default:
        //!< @todo リファクタリング前のコードもTRUEだった、FALSEにすべきでは.
        return true;
    }
}
