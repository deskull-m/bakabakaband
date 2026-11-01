/*!
 * @file loot-notoriety.cpp
 * @brief 他者の正当な所有物を略奪したことによる社会的問題
 * @details 設計と効果の一覧は loot-notoriety.h のコメントを参照。
 */

#include "market/loot-notoriety.h"
#include "alliance/alliance.h"
#include "monster-race/race-wilderness-flags.h"
#include "object/object-value.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/store-sale-type.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "view/display-messages.h"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace {
/*!
 * @brief 社会的地位が 1 下がる略奪数の間隔 (系統 1)
 * @details 累計略奪数がこの倍数に達するごとに `prestige` が 1 減る (0 が下限)。
 *          小さくするほど評判の低下が速い。
 */
constexpr auto LOOT_PRESTIGE_INTERVAL = 10;

/*!
 * @brief 刻印付きアイテムの買取価格割引率 [%] の基本値 (系統 2)
 * @details 賞金首でなくとも盗品の値は渋られる。
 */
constexpr auto LOOT_STORE_SALE_DISCOUNT_BASE = 10;

/*!
 * @brief 賞金首ランク 1 あたりの買取価格割引率 [%] の増分 (系統 2)
 */
constexpr auto LOOT_STORE_SALE_DISCOUNT_PER_RANK = 10;

/*!
 * @brief 買取価格割引率 [%] の上限 (系統 2)
 * @details 盗品が完全に換金不能にならないよう上限を設ける。闇市は元から半値なので、
 *          上限時の手取りは「基本価格の半分のさらに (100-上限)%」になる。
 */
constexpr auto LOOT_STORE_SALE_DISCOUNT_MAX = 60;

/*!
 * @brief 賞金首ランクが 1 上がる略奪総額 (系統 3)
 */
constexpr auto LOOT_BOUNTY_VALUE_UNIT = 20000;

/*!
 * @brief 賞金首ランクの上限 (系統 3)
 */
constexpr auto LOOT_BOUNTY_LEVEL_MAX = 5;

/*!
 * @brief 正業の店が盗品の買取を拒否し始める賞金首ランク (系統 2)
 * @details ブラック・マーケットは例外で、賞金首でも盗品を買う。
 */
constexpr auto LOOT_STORE_REFUSAL_BOUNTY_LEVEL = 1;

/*!
 * @brief 略奪 1 個あたりのアライアンス印象値ペナルティ (系統 4)
 */
constexpr auto LOOT_ALLIANCE_IMPRESSION_PER_ITEM = 50;

/*!
 * @brief アライアンス印象値ペナルティの上限 (系統 4)
 * @details 単一の略奪行為で敵対度が振り切れないよう有界にしておく。
 */
constexpr auto LOOT_ALLIANCE_IMPRESSION_PENALTY_MAX = 20000;

/*!
 * @brief インシデントの集計値を読む (const 参照用)
 * @param creature 対象のクリーチャー
 * @param incident_id インシデント ID
 * @return 記録が無ければ 0
 * @details `incident_tree[]` は非 const なので、const 参照からは find() で読む。
 */
int peek_incident(const CreatureEntity &creature, const std::string &incident_id)
{
    const auto it = creature.incident_tree.find(incident_id);
    return (it == creature.incident_tree.end()) ? 0 : it->second;
}

/*!
 * @brief インシデントへ加算する (int32 の範囲に収める)
 * @param creature 対象のクリーチャー
 * @param incident_id インシデント ID
 * @param delta 加算量 (0 以下なら何もしない)
 * @details 略奪総額は高価なアーティファクトを拾い続けると int32 を溢れうるため、
 *          加算側で飽和させる。
 */
void add_incident_saturating(CreatureEntity &creature, const std::string &incident_id, int64_t delta)
{
    if (delta <= 0) {
        return;
    }

    const auto room = std::numeric_limits<int32_t>::max() - static_cast<int64_t>(peek_incident(creature, incident_id));
    const auto added = static_cast<int>(std::min<int64_t>(delta, room));
    if (added > 0) {
        creature.plus_incident_tree(incident_id, added);
    }
}

/*!
 * @brief 略奪した相手のアライアンス別インシデント ID を組み立てる
 * @param alliance_id アライアンス
 * @return インシデント ID
 */
std::string make_loot_alliance_incident_id(AllianceType alliance_id)
{
    return std::string(INCIDENT_LOOT_ALLIANCE_PREFIX) + get_alliance_type_tag(alliance_id);
}

/*!
 * @brief 系統 1: 累計略奪数に応じて社会的地位を下げる
 * @param creature 対象のクリーチャー
 * @param count_before 今回の略奪前の累計略奪数
 * @param count_after 今回の略奪後の累計略奪数
 * @details 評判は「噂が広まる」ものなので、拾った瞬間ではなく一定数ごとに段階的に
 *          下がる。`prestige` は 0 を下限とし、それ以上は下がらない。
 */
void apply_loot_prestige_penalty(CreatureEntity &creature, int count_before, int count_after)
{
    const auto steps = (count_after / LOOT_PRESTIGE_INTERVAL) - (count_before / LOOT_PRESTIGE_INTERVAL);
    if (steps <= 0) {
        return;
    }

    const auto lost = std::min<int>(steps, creature.get_prestige());
    if (lost <= 0) {
        return;
    }

    creature.set_prestige(static_cast<int16_t>(creature.get_prestige() - lost));
    add_incident_saturating(creature, INCIDENT_LOOT_PRESTIGE_LOST, lost);
    msg_print(_("亡き者の持ち物を剥いで回っているという噂が広まっている…", "Word spreads that you strip the fallen of their belongings..."));
}

/*!
 * @brief 系統 3: 略奪総額に応じた賞金首ランクの上昇を記録する
 * @param creature 対象のクリーチャー
 * @param level_before 今回の略奪前の賞金首ランク
 * @details `INCIDENT_LOOT_BOUNTY_LEVEL` には上昇分を加算するので、記録値は
 *          「到達した最高ランク」になる (略奪総額は減らないため単調増加)。
 */
void record_loot_bounty_level(CreatureEntity &creature, int level_before)
{
    const auto level_after = get_loot_bounty_level(creature);
    if (level_after <= level_before) {
        return;
    }

    add_incident_saturating(creature, INCIDENT_LOOT_BOUNTY_LEVEL, level_after - level_before);
    msg_print(_("あなたの首に賞金が懸けられた！", "A bounty has been placed on your head!"));
}
}

/*!
 * @brief 他者の正当な所有物を手に入れたことを記録し、社会的問題を発生させる
 * @param creature 略奪したクリーチャー
 * @param item 手に入れた品
 * @param number 手に入れた個数
 * @details 刻印の無い品では何も起きない。撃破して略奪した品はこの刻印が付いたまま
 *          床へ落ちるため、拾った時点が「略奪の成立」になる。
 *          累計価値は鑑定状態に依らない `object_value_real()` で量る (`calc_price()` は
 *          未鑑定だと当て推量になり、装備品は 0 と評価されてしまう)。
 */
void record_looted_rightful_property(CreatureEntity &creature, const ItemEntity &item, int number)
{
    if (!item.has_rightful_owner()) {
        return;
    }

    const auto count = std::max(1, number);
    const auto count_before = get_looted_item_count(creature);
    const auto bounty_level_before = get_loot_bounty_level(creature);

    add_incident_saturating(creature, INCIDENT_LOOT_COUNT, count);
    add_incident_saturating(creature, INCIDENT_LOOT_VALUE, static_cast<int64_t>(object_value_real(&item)) * count);

    const auto &monrace = MonraceList::get_instance().get_monrace(item.get_rightful_owner());
    if (monrace.alliance_idx != AllianceType::NONE) {
        add_incident_saturating(creature, make_loot_alliance_incident_id(monrace.alliance_idx), count);
    }

    apply_loot_prestige_penalty(creature, count_before, get_looted_item_count(creature));
    record_loot_bounty_level(creature, bounty_level_before);
}

/*!
 * @brief 略奪した刻印付きアイテムの累計個数を返す
 */
int get_looted_item_count(const CreatureEntity &creature)
{
    return peek_incident(creature, INCIDENT_LOOT_COUNT);
}

/*!
 * @brief 略奪した刻印付きアイテムの累計価値を返す
 */
int get_looted_value(const CreatureEntity &creature)
{
    return peek_incident(creature, INCIDENT_LOOT_VALUE);
}

/*!
 * @brief 指定アライアンスに属する者から略奪した累計個数を返す
 * @param creature 対象のクリーチャー
 * @param alliance_id アライアンス
 * @return 累計個数。無所属 (NONE) は常に 0
 */
int get_looted_count_of_alliance(const CreatureEntity &creature, AllianceType alliance_id)
{
    if (alliance_id == AllianceType::NONE) {
        return 0;
    }

    return peek_incident(creature, make_loot_alliance_incident_id(alliance_id));
}

/*!
 * @brief 賞金首ランクを返す (系統 3)
 * @param creature 対象のクリーチャー
 * @return 0 (賞金首でない) 〜 LOOT_BOUNTY_LEVEL_MAX
 */
int get_loot_bounty_level(const CreatureEntity &creature)
{
    return std::min(get_looted_value(creature) / LOOT_BOUNTY_VALUE_UNIT, LOOT_BOUNTY_LEVEL_MAX);
}

/*!
 * @brief 賞金首ランクの上限を返す (表示用)
 */
int get_loot_bounty_level_max()
{
    return LOOT_BOUNTY_LEVEL_MAX;
}

/*!
 * @brief 刻印付きアイテムの買取価格割引率 [%] を返す (系統 2)
 * @param creature 売ろうとしているクリーチャー
 * @param item 売ろうとしているアイテム
 * @return 0 (割引なし) 〜 LOOT_STORE_SALE_DISCOUNT_MAX
 * @details 盗品は買い叩かれる。刻印の無い品では 0。
 *          **markup ではなく基本価格に掛けること。** `calc_store_price()` の売却補正は
 *          `min(400 - markup, 100)` で、markup が 300 以下では飽和して効かず、
 *          400 を超えると売値が 1 に落ちるため、markup を動かすと「店主の強欲さ次第で
 *          無効か崖か」という使えないレバーになる (実測: 闇市で売値 $1)。
 */
int get_loot_store_sale_discount_percent(const CreatureEntity &creature, const ItemEntity &item)
{
    if (!item.has_rightful_owner()) {
        return 0;
    }

    const auto discount = LOOT_STORE_SALE_DISCOUNT_BASE + LOOT_STORE_SALE_DISCOUNT_PER_RANK * get_loot_bounty_level(creature);
    return std::min(discount, LOOT_STORE_SALE_DISCOUNT_MAX);
}

/*!
 * @brief 店が盗品の買取を拒否するかを返す (系統 2)
 * @param creature 売ろうとしているクリーチャー
 * @param item 売ろうとしている品
 * @param store_num 店舗種別
 * @return 拒否するなら true
 * @details 正業の店は、賞金首が持ち込んだ刻印付きの品を扱わない。
 *          ブラック・マーケットは盗品でも買う (半値だが売却経路が残る)。
 *          我が家・博物館は売買ではないので対象外。
 */
bool store_refuses_stolen_goods(const CreatureEntity &creature, const ItemEntity &item, StoreSaleType store_num)
{
    if (!item.has_rightful_owner()) {
        return false;
    }

    switch (store_num) {
    case StoreSaleType::BLACK:
    case StoreSaleType::HOME:
    case StoreSaleType::MUSEUM:
        return false;
    default:
        break;
    }

    return get_loot_bounty_level(creature) >= LOOT_STORE_REFUSAL_BOUNTY_LEVEL;
}

/*!
 * @brief 盗品の買取を拒否されたことを表示・記録する (系統 2)
 * @param creature 売ろうとしたクリーチャー
 */
void notify_store_refusal_of_stolen_goods(CreatureEntity &creature)
{
    msg_print(_("「おっと、そいつは他人様の持ち物だね。うちじゃ扱えないよ。」", "'Hold on, that belongs to someone else. I cannot handle it here.'"));
    creature.plus_incident_tree(INCIDENT_LOOT_STORE_REFUSED, 1);
}

/*!
 * @brief 略奪によるアライアンス印象値ペナルティを返す (系統 4)
 * @param creature 対象のクリーチャー
 * @param alliance_id アライアンス
 * @return 印象値から差し引く値 (0 以上)
 * @details そのアライアンスに属する者の持ち物を剥いだ数に比例する。無所属 (NONE) は
 *          所有権の記録対象にならないので常に 0。
 */
int get_loot_alliance_impression_penalty(const CreatureEntity &creature, AllianceType alliance_id)
{
    const auto count = get_looted_count_of_alliance(creature, alliance_id);
    if (count <= 0) {
        return 0;
    }

    return static_cast<int>(std::min<int64_t>(
        static_cast<int64_t>(count) * LOOT_ALLIANCE_IMPRESSION_PER_ITEM, LOOT_ALLIANCE_IMPRESSION_PENALTY_MAX));
}

/*!
 * @brief 賞金首として街の住人に敵対されるかを返す (系統 3)
 * @param creature 対象のクリーチャー
 * @param monrace 生成しようとしているモンスター種族
 * @return 敵対して生成すべきなら true
 * @details 賞金首になると、街に湧く住人は友好的に生成されなくなる。既に場に居る
 *          個体の態度は変えない (噂が広まるのは次に湧く個体から)。
 */
bool is_wanted_by_townsfolk(const CreatureEntity &creature, const MonraceDefinition &monrace)
{
    if (get_loot_bounty_level(creature) <= 0) {
        return false;
    }

    return monrace.wilderness_flags.has(MonsterWildernessType::WILD_TOWN);
}
