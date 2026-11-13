/*!
 * @brief モンスターの死亡時詠唱 (Issue #6768)
 * @details 「死に際に最後の一手として魔法・特技を放つ」処理。種族定義の
 * `death_spells` (JSON キー) に並んだ特技を、リスト順に独立して確率抽選し、
 * **既存の mspell 経路 (`monspell_to_player()`) をそのまま使って**撃たせる。
 *
 * 通常の詠唱 (`make_attack_spell()`) と違い、頻度 (`freq_spell`) も特技の選定も
 * 行わない。撃つ特技と確率はデータが名指しするためである。一方で「どんな状況でも
 * 撃てる」わけではなく、通常の詠唱で課されている以下のゲートは揃えてある。
 */

#include "mspell/mspell-dying.h"
#include "dungeon/dungeon-flag-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-behavior-flags.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/mspell-result.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "target/projection-path-calculator.h"
#include "term/z-rand.h"

namespace {

/*!
 * @brief NO_MAGIC ダンジョンで封じられる特技かどうかを返す
 * @details `check_mspell_stupid()` と同じ判定。NO_MAGIC の地下では非 STUPID の
 *          個体は `RF_ABILITY_NOMAGIC_MASK` (ブレス・SHRIEK・ROCKET・SHOOT・
 *          SPECIAL = 魔法でない特技) 以外を使えない。
 */
bool is_sealed_in_no_magic_dungeon(const FloorType &floor, const MonraceDefinition &monrace, MonsterAbilityType ability)
{
    if (monrace.behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return false;
    }

    auto is_in_no_magic_dungeon = floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MAGIC);
    is_in_no_magic_dungeon &= floor.is_underground();
    is_in_no_magic_dungeon &= !floor.is_in_quest() || QuestType::is_fixed(floor.quest_number);
    if (!is_in_no_magic_dungeon) {
        return false;
    }

    return RF_ABILITY_NOMAGIC_MASK.has_not(ability);
}

/*!
 * @brief 闘技場・モンスター闘技場で封じられる特技かどうかを返す
 * @details `check_mspell_arena()` と同じ判定。召喚で闘技場を埋められると困るため。
 */
bool is_sealed_in_arena(const FloorType &floor, MonsterAbilityType ability)
{
    if (!floor.inside_arena && !AngbandSystem::get_instance().is_phase_out()) {
        return false;
    }

    return RF_ABILITY_SUMMON_MASK.has(ability) || (ability == MonsterAbilityType::TELE_LEVEL);
}

}

/*!
 * @brief 死に際の詠唱を行う (Issue #6768)
 * @param creature プレイヤーへの参照 (詠唱の対象)
 * @param m_idx 死亡したモンスターのID
 * @details 呼出時点でモンスターの実体がまだ有効で、自身のグリッドに居ることが前提
 *          (`monster_death()` の前半で呼ぶ)。対象は常にプレイヤーで、
 *          **プレイヤーへ撃てる状況でなければ何も唱えない** (下記のゲート)。
 *          よってペットや、プレイヤーと無関係にモンスター同士の戦闘で倒れた個体は
 *          発動しない。
 */
void cast_dying_spells(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    auto &floor = *creature.get_floor();
    auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();

    // 指定の無い種族 (大多数) はここで抜ける。死亡処理に負荷を掛けない。
    if (monrace.death_spells.empty()) {
        return;
    }

    // **MonsterTemporaryFlagType::PREVENT_MAGIC は意図的に見ない。**
    // あれは「出現した直後の手番に即魔法を使わせない」出生時ガード
    // (`one-monster-placer.cpp` で全個体に立ち、プレイヤーの次の手番で落ちる) で、
    // 「反魔法状態」ではない。死亡時詠唱はモンスターの手番の行動ではなく
    // 撃破への反応なので、湧いた直後に倒された個体でも断末魔は上げる。
    // (これを見ていた当初の実装では、召喚した個体をその手番に倒すと発動せず、
    //  実機検証が空振りした)

    // 混乱していると通常の詠唱もできないので、死に際でもできない。
    if (monster.is_confused()) {
        return;
    }

    // 対象はプレイヤーなので、プレイヤーへ敵対していなければ撃たない (ペット等)。
    if (!monster.is_hostile()) {
        return;
    }

    // 通常の詠唱と同じく、射程外・射線なしでは撃たない。
    const auto pos_monster = monster.get_position();
    const auto pos_player = creature.get_position();
    if (Grid::calc_distance(pos_player, pos_monster) > AngbandSystem::get_instance().get_max_range()) {
        return;
    }

    if (!projectable(floor, pos_monster, pos_player)) {
        return;
    }

    for (const auto &death_spell : monrace.death_spells) {
        if (is_sealed_in_no_magic_dungeon(floor, monrace, death_spell.ability)) {
            continue;
        }

        if (is_sealed_in_arena(floor, death_spell.ability)) {
            continue;
        }

        if (!evaluate_percent(death_spell.probability)) {
            continue;
        }

        // 思い出フラグ (r_ability_flags) は立てない。death_spells は通常の
        // ability_flags とは独立のリストなので、立てると「普段も唱える特技」として
        // 思い出に出て実態と食い違う (死亡時詠唱の思い出は種族定義から直接出す)。
        (void)monspell_to_player(creature, death_spell.ability, pos_player.y, pos_player.x, m_idx);
    }
}
