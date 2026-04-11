#pragma once

/*!
 * @file damage-dispatcher-internal.h
 * @brief ダメージディスパッチャ内部実装用ヘッダ（Phase 4）
 * @details
 * このヘッダは damage-dispatcher.cpp と player-damage.cpp からのみインクルード
 * されることを想定した私的ヘッダ。apply_damage_to_creature() のプレイヤー経路
 * 実装である apply_damage_to_player_impl() を宣言する。
 *
 * 以前は take_hit() として player-damage.h で公開されていたが、
 * Phase 4 の完全吸収により公開 API からは消滅し、ディスパッチャ内部からのみ
 * 呼び出せる実装詳細となった。
 */

#include "system/enums/monrace/monrace-id.h"
#include <string_view>

class CreatureEntity;

/*!
 * @brief プレイヤーへのダメージ適用実装（旧 take_hit）
 * @param creature 被害者（プレイヤー）
 * @param damage_type DAMAGE_ATTACK 等 (src/player/player-damage.h)
 * @param damage 与えるダメージ量
 * @param hit_from 死亡原因・攻撃元の説明文
 * @param killer_monrace_id プレイヤー死因となったモンスター種族
 * @return 実際に適用されたダメージ量
 * @note
 * apply_damage_to_creature() から呼び出される内部関数。
 * プレイヤー固有の事前処理（捨て身・居合・多重影分身・幽体化 等）、
 * ダメージ適用、死亡処理、警告音、野外モード処理などを担当する。
 */
int apply_damage_to_player_impl(CreatureEntity &creature, int damage_type, int damage,
    std::string_view hit_from, MonraceId killer_monrace_id = static_cast<MonraceId>(0));
