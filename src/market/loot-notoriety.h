#pragma once

/*!
 * @file loot-notoriety.h
 * @brief 他者の正当な所有物 (ItemEntity::rightful_owner) を略奪したことによる社会的問題
 * @details
 * 刻印 (`ItemEntity::rightful_owner`) の付いた品を拾った時点を「略奪の成立」とみなし、
 * インシデント (`CreatureEntity::incident_tree`) に記録した上で 4 系統の社会的問題を
 * 引き起こす。記録は全てインシデント経由なので savefile に保存され、効果は
 * インシデントの集計値から都度算出する (フィールドを増やさない)。
 *
 * | 系統 | 効果 | 配線先 |
 * |---|---|---|
 * | 1. 街の評判低下 | 累計略奪数に応じて社会的地位 (`prestige`) が下がる | 拾得時 |
 * | 2. 店の買い叩き・買取拒否 | 刻印付きの品は買取価格を割り引かれる。賞金首なら正業の店は買取拒否 | `price_item` / `store_sell` |
 * | 3. 賞金首化 | 略奪総額に応じた賞金首ランク。街の住人が敵対して生成される | `one_monster_placer` |
 * | 4. アライアンス敵対度上昇 | 略奪した相手のアライアンスの印象値が下がる | `Alliance::calc_impression_point_total` |
 * | 5. 死体を漁る悪行 | 死体を漁るたびに名誉 (`Virtue::HONOUR`) が下がる | `rummage_corpses_here` |
 *
 * 系統 5 は「死体を漁る行為そのもの」に対する記録・処罰で、刻印の有無に依らない
 * (無刻印の品しか持たない死体を漁っても記録される)。刻印付きの品を取り出した
 * ぶんは系統 1〜4 が別に発火する。
 */

#include <string>

enum class AllianceType : int;
enum class MonraceId : short;
enum class StoreSaleType : int;
class CreatureEntity;
class ItemEntity;
class MonraceDefinition;

//!< 略奪した刻印付きアイテムの累計個数
constexpr auto INCIDENT_LOOT_COUNT = "LOOT_RIGHTFUL_PROPERTY";
//!< 略奪した刻印付きアイテムの累計価値 (金額)
constexpr auto INCIDENT_LOOT_VALUE = "LOOT_RIGHTFUL_PROPERTY/VALUE";
//!< 略奪した刻印付きアイテムの累計個数 (所有者のアライアンス別)。後続はアライアンスタグ
constexpr auto INCIDENT_LOOT_ALLIANCE_PREFIX = "LOOT_RIGHTFUL_PROPERTY/ALLIANCE/";
//!< 略奪により失った社会的地位の累計 (系統 1)
constexpr auto INCIDENT_LOOT_PRESTIGE_LOST = "LOOT_NOTORIETY/PRESTIGE_LOST";
//!< 盗品の買取を店に拒否された累計回数 (系統 2)
constexpr auto INCIDENT_LOOT_STORE_REFUSED = "LOOT_NOTORIETY/STORE_REFUSED";
//!< 到達した賞金首ランク (系統 3)
constexpr auto INCIDENT_LOOT_BOUNTY_LEVEL = "LOOT_NOTORIETY/BOUNTY_LEVEL";
//!< 死体を漁った累計回数 (系統 5)
constexpr auto INCIDENT_RUMMAGE_CORPSE_COUNT = "LOOT_CORPSE";
//!< 死体から取り出した累計個数 (系統 5)
constexpr auto INCIDENT_RUMMAGE_CORPSE_ITEM = "LOOT_CORPSE/ITEM";
//!< 死体を漁った累計回数 (死体の種族別、系統 5)。後続は MonraceId の数値
constexpr auto INCIDENT_RUMMAGE_CORPSE_MONRACE_PREFIX = "LOOT_CORPSE/MONRACE/";

void record_looted_rightful_property(CreatureEntity &creature, const ItemEntity &item, int number);
void record_rummaging_corpse(CreatureEntity &creature, MonraceId monrace_id);
void record_rummaged_corpse_item(CreatureEntity &creature, int number);

int get_rummaged_corpse_count(const CreatureEntity &creature);
int get_rummaged_corpse_item_count(const CreatureEntity &creature);
std::string make_rummage_corpse_incident_id(MonraceId monrace_id);

int get_looted_item_count(const CreatureEntity &creature);
int get_looted_value(const CreatureEntity &creature);
int get_looted_count_of_alliance(const CreatureEntity &creature, AllianceType alliance_id);
int get_loot_bounty_level(const CreatureEntity &creature);
int get_loot_bounty_level_max();

int get_loot_store_sale_discount_percent(const CreatureEntity &creature, const ItemEntity &item);
bool store_refuses_stolen_goods(const CreatureEntity &creature, const ItemEntity &item, StoreSaleType store_num);
void notify_store_refusal_of_stolen_goods(CreatureEntity &creature);

int get_loot_alliance_impression_penalty(const CreatureEntity &creature, AllianceType alliance_id);
bool is_wanted_by_townsfolk(const CreatureEntity &creature, const MonraceDefinition &monrace);
