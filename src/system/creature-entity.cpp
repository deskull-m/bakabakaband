#include "system/creature-entity.h"
#include "core/speed-table.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "market/arena-entry.h"
#include "mind/mind-elementalist.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-sex-const.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-pain-describer.h"
#include "monster/monster-timed-effects.h"
#include "monster/monster-util.h"
#include "object/object-info.h"
#include "player-ability/player-ability-types.h"
#include "player-info/bard-data-type.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player-info/sniper-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "player/player-personality.h"
#include "player/player-status-flags.h"
#include "player/race-info-table.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/body-structure-policy.h"
#include "system/monrace/extended-slot.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-profile.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "term/z-rand.h"
#include "term/z-util.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <range/v3/algorithm.hpp>

/*!
 * @brief CreatureEntity のデフォルトコンストラクタ
 * @details プレイヤー・モンスター共通で inventory を初期化する。
 * これによりプレイヤー用経路に流されたモンスターでも
 * null 参照・out-of-bounds を起こさない。
 * モンスター固有データ (MonsterProfile) は init_monster_profile() を
 * 別途呼び出して設定する。
 */
CreatureEntity::CreatureEntity()
{
    this->inventory.resize(INVEN_TOTAL);
    ranges::generate(this->inventory, [] { return std::make_shared<ItemEntity>(); });
}

bool CreatureEntity::try_set_position(const Pos2D &pos)
{
    if (this->get_floor()->get_grid(pos).has_monster()) {
        return false;
    }

    this->y = pos.y;
    this->x = pos.x;
    return true;
}

bool CreatureEntity::is_fully_healthy() const
{
    auto is_healthy = this->hp == this->maxhp;
    is_healthy &= this->csp >= this->msp;
    is_healthy &= !this->is_blind();
    is_healthy &= !this->is_confused();
    is_healthy &= !this->is_poisoned();
    is_healthy &= !this->is_fearful();
    is_healthy &= !this->is_stunned();
    is_healthy &= !this->is_cut();
    is_healthy &= !this->is_decelerated();
    is_healthy &= !this->is_paralyzed();
    is_healthy &= !this->is_hallucinated();
    is_healthy &= !this->get_timed_effect(CreatureTimedEffect::WORD_RECALL);
    is_healthy &= !this->get_timed_effect(CreatureTimedEffect::ALTER_REALITY);
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
        this->get_floor()->get_monster(this->riding).reset_constant_flag(MonsterConstantFlagType::RIDING);
    }

    this->riding = m_idx;

    if (is_monster(m_idx)) {
        this->get_floor()->get_monster(m_idx).set_constant_flag(MonsterConstantFlagType::RIDING);
    }
}

void CreatureEntity::plus_incident(INCIDENT incidentID, int num)
{
    if (this->incident.count(incidentID) == 0) {
        this->incident[incidentID] = 0;
    }
    this->incident[incidentID] += num;
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
    if (!this->is_chameleon()) {
        return this->r_idx;
    }

    return this->get_monrace().kind_flags.has(MonsterKindType::UNIQUE) ? MonraceId::CHAMELEON_K : MonraceId::CHAMELEON;
}

MonraceDefinition &CreatureEntity::get_real_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->get_real_monrace_id());
}

const player_sex_type &CreatureEntity::get_sex_info() const
{
    // モンスターの psex は one_monster_placer で kind_flags MALE/FEMALE
    // から計算済み。プレイヤーは birth で選択済み。両者ともそのまま参照する。
    // SEX_NONE は生成途中等の中間値として末尾エントリ ("未設定") を返す。
    const auto psex_safe = (this->psex < MAX_SEXES) ? this->psex : SEX_NONE;
    return sex_info[psex_safe];
}

const player_personality *CreatureEntity::get_personality_info() const
{
    if (this->personality) {
        return this->personality;
    }

    static const player_personality null_personality{};
    return &null_personality;
}

const player_race_info *CreatureEntity::get_race_info() const
{
    if (this->race) {
        return this->race;
    }

    // モンスター等で race ポインタが未設定の場合、空のフォールバックを返す。
    // title が空文字列となるためモンスター c コマンド 3 ページ目の表示で
    // クラッシュしない (旧コードでは null deref で UB だった)。
    static const player_race_info null_race{};
    return &null_race;
}

const player_class_info *CreatureEntity::get_class_info() const
{
    if (this->pclass_ref) {
        return this->pclass_ref;
    }

    static const player_class_info null_class{};
    return &null_class;
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
    return (this->psex == SEX_MALE) || (this->psex == SEX_BISEXUAL);
}

bool CreatureEntity::is_female() const
{
    const auto has_female_psex = (this->psex == SEX_FEMALE) || (this->psex == SEX_BISEXUAL);
    return has_female_psex || this->is_waifuized();
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
    if (this->get_timed_effect(CreatureTimedEffect::TIM_ESP) > 0) {
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
    if (this->get_timed_effect(CreatureTimedEffect::TIM_STEALTH) > 0) {
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

bool CreatureEntity::is_hallucinated() const
{
    return this->get_timed_effect(CreatureTimedEffect::HALLUCINATION) > 0;
}

bool CreatureEntity::is_paralyzed() const
{
    return this->get_timed_effect(CreatureTimedEffect::PARALYSIS) > 0;
}

bool CreatureEntity::is_cut() const
{
    return this->get_timed_effect(CreatureTimedEffect::CUT) > 0;
}

bool CreatureEntity::is_poisoned() const
{
    return this->get_timed_effect(CreatureTimedEffect::POISON) > 0;
}

bool CreatureEntity::is_protected_from_evil() const
{
    return this->get_timed_effect(CreatureTimedEffect::PROTECTION) > 0;
}

int CreatureEntity::get_stun_magic_chance_penalty() const
{
    return PlayerStun::get_magic_chance_penalty(this->get_timed_effect(CreatureTimedEffect::STUN));
}

int CreatureEntity::get_stun_item_chance_penalty() const
{
    return PlayerStun::get_item_chance_penalty(this->get_timed_effect(CreatureTimedEffect::STUN));
}

short CreatureEntity::get_stun_damage_penalty() const
{
    return PlayerStun::get_damage_penalty(this->get_timed_effect(CreatureTimedEffect::STUN));
}

std::pair<TERM_COLOR, std::string> CreatureEntity::get_stun_expr() const
{
    const auto [color, text] = PlayerStun::get_expr(this->get_timed_effect(CreatureTimedEffect::STUN));
    return { color, std::string(text) };
}

std::pair<TERM_COLOR, std::string> CreatureEntity::get_cut_expr() const
{
    const auto [color, text] = PlayerCut::get_expr(this->get_timed_effect(CreatureTimedEffect::CUT));
    return { color, text };
}

int CreatureEntity::get_cut_damage_per_turn() const
{
    return PlayerCut::get_damage(this->get_timed_effect(CreatureTimedEffect::CUT));
}

bool CreatureEntity::is_blessed() const
{
    if (this->get_timed_effect(CreatureTimedEffect::BLESSED) > 0) {
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
    if (this->get_timed_effect(CreatureTimedEffect::HERO) > 0) {
        return true;
    }

    const auto *bard = std::get_if<std::shared_ptr<bard_data_type>>(&this->class_specific_data);
    return bard && *bard && ((*bard)->singing_song == MUSIC_HERO || (*bard)->singing_song == MUSIC_SHERO);
}

bool CreatureEntity::is_shero() const
{
    return this->get_timed_effect(CreatureTimedEffect::BERSERK) > 0 || this->pclass == PlayerClassType::BERSERKER;
}

bool CreatureEntity::is_echizen() const
{
    return this->ppersonality == PERSONALITY_COMBAT || this->is_wielding(FixedArtifactId::CRIMSON);
}

short CreatureEntity::get_timed_effect(CreatureTimedEffect effect) const
{
    // 提案 5 (最終): SLEEP_OR_PARALYSIS は PARALYSIS と同一エントリで扱う
    auto key = (effect == CreatureTimedEffect::SLEEP_OR_PARALYSIS) ? CreatureTimedEffect::PARALYSIS : effect;
    const auto it = this->timed_effects_map.find(key);
    return (it != this->timed_effects_map.end()) ? it->second : 0;
}

void CreatureEntity::set_timed_effect(CreatureTimedEffect effect, short value)
{
    auto key = (effect == CreatureTimedEffect::SLEEP_OR_PARALYSIS) ? CreatureTimedEffect::PARALYSIS : effect;
    this->timed_effects_map[key] = value;
}

int CreatureEntity::get_base_natural_regen_amount() const
{
    return PY_REGEN_NORMAL;
}

int16_t CreatureEntity::store_item(const ItemEntity &item)
{
    auto clone = item.clone();
    return store_item_to_inventory(*this, &clone);
}

bool CreatureEntity::can_store_item(const ItemEntity &item) const
{
    return check_store_item_to_inventory(*this, &item);
}

int16_t CreatureEntity::acquire_item(const ItemEntity &item)
{
    // [フェーズ C-2] 装備可能なアイテムは空きスロットがあれば自動装備
    const auto eq_slot = wield_slot(*this, item);
    const bool can_auto_equip = (eq_slot >= INVEN_MAIN_HAND) && (eq_slot < INVEN_TOTAL) && !this->inventory[eq_slot]->is_valid() && (item.number == 1);
    if (can_auto_equip) {
        *this->inventory[eq_slot] = item.clone();
        return eq_slot;
    }

    // [Phase 2.5] 拡張スロットへの装備
    if (eq_slot >= INVEN_EXTENDED_BASE && item.number == 1) {
        const auto ext_idx = static_cast<size_t>(eq_slot - INVEN_EXTENDED_BASE);
        if (ext_idx < this->extended_inventory.size()) {
            if (!this->extended_inventory[ext_idx]) {
                this->extended_inventory[ext_idx] = std::make_shared<ItemEntity>();
            }
            *this->extended_inventory[ext_idx] = item.clone();
            return eq_slot;
        }
    }

    return this->store_item(item);
}

void CreatureEntity::drop_all_inventory(CreatureEntity &dropper)
{
    for (size_t i = 0; i < this->inventory.size(); i++) {
        auto &held = *this->inventory[i];
        if (!held.is_valid()) {
            continue;
        }
        auto drop_item = held.clone();
        drop_item.held_m_idx = 0;
        (void)drop_near(dropper, drop_item, this->get_position());
        held.wipe();
    }
}

short CreatureEntity::get_inven_cnt() const
{
    short cnt = 0;
    for (auto i = 0; i < INVEN_PACK; i++) {
        if (this->inventory[i] && this->inventory[i]->is_valid()) {
            cnt++;
        }
    }
    return cnt;
}

short CreatureEntity::get_equip_cnt() const
{
    short cnt = 0;
    for (auto i = static_cast<int>(INVEN_MAIN_HAND); i < INVEN_TOTAL; i++) {
        if (this->inventory[i] && this->inventory[i]->is_valid()) {
            cnt++;
        }
    }
    return cnt;
}

bool CreatureEntity::can_equip_to(int slot) const
{
    if (slot < INVEN_MAIN_HAND || slot >= INVEN_TOTAL) {
        return false;
    }

    // プレイヤーは常に HUMANOID 想定で全スロット許可
    if (this->is_player()) {
        return true;
    }

    // モンスター: 種族の body_structure に対応するポリシーを参照
    const auto &monrace = this->get_monrace();
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    return policy.is_allowed(slot);
}

size_t CreatureEntity::get_extended_slot_count() const
{
    if (this->is_player()) {
        return 0;
    }
    const auto &monrace = this->get_monrace();
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    return policy.get_extended_slots().size();
}

ExtendedSlotType CreatureEntity::get_extended_slot_type(size_t idx) const
{
    if (this->is_player()) {
        return ExtendedSlotType::MAX;
    }
    const auto &monrace = this->get_monrace();
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    const auto &slots = policy.get_extended_slots();
    if (idx >= slots.size()) {
        return ExtendedSlotType::MAX;
    }
    return slots[idx];
}

void CreatureEntity::init_extended_inventory()
{
    const auto count = this->get_extended_slot_count();
    this->extended_inventory.resize(count);
    for (auto &slot : this->extended_inventory) {
        if (!slot) {
            slot = std::make_shared<ItemEntity>();
        }
    }
}

void CreatureEntity::add_csp_with_frac(int delta, uint32_t delta_frac)
{
    s64b_add(&this->csp, &this->csp_frac, delta, delta_frac);
}

void CreatureEntity::sub_csp_with_frac(int delta, uint32_t delta_frac)
{
    s64b_sub(&this->csp, &this->csp_frac, delta, delta_frac);
}

void CreatureEntity::add_exp_with_frac(EXP delta, uint32_t delta_frac)
{
    s64b_add(&this->exp, &this->exp_frac, delta, delta_frac);
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

    if (this->get_alliance_idx() != other.get_alliance_idx()) {
        return true;
    } else if (this->is_hostile_align(other.get_sub_align())) {
        if (!this->is_chameleon() || !other.is_chameleon()) {
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
    return CreatureEntity::check_sub_alignments(this->get_sub_align(), other_sub_align);
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

    this->reset_constant_flags({ MonsterConstantFlagType::PET, MonsterConstantFlagType::FRIENDLY });

    if (this->get_alliance_idx() != AllianceType::NONE) {
        for (auto &monster : this->get_floor()->m_list) {
            if (monster.get_alliance_idx() == this->get_alliance_idx()) {
                monster.reset_constant_flags({ MonsterConstantFlagType::PET, MonsterConstantFlagType::FRIENDLY });
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

// [提案 14] AI ターゲット選定の共通化実装
MONSTER_IDX CreatureEntity::find_nearest_creature(const CreaturePredicate &predicate, bool require_projectable) const
{
    const auto &floor = *this->get_floor();
    const auto self_pos = this->get_position();
    MONSTER_IDX best_idx = 0;
    int best_dist = INT_MAX;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        const auto &candidate = floor.get_monster(i);
        if (!candidate.is_valid() || !predicate(candidate)) {
            continue;
        }
        const auto c_pos = candidate.get_position();
        if (require_projectable && !projectable(floor, self_pos, c_pos)) {
            continue;
        }
        const auto dist = Grid::calc_distance(self_pos, c_pos);
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
        }
    }
    return best_idx;
}

bool CreatureEntity::has_visible_creature(const CreaturePredicate &predicate) const
{
    const auto &floor = *this->get_floor();
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        const auto &candidate = floor.get_monster(i);
        if (candidate.is_valid() && predicate(candidate)) {
            return true;
        }
    }
    return false;
}

std::vector<MONSTER_IDX> CreatureEntity::collect_creatures(const CreaturePredicate &predicate) const
{
    const auto &floor = *this->get_floor();
    std::vector<MONSTER_IDX> result;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        const auto &candidate = floor.get_monster(i);
        if (candidate.is_valid() && predicate(candidate)) {
            result.push_back(i);
        }
    }
    return result;
}

int CreatureEntity::get_ac() const
{
    if (this->is_naked()) {
        return 0;
    }

    int total_ac = this->ac + this->to_a;

    // [フェーズ B-2] モンスターは装備品の AC ボーナスをここで集計する。
    // プレイヤーは calc_base_ac() / calc_to_ac() が update_creature() 経由で
    // 既に creature.ac と creature.to_a に含めているため再加算しない。
    if (this->has_monster_profile()) {
        // 通常装備
        for (size_t i = INVEN_MAIN_HAND; i < this->inventory.size(); i++) {
            const auto &item = *this->inventory[i];
            if (!item.is_valid()) {
                continue;
            }
            total_ac += item.ac + item.to_a;
        }
        // [Phase 2.6] 拡張装備スロット (尾の指輪・翼装飾など)
        for (const auto &item_ptr : this->extended_inventory) {
            if (!item_ptr || !item_ptr->is_valid()) {
                continue;
            }
            total_ac += item_ptr->ac + item_ptr->to_a;
        }
    }

    return total_ac;
}

std::string CreatureEntity::build_looking_description(bool needs_attitude) const
{
    const auto description = this->build_damage_description();
    const auto attitude = needs_attitude ? this->build_attitude_description() : "";
    const std::string clone(this->has_monster_profile() && this->is_cloned() ? ", clone" : "");
    const auto &apparent_monrace = this->get_appearance_monrace();

    const bool show_alliance = this->has_monster_profile() && !this->is_pet() && this->get_alliance_idx() != AllianceType::NONE;
    const std::string alliance_part = show_alliance ? format("(%s)", alliance_list.at(this->get_alliance_idx())->name.data()) : "";

    if ((apparent_monrace.r_tkills > 0) && !this->is_kage()) {
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
    if (!this->is_visible_on_map()) {
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
    return MonsterPainDescriber(monrace.idx, monrace.symbol_definition.character, monster_name).describe(this->hp, damage, this->is_visible_on_map());
}

byte CreatureEntity::get_temporary_speed() const
{
    auto current_speed = this->speed;
    if (ironman_nightmare) {
        current_speed += 5;
    }

    if (this->is_accelerated()) {
        current_speed += 10;
    }

    if (this->is_decelerated()) {
        current_speed -= 10;
    }

    if (this->has_monster_profile()) {
        if (this->is_fat()) {
            current_speed -= 5;
        }

        if (this->is_frenzied()) {
            current_speed += 10;
        }
    }

    return static_cast<byte>(current_speed);
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

PLAYER_LEVEL CreatureEntity::get_level() const
{
    if (this->level > 0) {
        return this->level;
    }
    return static_cast<PLAYER_LEVEL>(this->get_monrace().level / 2);
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
    auto new_speed = monrace.speed;
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !force_fixed_speed) {
        /* Allow some small variation per monster */
        int i = speed_to_energy(monrace.speed) / (one_in_(4) ? 3 : 10);
        if (i) {
            new_speed += static_cast<uint8_t>(rand_spread(0, i));
        }
    }

    if (new_speed > STANDARD_SPEED + 99) {
        new_speed = STANDARD_SPEED + 99;
    }

    this->speed = new_speed;
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

void CreatureEntity::init_monster_profile()
{
    this->monster_profile.emplace();
    for (const auto mte : MONSTER_TIMED_EFFECT_LIST) {
        this->set_timed_effect(mte, 0);
    }

    // プレイヤー固有フィールドのデフォルト値（HUMAN / WARRIOR 等）は
    // モンスターに対して意味をなさないため、明示的に無効値に初期化する。
    // 将来モンスターにも種族・職業・魔法領域を持たせて運用する際は、
    // 対応する MonsterProfile 側の設定値からここに反映させる。
    // psex は one_monster_placer で kind_flags MALE/FEMALE から再設定されるが、
    // ここで SEX_NONE に明示初期化することで生成途中の中間状態を区別可能にする。
    this->prace = PlayerRaceType::NONE;
    this->pclass = PlayerClassType::NONE;
    this->ppersonality = PERSONALITY_NONE;
    this->psex = SEX_NONE;
    this->realm1 = RealmType::NONE;
    this->realm2 = RealmType::NONE;
    this->element_realm = ElementRealmType::NONE;
}

void CreatureEntity::wipe()
{
    const bool was_monster = this->has_monster_profile();
    *this = {};
    if (was_monster) {
        this->init_monster_profile();
    }
}

// 耐性系 virtual メソッドのデフォルト実装。
// モンスター (monster_profile 保持) の場合は種族 resistance_flags を参照。
// プレイヤー (or monster_profile なし) の場合は player-status-flags 自由関数に
// 委譲してプレイヤー装備・職業・種族・時限効果から集計。
// 戻り値は BIT_FLAGS_CAUSE_* のビット集合で、0 は非耐性。

namespace {

// モンスターの種族フラグから BIT_FLAGS_CAUSE_RACE を返すヘルパ
static inline BIT_FLAGS monster_race_flag_cause(CreatureEntity &creature, MonsterResistanceType flag)
{
    const auto &monrace = creature.get_monrace();
    return monrace.resistance_flags.has(flag) ? static_cast<BIT_FLAGS>(FLAG_CAUSE_RACE) : 0U;
}

// RESIST/IMMUNE いずれかが立っている場合、RACE 起因として扱う
static inline BIT_FLAGS monster_race_flag_any(CreatureEntity &creature, std::initializer_list<MonsterResistanceType> flags)
{
    const auto &monrace = creature.get_monrace();
    for (auto f : flags) {
        if (monrace.resistance_flags.has(f)) {
            return static_cast<BIT_FLAGS>(FLAG_CAUSE_RACE);
        }
    }
    return 0U;
}

}

// clang-format off
BIT_FLAGS CreatureEntity::has_resist_fire()
{
    BIT_FLAGS result = ::has_resist_fire(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_any(*this, { MonsterResistanceType::RESIST_FIRE, MonsterResistanceType::IMMUNE_FIRE });
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_cold()
{
    BIT_FLAGS result = ::has_resist_cold(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_any(*this, { MonsterResistanceType::RESIST_COLD, MonsterResistanceType::IMMUNE_COLD });
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_elec()
{
    BIT_FLAGS result = ::has_resist_elec(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_any(*this, { MonsterResistanceType::RESIST_ELEC, MonsterResistanceType::IMMUNE_ELEC });
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_acid()
{
    BIT_FLAGS result = ::has_resist_acid(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_any(*this, { MonsterResistanceType::RESIST_ACID, MonsterResistanceType::IMMUNE_ACID });
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_pois()
{
    BIT_FLAGS result = ::has_resist_pois(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_any(*this, { MonsterResistanceType::RESIST_POISON, MonsterResistanceType::IMMUNE_POISON });
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_conf()
{
    BIT_FLAGS result = ::has_resist_conf(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::NO_CONF);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_sound()
{
    BIT_FLAGS result = ::has_resist_sound(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_SOUND);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_lite()
{
    BIT_FLAGS result = ::has_resist_lite(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_LITE);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_dark()
{
    BIT_FLAGS result = ::has_resist_dark(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_DARK);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_chaos()
{
    BIT_FLAGS result = ::has_resist_chaos(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_CHAOS);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_disen()
{
    BIT_FLAGS result = ::has_resist_disen(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_DISENCHANT);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_shard()
{
    BIT_FLAGS result = ::has_resist_shard(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_SHARDS);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_nexus()
{
    BIT_FLAGS result = ::has_resist_nexus(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_NEXUS);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_blind()
{
    if (this->has_monster_profile()) {
        // モンスターには専用 race フラグなし。装備品由来のみを ::has_resist_blind() で集計
        return ::has_resist_blind(*this);
    }
    return ::has_resist_blind(*this);
}
BIT_FLAGS CreatureEntity::has_resist_neth()
{
    BIT_FLAGS result = ::has_resist_neth(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_NETHER);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_time()
{
    BIT_FLAGS result = ::has_resist_time(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_TIME);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_water()
{
    BIT_FLAGS result = ::has_resist_water(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::RESIST_WATER);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_fear()
{
    BIT_FLAGS result = ::has_resist_fear(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::NO_FEAR);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_resist_curse()
{
    if (this->has_monster_profile()) {
        return ::has_resist_curse(*this); // 装備品由来のみ
    }
    return ::has_resist_curse(*this);
}
BIT_FLAGS CreatureEntity::has_vuln_curse()
{
    return ::has_vuln_curse(*this); // 装備品由来のみ (モンスター側に race フラグなし)
}
BIT_FLAGS CreatureEntity::has_vuln_acid()
{
    BIT_FLAGS result = ::has_vuln_acid(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::HURT_ACID);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_vuln_elec()
{
    BIT_FLAGS result = ::has_vuln_elec(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::HURT_ELEC);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_vuln_fire()
{
    BIT_FLAGS result = ::has_vuln_fire(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::HURT_FIRE);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_vuln_cold()
{
    BIT_FLAGS result = ::has_vuln_cold(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::HURT_COLD);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_vuln_lite()
{
    BIT_FLAGS result = ::has_vuln_lite(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::HURT_LITE);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_immune_fire()
{
    BIT_FLAGS result = ::has_immune_fire(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::IMMUNE_FIRE);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_immune_cold()
{
    BIT_FLAGS result = ::has_immune_cold(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::IMMUNE_COLD);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_immune_acid()
{
    BIT_FLAGS result = ::has_immune_acid(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::IMMUNE_ACID);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_immune_elec()
{
    BIT_FLAGS result = ::has_immune_elec(*this);
    if (this->has_monster_profile()) {
        result |= monster_race_flag_cause(*this, MonsterResistanceType::IMMUNE_ELEC);
    }
    return result;
}
BIT_FLAGS CreatureEntity::has_immune_dark()
{
    return ::has_immune_dark(*this); // モンスターは装備品由来のみ
}
BIT_FLAGS CreatureEntity::has_immune_lite()
{
    return ::has_immune_lite(*this); // モンスターは装備品由来のみ
}

// 装備集計系 virtual メソッドにもモンスター経路を実装。
bool CreatureEntity::has_pass_wall()
{
    if (this->has_monster_profile()) {
        return this->get_monrace().feature_flags.has(MonsterFeatureType::PASS_WALL);
    }
    return ::has_pass_wall(*this);
}
bool CreatureEntity::has_kill_wall()
{
    if (this->has_monster_profile()) {
        return this->get_monrace().feature_flags.has(MonsterFeatureType::KILL_WALL);
    }
    return ::has_kill_wall(*this);
}
BIT_FLAGS CreatureEntity::has_reflect()
{
    if (this->has_monster_profile()) {
        return this->get_monrace().misc_flags.has(MonsterMiscType::REFLECTING) ? static_cast<BIT_FLAGS>(FLAG_CAUSE_RACE) : 0U;
    }
    return ::has_reflect(*this);
}
bool CreatureEntity::has_two_handed_weapons()
{
    if (this->has_monster_profile()) {
        return false;
    }
    return ::has_two_handed_weapons(*this);
}
BIT_FLAGS CreatureEntity::has_sh_fire()
{
    if (this->has_monster_profile()) {
        return 0; // モンスターの炎オーラは別系統 (AuraType) で管理
    }
    return ::has_sh_fire(*this);
}
BIT_FLAGS CreatureEntity::has_sh_elec()
{
    if (this->has_monster_profile()) {
        return 0;
    }
    return ::has_sh_elec(*this);
}
BIT_FLAGS CreatureEntity::has_sh_cold()
{
    if (this->has_monster_profile()) {
        return 0;
    }
    return ::has_sh_cold(*this);
}
BIT_FLAGS CreatureEntity::has_down_saving()
{
    if (this->has_monster_profile()) {
        return 0;
    }
    return ::has_down_saving(*this);
}
BIT_FLAGS CreatureEntity::has_no_ac()
{
    if (this->has_monster_profile()) {
        return 0;
    }
    return ::has_no_ac(*this);
}
BIT_FLAGS CreatureEntity::has_easy2_weapon()
{
    if (this->has_monster_profile()) {
        return 0;
    }
    return ::has_easy2_weapon(*this);
}

bool CreatureEntity::has_can_swim() const
{
    if (this->has_monster_profile()) {
        return this->get_monrace().feature_flags.has(MonsterFeatureType::CAN_SWIM);
    }
    return this->can_swim;
}

bool CreatureEntity::has_levitation() const
{
    if (this->has_monster_profile()) {
        return this->get_monrace().feature_flags.has(MonsterFeatureType::CAN_FLY);
    }
    return this->levitation != 0;
}

bool CreatureEntity::has_regen_flag() const
{
    if (this->has_monster_profile()) {
        return this->get_monrace().misc_flags.has(MonsterMiscType::REGENERATE);
    }
    return this->regenerate != 0;
}

bool CreatureEntity::has_lite_flag() const
{
    if (this->has_monster_profile()) {
        const auto &flags = this->get_monrace().brightness_flags;
        return flags.has_any_of({ MonsterBrightnessType::HAS_LITE_1, MonsterBrightnessType::HAS_LITE_2,
            MonsterBrightnessType::SELF_LITE_1, MonsterBrightnessType::SELF_LITE_2 });
    }
    return this->lite != 0;
}

bool CreatureEntity::has_anti_tele() const
{
    if (this->has_monster_profile()) {
        return this->get_monrace().resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT);
    }
    return this->anti_tele != 0;
}
// clang-format on
