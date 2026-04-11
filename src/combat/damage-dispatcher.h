#pragma once

/*!
 * @file damage-dispatcher.h
 * @brief プレイヤー・モンスター両対応のダメージ処理統一エントリポイント（Phase 4）
 * @details
 * 現状プレイヤーとモンスターでダメージ処理のエントリが分離しているため、
 * 呼び出し元が被害者の種別を気にせず統一的に書けるようにする薄いディスパッチャ。
 * 内部では is_player() で振り分けて既存の take_hit() または
 * MonsterDamageProcessor::mon_take_hit() に委譲する。
 * 将来的に両者を統合する際のリファクタリングの起点として機能する。
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
 * @return プレイヤー経路: 実際に適用されたダメージ量 / モンスター経路: 死亡したら 1、さもなくば 0
 * @note
 * 過渡的な薄いラッパー。プレイヤー経路では ctx.damage_type / cause / killer_monrace_id を使用し、
 * モンスター経路では ctx.attacker / victim_m_idx / attribute_flags / fear / cause を使用する。
 * 将来的に両者が完全統合されたら本関数の戻り値型・引数を整理する。
 */
int apply_damage_to_creature(CreatureEntity &victim, int damage, const DamageContext &ctx);

/*!
 * @brief 環境ダメージ用の便利オーバーロード（既存 take_hit() と同一シグネチャ）
 * @param victim 被害者
 * @param damage_type DAMAGE_ATTACK 等 (src/player/player-damage.h)
 * @param damage 与えるダメージ量
 * @param cause 死亡原因の説明文
 * @param killer_monrace_id プレイヤー死因となったモンスター種族（環境ダメージなら省略可）
 * @return プレイヤー経路: 実際に適用されたダメージ量 / モンスター経路: 死亡したら 1、さもなくば 0
 * @note
 * トラップ・飢餓・呪い装備等の環境ダメージを take_hit() から置換しやすくするためのショートカット。
 * モンスター経路では加害者が無いため、現状は 0 を返すのみ（環境効果が直接モンスターを殺すケースは未実装）。
 */
int apply_damage_to_creature(CreatureEntity &victim, int damage_type, int damage, std::string_view cause,
    MonraceId killer_monrace_id = static_cast<MonraceId>(0));
