#pragma once

/*!
 * @file damage-dispatcher.h
 * @brief プレイヤー・モンスター両対応のダメージ処理統一エントリポイント（Phase 4）
 * @details
 * プレイヤー・モンスター双方のダメージ処理エントリを統一する公開 API。
 * 呼び出し元は被害者の種別を気にせず本関数を呼べばよい。
 * 内部では is_player() で振り分けて、プレイヤー経路は
 * apply_damage_to_player_impl() （damage-dispatcher-internal.h）、
 * モンスター経路は MonsterDamageProcessor::mon_take_hit() に委譲する。
 * 旧 take_hit() 自由関数はこの吸収により公開 API から削除された。
 */

#include "effect/attribute-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/h-type.h"
#include <string_view>

class CreatureEntity;

/*!
 * @brief ダメージ適用時の追加コンテキスト
 * @details プレイヤー経路・モンスター経路で必要となるパラメータを束ねる。
 * すべてのフィールドが全経路で使われるわけではない。
 */
struct DamageContext {
    /* 共通 */
    CreatureEntity *attacker = nullptr; /*!< 加害者（環境ダメージの場合は nullptr） */
    std::string_view cause{}; /*!< 死亡原因・攻撃元の説明文（hit_from / note） */

    /* プレイヤー経路用 */
    int damage_type = 0; /*!< DAMAGE_ATTACK 等 (src/player/player-damage.h) */
    MonraceId killer_monrace_id = static_cast<MonraceId>(0); /*!< プレイヤー死因となったモンスター種族 */

    /* モンスター経路用 */
    MONSTER_IDX victim_m_idx = 0; /*!< 被害者モンスターのフロア上インデックス */
    AttributeFlags attribute_flags{}; /*!< 与えたダメージ属性 */
    bool *fear = nullptr; /*!< ダメージによって恐慌状態になったか格納する先 */
};

/*!
 * @brief クリーチャーへダメージを適用する統一エントリポイント
 * @param victim 被害者
 * @param damage 与えるダメージ量
 * @param ctx 追加コンテキスト
 * @return 実際に適用されたダメージ量（0 以上の整数）
 * @note
 * プレイヤー経路では ctx.damage_type / cause / killer_monrace_id を使用し、
 * モンスター経路では ctx.attacker / victim_m_idx / attribute_flags / fear / cause を使用する。
 *
 * 戻り値セマンティクスはプレイヤー・モンスター両経路で統一されている:
 *   「実際に適用されたダメージ量」
 * - プレイヤー経路: 無敵・多重影分身・幽体化等の mitigation 後の適用量
 *   （無敵等で完全回避の場合は 0）
 * - モンスター経路: 入力 damage と同値を返す（事前に耐性・防御計算済みの
 *   damage を受け取る設計のため）。attacker が nullptr の場合は 0 を返す
 *
 * 生死情報が必要な場合は呼び出し後に victim.is_dead() を参照すること。
 */
int apply_damage_to_creature(CreatureEntity &victim, int damage, const DamageContext &ctx);

/*!
 * @brief 環境ダメージ用の便利オーバーロード（旧 take_hit() と同一シグネチャ）
 * @param victim 被害者
 * @param damage_type DAMAGE_ATTACK 等 (src/player/player-damage.h)
 * @param damage 与えるダメージ量
 * @param cause 死亡原因の説明文
 * @param killer_monrace_id プレイヤー死因となったモンスター種族（環境ダメージなら省略可）
 * @return 実際に適用されたダメージ量（0 以上の整数）
 * @note
 * トラップ・飢餓・呪い装備等の環境ダメージからの呼び出しに適したショートカット。
 * 旧 take_hit() との移行互換性のためのシグネチャでもある。
 * モンスター経路では加害者が無いため 0 を返す（環境効果が直接モンスターを殺す
 * ケースは現状未実装）。
 */
int apply_damage_to_creature(CreatureEntity &victim, int damage_type, int damage, std::string_view cause,
    MonraceId killer_monrace_id = static_cast<MonraceId>(0));
