#include "system/monster-entity.h"
#include "alliance/alliance.h"
#include "core/speed-table.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-status.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include <algorithm>

MonsterEntity::MonsterEntity()
{
    this->monster_profile.emplace();
    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->get_monster_profile().mtimed[mte] = 0;
    }

    // CreatureEntityの基本メンバーを初期化
    this->r_idx = MonraceId::PLAYER; // デフォルトはプレイヤー（無効な状態）
    this->ap_r_idx = MonraceId::PLAYER;
    this->patron = 0; // パトロンなし
}

void MonsterEntity::wipe()
{
    *this = {};
    this->monster_profile.emplace();
    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->get_monster_profile().mtimed[mte] = 0;
    }
}

MonsterEntity MonsterEntity::clone() const
{
    return *this;
}

/*!
 * @brief モンスターの属性とアライアンスに基づいた敵対関係の有無を返す
 * @param other 比較対象モンスターへの参照
 * @return 敵対関係にあるか否か
 */
bool MonsterEntity::is_hostile_to_melee(const CreatureEntity &other) const
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return !this->is_pet() && !other.is_pet();
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto is_m1_wild = monrace1.wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
    const auto is_m2_wild = monrace2.wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
    if (is_m1_wild && is_m2_wild) {
        if (!this->is_pet() && !other.is_pet()) {
            return false;
        }
    }

    if (this->get_monster_profile().alliance_idx != other.get_monster_profile().alliance_idx) {
        return true;
    } else if (this->is_hostile_align(other.get_monster_profile().sub_align)) {
        if (this->get_monster_profile().mflag2.has_not(MonsterConstantFlagType::CHAMELEON) || other.get_monster_profile().mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
            return true;
        }
    }

    return this->is_hostile() != other.is_hostile();
}

/*!
 * @brief モンスターの属性に基づいた敵対関係の有無を返す
 * @param other 比較対象のサブフラグ
 * @return 敵対関係にあるか否か
 */
bool MonsterEntity::is_hostile_align(const byte other_sub_align) const
{
    return CreatureEntity::check_sub_alignments(this->get_monster_profile().sub_align, other_sub_align);
}

/*!
 * @brief モンスターがアイテム類に擬態しているかどうかを返す
 * @param m_ptr モンスターの参照ポインタ
 * @return モンスターがアイテム類に擬態しているならTRUE、そうでなければFALSE
 * @details
 * ユニークミミックは常時擬態状態
 * 一般モンスターもビハインダーだけ特別扱い
 * その他増やしたい時はis_special_mimic に「|=」で追加すること
 *
 */
bool MonsterEntity::is_mimicry() const
{
    auto is_special_mimic = this->ap_r_idx == MonraceId::BEHINDER;
    if (is_special_mimic) {
        return true;
    }

    const auto &monrace = this->get_appearance_monrace();
    if (!monrace.symbol_char_is_any_of(R"(/|\()[]="$,.!?&`#%<>+~)")) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        return true;
    }

    return monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || this->is_asleep();
}

bool MonsterEntity::is_dead() const
{
    return this->hp < 0;
}

/*!
 * @brief モンスターが生命体かどうかを返す
 * @param is_apperance たぬき、カメレオン、各種誤認ならtrue
 * @return 生命体ならばtrue
 * @todo kind_flags をMonsterEntityへコピーする (将来的なモンスター仕様の拡張)
 */
tl::optional<bool> MonsterEntity::order_pet_whistle(const CreatureEntity &other) const
{
    const auto is_ordered_name = this->order_pet_named(other);
    if (is_ordered_name) {
        return *is_ordered_name;
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto is_ordered_race = monrace1.order_pet(monrace2);
    if (is_ordered_race) {
        return *is_ordered_race;
    }

    return this->order_pet_hp(other);
}

tl::optional<bool> MonsterEntity::order_pet_dismission(const CreatureEntity &other) const
{
    const auto is_ordered_name = this->order_pet_named(other);
    if (is_ordered_name) {
        return *is_ordered_name;
    }

    if (!this->has_parent() && other.has_parent()) {
        return true;
    }

    if (this->has_parent() && !other.has_parent()) {
        return false;
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto is_ordered_race = monrace1.order_pet(monrace2);
    if (is_ordered_race) {
        return *is_ordered_race;
    }

    return this->order_pet_hp(other);
}

/*!
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param force_fixed_speed 速度を固定にする(個体差を適用しない)か否か
 */
void MonsterEntity::set_individual_speed(bool force_fixed_speed)
{
    const auto &monrace = this->get_monrace();
    auto speed = monrace.speed;
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !force_fixed_speed) {
        /* Allow some small variation per monster */
        int i = speed_to_energy(monrace.speed) / (one_in_(4) ? 3 : 10);
        if (i) {
            speed += static_cast<uint8_t>(rand_spread(0, i));
        }
    }

    if (speed > STANDARD_SPEED + 99) {
        speed = STANDARD_SPEED + 99;
    }

    this->speed = speed;
}

/*!
 * @brief モンスターを敵に回す
 */
void MonsterEntity::set_hostile()
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    this->get_monster_profile().mflag2.reset({ MonsterConstantFlagType::PET, MonsterConstantFlagType::FRIENDLY });

    if (this->get_monster_profile().alliance_idx != AllianceType::NONE) {
        for (auto &monster : this->current_floor_ptr->m_list) {
            if (monster.get_monster_profile().alliance_idx == this->get_monster_profile().alliance_idx) {
                monster.get_monster_profile().mflag2.reset({ MonsterConstantFlagType::PET, MonsterConstantFlagType::FRIENDLY });
            }
        }
    }
}

tl::optional<bool> MonsterEntity::order_pet_named(const CreatureEntity &other) const
{
    if (this->is_named() && !other.is_named()) {
        return true;
    }

    if (!this->is_named() && other.is_named()) {
        return false;
    }

    return tl::nullopt;
}

tl::optional<bool> MonsterEntity::order_pet_hp(const CreatureEntity &other) const
{
    if (this->hp > other.hp) {
        return true;
    }

    if (this->hp < other.hp) {
        return false;
    }

    return tl::nullopt;
}

bool MonsterEntity::can_ring_boss_call_nazgul() const
{
    auto is_boss = this->r_idx == MonraceId::MORGOTH;
    is_boss |= this->r_idx == MonraceId::SAURON;
    is_boss |= this->r_idx == MonraceId::ANGMAR;
    const auto &nazgul = MonraceList::get_instance().get_monrace(MonraceId::NAZGUL);
    const auto is_nazgul_alive = (nazgul.cur_num + 2) < nazgul.max_num;
    return is_boss && is_nazgul_alive;
}

int MonsterEntity::get_level() const
{
    // 個体レベルが設定されていればそれを使用、未設定なら種族レベルを使用
    if (this->level > 0) {
        return this->level;
    }
    return this->get_monrace().level / 2;
}
