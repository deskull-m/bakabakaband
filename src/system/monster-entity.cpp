#include "system/monster-entity.h"
#include "alliance/alliance.h"
#include "core/speed-table.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-status.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/race-info-table.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/lore-tracker.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
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

bool MonsterEntity::is_valid() const
{
    return MonraceList::is_valid(this->r_idx);
}

bool MonsterEntity::is_dead() const
{
    return this->hp < 0;
}

short MonsterEntity::get_timed_effect(CreatureTimedEffect effect) const
{
    switch (effect) {
    case CreatureTimedEffect::STUN:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::STUN);
    case CreatureTimedEffect::CONFUSION:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::CONFUSION);
    case CreatureTimedEffect::FEAR:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::FEAR);
    case CreatureTimedEffect::INVULNERABILITY:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::INVULNERABILITY);
    case CreatureTimedEffect::ACCELERATION:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::FAST);
    case CreatureTimedEffect::DECELERATION:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::SLOW);
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
        return this->get_monster_profile().mtimed.at(MonsterTimedEffect::SLEEP);
    default:
        return 0;
    }
}

void MonsterEntity::set_timed_effect(CreatureTimedEffect effect, short value)
{
    switch (effect) {
    case CreatureTimedEffect::STUN:
        this->get_monster_profile().mtimed[MonsterTimedEffect::STUN] = value;
        break;
    case CreatureTimedEffect::CONFUSION:
        this->get_monster_profile().mtimed[MonsterTimedEffect::CONFUSION] = value;
        break;
    case CreatureTimedEffect::FEAR:
        this->get_monster_profile().mtimed[MonsterTimedEffect::FEAR] = value;
        break;
    case CreatureTimedEffect::INVULNERABILITY:
        this->get_monster_profile().mtimed[MonsterTimedEffect::INVULNERABILITY] = value;
        break;
    case CreatureTimedEffect::ACCELERATION:
        this->get_monster_profile().mtimed[MonsterTimedEffect::FAST] = value;
        break;
    case CreatureTimedEffect::DECELERATION:
        this->get_monster_profile().mtimed[MonsterTimedEffect::SLOW] = value;
        break;
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
        this->get_monster_profile().mtimed[MonsterTimedEffect::SLEEP] = value;
        break;
    default:
        break;
    }
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

/*
 * 捕獲・死亡の際にカメレオンの変身を元に戻す
 */
void MonsterEntity::reset_chameleon_polymorph()
{
    auto real_monrace_id = this->get_real_monrace_id();
    this->r_idx = real_monrace_id;
    this->ap_r_idx = real_monrace_id;
}

std::string MonsterEntity::get_pronoun_of_summoned_kin() const
{
    return this->get_monrace().get_pronoun_of_summoned_kin();
}

void MonsterEntity::make_lore_treasure(int num_item, int num_gold) const
{
    auto &monrace = this->get_monrace();
    if (!this->is_original_ap()) {
        return;
    }

    monrace.make_lore_treasure(num_item, num_gold);
    if (LoreTracker::get_instance().is_tracking(this->r_idx)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
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

/*!
 * @brief モンスターを友好的にする
 */
void MonsterEntity::set_friendly()
{
    this->get_monster_profile().mflag2.set(MonsterConstantFlagType::FRIENDLY);
}

/*!
 * @brief モンスターのフラグに基づいて対応するプレイヤー種族IDを初期化する
 * @details モンスターのMonsterKindTypeフラグをチェックし、
 * プレイヤー種族と同等の名前のフラグがあればequivalent_player_racesに追加する
 */
void MonsterEntity::initialize_equivalent_player_races()
{
    this->get_monster_profile().equivalent_player_races.clear();
    const auto &monrace = this->get_monrace();

    // モンスターのフラグとプレイヤー種族の対応関係をチェック
    if (monrace.kind_flags.has(MonsterKindType::HUMAN)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HUMAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::ELF)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::ELF);
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HALF_ELF);
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HIGH_ELF);
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::DARK_ELF);
    }
    if (monrace.kind_flags.has(MonsterKindType::DWARF)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::DWARF);
    }
    if (monrace.kind_flags.has(MonsterKindType::HOBBIT)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HOBBIT);
    }
    if (monrace.kind_flags.has(MonsterKindType::GNOME)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::GNOME);
    }
    if (monrace.kind_flags.has(MonsterKindType::ORC)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HALF_ORC);
    }
    if (monrace.kind_flags.has(MonsterKindType::TROLL)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HALF_TROLL);
    }
    if (monrace.kind_flags.has(MonsterKindType::GIANT)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HALF_GIANT);
    }
    if (monrace.kind_flags.has(MonsterKindType::OGRE)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::HALF_OGRE);
    }
    if (monrace.kind_flags.has(MonsterKindType::AMBERITE)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::AMBERITE);
    }
    if (monrace.kind_flags.has(MonsterKindType::YEEK)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::YEEK);
    }
    if (monrace.kind_flags.has(MonsterKindType::KOBOLD)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::KOBOLD);
    }
    if (monrace.kind_flags.has(MonsterKindType::NIBELUNG)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::NIBELUNG);
    }
    if (monrace.kind_flags.has(MonsterKindType::DRAGON)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::DRACONIAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::MINDFLAYER)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::MIND_FLAYER);
    }
    if (monrace.kind_flags.has(MonsterKindType::DEMON)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::IMP);
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::BALROG);
    }
    if (monrace.kind_flags.has(MonsterKindType::GOLEM)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::GOLEM);
    }
    if (monrace.kind_flags.has(MonsterKindType::SKELETON)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::SKELETON);
    }
    if (monrace.kind_flags.has(MonsterKindType::ZOMBIE)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::ZOMBIE);
    }
    if (monrace.kind_flags.has(MonsterKindType::VAMPIRE)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::VAMPIRE);
    }
    if (monrace.kind_flags.has(MonsterKindType::UNDEAD)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::SPECTRE);
    }
    if (monrace.kind_flags.has(MonsterKindType::FAIRY)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::SPRITE);
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::S_FAIRY);
    }
    if (monrace.kind_flags.has(MonsterKindType::BEAST)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::BEASTMAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::TREEFOLK)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::ENT);
    }
    if (monrace.kind_flags.has(MonsterKindType::ANGEL)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::ARCHON);
    }
    if (monrace.kind_flags.has(MonsterKindType::ROBOT)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::ANDROID);
    }
    if (monrace.kind_flags.has(MonsterKindType::MERFOLK)) {
        this->get_monster_profile().equivalent_player_races.push_back(PlayerRaceType::MERFOLK);
    }

    // equivalent_player_racesの最初の種族をraceポインタに設定
    if (!this->get_monster_profile().equivalent_player_races.empty()) {
        this->race = &race_info[enum2i(this->get_monster_profile().equivalent_player_races[0])];
        this->prace = this->get_monster_profile().equivalent_player_races[0];
    } else {
        this->race = nullptr;
        this->prace = PlayerRaceType::HUMAN; // デフォルト
    }
}

/*!
 * @brief モンスターのフラグに基づいて対応するプレイヤー職業IDを初期化する
 * @details モンスターのMonsterKindTypeフラグをチェックし、
 * プレイヤー職業と同等の名前のフラグがあればequivalent_player_classesに追加する
 */
void MonsterEntity::initialize_equivalent_player_classes()
{
    this->get_monster_profile().equivalent_player_classes.clear();
    const auto &monrace = this->get_monrace();

    // モンスターのフラグとプレイヤー職業の対応関係をチェック
    if (monrace.kind_flags.has(MonsterKindType::WARRIOR)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::WARRIOR);
    }
    if (monrace.kind_flags.has(MonsterKindType::MAGE)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::MAGE);
    }
    if (monrace.kind_flags.has(MonsterKindType::PRIEST)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::PRIEST);
    }
    if (monrace.kind_flags.has(MonsterKindType::ROGUE)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::ROGUE);
    }
    if (monrace.kind_flags.has(MonsterKindType::RANGER)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::RANGER);
    }
    if (monrace.kind_flags.has(MonsterKindType::PALADIN)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::PALADIN);
    }
    if (monrace.kind_flags.has(MonsterKindType::SAMURAI)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::SAMURAI);
    }
    if (monrace.kind_flags.has(MonsterKindType::NINJA)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::NINJA);
    }
    if (monrace.kind_flags.has(MonsterKindType::MINDCRAFTER)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::MINDCRAFTER);
    }
    if (monrace.kind_flags.has(MonsterKindType::ARCHER)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::ARCHER);
    }
    if (monrace.kind_flags.has(MonsterKindType::BARD)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::BARD);
    }
    if (monrace.kind_flags.has(MonsterKindType::SMITH)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::SMITH);
    }
    if (monrace.kind_flags.has(MonsterKindType::KARATEKA)) {
        this->get_monster_profile().equivalent_player_classes.push_back(PlayerClassType::MONK);
    }

    // equivalent_player_classesの最初の職業をpclass_refポインタに設定
    if (!this->get_monster_profile().equivalent_player_classes.empty()) {
        this->pclass_ref = &class_info.at(this->get_monster_profile().equivalent_player_classes[0]);
        this->pclass = this->get_monster_profile().equivalent_player_classes[0];
    } else {
        this->pclass_ref = nullptr;
        this->pclass = PlayerClassType::WARRIOR; // デフォルト
    }
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

std::string MonsterEntity::build_looking_description(bool needs_attitude) const
{
    const auto description = this->build_damage_description();
    const auto attitude = needs_attitude ? this->build_attitude_description() : "";
    const std::string clone(this->get_monster_profile().mflag2.has(MonsterConstantFlagType::CLONED) ? ", clone" : "");
    const auto &apparent_monrace = this->get_appearance_monrace();

    // ペットの場合はアライアンス表示を省略
    const bool show_alliance = !this->is_pet() && this->get_monster_profile().alliance_idx != AllianceType::NONE;
    const std::string alliance_part = show_alliance ? format("(%s)", alliance_list.at(this->get_monster_profile().alliance_idx)->name.data()) : "";

    if ((apparent_monrace.r_tkills > 0) && this->get_monster_profile().mflag2.has_not(MonsterConstantFlagType::KAGE)) {
        constexpr auto fmt = _("レベル%d, %s%s%s%s", "Level %d, %s%s%s%s");
        return format(fmt, apparent_monrace.level, description.data(), attitude.data(), clone.data(), alliance_part.data());
    }

    constexpr auto fmt = _("レベル???, %s%s%s%s", "Level ???, %s%s%s%s");
    return format(fmt, description.data(), attitude.data(), clone.data(), alliance_part.data());
}

std::string MonsterEntity::build_damage_description() const
{
    const auto is_living = this->has_living_flag(true);
    const auto damage_ratio = this->maxhp > 0 ? 100L * this->hp / this->maxhp : 0;
    if (!this->get_monster_profile().ml) {
        return _("損傷具合不明", "damage unknown");
    }

    if (this->hp >= this->maxhp) {
        return is_living ? _("無傷", "unhurt") : _("無ダメージ", "undamaged");
    }

    if (damage_ratio >= 60) {
        return is_living ? _("軽傷", "somewhat wounded") : _("小ダメージ", "somewhat damaged");
    }

    if (damage_ratio >= 25) {
        return is_living ? _("負傷", "wounded") : _("中ダメージ", "damaged");
    }

    if (damage_ratio >= 10) {
        return is_living ? _("重傷", "badly wounded") : _("大ダメージ", "badly damaged");
    }

    return is_living ? _("半死半生", "almost dead") : _("倒れかけ", "almost destroyed");
}

std::string MonsterEntity::build_attitude_description() const
{
    if (this->is_pet()) {
        return _(", ペット", ", pet");
    }

    if (this->is_friendly()) {
        return _(", 友好的", ", friendly");
    }

    return "";
}

int MonsterEntity::get_ac() const
{
    if (this->get_monster_profile().mflag2.has(MonsterConstantFlagType::NAKED)) {
        return 0;
    }
    return this->ac;
}

int MonsterEntity::get_level() const
{
    // 個体レベルが設定されていればそれを使用、未設定なら種族レベルを使用
    if (this->level > 0) {
        return this->level;
    }
    return this->get_monrace().level / 2;
}

bool MonsterEntity::is_player() const
{
    return false;
}

void MonsterEntity::on_take_hit(int damage)
{
    this->dealt_damage += damage;
    if (this->dealt_damage > this->max_maxhp * 100) {
        this->dealt_damage = this->max_maxhp * 100;
    }
}

void MonsterEntity::on_death([[maybe_unused]] std::string_view cause)
{
    // モンスター死亡時の経験値・徳処理は MonsterDamageProcessor::process_dead_exp_virtue() で行う
}
