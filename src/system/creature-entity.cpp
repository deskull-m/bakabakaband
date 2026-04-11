#include "system/creature-entity.h"
#include "core/speed-table.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-slot-types.h"
#include "market/arena-entry.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-pain-describer.h"
#include "monster/monster-util.h"
#include "player-ability/player-ability-types.h"
#include "player-info/bard-data-type.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player-info/sniper-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "player/race-info-table.h"
#include "realm/realm-song-numbers.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-profile.h"
#include "system/redrawing-flags-updater.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "term/z-rand.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <range/v3/algorithm.hpp>

bool CreatureEntity::try_set_position(const Pos2D &pos)
{
    if (this->current_floor_ptr->get_grid(pos).has_monster()) {
        return false;
    }

    this->y = pos.y;
    this->x = pos.x;
    return true;
}

bool CreatureEntity::is_fully_healthy() const
{
    auto effects = this->effects();
    auto is_healthy = this->hp == this->maxhp;
    is_healthy &= this->csp >= this->msp;
    is_healthy &= !effects->blindness().is_blind();
    is_healthy &= !effects->confusion().is_confused();
    is_healthy &= !effects->poison().is_poisoned();
    is_healthy &= !effects->fear().is_fearful();
    is_healthy &= !effects->stun().is_stunned();
    is_healthy &= !effects->cut().is_cut();
    is_healthy &= !effects->deceleration().is_slow();
    is_healthy &= !effects->paralysis().is_paralyzed();
    is_healthy &= !effects->hallucination().is_hallucinated();
    is_healthy &= !this->word_recall;
    is_healthy &= !this->alter_reality;
    return is_healthy;
}

bool CreatureEntity::is_true_winner() const
{
    const auto &world = AngbandWorld::get_instance();
    const auto &entries = ArenaEntryList::get_instance();
    return (world.total_winner > 0) && (entries.is_player_true_victor());
}

bool CreatureEntity::is_located_at_running_destination() const
{
    return (this->y == this->run_py) && (this->x == this->run_px);
}

bool CreatureEntity::try_resist_eldritch_horror() const
{
    return evaluate_percent(this->skill_sav) || one_in_(2);
}

void CreatureEntity::ride_monster(MONSTER_IDX m_idx)
{
    if (is_monster(this->riding)) {
        this->current_floor_ptr->get_monster(this->riding).get_monster_profile().mflag2.reset(MonsterConstantFlagType::RIDING);
    }

    this->riding = m_idx;

    if (is_monster(m_idx)) {
        this->current_floor_ptr->get_monster(m_idx).get_monster_profile().mflag2.set(MonsterConstantFlagType::RIDING);
    }
}

bool CreatureEntity::check_sub_alignments(const byte sub_align1, const byte sub_align2)
{
    if (sub_align1 == sub_align2) {
        return false;
    }

    auto this_evil = any_bits(sub_align1, SUB_ALIGN_EVIL);
    this_evil &= any_bits(sub_align2, SUB_ALIGN_GOOD);
    if (this_evil) {
        return true;
    }

    auto this_good = any_bits(sub_align1, SUB_ALIGN_GOOD);
    this_good &= any_bits(sub_align2, SUB_ALIGN_EVIL);
    return this_good;
}

MonraceDefinition &CreatureEntity::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->r_idx);
}

MonraceDefinition &CreatureEntity::get_appearance_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->ap_r_idx);
}

MonraceId CreatureEntity::get_real_monrace_id() const
{
    if (!this->has_monster_profile() || this->get_monster_profile().mflag2.has_not(MonsterConstantFlagType::CHAMELEON)) {
        return this->r_idx;
    }

    return this->get_monrace().kind_flags.has(MonsterKindType::UNIQUE) ? MonraceId::CHAMELEON_K : MonraceId::CHAMELEON;
}

MonraceDefinition &CreatureEntity::get_real_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->get_real_monrace_id());
}

bool CreatureEntity::has_living_flag(bool is_appearance) const
{
    const auto &monrace = is_appearance ? this->get_appearance_monrace() : this->get_monrace();
    return monrace.has_living_flag();
}

bool CreatureEntity::has_demon_flag(bool is_appearance) const
{
    const auto &monrace = is_appearance ? this->get_appearance_monrace() : this->get_monrace();
    return monrace.has_demon_flag();
}

bool CreatureEntity::has_undead_flag(bool is_appearance) const
{
    const auto &monrace = is_appearance ? this->get_appearance_monrace() : this->get_monrace();
    return monrace.has_undead_flag();
}

bool CreatureEntity::is_explodable() const
{
    return this->get_monrace().is_explodable();
}

std::string CreatureEntity::get_died_message() const
{
    return this->get_monrace().get_died_message();
}

bool CreatureEntity::is_male() const
{
    return this->get_monrace().is_male();
}

bool CreatureEntity::is_female() const
{
    const auto &monrace = this->get_monrace();
    return monrace.is_female() || (this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::WAIFUIZED));
}

std::pair<TERM_COLOR, int> CreatureEntity::get_hp_bar_data() const
{
    const auto percent = (this->maxhp > 0) ? (100 * this->hp / this->maxhp) : 0;
    const auto len = std::clamp(percent / 10 + 1, 1, 10);

    if (this->is_invulnerable()) {
        return { TERM_WHITE, len };
    }
    if (this->is_asleep()) {
        return { TERM_BLUE, len };
    }
    if (percent >= 100) {
        return { TERM_L_GREEN, len };
    }
    if (percent >= 60) {
        return { TERM_YELLOW, len };
    }
    if (percent >= 25) {
        return { TERM_ORANGE, len };
    }
    if (percent >= 10) {
        return { TERM_L_RED, len };
    }
    return { TERM_RED, len };
}

bool CreatureEntity::is_time_limit_esp() const
{
    if (this->tim_esp > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    if (bard && *bard && (*bard)->singing_song == MUSIC_MIND) {
        return true;
    }

    const auto *sniper = std::get_if<std::shared_ptr<SniperData>>(&this->class_specific_data);
    const auto sniper_concent = sniper && *sniper ? (*sniper)->concent : 0;
    return sniper_concent >= CONCENT_TELE_THRESHOLD;
}

bool CreatureEntity::is_time_limit_stealth() const
{
    if (this->tim_stealth > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    return bard && *bard && (*bard)->singing_song == MUSIC_STEALTH;
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

bool CreatureEntity::is_asleep() const
{
    return this->get_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS) > 0;
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
    if (this->get_timed_effect(CreatureTimedEffect::INVULNERABILITY) > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    return bard && *bard && (*bard)->singing_song == MUSIC_INVULN;
}

bool CreatureEntity::is_fast() const
{
    if (this->get_timed_effect(CreatureTimedEffect::ACCELERATION) > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    return bard && *bard && ((*bard)->singing_song == MUSIC_SPEED || (*bard)->singing_song == MUSIC_SHERO);
}

bool CreatureEntity::is_accelerated() const
{
    return this->get_timed_effect(CreatureTimedEffect::ACCELERATION) > 0;
}

bool CreatureEntity::is_decelerated() const
{
    return this->get_timed_effect(CreatureTimedEffect::DECELERATION) > 0;
}

bool CreatureEntity::is_blind() const
{
    return this->get_timed_effect(CreatureTimedEffect::BLINDNESS) > 0;
}

bool CreatureEntity::is_paralyzed() const
{
    return this->get_timed_effect(CreatureTimedEffect::PARALYSIS) > 0;
}

bool CreatureEntity::is_blessed() const
{
    if (this->blessed > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    if (bard && *bard && (*bard)->singing_song == MUSIC_BLESS) {
        return true;
    }

    const auto *hex = std::get_if<std::shared_ptr<spell_hex_data_type>>(&this->class_specific_data);
    return hex && *hex && (*hex)->casting_spells.has(HEX_BLESS);
}

bool CreatureEntity::is_hero() const
{
    if (this->hero > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    return bard && *bard && ((*bard)->singing_song == MUSIC_HERO || (*bard)->singing_song == MUSIC_SHERO);
}

bool CreatureEntity::is_shero() const
{
    return this->berserk > 0 || this->pclass == PlayerClassType::BERSERKER;
}

bool CreatureEntity::is_echizen() const
{
    return this->ppersonality == PERSONALITY_COMBAT || this->is_wielding(FixedArtifactId::CRIMSON);
}

short CreatureEntity::get_timed_effect(CreatureTimedEffect effect) const
{
    if (!this->has_monster_profile()) {
        return 0;
    }

    const auto &mp = this->get_monster_profile();
    switch (effect) {
    case CreatureTimedEffect::STUN:
        return mp.mtimed.at(MonsterTimedEffect::STUN);
    case CreatureTimedEffect::CONFUSION:
        return mp.mtimed.at(MonsterTimedEffect::CONFUSION);
    case CreatureTimedEffect::FEAR:
        return mp.mtimed.at(MonsterTimedEffect::FEAR);
    case CreatureTimedEffect::INVULNERABILITY:
        return mp.mtimed.at(MonsterTimedEffect::INVULNERABILITY);
    case CreatureTimedEffect::ACCELERATION:
        return mp.mtimed.at(MonsterTimedEffect::FAST);
    case CreatureTimedEffect::DECELERATION:
        return mp.mtimed.at(MonsterTimedEffect::SLOW);
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
        return mp.mtimed.at(MonsterTimedEffect::SLEEP);
    default:
        return 0;
    }
}

void CreatureEntity::set_timed_effect(CreatureTimedEffect effect, short value)
{
    if (!this->has_monster_profile()) {
        return;
    }

    auto &mp = this->get_monster_profile();
    switch (effect) {
    case CreatureTimedEffect::STUN:
        mp.mtimed[MonsterTimedEffect::STUN] = value;
        break;
    case CreatureTimedEffect::CONFUSION:
        mp.mtimed[MonsterTimedEffect::CONFUSION] = value;
        break;
    case CreatureTimedEffect::FEAR:
        mp.mtimed[MonsterTimedEffect::FEAR] = value;
        break;
    case CreatureTimedEffect::INVULNERABILITY:
        mp.mtimed[MonsterTimedEffect::INVULNERABILITY] = value;
        break;
    case CreatureTimedEffect::ACCELERATION:
        mp.mtimed[MonsterTimedEffect::FAST] = value;
        break;
    case CreatureTimedEffect::DECELERATION:
        mp.mtimed[MonsterTimedEffect::SLOW] = value;
        break;
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS:
        mp.mtimed[MonsterTimedEffect::SLEEP] = value;
        break;
    default:
        break;
    }
}

void CreatureEntity::make_lore_treasure(int num_item, int num_gold) const
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

bool CreatureEntity::is_valid() const
{
    return MonraceList::is_valid(this->r_idx);
}

bool CreatureEntity::is_hostile_to_melee(const CreatureEntity &other) const
{
    if (!this->has_monster_profile() || !other.has_monster_profile()) {
        return false;
    }

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

bool CreatureEntity::is_hostile_align(byte other_sub_align) const
{
    if (!this->has_monster_profile()) {
        return false;
    }
    return CreatureEntity::check_sub_alignments(this->get_monster_profile().sub_align, other_sub_align);
}

bool CreatureEntity::is_mimicry() const
{
    if (this->ap_r_idx == MonraceId::BEHINDER) {
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

void CreatureEntity::set_hostile()
{
    if (!this->has_monster_profile()) {
        return;
    }

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

tl::optional<bool> CreatureEntity::order_pet_whistle(const CreatureEntity &other) const
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

tl::optional<bool> CreatureEntity::order_pet_dismission(const CreatureEntity &other) const
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

tl::optional<bool> CreatureEntity::order_pet_named(const CreatureEntity &other) const
{
    if (this->is_named() && !other.is_named()) {
        return true;
    }

    if (!this->is_named() && other.is_named()) {
        return false;
    }

    return tl::nullopt;
}

tl::optional<bool> CreatureEntity::order_pet_hp(const CreatureEntity &other) const
{
    if (this->hp > other.hp) {
        return true;
    }

    if (this->hp < other.hp) {
        return false;
    }

    return tl::nullopt;
}

void CreatureEntity::initialize_equivalent_player_races()
{
    if (!this->has_monster_profile()) {
        return;
    }

    auto &mp = this->get_monster_profile();
    mp.equivalent_player_races.clear();
    const auto &monrace = this->get_monrace();

    if (monrace.kind_flags.has(MonsterKindType::HUMAN)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HUMAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::ELF)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ELF);
        mp.equivalent_player_races.push_back(PlayerRaceType::HALF_ELF);
        mp.equivalent_player_races.push_back(PlayerRaceType::HIGH_ELF);
        mp.equivalent_player_races.push_back(PlayerRaceType::DARK_ELF);
    }
    if (monrace.kind_flags.has(MonsterKindType::DWARF)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::DWARF);
    }
    if (monrace.kind_flags.has(MonsterKindType::HOBBIT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HOBBIT);
    }
    if (monrace.kind_flags.has(MonsterKindType::GNOME)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::GNOME);
    }
    if (monrace.kind_flags.has(MonsterKindType::ORC)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HALF_ORC);
    }
    if (monrace.kind_flags.has(MonsterKindType::TROLL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HALF_TROLL);
    }
    if (monrace.kind_flags.has(MonsterKindType::GIANT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HALF_GIANT);
    }
    if (monrace.kind_flags.has(MonsterKindType::OGRE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HALF_OGRE);
    }
    if (monrace.kind_flags.has(MonsterKindType::AMBERITE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::AMBERITE);
    }
    if (monrace.kind_flags.has(MonsterKindType::YEEK)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::YEEK);
    }
    if (monrace.kind_flags.has(MonsterKindType::KOBOLD)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::KOBOLD);
    }
    if (monrace.kind_flags.has(MonsterKindType::NIBELUNG)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::NIBELUNG);
    }
    if (monrace.kind_flags.has(MonsterKindType::DRAGON)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::DRACONIAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::MINDFLAYER)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::MIND_FLAYER);
    }
    if (monrace.kind_flags.has(MonsterKindType::DEMON)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::IMP);
        mp.equivalent_player_races.push_back(PlayerRaceType::BALROG);
    }
    if (monrace.kind_flags.has(MonsterKindType::GOLEM)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::GOLEM);
    }
    if (monrace.kind_flags.has(MonsterKindType::SKELETON)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SKELETON);
    }
    if (monrace.kind_flags.has(MonsterKindType::ZOMBIE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ZOMBIE);
    }
    if (monrace.kind_flags.has(MonsterKindType::VAMPIRE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::VAMPIRE);
    }
    if (monrace.kind_flags.has(MonsterKindType::UNDEAD)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SPECTRE);
    }
    if (monrace.kind_flags.has(MonsterKindType::FAIRY)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SPRITE);
        mp.equivalent_player_races.push_back(PlayerRaceType::S_FAIRY);
    }
    if (monrace.kind_flags.has(MonsterKindType::BEAST)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BEASTMAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::TREEFOLK)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ENT);
    }
    if (monrace.kind_flags.has(MonsterKindType::ANGEL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ARCHON);
    }
    if (monrace.kind_flags.has(MonsterKindType::ROBOT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ANDROID);
    }
    if (monrace.kind_flags.has(MonsterKindType::MERFOLK)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::MERFOLK);
    }

    if (!mp.equivalent_player_races.empty()) {
        this->race = &race_info[enum2i(mp.equivalent_player_races[0])];
        this->prace = mp.equivalent_player_races[0];
    } else {
        this->race = nullptr;
        this->prace = PlayerRaceType::HUMAN;
    }
}

void CreatureEntity::initialize_equivalent_player_classes()
{
    if (!this->has_monster_profile()) {
        return;
    }

    auto &mp = this->get_monster_profile();
    mp.equivalent_player_classes.clear();
    const auto &monrace = this->get_monrace();

    if (monrace.kind_flags.has(MonsterKindType::WARRIOR)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::WARRIOR);
    }
    if (monrace.kind_flags.has(MonsterKindType::MAGE)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::MAGE);
    }
    if (monrace.kind_flags.has(MonsterKindType::PRIEST)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::PRIEST);
    }
    if (monrace.kind_flags.has(MonsterKindType::ROGUE)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::ROGUE);
    }
    if (monrace.kind_flags.has(MonsterKindType::RANGER)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::RANGER);
    }
    if (monrace.kind_flags.has(MonsterKindType::PALADIN)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::PALADIN);
    }
    if (monrace.kind_flags.has(MonsterKindType::SAMURAI)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::SAMURAI);
    }
    if (monrace.kind_flags.has(MonsterKindType::NINJA)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::NINJA);
    }
    if (monrace.kind_flags.has(MonsterKindType::MINDCRAFTER)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::MINDCRAFTER);
    }
    if (monrace.kind_flags.has(MonsterKindType::ARCHER)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::ARCHER);
    }
    if (monrace.kind_flags.has(MonsterKindType::BARD)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::BARD);
    }
    if (monrace.kind_flags.has(MonsterKindType::SMITH)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::SMITH);
    }
    if (monrace.kind_flags.has(MonsterKindType::KARATEKA)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::MONK);
    }

    if (!mp.equivalent_player_classes.empty()) {
        this->pclass_ref = &class_info.at(mp.equivalent_player_classes[0]);
        this->pclass = mp.equivalent_player_classes[0];
    } else {
        this->pclass_ref = nullptr;
        this->pclass = PlayerClassType::WARRIOR;
    }
}

int CreatureEntity::get_ac() const
{
    if (this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::NAKED)) {
        return 0;
    }
    return this->ac + this->to_a;
}

std::string CreatureEntity::build_looking_description(bool needs_attitude) const
{
    const auto description = this->build_damage_description();
    const auto attitude = needs_attitude ? this->build_attitude_description() : "";
    const std::string clone(this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::CLONED) ? ", clone" : "");
    const auto &apparent_monrace = this->get_appearance_monrace();

    const bool show_alliance = this->has_monster_profile() && !this->is_pet() && this->get_monster_profile().alliance_idx != AllianceType::NONE;
    const std::string alliance_part = show_alliance ? format("(%s)", alliance_list.at(this->get_monster_profile().alliance_idx)->name.data()) : "";

    if ((apparent_monrace.r_tkills > 0) && (!this->has_monster_profile() || this->get_monster_profile().mflag2.has_not(MonsterConstantFlagType::KAGE))) {
        constexpr auto fmt = _("レベル%d, %s%s%s%s", "Level %d, %s%s%s%s");
        return format(fmt, apparent_monrace.level, description.data(), attitude.data(), clone.data(), alliance_part.data());
    }

    constexpr auto fmt = _("レベル???, %s%s%s%s", "Level ???, %s%s%s%s");
    return format(fmt, description.data(), attitude.data(), clone.data(), alliance_part.data());
}

std::string CreatureEntity::build_damage_description() const
{
    if (!this->has_monster_profile()) {
        return "";
    }

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

std::string CreatureEntity::build_attitude_description() const
{
    if (this->is_pet()) {
        return _(", ペット", ", pet");
    }

    if (this->is_friendly()) {
        return _(", 友好的", ", friendly");
    }

    return "";
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

tl::optional<std::string> CreatureEntity::get_pain_message(std::string_view monster_name, int damage) const
{
    if (!this->has_monster_profile()) {
        return tl::nullopt;
    }

    auto &monrace = this->get_monrace();
    return MonsterPainDescriber(monrace.idx, monrace.symbol_definition.character, monster_name).describe(this->hp, damage, this->get_monster_profile().ml);
}

byte CreatureEntity::get_temporary_speed() const
{
    auto speed = this->speed;
    if (ironman_nightmare) {
        speed += 5;
    }

    if (this->is_accelerated()) {
        speed += 10;
    }

    if (this->is_decelerated()) {
        speed -= 10;
    }

    if (this->has_monster_profile()) {
        if (this->get_monster_profile().mflag2.has(MonsterConstantFlagType::FAT)) {
            speed -= 5;
        }

        if (this->get_monster_profile().mflag2.has(MonsterConstantFlagType::FRENZY)) {
            speed += 10;
        }
    }

    return speed;
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

int CreatureEntity::get_level() const
{
    if (this->level > 0) {
        return this->level;
    }
    return this->get_monrace().level / 2;
}

/*!
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param force_fixed_speed 速度を固定にする(個体差を適用しない)か否か
 */
void CreatureEntity::set_individual_speed(bool force_fixed_speed)
{
    if (!this->has_monster_profile()) {
        return;
    }

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

bool CreatureEntity::can_ring_boss_call_nazgul() const
{
    auto is_boss = this->r_idx == MonraceId::MORGOTH;
    is_boss |= this->r_idx == MonraceId::SAURON;
    is_boss |= this->r_idx == MonraceId::ANGMAR;
    const auto &nazgul = MonraceList::get_instance().get_monrace(MonraceId::NAZGUL);
    const auto is_nazgul_alive = (nazgul.cur_num + 2) < nazgul.max_num;
    return is_boss && is_nazgul_alive;
}
