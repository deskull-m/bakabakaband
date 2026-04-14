#pragma once

#include "alliance/alliance.h"
#include "monster/monster-flag-types.h"
#include "monster/smart-learn-types.h"
#include "object/object-index-list.h"
#include "system/creature-timed-effect-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "util/flag-group.h"
#include <map>
#include <vector>

enum class PlayerRaceType;
enum class PlayerClassType : short;

/* Sub-alignment flags for neutral monsters */
#define SUB_ALIGN_NEUTRAL 0x0000 /*!< モンスターのサブアライメント:中立 */
#define SUB_ALIGN_EVIL 0x0001 /*!< モンスターのサブアライメント:善 */
#define SUB_ALIGN_GOOD 0x0002 /*!< モンスターのサブアライメント:悪 */

/*!
 * @brief モンスターインスタンス固有データ
 */
struct MonsterProfile {
    BIT_FLAGS8 sub_align{}; /*!< 中立属性のモンスターが召喚主のアライメントに従い一時的に立っている善悪陣営 / Sub-alignment for a neutral monster */
    AllianceType alliance_idx{}; /*!< 現在の所属アライアンス */
    std::vector<PlayerRaceType> equivalent_player_races{}; /*!< モンスターのフラグに基づいて対応するプレイヤー種族IDリスト */
    std::vector<PlayerClassType> equivalent_player_classes{}; /*!< モンスターのフラグに基づいて対応するプレイヤー職業IDリスト */
    int death_count{}; /*!< 自壊するまでの残りターン数 */
    std::map<CreatureTimedEffect, short> mtimed{}; /*!< 与えられた時限効果の残りターン / Timed status counter */
    EnumClassFlagGroup<MonsterTemporaryFlagType> mflag{}; /*!< モンスター個体に与えられた特殊フラグ1 (セーブ不要) / Extra monster flags */
    EnumClassFlagGroup<MonsterConstantFlagType> mflag2{}; /*!< モンスター個体に与えられた特殊フラグ2 (セーブ必要) / Extra monster flags */
    bool ml{}; /*!< モンスターがプレイヤーにとって視認できるか(処理のためのテンポラリ変数) Monster is "visible" */
    ObjectIndexList hold_o_idx_list{}; /*!< モンスターが所持しているアイテムのリスト / Object list being held (if any) */
    EnumClassFlagGroup<MonsterSmartLearnType> smart{}; /*!< モンスターのプレイヤーに対する学習状態 / Field for "smart_learn" */
    MONSTER_IDX parent_m_idx{}; /*!< 召喚主のモンスターID */
    MonraceId transform_r_idx{}; /*!< 変身先モンスター種族ID */
    PERCENTAGE transform_hp_threshold{}; /*!< 変身するHP閾値(最大HPの%) */
    bool has_transformed{}; /*!< 既に変身済みかどうかのフラグ */
};
