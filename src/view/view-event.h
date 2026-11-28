#pragma once

/*!
 * @file view-event.h
 * @brief 表示用のゲームイベント (3D 表示のアニメーション・エフェクト用)
 * @details
 * 端末版はボルトの軌跡や攻撃をセルの書き換えとメッセージでしか表さない。
 * 3D 表示などで演出するため、プレイヤーが知覚できた出来事を型付きで外部へ流す。
 *
 * - 受け手 (ViewEventSink) は 1 つだけ登録できる。未登録 (既定) なら呼出側は
 *   is_view_event_sink_active() の判定 1 回で何もしない
 * - 通知はゲームスレッドから呼ばれる。受け手はゲームの状態を変えてはならず、乱数も消費しない
 * - 演出のためにゲームを待たせることはしない (受け手は後追いで再生する)
 * - 移動はここでは流さない。ワールドスナップショットのクリーチャーの表示用 ID と座標の変化から求める
 */

#include "util/point-2d.h"
#include <cstdint>
#include <utility>
#include <vector>

class CreatureEntity;

/*!
 * @brief イベントの種類
 * @details 値は外部 (Godot) へそのまま渡すため、追加は末尾に限る。
 */
enum class ViewEventType : uint8_t {
    MELEE = 1, //!< 近接攻撃の 1 打撃 (命中・空振り)
    PROJECTION = 2, //!< ボルト・ボール・ビーム・ブレスなど project() の軌跡と効果範囲
    DAMAGE = 3, //!< HP が減った
    DEATH = 4, //!< 死亡した

    // 以下はゲームコアからは流さない。受け手 (Godot のブリッジ) がワールドスナップショットの
    // クリーチャーの表示用 ID と座標の変化から作る
    MOVE = 5, //!< 隣のグリッドへ歩いた
    TELEPORT = 6, //!< 2 グリッド以上離れた所へ移った
    APPEAR = 7, //!< 見えるようになった (生成・視界に入った・感知した)
    DISAPPEAR = 8, //!< 見えなくなった (死亡・視界から出た)
};

/*!
 * @brief イベントの当事者
 */
struct ViewEventActor {
    uint32_t id = 0; //!< 表示用の識別番号 (CreatureEntity::get_display_id())。見えない・いないなら 0
    bool is_player = false;
    Pos2D pos{ 0, 0 };
    bool has_pos = false; //!< 座標が分かるか
};

/*!
 * @brief 表示用のゲームイベント
 */
struct ViewEvent {
    ViewEventType type = ViewEventType::MELEE;
    ViewEventActor actor; //!< 起こした側 (攻撃者・発射元・ダメージを受けた/死んだクリーチャー)
    ViewEventActor target; //!< 受けた側 (近接攻撃の相手・project() の狙った先)
    bool hit = false; //!< MELEE: 命中したか
    int method = 0; //!< MELEE: モンスターの打撃の方法 (RaceBlowMethodType)。プレイヤーの攻撃は 0
    int value = 0; //!< DAMAGE: ダメージ量
    int attribute = 0; //!< PROJECTION: 属性 (AttributeType)
    int radius = 0; //!< PROJECTION: 効果範囲の半径
    bool is_beam = false; //!< PROJECTION: ビーム
    bool is_breath = false; //!< PROJECTION: ブレス
    std::vector<Pos2D> path; //!< PROJECTION: 軌跡のうちプレイヤーから見えるグリッド (発射元から順)
    std::vector<std::pair<int, Pos2D>> area; //!< PROJECTION: 効果範囲のうちプレイヤーから見えるグリッド (中心からの距離, 座標)
};

class ViewEventSink {
public:
    virtual ~ViewEventSink() = default;
    virtual void on_view_event(const ViewEvent &event) = 0;
};

/// 受け手を登録する (nullptr で解除)
void set_view_event_sink(ViewEventSink *sink);

/// 受け手が登録されているか (イベントを組み立てる前に呼ぶ)
bool is_view_event_sink_active();

void notify_view_event(const ViewEvent &event);

/// プレイヤーから見える当事者の情報を作る (見えないモンスターなら識別番号・座標を伏せる)
ViewEventActor make_view_event_actor(const CreatureEntity &creature);

/// 近接攻撃の 1 打撃を知らせる (どちらも見えなければ何もしない)
void notify_view_melee(const CreatureEntity &attacker, const CreatureEntity &defender, bool hit, int method = 0);

/// HP が減ったことを知らせる (見えないモンスターなら何もしない)
void notify_view_damage(const CreatureEntity &victim, int damage);

/// 死亡したことを知らせる (見えないモンスターなら何もしない)
void notify_view_death(const CreatureEntity &victim);
