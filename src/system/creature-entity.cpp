#include "system/creature-entity.h"
#include "floor/geometry.h"
#include "inventory/inventory-slot-types.h"
#include "player-ability/player-ability-types.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "term/z-form.h"
#include "term/z-rand.h"
#include "timed-effect/timed-effects.h"
#include <range/v3/algorithm.hpp>

bool CreatureEntity::is_time_limit_esp() const
{
    return this->tim_esp > 0;
}

bool CreatureEntity::is_time_limit_stealth() const
{
    return this->tim_stealth > 0;
}

/*!
 * @brief 指定した固定アーティファクトを装備しているかどうか調べる
 *
 * @param fa_id 固定アーティファクトのID
 * @return 装備していればtrue、そうでなければfalse
 */
bool CreatureEntity::is_wielding(FixedArtifactId fa_id) const
{
    return ranges::any_of(INVEN_WIELDING_SLOTS,
        [&](auto slot) {
            const auto &item = this->inventory[slot];
            return item->is_valid() && item->is_specific_artifact(fa_id);
        });
}

/*!
 * @brief 時限効果管理オブジェクトを取得
 * @return 時限効果管理オブジェクトへの共有ポインタ
 */
std::shared_ptr<TimedEffects> CreatureEntity::effects() const
{
    return this->timed_effects;
}

/*!
 * @brief 現在地の隣 (瞬時値)または現在地を返す
 * @param dir 隣を表す方向番号
 * @details クリーチャーが移動する前後の文脈で使用すると不整合を起こすので注意
 * 方向番号による位置取りは以下の通り. 0と5は現在地.
 * 789 ...
 * 456 .@.
 * 123 ...
 */
Pos2D CreatureEntity::get_neighbor(int dir) const
{
    return this->get_position() + Direction(dir).vec();
}

/*!
 * @brief 現在地の隣 (瞬時値)または現在地を返す
 * @param dir 隣を表す方向
 * @attention クリーチャーが移動する前後の文脈で使用すると不整合を起こすので注意
 */
Pos2D CreatureEntity::get_neighbor(const Direction &dir) const
{
    return this->get_position() + dir.vec();
}

/*!
 * @brief クリーチャーの攻撃目標座標を設定
 * @param pos 目標座標
 */
void CreatureEntity::set_target(const Pos2D &pos)
{
    this->target = pos;
}

/*!
 * @brief クリーチャーの攻撃目標座標をリセット
 */
void CreatureEntity::reset_target()
{
    this->target = {};
}

/*!
 * @brief クリーチャーの攻撃目標座標を取得
 * @return 目標座標
 */
Pos2D CreatureEntity::get_target_position() const
{
    return this->target;
}

bool CreatureEntity::is_stunned() const
{
    return this->get_timed_effect(CreatureTimedEffect::STUN) > 0;
}

bool CreatureEntity::is_confused() const
{
    return this->get_timed_effect(CreatureTimedEffect::CONFUSION) > 0;
}

bool CreatureEntity::is_fearful() const
{
    return this->get_timed_effect(CreatureTimedEffect::FEAR) > 0;
}

bool CreatureEntity::is_invulnerable() const
{
    return this->get_timed_effect(CreatureTimedEffect::INVULNERABILITY) > 0;
}

bool CreatureEntity::is_fast() const
{
    return this->get_timed_effect(CreatureTimedEffect::ACCELERATION) > 0;
}

bool CreatureEntity::is_decelerated() const
{
    return this->get_timed_effect(CreatureTimedEffect::DECELERATION) > 0;
}

/*!
 * @brief ツリー構造インシデント数加算
 * @param incident_id 階層構造のインシデントID（例: "root/attack/critical"）
 * @param num 加算量
 */
void CreatureEntity::plus_incident_tree(const std::string &incident_id, int num)
{
    if (this->incident_tree.count(incident_id) == 0) {
        this->incident_tree[incident_id] = 0;
    }
    this->incident_tree[incident_id] += num;
}

std::string CreatureEntity::decrease_ability_random()
{
    constexpr std::array<std::pair<int, std::string_view>, 6> candidates = { {
        { A_STR, _("強く", "strong") },
        { A_INT, _("聡明で", "bright") },
        { A_WIS, _("賢明で", "wise") },
        { A_DEX, _("器用で", "agile") },
        { A_CON, _("健康で", "hale") },
        { A_CHR, _("美しく", "beautiful") },
    } };

    const auto &[k, act] = rand_choice(candidates);
    this->stat_cur[k] = (this->stat_cur[k] * 3) / 4;
    if (this->stat_cur[k] < 30) {
        this->stat_cur[k] = 30;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    return format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), act.data());
}

std::string CreatureEntity::decrease_ability_all()
{
    for (auto i = 0; i < A_MAX; i++) {
        this->stat_cur[i] = (this->stat_cur[i] * 7) / 8;
        if (this->stat_cur[i] < 30) {
            this->stat_cur[i] = 30;
        }
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    return _("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be...");
}

int CreatureEntity::calc_life_rating() const
{
    const auto actual_hp = this->player_hp[PY_MAX_LEVEL - 1];

    // ダイスによる上昇回数は52回（初期3回+LV50までの49回）なので
    // 期待値計算のため2で割っても端数は出ない
    constexpr auto roll_num = 3 + PY_MAX_LEVEL - 1;
    const auto expected_hp = this->hit_dice.maxroll() + this->hit_dice.floored_expected_value_multiplied_by(roll_num);

    return actual_hp * 100 / expected_hp;
}
