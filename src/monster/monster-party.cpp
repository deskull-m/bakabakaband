/*!
 * @brief クリーチャーパーティ (集団) の AI 処理
 * @details パーティ所属は `MonsterProfile::party_leader_m_idx` が表す。同値を持つ
 *          個体が 1 つの集団で、リーダー自身は自分の m_idx を持つ。m_idx は
 *          フロア内で一意なので、これがそのままグループ識別子になる
 *          (同一パーティ定義の複数実体も区別できる)。
 *          生成は `alloc_creature_party()` (`monster-floor/party-generator.cpp`)。
 */

#include "monster/monster-party.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-status-setter.h"
#include "system/creature-entity.h"
#include "system/creature-party/creature-party-definition.h"
#include "system/creature-party/creature-party-list.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "term/z-rand.h"
#include "view/display-messages.h"
#include <string>
#include <tl/optional.hpp>
#include <vector>

namespace {
/*!
 * @brief 覚醒伝播が届く距離 (グリッド)
 * @details パーティは中心から半径 5 に散開して配置されるため、散開直径 10 を
 *          包含しつつ無関係な遠方まで広がらない値にしてある。見通しは要求しない
 *          (仲間の悲鳴や物音が壁越しに届く表現)。
 */
constexpr auto PARTY_ALERT_RADIUS = 12;

/*!
 * @brief リーダー撃破時に生存メンバーが受ける恐怖のターン数の基準値
 * @details `PARTY_MORALE_FEAR_BASE + randint1(PARTY_MORALE_FEAR_DICE)`。
 *          一時的に逃げ散るが立て直す程度の量。
 */
constexpr auto PARTY_MORALE_FEAR_BASE = 10;
constexpr auto PARTY_MORALE_FEAR_DICE = 20;

/*!
 * @brief 同一パーティの生存メンバーの m_idx を集める
 * @param floor フロアへの参照
 * @param leader_m_idx グループ識別子 (リーダーの m_idx)
 * @param exclude_m_idx 除外する m_idx (0 なら除外しない)
 */
std::vector<MONSTER_IDX> collect_party_members(const FloorType &floor, MONSTER_IDX leader_m_idx, MONSTER_IDX exclude_m_idx)
{
    std::vector<MONSTER_IDX> members;
    if (leader_m_idx <= 0) {
        return members;
    }

    for (auto m_idx = 1; m_idx < floor.m_max; m_idx++) {
        if (m_idx == exclude_m_idx) {
            continue;
        }

        const auto &monster = floor.get_monster(static_cast<MONSTER_IDX>(m_idx));
        if (!monster.is_valid() || (monster.get_party_leader_m_idx() != leader_m_idx)) {
            continue;
        }

        members.push_back(static_cast<MONSTER_IDX>(m_idx));
    }

    return members;
}

/*!
 * @brief パーティ名を取得する (定義が失われていれば nullopt)
 * @details `party_id` は JSON 由来で増減しうるため、旧セーブに残った ID を
 *          そのまま `get_party()` へ渡すと例外を踏む。
 */
tl::optional<std::string> describe_party(const CreatureEntity &monster)
{
    const auto party_id = monster.get_party_id();
    if (!CreaturePartyList::is_valid(party_id)) {
        return tl::nullopt;
    }

    // パーティ定義は JSON 由来で増減しうるため、旧セーブに残った ID が現在の
    // 定義に無い場合に get_party() の例外を踏まないよう存在も確認する
    // (ItemEntity::has_rightful_owner() と同じ方針)。
    const auto &party_list = CreaturePartyList::get_instance();
    if (!party_list.contains(party_id)) {
        return tl::nullopt;
    }

    return party_list.get_party(party_id).name.string();
}
}

/*!
 * @brief パーティから抜ける個体がリーダーだった場合、生存メンバーへリーダーを継承させる
 * @param floor フロアへの参照
 * @param leaving_m_idx 抜ける個体の m_idx
 * @details リーダーでなければ何もしない。生存メンバーが居なければ集団は消滅する。
 *          **呼出側は `leaving_m_idx` の個体をまだ無効化していないこと**
 *          (この関数が除外対象として参照する)。
 */
void succeed_party_leader(FloorType &floor, MONSTER_IDX leaving_m_idx)
{
    const auto &leaving = floor.get_monster(leaving_m_idx);
    if (!leaving.is_party_leader(leaving_m_idx)) {
        return;
    }

    const auto members = collect_party_members(floor, leaving_m_idx, leaving_m_idx);
    if (members.empty()) {
        return;
    }

    // 次のリーダーは生存メンバーのうち種族レベルが最も高い個体 (生成時と同じ基準)
    auto new_leader_m_idx = members.front();
    for (const auto m_idx : members) {
        if (floor.get_monster(m_idx).get_monrace().level > floor.get_monster(new_leader_m_idx).get_monrace().level) {
            new_leader_m_idx = m_idx;
        }
    }

    for (const auto m_idx : members) {
        floor.get_monster(m_idx).set_party_leader_m_idx(new_leader_m_idx);
    }
}

/*!
 * @brief 個体をパーティから離脱させる (リーダーなら継承してから抜ける)
 */
void leave_party_and_succeed_leader(FloorType &floor, CreatureEntity &monster)
{
    succeed_party_leader(floor, monster.get_self_m_idx());
    monster.leave_party();
}

/*!
 * @brief 目を覚ましたメンバーが同じパーティの仲間を起こす (覚醒伝播)
 * @param creature クリーチャーへの参照 (視点となるプレイヤー)
 * @param waker_m_idx 目を覚ました個体の m_idx
 * @details 1 体が気付けば一団として動き出すという集団の振舞い。起こす対象は
 *          `PARTY_ALERT_RADIUS` 内の同一パーティの個体に限る。
 *          **眠りの深さを削るのではなく即座に起こす**ので、隠密の意味は
 *          「最初の 1 体に気付かれないこと」に移る。
 */
void alert_party_members(CreatureEntity &creature, MONSTER_IDX waker_m_idx)
{
    auto &floor = *creature.get_floor();
    const auto &waker = floor.get_monster(waker_m_idx);
    if (!waker.belongs_to_party()) {
        return;
    }

    const auto waker_pos = waker.get_position();
    auto awakened_count = 0;
    auto is_any_visible = false;
    for (const auto m_idx : collect_party_members(floor, waker.get_party_leader_m_idx(), waker_m_idx)) {
        auto &member = floor.get_monster(m_idx);
        if (!member.is_asleep()) {
            continue;
        }

        if (Grid::calc_distance(waker_pos, member.get_position()) > PARTY_ALERT_RADIUS) {
            continue;
        }

        if (!set_monster_csleep(floor, m_idx, 0)) {
            continue;
        }

        awakened_count++;
        is_any_visible |= member.is_visible_on_map();
    }

    if (awakened_count == 0) {
        return;
    }

    // 起きた仲間を 1 体でも視認できていれば一団が動き出したことを伝える
    // (個別の「目を覚ました」は set_monster_csleep() 側では出ないため、
    //  ここでまとめて 1 行だけ出す)
    const auto party_name = describe_party(waker);
    if (!is_any_visible || !party_name) {
        return;
    }

    msg_print(_("{}が気付いて動き出した！", "The {} stir!"), *party_name);
}

/*!
 * @brief パーティのリーダーが倒れたときに生存メンバーの士気を崩壊させる
 * @param creature クリーチャーへの参照 (視点となるプレイヤー)
 * @param dead_m_idx 倒れた個体の m_idx
 * @details リーダー以外が倒れた場合は何もしない。恐怖耐性 (`NO_FEAR` /
 *          種族恐怖耐性 / FEARLESS 変異) を持つ個体は動じない。
 *          併せて次のリーダーを立てるため、集団は壊滅せず立て直す。
 */
void collapse_party_morale(CreatureEntity &creature, MONSTER_IDX dead_m_idx)
{
    auto &floor = *creature.get_floor();
    const auto &dead = floor.get_monster(dead_m_idx);
    if (!dead.belongs_to_party() || !dead.is_party_leader(dead_m_idx)) {
        return;
    }

    const auto members = collect_party_members(floor, dead_m_idx, dead_m_idx);
    if (members.empty()) {
        return;
    }

    const auto party_name = describe_party(dead);
    auto feared_count = 0;
    auto is_any_visible = false;
    for (const auto m_idx : members) {
        auto &member = floor.get_monster(m_idx);
        if (member.has_resist_fear()) {
            continue;
        }

        const auto fear = member.get_timed_effect(CreatureTimedEffect::FEAR) + PARTY_MORALE_FEAR_BASE + randint1(PARTY_MORALE_FEAR_DICE);
        if (!set_monster_monfear(floor, m_idx, fear)) {
            continue;
        }

        feared_count++;
        is_any_visible |= member.is_visible_on_map();
    }

    // リーダーを継承させてから抜ける (dead は呼出時点でまだ有効)
    succeed_party_leader(floor, dead_m_idx);

    if ((feared_count == 0) || !is_any_visible || !party_name) {
        return;
    }

    msg_print(_("頭を失った{}は怯えて散り散りになった！", "The leaderless {} scatters in terror!"), *party_name);
}
