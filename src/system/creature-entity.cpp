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
#include "monster-attack/monster-attack-table.h"
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
#include "object-enchant/tr-types.h"
#include "object/object-info.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-race.h"
#include "player-info/bard-data-type.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player-info/sniper-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/race-info-table.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/body-structure-policy.h"
#include "system/monrace/extended-slot.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-records.h"
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
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
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
    is_healthy &= this->current_mp >= this->max_mp;
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

void CreatureEntity::consume_energy_by_speed(int speed_value)
{
    this->sub_energy_need(speed_to_energy(static_cast<byte>(speed_value)));
}

bool CreatureEntity::does_save_against(int power) const
{
    return randint0(100 + power / 2) < this->get_skill_save();
}

void CreatureEntity::notify_self(std::string_view message) const
{
    if (this->is_player()) {
        msg_print(message);
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

std::shared_ptr<MonraceDefinition> CreatureEntity::get_monrace_shared()
{
    return MonraceList::get_instance().get_monrace_shared(this->r_idx);
}

std::shared_ptr<const MonraceDefinition> CreatureEntity::get_monrace_shared() const
{
    return MonraceList::get_instance().get_monrace_shared(this->r_idx);
}

MonraceDefinition &CreatureEntity::get_apparent_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->ap_r_idx);
}

std::shared_ptr<MonraceDefinition> CreatureEntity::get_apparent_monrace_shared()
{
    return MonraceList::get_instance().get_monrace_shared(this->ap_r_idx);
}

std::shared_ptr<const MonraceDefinition> CreatureEntity::get_apparent_monrace_shared() const
{
    return MonraceList::get_instance().get_monrace_shared(this->ap_r_idx);
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
    const auto &monrace = is_appearance ? this->get_apparent_monrace() : this->get_monrace();
    return monrace.has_living_flag();
}

bool CreatureEntity::has_demon_flag(bool is_appearance) const
{
    const auto &monrace = is_appearance ? this->get_apparent_monrace() : this->get_monrace();
    return monrace.has_demon_flag();
}

bool CreatureEntity::has_undead_flag(bool is_appearance) const
{
    const auto &monrace = is_appearance ? this->get_apparent_monrace() : this->get_monrace();
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
    if (this->get_timed_effect(CreatureTimedEffect::BERSERK) > 0) {
        return true;
    }
    // BERSERKER クラスのパッシブはプレイヤー固有。モンスターは pclass を持ち得ても
    // クラスに紐づく常時シェロ化の意味論は持たないため、is_player() で限定する。
    return this->is_player() && this->pclass == PlayerClassType::BERSERKER;
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

namespace {
/*!
 * @brief モンスターの時限効果が 0 をまたいで変化した際に mproc キャッシュを保守する
 * @param creature 対象クリーチャー
 * @param effect 変化した時限効果
 * @param became_active true なら新規付与 (add)、false なら解除 (remove)
 * @details mproc はモンスター時限効果の毎ターン処理用の派生キャッシュ。プレイヤー・
 *          未配置モンスター (ロード中等) は対象外で、その場合は FloorType::reset_mproc()
 *          による再構築 (フロア入場時等) に委ねる。これによりモンスターへ
 *          set_timed_effect() を直接呼んでも mproc 保守が漏れない (従来は
 *          set_monster_* 経由でのみ保守されていた footgun を解消)。
 */
void maintain_monster_mproc_on_toggle(CreatureEntity &creature, CreatureTimedEffect effect, bool became_active)
{
    if (creature.is_player() || !creature.has_monster_profile()) {
        return;
    }

    const auto is_mproc_effect = std::find(MONSTER_TIMED_EFFECT_LIST.begin(), MONSTER_TIMED_EFFECT_LIST.end(), effect) != MONSTER_TIMED_EFFECT_LIST.end();
    if (!is_mproc_effect) {
        return;
    }

    const auto m_idx = creature.get_self_m_idx();
    if (m_idx <= 0) {
        return; // グリッド未配置 (ロード中等)。reset_mproc() による再構築に委ねる。
    }

    auto &floor = *creature.get_floor();
    if (became_active) {
        floor.add_mproc(m_idx, effect);
    } else {
        floor.remove_mproc(m_idx, effect);
    }
}
}

MONSTER_IDX CreatureEntity::get_self_m_idx() const
{
    const auto floor = this->get_floor();
    if (floor == nullptr) {
        return 0;
    }

    const auto m_idx = floor->get_grid(this->get_position()).m_idx;
    if (m_idx <= 0) {
        return 0;
    }

    // グリッド上のモンスターが自分自身であることを確認する。座標が古い / 別モンスターが
    // 立っている / プレイヤー等で不一致の場合は 0 を返し、他モンスターの m_idx 誤取得を防ぐ。
    if (&floor->get_monster(m_idx) != this) {
        return 0;
    }

    return m_idx;
}

void CreatureEntity::set_timed_effect(CreatureTimedEffect effect, short value)
{
    const auto was_active = this->get_timed_effect(effect) > 0;
    auto key = (effect == CreatureTimedEffect::SLEEP_OR_PARALYSIS) ? CreatureTimedEffect::PARALYSIS : effect;
    this->timed_effects_map[key] = value;

    const auto is_active = value > 0;
    if (was_active != is_active) {
        maintain_monster_mproc_on_toggle(*this, effect, is_active);
    }
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

short CreatureEntity::select_melee_weapon_slot(int blow_index, RaceBlowMethodType method) const
{
    switch (method) {
    case RaceBlowMethodType::HIT:
    case RaceBlowMethodType::PUNCH:
    case RaceBlowMethodType::SLASH:
    case RaceBlowMethodType::STING: {
        const auto main_valid = this->inventory[INVEN_MAIN_HAND]->is_valid() && this->inventory[INVEN_MAIN_HAND]->is_melee_weapon();
        const auto sub_valid = this->inventory[INVEN_SUB_HAND]->is_valid() && this->inventory[INVEN_SUB_HAND]->is_melee_weapon();
        if (main_valid && sub_valid) {
            return (blow_index % 2 == 0) ? INVEN_MAIN_HAND : INVEN_SUB_HAND;
        }
        if (main_valid) {
            return INVEN_MAIN_HAND;
        }
        if (sub_valid) {
            return INVEN_SUB_HAND;
        }
        return -1;
    }
    default:
        return -1;
    }
}

bool CreatureEntity::can_equip_to(int slot) const
{
    if (slot < INVEN_MAIN_HAND || slot >= INVEN_TOTAL) {
        return false;
    }

    // プレイヤーで monster へ polymorph していない通常状態: HUMANOID 想定で全スロット許可
    if (this->is_player() && this->get_r_idx() == MonraceId::PLAYER) {
        return true;
    }

    // モンスター、または monster 化したプレイヤー: 種族の body_structure に対応するポリシーを参照
    const auto &monrace = this->get_monrace();
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    return policy.is_allowed(slot);
}

size_t CreatureEntity::get_extended_slot_count() const
{
    if (this->is_player() && this->get_r_idx() == MonraceId::PLAYER) {
        return 0;
    }
    const auto &monrace = this->get_monrace();
    // [Phase 2.7] JSON で extended_slots_override が指定されていればそれを優先
    if (!monrace.extended_slots_override.empty()) {
        return monrace.extended_slots_override.size();
    }
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    return policy.get_extended_slots().size();
}

ExtendedSlotType CreatureEntity::get_extended_slot_type(size_t idx) const
{
    if (this->is_player() && this->get_r_idx() == MonraceId::PLAYER) {
        return ExtendedSlotType::MAX;
    }
    const auto &monrace = this->get_monrace();
    if (!monrace.extended_slots_override.empty()) {
        if (idx >= monrace.extended_slots_override.size()) {
            return ExtendedSlotType::MAX;
        }
        return monrace.extended_slots_override[idx];
    }
    const auto &policy = get_body_slot_policy(monrace.body_structure);
    const auto &slots = policy.get_extended_slots();
    if (idx >= slots.size()) {
        return ExtendedSlotType::MAX;
    }
    return slots[idx];
}

void CreatureEntity::init_extended_inventory()
{
    const auto slot_count = this->get_extended_slot_count();
    this->extended_inventory.resize(slot_count);
    for (auto &slot : this->extended_inventory) {
        if (!slot) {
            slot = std::make_shared<ItemEntity>();
        }
    }
}

size_t CreatureEntity::get_extended_inventory_size() const
{
    return this->extended_inventory.size();
}

const std::shared_ptr<ItemEntity> &CreatureEntity::get_extended_item(size_t idx) const
{
    return this->extended_inventory[idx];
}

const std::vector<std::shared_ptr<ItemEntity>> &CreatureEntity::get_extended_inventory() const
{
    return this->extended_inventory;
}

ItemEntity &CreatureEntity::ensure_extended_item(size_t idx)
{
    if (!this->extended_inventory[idx]) {
        this->extended_inventory[idx] = std::make_shared<ItemEntity>();
    }

    return *this->extended_inventory[idx];
}

void CreatureEntity::add_current_mp_with_frac(int delta, uint32_t delta_frac)
{
    s64b_add(&this->current_mp, &this->current_mp_frac, delta, delta_frac);
}

void CreatureEntity::sub_current_mp_with_frac(int delta, uint32_t delta_frac)
{
    s64b_sub(&this->current_mp, &this->current_mp_frac, delta, delta_frac);
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

    const auto &monrace = this->get_apparent_monrace();
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

const std::vector<CreatureMaterialType> &CreatureEntity::get_materials() const
{
    return this->materials;
}

bool CreatureEntity::has_material(CreatureMaterialType material) const
{
    return std::find(this->materials.begin(), this->materials.end(), material) != this->materials.end();
}

void CreatureEntity::add_material(CreatureMaterialType material)
{
    if (!this->has_material(material)) {
        this->materials.push_back(material);
    }
}

void CreatureEntity::remove_material(CreatureMaterialType material)
{
    this->materials.erase(std::remove(this->materials.begin(), this->materials.end(), material), this->materials.end());
}

void CreatureEntity::clear_materials()
{
    this->materials.clear();
}

void CreatureEntity::set_materials(const std::vector<CreatureMaterialType> &new_materials)
{
    this->materials = new_materials;
}

int CreatureEntity::get_material_stat_modifier(int stat) const
{
    auto total = 0;
    for (const auto material : this->materials) {
        // 材質定義は表示単位 (1 = +1.0) で保持しているため内部 10 単位へ変換する。
        total += get_material_definition(material).stat_modifiers[stat] * 10;
    }
    return total;
}

int CreatureEntity::get_material_ac_modifier() const
{
    auto total = 0;
    for (const auto material : this->materials) {
        total += get_material_definition(material).ac_modifier;
    }
    return total;
}

int CreatureEntity::get_material_gold_drop_percent() const
{
    if (this->materials.empty()) {
        return 100;
    }
    auto percent = 0;
    for (const auto material : this->materials) {
        percent = std::max(percent, get_material_definition(material).gold_drop_percent);
    }
    return percent;
}

void CreatureEntity::apply_material_stat_modifiers()
{
    for (auto stat = 0; stat < A_MAX; ++stat) {
        const auto mod = this->get_material_stat_modifier(stat);
        if (mod == 0) {
            continue;
        }
        auto adjusted = static_cast<int>(this->get_stat_max(stat)) + mod;
        adjusted = std::clamp(adjusted, STAT_MIN_VALUE, STAT_MAX_VALUE);
        this->set_stat_max(stat, static_cast<short>(adjusted));
        this->set_stat_cur(stat, static_cast<short>(adjusted));
        if (this->get_stat_max_max(stat) < this->get_stat_max(stat)) {
            this->set_stat_max_max(stat, this->get_stat_max(stat));
        }
        this->set_stat_use(stat, this->get_stat_max(stat));
    }
}

void CreatureEntity::initialize_materials()
{
    if (!this->has_monster_profile()) {
        return;
    }

    this->materials.clear();
    const auto &monrace = this->get_monrace();

    // JSONc "materials" 指定を反映する。
    for (const auto material : monrace.materials) {
        this->add_material(material);
    }

    // 旧・材質種族 (IRON / GOLD 等) を表していた kind_flags を材質へ移植する。
    static const std::array<std::pair<MonsterKindType, CreatureMaterialType>, 13> flag_materials = { {
        { MonsterKindType::FLESH, CreatureMaterialType::FLESH },
        { MonsterKindType::WOODEN, CreatureMaterialType::WOODEN },
        { MonsterKindType::PAPER, CreatureMaterialType::PAPER },
        { MonsterKindType::STONE, CreatureMaterialType::STONE },
        { MonsterKindType::IRON, CreatureMaterialType::IRON },
        { MonsterKindType::COPPER, CreatureMaterialType::COPPER },
        { MonsterKindType::SILVER, CreatureMaterialType::SILVER },
        { MonsterKindType::GOLD, CreatureMaterialType::GOLD },
        { MonsterKindType::MITHRIL, CreatureMaterialType::MITHRIL },
        { MonsterKindType::ADAMANTITE, CreatureMaterialType::ADAMANTITE },
        { MonsterKindType::DARKSTEEL, CreatureMaterialType::DARKSTEEL },
        { MonsterKindType::WARPSTONE, CreatureMaterialType::WARPSTONE },
        { MonsterKindType::FECES, CreatureMaterialType::FECES },
    } };
    for (const auto &[kind_flag, material] : flag_materials) {
        if (monrace.kind_flags.has(kind_flag)) {
            this->add_material(material);
        }
    }
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
        mp.equivalent_player_races.push_back(PlayerRaceType::ORC);
    }
    if (monrace.kind_flags.has(MonsterKindType::TROLL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::TROLL);
    }
    if (monrace.kind_flags.has(MonsterKindType::GIANT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::GIANT);
    }
    if (monrace.kind_flags.has(MonsterKindType::OGRE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::OGRE);
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
    if (monrace.kind_flags.has(MonsterKindType::CAT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::CAT);
    }
    if (monrace.kind_flags.has(MonsterKindType::DOG)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::DOG);
    }
    if (monrace.kind_flags.has(MonsterKindType::HORSE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HORSE);
    }
    if (monrace.kind_flags.has(MonsterKindType::BIRD)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BIRD);
    }
    if (monrace.kind_flags.has(MonsterKindType::RAT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::RAT);
    }
    if (monrace.kind_flags.has(MonsterKindType::BEAR)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BEAR);
    }
    if (monrace.kind_flags.has(MonsterKindType::SNAKE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SNAKE);
    }
    if (monrace.kind_flags.has(MonsterKindType::FISH)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::FISH);
    }
    if (monrace.kind_flags.has(MonsterKindType::INSECT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::INSECT);
    }
    if (monrace.kind_flags.has(MonsterKindType::SPIDER)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SPIDER);
    }
    if (monrace.kind_flags.has(MonsterKindType::FROG)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::FROG);
    }
    if (monrace.kind_flags.has(MonsterKindType::BAT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BAT);
    }
    if (monrace.kind_flags.has(MonsterKindType::TURTLE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::TURTLE);
    }
    if (monrace.kind_flags.has(MonsterKindType::APE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::APE);
    }
    if (monrace.kind_flags.has(MonsterKindType::AQUATIC_MAMMAL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::AQUATIC_MAMMAL);
    }
    if (monrace.kind_flags.has(MonsterKindType::DINOSAUR)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::DINOSAUR);
    }
    if (monrace.kind_flags.has(MonsterKindType::BOVINE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BOVINE);
    }
    if (monrace.kind_flags.has(MonsterKindType::SHARK)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SHARK);
    }
    if (monrace.kind_flags.has(MonsterKindType::HYDRA)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HYDRA);
    }
    if (monrace.kind_flags.has(MonsterKindType::SLUG)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SLUG);
    }
    if (monrace.kind_flags.has(MonsterKindType::OCTOPUS)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::OCTOPUS);
    }
    if (monrace.kind_flags.has(MonsterKindType::SQUID)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SQUID);
    }
    if (monrace.kind_flags.has(MonsterKindType::HARPY)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HARPY);
    }
    if (monrace.kind_flags.has(MonsterKindType::DEER)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::DEER);
    }
    if (monrace.kind_flags.has(MonsterKindType::ELEPHANT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ELEPHANT);
    }
    if (monrace.kind_flags.has(MonsterKindType::LIZARD)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::LIZARD);
    }
    if (monrace.kind_flags.has(MonsterKindType::HIPPO)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HIPPO);
    }
    if (monrace.kind_flags.has(MonsterKindType::BOAR)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BOAR);
    }
    if (monrace.kind_flags.has(MonsterKindType::RABBIT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::RABBIT);
    }
    if (monrace.kind_flags.has(MonsterKindType::SCORPION)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SCORPION);
    }
    if (monrace.kind_flags.has(MonsterKindType::TANUKI)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::TANUKI);
    }
    if (monrace.kind_flags.has(MonsterKindType::SQUIRREL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SQUIRREL);
    }
    if (monrace.kind_flags.has(MonsterKindType::WEREWOLF)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::WEREWOLF);
    }
    if (monrace.kind_flags.has(MonsterKindType::NAGA)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::NAGA);
    }
    if (monrace.kind_flags.has(MonsterKindType::CANCER)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::CANCER);
    }
    if (monrace.kind_flags.has(MonsterKindType::WORM)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::WORM);
    }
    if (monrace.kind_flags.has(MonsterKindType::KRAKEN)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::KRAKEN);
    }
    if (monrace.kind_flags.has(MonsterKindType::DEEPONE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::DEEPONE);
    }
    if (monrace.kind_flags.has(MonsterKindType::LEECH)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::LEECH);
    }
    if (monrace.kind_flags.has(MonsterKindType::JELLYFISH)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::JELLYFISH);
    }
    if (monrace.kind_flags.has(MonsterKindType::MINOTAUR)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::MINOTAUR);
    }
    if (monrace.kind_flags.has(MonsterKindType::SPHINX)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SPHINX);
    }
    if (monrace.kind_flags.has(MonsterKindType::BEHOLDER)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::BEHOLDER);
    }
    if (monrace.kind_flags.has(MonsterKindType::EYE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::EYE);
    }
    if (monrace.kind_flags.has(MonsterKindType::VORTEX)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::VORTEX);
    }
    if (monrace.kind_flags.has(MonsterKindType::OOZE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::OOZE);
    }
    if (monrace.kind_flags.has(MonsterKindType::GHOST)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::GHOST);
    }
    if (monrace.kind_flags.has(MonsterKindType::LICH)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::LICH);
    }
    if (monrace.kind_flags.has(MonsterKindType::PLANT)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::PLANT);
    }
    if (monrace.kind_flags.has(MonsterKindType::FUNGUS)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::FUNGUS);
    }
    if (monrace.kind_flags.has(MonsterKindType::ELEMENTAL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ELEMENTAL);
    }
    if (monrace.kind_flags.has(MonsterKindType::HORROR)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HORROR);
    }
    if (monrace.kind_flags.has(MonsterKindType::NIGHTSHADE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::NIGHTSHADE);
    }
    if (monrace.kind_flags.has(MonsterKindType::QUYLTHLUG)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::QUYLTHLUG);
    }
    if (monrace.kind_flags.has(MonsterKindType::IXITXACHITL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::IXITXACHITL);
    }
    if (monrace.kind_flags.has(MonsterKindType::MIMIC)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::MIMIC);
    }
    if (monrace.kind_flags.has(MonsterKindType::MONKEY_SPACE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::MONKEY_SPACE);
    }
    if (monrace.kind_flags.has(MonsterKindType::PUYO)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::PUYO);
    }
    if (monrace.kind_flags.has(MonsterKindType::YAZYU)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::YAZYU);
    }
    if (monrace.kind_flags.has(MonsterKindType::QUANTUM)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::QUANTUM);
    }
    if (monrace.kind_flags.has(MonsterKindType::ELDRAZI)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ELDRAZI);
    }
    if (monrace.kind_flags.has(MonsterKindType::SKAVEN)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SKAVEN);
    }
    if (monrace.kind_flags.has(MonsterKindType::ALIEN)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ALIEN);
    }
    if (monrace.kind_flags.has(MonsterKindType::PHYREXIAN)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::PHYREXIAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::GREAT_OLD_ONE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::GREAT_OLD_ONE);
    }
    if (monrace.kind_flags.has(MonsterKindType::AVATAR)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::AVATAR);
    }
    if (monrace.kind_flags.has(MonsterKindType::PLANESWALKER)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::PLANESWALKER);
    }
    if (monrace.kind_flags.has(MonsterKindType::VIRUS)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::VIRUS);
    }
    if (monrace.kind_flags.has(MonsterKindType::CHOASIAN)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::CHOASIAN);
    }
    if (monrace.kind_flags.has(MonsterKindType::FACE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::FACE);
    }
    if (monrace.kind_flags.has(MonsterKindType::HAND)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::HAND);
    }
    if (monrace.kind_flags.has(MonsterKindType::WALL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::WALL);
    }
    if (monrace.kind_flags.has(MonsterKindType::SHIP)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::SHIP);
    }
    if (monrace.kind_flags.has(MonsterKindType::WHEEL)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::WHEEL);
    }
    if (monrace.kind_flags.has(MonsterKindType::EXPLOSIVE)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::EXPLOSIVE);
    }
    if (monrace.kind_flags.has(MonsterKindType::ALARM)) {
        mp.equivalent_player_races.push_back(PlayerRaceType::ALARM);
    }

    // 材質系 kind_flags (WOODEN / IRON / GOLD 等) は種族ではなく材質 (副種族) として
    // initialize_materials() で扱う。ここでは equivalent_player_races に追加しない。

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
    if (monrace.kind_flags.has(MonsterKindType::SOLDIER)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::SOLDIER);
    }
    if (monrace.kind_flags.has(MonsterKindType::PEASANT)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::PEASANT);
    }
    if (monrace.kind_flags.has(MonsterKindType::NOBLE)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::NOBLE);
    }
    if (monrace.kind_flags.has(MonsterKindType::CITIZEN)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::CITIZEN);
    }
    if (monrace.kind_flags.has(MonsterKindType::RABBLE)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::RABBLE);
    }
    if (monrace.kind_flags.has(MonsterKindType::YAKUZA)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::YAKUZA);
    }
    if (monrace.kind_flags.has(MonsterKindType::SUMOU_WRESTLER)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::SUMOU_WRESTLER);
    }
    if (monrace.kind_flags.has(MonsterKindType::GUNNER)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::GUNNER);
    }
    if (monrace.kind_flags.has(MonsterKindType::BERSERK)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::BERSERK);
    }
    if (monrace.kind_flags.has(MonsterKindType::TANK)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::TANK);
    }
    if (monrace.kind_flags.has(MonsterKindType::MAGICAL_GIRL)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::MAGICAL_GIRL);
    }
    if (monrace.kind_flags.has(MonsterKindType::GRANDMA)) {
        mp.equivalent_player_classes.push_back(PlayerClassType::GRANDMA);
    }

    if (!mp.equivalent_player_classes.empty()) {
        this->pclass_ref = &class_info.at(mp.equivalent_player_classes[0]);
        this->pclass = mp.equivalent_player_classes[0];
    } else {
        this->pclass_ref = nullptr;
        this->pclass = PlayerClassType::WARRIOR;
    }
}

void CreatureEntity::set_personality(player_personality_type value)
{
    this->ppersonality = value;
    this->personality = &personality_info[value];
}

void CreatureEntity::assign_random_personality()
{
    // EMPTY_MIND (無心) のモンスターは常に性格「なし」(PERSONALITY_EMPTY) を充てる
    if (this->has_monster_profile() && this->get_monrace().misc_flags.has(MonsterMiscType::EMPTY_MIND)) {
        this->set_personality(PERSONALITY_EMPTY);
        return;
    }

    // モンスター種族に性格が固定指定されている場合は常にそれを使う
    if (this->has_monster_profile()) {
        const auto fixed = this->get_monrace().personality;
        if (fixed != PERSONALITY_NONE) {
            this->set_personality(fixed);
            return;
        }
    }

    const auto psex_value = static_cast<int>(this->get_psex());
    int k;
    do {
        k = randint0(MAX_SELECTABLE_PERSONALITIES);
    } while ((k == PERSONALITY_MUNCHKIN) || ((personality_info[k].sex != 0) && (personality_info[k].sex != (psex_value + 1))));

    this->set_personality(static_cast<player_personality_type>(k));
}

void CreatureEntity::assign_random_realm()
{
    // 領域を選択可能な職業のみ対象とする (戦士・忍者等は領域なし)
    const auto choices = PlayerRealm::get_realm1_choices(this->pclass);
    if (choices.count() == 0) {
        return;
    }

    // 職業の選択肢に縛られず全領域 (魔法 + 技術) から完全ランダムに 1 つ選ぶ
    std::vector<RealmType> all_realms;
    for (auto realm : MAGIC_REALM_RANGE) {
        all_realms.push_back(realm);
    }
    for (auto realm : TECHNIC_REALM_RANGE) {
        all_realms.push_back(realm);
    }

    PlayerRealm pr(*this);
    pr.reset();
    pr.set(all_realms[randint0(static_cast<int>(all_realms.size()))]);
}

void CreatureEntity::assign_fixed_player_race_and_class()
{
    if (!this->has_monster_profile()) {
        return;
    }

    const auto &monrace = this->get_monrace();
    if (monrace.player_race != PlayerRaceType::NONE) {
        this->set_prace(monrace.player_race);
    }
    if (monrace.player_class != PlayerClassType::NONE) {
        this->set_pclass(monrace.player_class);
    }
}

void CreatureEntity::assign_fixed_mutations()
{
    if (!this->has_monster_profile()) {
        return;
    }

    std::vector<PlayerMutationType> fixed_mutations;
    EnumClassFlagGroup<PlayerMutationType>::get_flags(this->get_monrace().mutations, std::back_inserter(fixed_mutations));
    for (const auto mutation : fixed_mutations) {
        this->add_mutation(mutation);
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

    // 材質 (副種族) による AC 修正を加算する。
    total_ac += this->get_material_ac_modifier();

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

        // [提案C2第3弾] 能力値(DEX)を AC へ反映 (applies_stat_combat_bonus・既定OFF)。
        // プレイヤーと同じ adj_dex_ta テーブルで DEX 補正を加算し、負値は 0 下限。
        if (this->get_monrace().applies_stat_combat_bonus) {
            const auto dex_index = stat_value_to_table_index(this->get_stat_use(A_DEX));
            total_ac += static_cast<int>(adj_dex_ta[dex_index]) - 128;
            if (total_ac < 0) {
                total_ac = 0;
            }
        }
    }

    return total_ac;
}

std::string CreatureEntity::build_looking_description(bool needs_attitude) const
{
    const auto description = this->build_damage_description();
    const auto attitude = needs_attitude ? this->build_attitude_description() : "";
    const std::string clone(this->has_monster_profile() && this->is_cloned() ? ", clone" : "");
    const auto &apparent_monrace = this->get_apparent_monrace();

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
    const auto actual_hp = this->hp_table[PY_MAX_LEVEL - 1];

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

    // monrace 由来のレベル算出はモンスター専用。プレイヤーは有効な monrace を
    // 持たないため、ここで get_monrace() を呼ぶと monraces.at() が無効キーで
    // std::out_of_range を投げる (例: キャラクター作成前の画面更新で
    // GODOT_RICH_UI の player_status_push が level==0 のまま get_level() を呼ぶ)。
    if (this->has_monster_profile()) {
        return static_cast<PLAYER_LEVEL>(this->get_monrace().level / 2);
    }

    return this->level;
}

int CreatureEntity::get_melee_proficiency_bonus() const
{
    if (!this->has_monster_profile()) {
        return 0;
    }

    const auto &monrace = this->get_monrace();
    if (!monrace.grows_melee_proficiency) {
        return 0;
    }

    const auto base_level = monrace.level / 2;
    const auto grown = static_cast<int>(this->get_level()) - base_level;
    return grown > 0 ? grown : 0;
}

int CreatureEntity::get_melee_stat_damage_bonus() const
{
    if (!this->has_monster_profile()) {
        return 0;
    }

    if (!this->get_monrace().applies_stat_combat_bonus) {
        return 0;
    }

    const auto index = stat_value_to_table_index(this->get_stat_use(A_STR));
    return static_cast<int>(adj_str_td[index]) - 128;
}

int CreatureEntity::get_melee_stat_hit_bonus() const
{
    if (!this->has_monster_profile()) {
        return 0;
    }

    if (!this->get_monrace().applies_stat_combat_bonus) {
        return 0;
    }

    const auto str_index = stat_value_to_table_index(this->get_stat_use(A_STR));
    const auto dex_index = stat_value_to_table_index(this->get_stat_use(A_DEX));
    return (static_cast<int>(adj_str_th[str_index]) - 128) + (static_cast<int>(adj_dex_th[dex_index]) - 128);
}

int CreatureEntity::get_save_stat_bonus() const
{
    if (!this->has_monster_profile()) {
        return 0;
    }

    if (!this->get_monrace().applies_stat_combat_bonus) {
        return 0;
    }

    // adj_wis_sav は直接加算テーブル (0-19, -128 オフセット無し)。
    const auto index = stat_value_to_table_index(this->get_stat_use(A_WIS));
    return static_cast<int>(adj_wis_sav[index]);
}

/*!
 * @brief 表示用の称号を取得する (提案 E5, 基底 = モンスター既定)
 * @details モンスターは称号を持たないため "なし" を返す。プレイヤーは
 * PlayerType::get_title() override が wizard / winner / 職業別称号を返す。
 */
std::string CreatureEntity::get_title() const
{
    return _("なし", "None");
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

bool CreatureEntity::race_grants_tr_flag(tr_type tr_flag) const
{
    // 付与された player_race が指定 tr_flag を持つか (opt-in 特典反映の共通述語・モンスター専用)。
    // 有効化フラグ (applies_player_race_*) の判定は呼出側が行う。
    if (this->is_player()) {
        return false;
    }
    if (this->get_prace() == PlayerRaceType::NONE) {
        return false;
    }

    return CreatureRace(const_cast<CreatureEntity *>(this)).tr_flags().has(tr_flag);
}

bool CreatureEntity::has_race_granted_reflection() const
{
    // [提案C1第8弾] 付与種族の反射をボルト反射へ反映 (opt-in・既定OFF・モンスター専用)
    return this->race_grants_tr_flag(TR_REFLECT) && this->get_monrace().applies_player_race_reflection;
}

bool CreatureEntity::has_race_granted_regeneration() const
{
    // [提案C1第10弾] 付与種族の再生を自然回復倍化へ反映 (opt-in・既定OFF・モンスター専用)
    return this->race_grants_tr_flag(TR_REGEN) && this->get_monrace().applies_player_race_regeneration;
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
        // [提案C1第10弾] 付与種族の再生 (TR_REGEN) も native REGENERATE と同様に自然回復を倍化 (opt-in・既定OFF)。
        return this->get_monrace().misc_flags.has(MonsterMiscType::REGENERATE) || this->has_race_granted_regeneration();
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

void CreatureEntity::increment_seen_count() const
{
    MonraceRecords::get_instance().increment_seen_count(this->get_r_idx());
}

int CreatureEntity::stat_value_to_table_index(int stat_value)
{
    // 内部 30-2000 -> 索引 0-197 へ変換 (PlayerBasicStatistics::update_index_status と同一式)。
    // 30-180:   0-15  (旧 表示 3-18 相当)
    // 190-2000: 16-197 (表示 18.0 超〜200.0 相当)
    int index;
    if (stat_value <= 180) {
        index = (stat_value - 30) / 10;
    } else if (stat_value <= STAT_MAX_VALUE) {
        index = 15 + (stat_value - 180) / 10;
    } else {
        index = static_cast<int>(STAT_TABLE_SIZE) - 1;
    }

    // 表示 3.0 未満 (index < 0) や上限超過でも配列範囲に収める。
    return std::clamp(index, 0, static_cast<int>(STAT_TABLE_SIZE) - 1);
}

int CreatureEntity::calc_max_hp_con_bonus() const
{
    const auto index = stat_value_to_table_index(this->get_stat_use(A_CON));
    return (static_cast<int>(adj_con_mhp[index]) - 128) * this->get_level() / 4;
}

void CreatureEntity::roll_hp_table()
{
    constexpr auto roll_num = 3 + PY_MAX_LEVEL - 1;
    const auto expected_hp = this->hit_dice.maxroll() + this->hit_dice.floored_expected_value_multiplied_by(roll_num);
    const auto min_value = expected_hp * 3 / 4;
    const auto max_value = expected_hp * 5 / 4;

    /* Rerate */
    while (true) {
        /* Pre-calculate level 1 hitdice */
        this->hp_table[0] = this->hit_dice.maxroll();

        for (int i = 1; i < 4; i++) {
            this->hp_table[0] += this->hit_dice.roll();
        }

        /* Roll the hitpoint values */
        for (int i = 1; i < PY_MAX_LEVEL; i++) {
            this->hp_table[i] = this->hp_table[i - 1] + this->hit_dice.roll();
        }

        /* Require "valid" hitpoints at highest level */
        if ((this->hp_table[PY_MAX_LEVEL - 1] >= min_value) && (this->hp_table[PY_MAX_LEVEL - 1] <= max_value)) {
            break;
        }
    }
}

/*!
 * @brief 敵モンスターのレベル別HPテーブル用 per-level ダイス (1レベルあたりHPダイス) を求める
 * @param creature 対象クリーチャー (モンスター)
 * @return per-level ダイス
 * @details JSON (MonraceDefinition::hit_dice_per_level) に明示指定があればそれを優先する
 *          (アンバランスな個体の手動調節用)。未指定時は既定値を hit_dice (旧 XdY) から
 *          算出する: 面数は旧 Y を維持し、ダイス数を期待値保存で較正する。
 *            E[n d Y] * L = n*(Y+1)/2 * L が旧単発ロール期待値 X*(Y+1)/2 と
 *            一致するよう n = round(X / L)。最低でも 1dY を保証する。
 *          較正に用いる L は種族本来のレベル (monrace.level/2) で固定し、生成後の動的な
 *          レベルアップ (grow_hp_table_to_level) でも成長ダイスが変動しないようにする。
 */
static Dice make_monster_per_level_die(CreatureEntity &creature)
{
    auto per_level_die = creature.get_monrace().hit_dice_per_level;
    if (!per_level_die.is_valid()) {
        const auto natural_level = std::clamp<int>(creature.get_monrace().level / 2, 1, PY_MAX_LEVEL);
        const auto num = std::max(1, (creature.hit_dice.num + natural_level / 2) / natural_level);
        per_level_die = Dice(num, creature.hit_dice.sides);
    }

    return per_level_die;
}

int CreatureEntity::roll_monster_hp_table(bool force_max)
{
    // 実効レベル (敵モンスターは get_level() = monrace.level/2)。hp_table[] の
    // 添字に収めるため [1, PY_MAX_LEVEL] にクランプする。
    const auto effective_level = std::clamp<int>(this->get_level(), 1, PY_MAX_LEVEL);
    const auto per_level_die = make_monster_per_level_die(*this);

    // FORCE_MAXHP は「種族 hit_dice の最大値 (maxroll() = num*sides)」を基礎HPと
    // する従来契約 (monster-status / monster-damage / lore 表示が前提) を維持する。
    // per-level ダイスの最大値を累積しても較正の丸めや JSON 上書きのぶんだけ
    // maxroll() からズレ得るため、force_max 時は maxroll() を各レベルに按分し、
    // 最終レベルで正確に maxroll() へ到達させる。非強制時は較正済み per-level
    // ダイスを通常ロールして累積する (スケール保存)。
    const auto max_total = this->hit_dice.maxroll();
    auto total = 0;
    for (auto i = 0; i < effective_level; i++) {
        if (force_max) {
            this->hp_table[i] = max_total * (i + 1) / effective_level;
        } else {
            total += per_level_die.roll();
            this->hp_table[i] = total;
        }
    }

    return this->hp_table[effective_level - 1];
}

void CreatureEntity::grow_hp_table_to_level(int new_level)
{
    // 本メソッドは敵モンスター専用 (プレイヤー / モンスター運用は roll_hp_table() と
    // 通常のレベルアップ経路で HP を成長させる)。
    if (!this->has_monster_profile()) {
        return;
    }

    const auto old_level = std::clamp<int>(this->get_level(), 1, PY_MAX_LEVEL);
    const auto target_level = std::clamp(new_level, 1, PY_MAX_LEVEL);
    if (target_level <= old_level) {
        return;
    }

    // 生成時と同じ per-level ダイスで hp_table を「生成時より上の添字へ」伸ばす。
    const auto per_level_die = make_monster_per_level_die(*this);
    const auto old_base = this->hp_table[old_level - 1];
    for (auto i = old_level; i < target_level; i++) {
        this->hp_table[i] = this->hp_table[i - 1] + per_level_die.roll();
    }
    const auto base_growth = this->hp_table[target_level - 1] - old_base;

    // CON 補正はレベル依存 (calc_max_hp_con_bonus が get_level() を参照) のため、
    // レベル確定の前後で差分を取って成長分に加える。
    const auto old_con_bonus = this->calc_max_hp_con_bonus();
    this->set_level(static_cast<int16_t>(target_level));
    const auto con_growth = this->calc_max_hp_con_bonus() - old_con_bonus;

    // 既存の最大HP (生成時のサイズ補正・状態補正等を含む) に基礎成長分と CON 補正
    // 差分を加算する。サイズ補正は生成時 1 回限りの乱数倍率のため成長分には
    // 乗じない (現在の最大HPは保ったまま上積みする加算的成長)。
    const auto grown = this->get_max_maxhp() + base_growth + con_growth;
    this->set_max_hp(std::clamp(grown, this->calc_min_max_hp(), MONSTER_MAXHP));
}

void CreatureEntity::grow_stats_by_levels(int levels_gained)
{
    if (levels_gained <= 0) {
        return;
    }

    // [提案 C2] レベルあたりの能力値成長量 (内部 10 単位 = 表示 0.1)。保守的な既定値で、
    // バランス調整はこの定数で行う。獲得レベル数は base_level 上限で有界のため成長も有界。
    constexpr int stat_growth_per_level = 2;
    const auto delta = levels_gained * stat_growth_per_level;
    for (auto stat = 0; stat < A_MAX; ++stat) {
        const auto grown = std::min<int>(static_cast<int>(this->get_stat_max(stat)) + delta, STAT_MAX_VALUE);
        this->set_stat_max(stat, static_cast<short>(grown));
        this->set_stat_cur(stat, static_cast<short>(grown));
        if (this->get_stat_max_max(stat) < this->get_stat_max(stat)) {
            this->set_stat_max_max(stat, this->get_stat_max(stat));
        }
        this->set_stat_use(stat, this->get_stat_max(stat));
    }
}

void CreatureEntity::set_max_hp(int full_max_hp)
{
    this->max_maxhp = full_max_hp;
    this->refresh_max_hp();
}

void CreatureEntity::refresh_max_hp()
{
    // 本来の最大HPから一時減少を差し引いた値を現在の最大HPとする (下限1)。
    this->maxhp = std::max(1, this->max_maxhp - this->get_maxhp_reduction());

    // 現在HPが新しい最大HPに達した/上回る場合は端数を含めて切り詰める。
    if (this->hp >= this->maxhp) {
        this->hp = this->maxhp;
        this->hp_frac = 0;
    }
}

int CreatureEntity::calc_max_hp_status_bonus()
{
    int bonus = 0;
    if (this->is_hero()) {
        bonus += 10;
    }
    if (this->is_shero()) {
        bonus += 30;
    }
    if (this->get_timed_effect(CreatureTimedEffect::TSUYOSHI) > 0) {
        bonus += 50;
    }

    // 呪術 (HEX) はプレイヤー専用。モンスターでは spell_hex_data が無く常に false。
    SpellHex spell_hex(*this);
    if (spell_hex.is_spelling_specific(HEX_XTRA_MIGHT)) {
        bonus += 15;
    }
    if (spell_hex.is_spelling_specific(HEX_BUILDING)) {
        bonus += 60;
    }

    return bonus;
}

// ==== 提案 E4: creature-entity.h からの inline virtual accessor 本体移設 ====

EXP CreatureEntity::get_max_exp() const
{
    return this->max_exp;
}

EXP CreatureEntity::get_max_max_exp() const
{
    return this->max_max_exp;
}

void CreatureEntity::add_exp(EXP delta)
{
    this->exp += delta;
}

void CreatureEntity::sub_exp(EXP delta)
{
    this->exp -= delta;
}

void CreatureEntity::add_max_exp(EXP delta)
{
    this->max_exp += delta;
}

void CreatureEntity::sub_max_exp(EXP delta)
{
    this->max_exp -= delta;
}

bool CreatureEntity::get_ambush_flag() const
{
    return this->ambush_flag;
}

void CreatureEntity::set_to_h_b(int16_t value)
{
    this->to_h_b = value;
}

void CreatureEntity::set_to_h_m(int16_t value)
{
    this->to_h_m = value;
}

void CreatureEntity::set_to_d_m(int16_t value)
{
    this->to_d_m = value;
}

void CreatureEntity::set_to_a(int16_t value)
{
    this->to_a = value;
}

void CreatureEntity::set_to_h(int hand, int16_t value)
{
    this->to_h[hand] = value;
}

void CreatureEntity::set_to_d(int hand, int16_t value)
{
    this->to_d[hand] = value;
}

int16_t CreatureEntity::get_to_h_b() const
{
    return this->to_h_b;
}

int16_t CreatureEntity::get_to_h_m() const
{
    return this->to_h_m;
}

int16_t CreatureEntity::get_to_d_m() const
{
    return this->to_d_m;
}

int16_t CreatureEntity::get_to_a() const
{
    return this->to_a;
}

int16_t CreatureEntity::get_to_h(int hand) const
{
    return this->to_h[hand];
}

int16_t CreatureEntity::get_to_d(int hand) const
{
    return this->to_d[hand];
}

HIT_PROB CreatureEntity::get_dis_to_h(int hand) const
{
    return this->dis_to_h[hand];
}

void CreatureEntity::set_dis_to_h(int hand, HIT_PROB value)
{
    this->dis_to_h[hand] = value;
}

HIT_PROB CreatureEntity::get_dis_to_h_b() const
{
    return this->dis_to_h_b;
}

void CreatureEntity::set_dis_to_h_b(HIT_PROB value)
{
    this->dis_to_h_b = value;
}

int CreatureEntity::get_dis_to_d(int hand) const
{
    return this->dis_to_d[hand];
}

void CreatureEntity::set_dis_to_d(int hand, int value)
{
    this->dis_to_d[hand] = value;
}

ARMOUR_CLASS CreatureEntity::get_dis_to_a() const
{
    return this->dis_to_a;
}

void CreatureEntity::set_dis_to_a(ARMOUR_CLASS value)
{
    this->dis_to_a = value;
}

ARMOUR_CLASS CreatureEntity::get_dis_ac() const
{
    return this->dis_ac;
}

void CreatureEntity::set_dis_ac(ARMOUR_CLASS value)
{
    this->dis_ac = value;
}

void CreatureEntity::set_ac(ARMOUR_CLASS value)
{
    this->ac = value;
}

int16_t CreatureEntity::get_num_blow(int hand) const
{
    return this->num_blow[hand];
}

void CreatureEntity::set_num_blow(int hand, int16_t value)
{
    this->num_blow[hand] = value;
}

int CreatureEntity::get_extra_blows(int hand) const
{
    return this->extra_blows[hand];
}

void CreatureEntity::set_extra_blows(int hand, int value)
{
    this->extra_blows[hand] = value;
}

void CreatureEntity::add_extra_blows(int hand, int delta)
{
    this->extra_blows[hand] += delta;
}

uint32_t CreatureEntity::get_count() const
{
    return this->count;
}

void CreatureEntity::set_count(uint32_t value)
{
    this->count = value;
}

int CreatureEntity::get_hp_table(int level_index) const
{
    return this->hp_table[level_index];
}

void CreatureEntity::set_hp_table(int level_index, int value)
{
    this->hp_table[level_index] = value;
}

int16_t CreatureEntity::get_num_fire() const
{
    return this->num_fire;
}

void CreatureEntity::set_num_fire(int16_t value)
{
    this->num_fire = value;
}

int16_t CreatureEntity::get_to_m_chance() const
{
    return this->to_m_chance;
}

void CreatureEntity::set_to_m_chance(int16_t value)
{
    this->to_m_chance = value;
}

POSITION CreatureEntity::get_cur_lite() const
{
    return this->cur_lite;
}

void CreatureEntity::set_cur_lite(POSITION value)
{
    this->cur_lite = value;
}

bool CreatureEntity::is_cumber_armor() const
{
    return this->cumber_armor;
}

void CreatureEntity::set_cumber_armor(bool value)
{
    this->cumber_armor = value;
}

bool CreatureEntity::is_cumber_glove() const
{
    return this->cumber_glove;
}

void CreatureEntity::set_cumber_glove(bool value)
{
    this->cumber_glove = value;
}

bool CreatureEntity::is_heavy_wield(int hand) const
{
    return this->heavy_wield[hand];
}

void CreatureEntity::set_heavy_wield(int hand, bool value)
{
    this->heavy_wield[hand] = value;
}

bool CreatureEntity::is_icky_wield(int hand) const
{
    return this->icky_wield[hand];
}

void CreatureEntity::set_icky_wield(int hand, bool value)
{
    this->icky_wield[hand] = value;
}

bool CreatureEntity::is_icky_riding_wield(int hand) const
{
    return this->icky_riding_wield[hand];
}

void CreatureEntity::set_icky_riding_wield(int hand, bool value)
{
    this->icky_riding_wield[hand] = value;
}

bool CreatureEntity::is_riding_ryoute() const
{
    return this->riding_ryoute;
}

void CreatureEntity::set_riding_ryoute(bool value)
{
    this->riding_ryoute = value;
}

bool CreatureEntity::is_monlite() const
{
    return this->monlite;
}

void CreatureEntity::set_monlite(bool value)
{
    this->monlite = value;
}

short CreatureEntity::get_stat_max(int idx) const
{
    return this->stat_max[idx];
}

short CreatureEntity::get_stat_cur(int idx) const
{
    return this->stat_cur[idx];
}

short CreatureEntity::get_stat_max_max(int idx) const
{
    return this->stat_max_max[idx];
}

int16_t CreatureEntity::get_stat_use(int idx) const
{
    return this->stat_use[idx];
}

int16_t CreatureEntity::get_stat_top(int idx) const
{
    return this->stat_top[idx];
}

int16_t CreatureEntity::get_stat_add(int idx) const
{
    return this->stat_add[idx];
}

int16_t CreatureEntity::get_stat_index(int idx) const
{
    return this->stat_index[idx];
}

void CreatureEntity::set_stat_max(int idx, short value)
{
    this->stat_max[idx] = value;
}

void CreatureEntity::set_stat_cur(int idx, short value)
{
    this->stat_cur[idx] = value;
}

void CreatureEntity::add_stat_cur(int idx, short delta)
{
    this->stat_cur[idx] += delta;
}

void CreatureEntity::set_stat_max_max(int idx, short value)
{
    this->stat_max_max[idx] = value;
}

void CreatureEntity::set_stat_use(int idx, int16_t value)
{
    this->stat_use[idx] = value;
}

void CreatureEntity::set_stat_top(int idx, int16_t value)
{
    this->stat_top[idx] = value;
}

void CreatureEntity::set_stat_add(int idx, int16_t value)
{
    this->stat_add[idx] = value;
}

void CreatureEntity::set_stat_index(int idx, int16_t value)
{
    this->stat_index[idx] = value;
}

bool CreatureEntity::is_kage() const
{
    return this->has_constant_flag(MonsterConstantFlagType::KAGE);
}

bool CreatureEntity::is_frenzied() const
{
    return this->has_constant_flag(MonsterConstantFlagType::FRENZY);
}

bool CreatureEntity::is_chameleon() const
{
    return this->has_constant_flag(MonsterConstantFlagType::CHAMELEON);
}

bool CreatureEntity::is_cloned() const
{
    return this->has_constant_flag(MonsterConstantFlagType::CLONED);
}

bool CreatureEntity::is_nopet() const
{
    return this->has_constant_flag(MonsterConstantFlagType::NOPET);
}

bool CreatureEntity::is_huge() const
{
    return this->has_constant_flag(MonsterConstantFlagType::HUGE);
}

bool CreatureEntity::is_large() const
{
    return this->has_constant_flag(MonsterConstantFlagType::LARGE);
}

bool CreatureEntity::is_small() const
{
    return this->has_constant_flag(MonsterConstantFlagType::SMALL);
}

bool CreatureEntity::is_fat() const
{
    return this->has_constant_flag(MonsterConstantFlagType::FAT);
}

bool CreatureEntity::is_gaunt() const
{
    return this->has_constant_flag(MonsterConstantFlagType::GAUNT);
}

bool CreatureEntity::is_lightweight() const
{
    return this->has_constant_flag(MonsterConstantFlagType::LIGHTWEIGHT);
}

bool CreatureEntity::is_naked() const
{
    return this->has_constant_flag(MonsterConstantFlagType::NAKED);
}

bool CreatureEntity::is_zombified() const
{
    return this->has_constant_flag(MonsterConstantFlagType::ZOMBIFIED);
}

bool CreatureEntity::is_illegal_modified() const
{
    return this->has_constant_flag(MonsterConstantFlagType::ILLEGAL_MODIFIED);
}

bool CreatureEntity::is_santa() const
{
    return this->has_constant_flag(MonsterConstantFlagType::SANTA);
}

bool CreatureEntity::is_angered() const
{
    return this->has_constant_flag(MonsterConstantFlagType::ANGER);
}

bool CreatureEntity::is_waifuized() const
{
    return this->has_constant_flag(MonsterConstantFlagType::WAIFUIZED);
}

bool CreatureEntity::is_quylthlug_born() const
{
    return this->has_constant_flag(MonsterConstantFlagType::QUYLTHLUG_BORN);
}

bool CreatureEntity::is_defecated() const
{
    return this->has_constant_flag(MonsterConstantFlagType::DEFECATED);
}

bool CreatureEntity::is_vomited() const
{
    return this->has_constant_flag(MonsterConstantFlagType::VOMITED);
}

bool CreatureEntity::has_noflow() const
{
    return this->has_constant_flag(MonsterConstantFlagType::NOFLOW);
}

bool CreatureEntity::is_nogeno() const
{
    return this->has_constant_flag(MonsterConstantFlagType::NOGENO);
}

const EnumClassFlagGroup<MonsterSmartLearnType> &CreatureEntity::get_smart_flags() const
{
    if (this->has_monster_profile()) {
        return this->get_monster_profile().smart;
    }
    static const EnumClassFlagGroup<MonsterSmartLearnType> empty{};
    return empty;
}

SUB_EXP CreatureEntity::get_spell_exp(int spell_idx) const
{
    return this->spell_exp[spell_idx];
}

SUB_EXP CreatureEntity::get_skill_exp(PlayerSkillKindType skill) const
{
    const auto it = this->skill_exp.find(skill);
    return (it != this->skill_exp.end()) ? it->second : 0;
}

SUB_EXP CreatureEntity::get_weapon_exp(ItemKindType tval, int sval) const
{
    const auto it = this->weapon_exp.find(tval);
    return (it != this->weapon_exp.end()) ? it->second[sval] : 0;
}

SUB_EXP CreatureEntity::get_weapon_exp_max(ItemKindType tval, int sval) const
{
    const auto it = this->weapon_exp_max.find(tval);
    return (it != this->weapon_exp_max.end()) ? it->second[sval] : 0;
}

void CreatureEntity::set_spell_exp(int spell_idx, SUB_EXP value)
{
    this->spell_exp[spell_idx] = value;
}

void CreatureEntity::add_spell_exp(int spell_idx, SUB_EXP delta)
{
    this->spell_exp[spell_idx] += delta;
}

void CreatureEntity::set_skill_exp(PlayerSkillKindType skill, SUB_EXP value)
{
    this->skill_exp[skill] = value;
}

void CreatureEntity::add_skill_exp(PlayerSkillKindType skill, SUB_EXP delta)
{
    this->skill_exp[skill] += delta;
}

void CreatureEntity::set_weapon_exp(ItemKindType tval, int sval, SUB_EXP value)
{
    this->weapon_exp[tval][sval] = value;
}

void CreatureEntity::add_weapon_exp(ItemKindType tval, int sval, SUB_EXP delta)
{
    this->weapon_exp[tval][sval] += delta;
}

void CreatureEntity::set_weapon_exp_max(ItemKindType tval, int sval, SUB_EXP value)
{
    this->weapon_exp_max[tval][sval] = value;
}

bool CreatureEntity::is_friendly() const
{
    return this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::FRIENDLY);
}

bool CreatureEntity::is_hostile() const
{
    return this->has_monster_profile() && !this->is_friendly() && !this->is_pet();
}

void CreatureEntity::set_friendly()
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2.set(MonsterConstantFlagType::FRIENDLY);
    }
}

bool CreatureEntity::is_riding() const
{
    return this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::RIDING);
}

bool CreatureEntity::has_parent() const
{
    return this->has_monster_profile() && this->get_monster_profile().parent_m_idx > 0;
}

bool CreatureEntity::has_telepathy() const
{
    return this->telepathy != 0;
}

bool CreatureEntity::has_esp_animal() const
{
    return this->esp_animal != 0;
}

bool CreatureEntity::has_esp_nasty() const
{
    return this->esp_nasty != 0;
}

bool CreatureEntity::has_esp_homo() const
{
    return this->esp_homo != 0;
}

bool CreatureEntity::has_esp_undead() const
{
    return this->esp_undead != 0;
}

bool CreatureEntity::has_esp_demon() const
{
    return this->esp_demon != 0;
}

bool CreatureEntity::has_esp_orc() const
{
    return this->esp_orc != 0;
}

bool CreatureEntity::has_esp_troll() const
{
    return this->esp_troll != 0;
}

bool CreatureEntity::has_esp_giant() const
{
    return this->esp_giant != 0;
}

bool CreatureEntity::has_esp_dragon() const
{
    return this->esp_dragon != 0;
}

bool CreatureEntity::has_esp_human() const
{
    return this->esp_human != 0;
}

bool CreatureEntity::has_esp_evil() const
{
    return this->esp_evil != 0;
}

bool CreatureEntity::has_esp_good() const
{
    return this->esp_good != 0;
}

bool CreatureEntity::has_esp_nonliving() const
{
    return this->esp_nonliving != 0;
}

bool CreatureEntity::has_esp_unique() const
{
    return this->esp_unique != 0;
}

bool CreatureEntity::can_see_invisible() const
{
    return this->see_inv != 0;
}

bool CreatureEntity::has_free_act() const
{
    return this->free_act != 0;
}

bool CreatureEntity::has_anti_magic() const
{
    return this->anti_magic != 0;
}

bool CreatureEntity::has_hold_exp() const
{
    return this->hold_exp != 0;
}

bool CreatureEntity::has_slow_digest_flag() const
{
    return this->slow_digest != 0;
}

bool CreatureEntity::has_see_nocto() const
{
    return this->see_nocto != 0;
}

bool CreatureEntity::has_special_attack(BIT_FLAGS flag) const
{
    return (this->special_attack & flag) != 0;
}

bool CreatureEntity::has_special_defense(BIT_FLAGS flag) const
{
    return (this->special_defense & flag) != 0;
}

bool CreatureEntity::has_warning_flag() const
{
    return this->warning != 0;
}

bool CreatureEntity::has_impact_flag() const
{
    return this->impact != 0;
}

bool CreatureEntity::has_earthquake_flag() const
{
    return this->earthquake != 0;
}

bool CreatureEntity::has_dec_mana() const
{
    return this->dec_mana != 0;
}

bool CreatureEntity::has_easy_spell() const
{
    return this->easy_spell != 0;
}

bool CreatureEntity::has_hard_spell() const
{
    return this->hard_spell != 0;
}

bool CreatureEntity::has_mighty_throw() const
{
    return this->mighty_throw != 0;
}

bool CreatureEntity::has_xtra_might() const
{
    return this->xtra_might != 0;
}

bool CreatureEntity::has_bless_blade() const
{
    return this->bless_blade != 0;
}

void CreatureEntity::set_telepathy(BIT_FLAGS value)
{
    this->telepathy = value;
}

void CreatureEntity::set_esp_animal(BIT_FLAGS value)
{
    this->esp_animal = value;
}

void CreatureEntity::set_esp_nasty(BIT_FLAGS value)
{
    this->esp_nasty = value;
}

void CreatureEntity::set_esp_homo(BIT_FLAGS value)
{
    this->esp_homo = value;
}

void CreatureEntity::set_esp_undead(BIT_FLAGS value)
{
    this->esp_undead = value;
}

void CreatureEntity::set_esp_demon(BIT_FLAGS value)
{
    this->esp_demon = value;
}

void CreatureEntity::set_esp_orc(BIT_FLAGS value)
{
    this->esp_orc = value;
}

void CreatureEntity::set_esp_troll(BIT_FLAGS value)
{
    this->esp_troll = value;
}

void CreatureEntity::set_esp_giant(BIT_FLAGS value)
{
    this->esp_giant = value;
}

void CreatureEntity::set_esp_dragon(BIT_FLAGS value)
{
    this->esp_dragon = value;
}

void CreatureEntity::set_esp_human(BIT_FLAGS value)
{
    this->esp_human = value;
}

void CreatureEntity::set_esp_evil(BIT_FLAGS value)
{
    this->esp_evil = value;
}

void CreatureEntity::set_esp_good(BIT_FLAGS value)
{
    this->esp_good = value;
}

void CreatureEntity::set_esp_nonliving(BIT_FLAGS value)
{
    this->esp_nonliving = value;
}

void CreatureEntity::set_esp_unique(BIT_FLAGS value)
{
    this->esp_unique = value;
}

void CreatureEntity::set_can_swim(bool value)
{
    this->can_swim = value;
}

void CreatureEntity::set_levitation(BIT_FLAGS value)
{
    this->levitation = value;
}

void CreatureEntity::set_free_act(BIT_FLAGS value)
{
    this->free_act = value;
}

void CreatureEntity::set_see_inv(BIT_FLAGS value)
{
    this->see_inv = value;
}

void CreatureEntity::set_regenerate(BIT_FLAGS value)
{
    this->regenerate = value;
}

void CreatureEntity::set_hold_exp(BIT_FLAGS value)
{
    this->hold_exp = value;
}

void CreatureEntity::set_slow_digest(BIT_FLAGS value)
{
    this->slow_digest = value;
}

void CreatureEntity::set_lite_flags(BIT_FLAGS value)
{
    this->lite = value;
}

void CreatureEntity::set_warning_flags(BIT_FLAGS value)
{
    this->warning = value;
}

void CreatureEntity::set_impact_flags(BIT_FLAGS value)
{
    this->impact = value;
}

void CreatureEntity::set_earthquake_flags(BIT_FLAGS value)
{
    this->earthquake = value;
}

void CreatureEntity::set_dec_mana(BIT_FLAGS value)
{
    this->dec_mana = value;
}

void CreatureEntity::set_easy_spell(BIT_FLAGS value)
{
    this->easy_spell = value;
}

void CreatureEntity::set_hard_spell(BIT_FLAGS value)
{
    this->hard_spell = value;
}

void CreatureEntity::set_mighty_throw(BIT_FLAGS value)
{
    this->mighty_throw = value;
}

void CreatureEntity::set_see_nocto(BIT_FLAGS value)
{
    this->see_nocto = value;
}

void CreatureEntity::set_anti_magic(BIT_FLAGS value)
{
    this->anti_magic = value;
}

void CreatureEntity::set_anti_tele(BIT_FLAGS value)
{
    this->anti_tele = value;
}

void CreatureEntity::set_bless_blade(BIT_FLAGS value)
{
    this->bless_blade = value;
}

void CreatureEntity::set_xtra_might(BIT_FLAGS value)
{
    this->xtra_might = value;
}

BIT_FLAGS CreatureEntity::get_impact_flags() const
{
    return this->impact;
}

BIT_FLAGS CreatureEntity::get_earthquake_flags() const
{
    return this->earthquake;
}

void CreatureEntity::set_special_attack_flags(BIT_FLAGS value)
{
    this->special_attack = value;
}

BIT_FLAGS CreatureEntity::get_special_attack_flags() const
{
    return this->special_attack;
}

void CreatureEntity::add_special_attack(BIT_FLAGS flag)
{
    this->special_attack |= flag;
}

void CreatureEntity::remove_special_attack(BIT_FLAGS flag)
{
    this->special_attack &= ~flag;
}

void CreatureEntity::set_special_defense_flags(BIT_FLAGS value)
{
    this->special_defense = value;
}

BIT_FLAGS CreatureEntity::get_special_defense_flags() const
{
    return this->special_defense;
}

void CreatureEntity::add_special_defense(BIT_FLAGS flag)
{
    this->special_defense |= flag;
}

void CreatureEntity::remove_special_defense(BIT_FLAGS flag)
{
    this->special_defense &= ~flag;
}

const EnumClassFlagGroup<PlayerMutationType> &CreatureEntity::get_mutations() const
{
    return this->muta;
}

const EnumClassFlagGroup<PlayerMutationType> &CreatureEntity::get_traits() const
{
    return this->trait;
}

const EnumClassFlagGroup<CurseTraitType> &CreatureEntity::get_cursed_flags() const
{
    return this->cursed;
}

const EnumClassFlagGroup<CurseSpecialTraitType> &CreatureEntity::get_cursed_special_flags() const
{
    return this->cursed_special;
}

bool CreatureEntity::has_mutation(PlayerMutationType m) const
{
    return this->muta.has(m);
}

void CreatureEntity::add_mutation(PlayerMutationType m)
{
    this->muta.set(m);
}

void CreatureEntity::remove_mutation(PlayerMutationType m)
{
    this->muta.reset(m);
}

void CreatureEntity::clear_mutations()
{
    this->muta.clear();
}

void CreatureEntity::set_mutations(const EnumClassFlagGroup<PlayerMutationType> &flags)
{
    this->muta = flags;
}

bool CreatureEntity::has_trait(PlayerMutationType t) const
{
    return this->trait.has(t);
}

void CreatureEntity::add_trait(PlayerMutationType t)
{
    this->trait.set(t);
}

void CreatureEntity::remove_trait(PlayerMutationType t)
{
    this->trait.reset(t);
}

void CreatureEntity::clear_traits()
{
    this->trait.clear();
}

void CreatureEntity::set_traits(const EnumClassFlagGroup<PlayerMutationType> &flags)
{
    this->trait = flags;
}

bool CreatureEntity::has_curse(CurseTraitType c) const
{
    return this->cursed.has(c);
}

void CreatureEntity::add_curse(CurseTraitType c)
{
    this->cursed.set(c);
}

void CreatureEntity::add_curses(const EnumClassFlagGroup<CurseTraitType> &flags)
{
    this->cursed.set(flags);
}

void CreatureEntity::remove_curse(CurseTraitType c)
{
    this->cursed.reset(c);
}

void CreatureEntity::clear_curses()
{
    this->cursed.clear();
}

void CreatureEntity::set_curses(const EnumClassFlagGroup<CurseTraitType> &flags)
{
    this->cursed = flags;
}

bool CreatureEntity::has_curse_special(CurseSpecialTraitType c) const
{
    return this->cursed_special.has(c);
}

void CreatureEntity::add_curse_special(CurseSpecialTraitType c)
{
    this->cursed_special.set(c);
}

void CreatureEntity::remove_curse_special(CurseSpecialTraitType c)
{
    this->cursed_special.reset(c);
}

void CreatureEntity::clear_curses_special()
{
    this->cursed_special.clear();
}

void CreatureEntity::set_curses_special(const EnumClassFlagGroup<CurseSpecialTraitType> &flags)
{
    this->cursed_special = flags;
}

ACTION_SKILL_POWER CreatureEntity::get_infravision() const
{
    return this->see_infra;
}

void CreatureEntity::set_infravision(ACTION_SKILL_POWER value)
{
    this->see_infra = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_disarm() const
{
    return this->skill_dis;
}

void CreatureEntity::set_skill_disarm(ACTION_SKILL_POWER value)
{
    this->skill_dis = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_device() const
{
    return this->skill_dev;
}

void CreatureEntity::set_skill_device(ACTION_SKILL_POWER value)
{
    this->skill_dev = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_save() const
{
    return this->skill_sav;
}

void CreatureEntity::set_skill_save(ACTION_SKILL_POWER value)
{
    this->skill_sav = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_stealth() const
{
    return this->skill_stl;
}

void CreatureEntity::set_skill_stealth(ACTION_SKILL_POWER value)
{
    this->skill_stl = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_search() const
{
    return this->skill_srh;
}

void CreatureEntity::set_skill_search(ACTION_SKILL_POWER value)
{
    this->skill_srh = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_perception() const
{
    return this->skill_fos;
}

void CreatureEntity::set_skill_perception(ACTION_SKILL_POWER value)
{
    this->skill_fos = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_to_hit_melee() const
{
    return this->skill_thn;
}

void CreatureEntity::set_skill_to_hit_melee(ACTION_SKILL_POWER value)
{
    this->skill_thn = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_to_hit_bow() const
{
    return this->skill_thb;
}

void CreatureEntity::set_skill_to_hit_bow(ACTION_SKILL_POWER value)
{
    this->skill_thb = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_to_hit_throw() const
{
    return this->skill_tht;
}

void CreatureEntity::set_skill_to_hit_throw(ACTION_SKILL_POWER value)
{
    this->skill_tht = value;
}

ACTION_SKILL_POWER CreatureEntity::get_skill_dig() const
{
    return this->skill_dig;
}

void CreatureEntity::set_skill_dig(ACTION_SKILL_POWER value)
{
    this->skill_dig = value;
}

MimicKindType CreatureEntity::get_mimic_form() const
{
    return this->mimic_form;
}

void CreatureEntity::set_mimic_form(MimicKindType form)
{
    this->mimic_form = form;
}

BIT_FLAGS CreatureEntity::get_spell_learned_flags(int realm_idx) const
{
    return (realm_idx == 0) ? this->spell_learned1 : this->spell_learned2;
}

BIT_FLAGS CreatureEntity::get_spell_worked_flags(int realm_idx) const
{
    return (realm_idx == 0) ? this->spell_worked1 : this->spell_worked2;
}

BIT_FLAGS CreatureEntity::get_spell_forgotten_flags(int realm_idx) const
{
    return (realm_idx == 0) ? this->spell_forgotten1 : this->spell_forgotten2;
}

void CreatureEntity::set_spell_learned_flags(int realm_idx, BIT_FLAGS value)
{
    (realm_idx == 0 ? this->spell_learned1 : this->spell_learned2) = value;
}

void CreatureEntity::set_spell_worked_flags(int realm_idx, BIT_FLAGS value)
{
    (realm_idx == 0 ? this->spell_worked1 : this->spell_worked2) = value;
}

void CreatureEntity::set_spell_forgotten_flags(int realm_idx, BIT_FLAGS value)
{
    (realm_idx == 0 ? this->spell_forgotten1 : this->spell_forgotten2) = value;
}

bool CreatureEntity::has_learned_spell(int realm_idx, int spell_id) const
{
    return (this->get_spell_learned_flags(realm_idx) & (1UL << spell_id)) != 0;
}

bool CreatureEntity::has_worked_spell(int realm_idx, int spell_id) const
{
    return (this->get_spell_worked_flags(realm_idx) & (1UL << spell_id)) != 0;
}

bool CreatureEntity::has_forgotten_spell(int realm_idx, int spell_id) const
{
    return (this->get_spell_forgotten_flags(realm_idx) & (1UL << spell_id)) != 0;
}

void CreatureEntity::set_learned_spell(int realm_idx, int spell_id, bool value)
{
    const auto bit = 1UL << spell_id;
    auto flags = this->get_spell_learned_flags(realm_idx);
    flags = value ? (flags | bit) : (flags & ~bit);
    this->set_spell_learned_flags(realm_idx, flags);
}

void CreatureEntity::set_worked_spell(int realm_idx, int spell_id, bool value)
{
    const auto bit = 1UL << spell_id;
    auto flags = this->get_spell_worked_flags(realm_idx);
    flags = value ? (flags | bit) : (flags & ~bit);
    this->set_spell_worked_flags(realm_idx, flags);
}

void CreatureEntity::set_forgotten_spell(int realm_idx, int spell_id, bool value)
{
    const auto bit = 1UL << spell_id;
    auto flags = this->get_spell_forgotten_flags(realm_idx);
    flags = value ? (flags | bit) : (flags & ~bit);
    this->set_spell_forgotten_flags(realm_idx, flags);
}

// ==== 提案 E4: creature-entity.h からの inline virtual accessor 本体移設 ====

AllianceType CreatureEntity::get_alliance_idx() const
{
    return this->has_monster_profile() ? this->get_monster_profile().alliance_idx : AllianceType::NONE;
}

BIT_FLAGS8 CreatureEntity::get_sub_align() const
{
    return this->has_monster_profile() ? this->get_monster_profile().sub_align : static_cast<BIT_FLAGS8>(SUB_ALIGN_NEUTRAL);
}

MONSTER_IDX CreatureEntity::get_parent_m_idx() const
{
    return this->has_monster_profile() ? this->get_monster_profile().parent_m_idx : 0;
}

void CreatureEntity::set_alliance_idx(AllianceType alliance)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().alliance_idx = alliance;
    }
}

void CreatureEntity::set_sub_align(BIT_FLAGS8 sub_align)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().sub_align = sub_align;
    }
}

void CreatureEntity::add_sub_align(BIT_FLAGS8 mask)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().sub_align |= mask;
    }
}

void CreatureEntity::set_parent_m_idx(MONSTER_IDX m_idx)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().parent_m_idx = m_idx;
    }
}

void CreatureEntity::add_smart_flag(MonsterSmartLearnType flag)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().smart.set(flag);
    }
}

void CreatureEntity::clear_smart_flags()
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().smart.clear();
    }
}

MonraceId CreatureEntity::get_transform_r_idx() const
{
    return this->has_monster_profile() ? this->get_monster_profile().transform_r_idx : MonraceId::PLAYER;
}

void CreatureEntity::set_transform_r_idx(MonraceId new_r_idx)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().transform_r_idx = new_r_idx;
    }
}

PERCENTAGE CreatureEntity::get_transform_hp_threshold() const
{
    return this->has_monster_profile() ? this->get_monster_profile().transform_hp_threshold : 0;
}

void CreatureEntity::set_transform_hp_threshold(PERCENTAGE threshold)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().transform_hp_threshold = threshold;
    }
}

bool CreatureEntity::has_transformed() const
{
    return this->has_monster_profile() && this->get_monster_profile().has_transformed;
}

void CreatureEntity::set_has_transformed(bool transformed)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().has_transformed = transformed;
    }
}

int CreatureEntity::get_death_count() const
{
    return this->has_monster_profile() ? this->get_monster_profile().death_count : 0;
}

void CreatureEntity::set_death_count(int new_count)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().death_count = new_count;
    }
}

int CreatureEntity::decrement_death_count()
{
    if (this->has_monster_profile()) {
        return --this->get_monster_profile().death_count;
    }
    return 0;
}

void CreatureEntity::set_constant_flag(MonsterConstantFlagType flag)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2.set(flag);
    }
}

void CreatureEntity::set_constant_flags(std::initializer_list<MonsterConstantFlagType> flags)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2.set(flags);
    }
}

void CreatureEntity::reset_constant_flag(MonsterConstantFlagType flag)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2.reset(flag);
    }
}

void CreatureEntity::reset_constant_flags(std::initializer_list<MonsterConstantFlagType> flags)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2.reset(flags);
    }
}

void CreatureEntity::assign_constant_flag(MonsterConstantFlagType flag, bool value)
{
    if (!this->has_monster_profile()) {
        return;
    }
    if (value) {
        this->get_monster_profile().mflag2.set(flag);
    } else {
        this->get_monster_profile().mflag2.reset(flag);
    }
}

void CreatureEntity::clear_constant_flags()
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2.clear();
    }
}

const EnumClassFlagGroup<MonsterConstantFlagType> &CreatureEntity::get_all_constant_flags() const
{
    static const EnumClassFlagGroup<MonsterConstantFlagType> empty{};
    return this->has_monster_profile() ? this->get_monster_profile().mflag2 : empty;
}

void CreatureEntity::set_all_constant_flags(const EnumClassFlagGroup<MonsterConstantFlagType> &flags)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag2 = flags;
    }
}

void CreatureEntity::clear_temporary_flags()
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag.clear();
    }
}

void CreatureEntity::set_temporary_flag(MonsterTemporaryFlagType flag)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag.set(flag);
    }
}

void CreatureEntity::reset_temporary_flag(MonsterTemporaryFlagType flag)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().mflag.reset(flag);
    }
}

bool CreatureEntity::is_in_view() const
{
    return this->has_temporary_flag(MonsterTemporaryFlagType::VIEW);
}

bool CreatureEntity::is_marked_for_los() const
{
    return this->has_temporary_flag(MonsterTemporaryFlagType::LOS);
}

bool CreatureEntity::is_sensed_by_esp() const
{
    return this->has_temporary_flag(MonsterTemporaryFlagType::ESP);
}

bool CreatureEntity::was_present_at_turn_start() const
{
    return this->has_temporary_flag(MonsterTemporaryFlagType::PRESENT_AT_TURN_START);
}

bool CreatureEntity::has_prevent_magic() const
{
    return this->has_temporary_flag(MonsterTemporaryFlagType::PREVENT_MAGIC);
}

bool CreatureEntity::has_sanity_blast() const
{
    return this->has_temporary_flag(MonsterTemporaryFlagType::SANITY_BLAST);
}

MonraceId CreatureEntity::get_r_idx() const
{
    return this->r_idx;
}

MonraceId CreatureEntity::get_ap_r_idx() const
{
    return this->ap_r_idx;
}

MONSTER_IDX CreatureEntity::get_riding() const
{
    return this->riding;
}

void CreatureEntity::set_r_idx(MonraceId new_r_idx)
{
    this->r_idx = new_r_idx;
}

void CreatureEntity::set_ap_r_idx(MonraceId new_ap_r_idx)
{
    this->ap_r_idx = new_ap_r_idx;
}

void CreatureEntity::polymorph_to(MonraceId new_r_idx)
{
    this->r_idx = new_r_idx;
    this->ap_r_idx = new_r_idx;
}

void CreatureEntity::set_riding(MONSTER_IDX m_idx)
{
    this->riding = m_idx;
}

bool CreatureEntity::has_pet_extra_flag(BIT_FLAGS16 flag) const
{
    return (this->pet_extra_flags & flag) != 0;
}

void CreatureEntity::add_pet_extra_flag(BIT_FLAGS16 flag)
{
    this->pet_extra_flags |= flag;
}

void CreatureEntity::remove_pet_extra_flag(BIT_FLAGS16 flag)
{
    this->pet_extra_flags &= static_cast<BIT_FLAGS16>(~flag);
}

BIT_FLAGS16 CreatureEntity::get_pet_extra_flags() const
{
    return this->pet_extra_flags;
}

void CreatureEntity::set_pet_extra_flags(BIT_FLAGS16 value)
{
    this->pet_extra_flags = value;
}

int16_t CreatureEntity::get_pet_follow_distance() const
{
    return this->pet_follow_distance;
}

void CreatureEntity::set_pet_follow_distance(int16_t value)
{
    this->pet_follow_distance = value;
}

MONSTER_IDX CreatureEntity::get_pet_t_m_idx() const
{
    return this->pet_t_m_idx;
}

void CreatureEntity::set_pet_t_m_idx(MONSTER_IDX value)
{
    this->pet_t_m_idx = value;
}

MONSTER_IDX CreatureEntity::get_riding_t_m_idx() const
{
    return this->riding_t_m_idx;
}

void CreatureEntity::set_riding_t_m_idx(MONSTER_IDX value)
{
    this->riding_t_m_idx = value;
}

POSITION CreatureEntity::get_old_lite() const
{
    return this->old_lite;
}

void CreatureEntity::set_old_lite(POSITION value)
{
    this->old_lite = value;
}

BIT_FLAGS CreatureEntity::get_old_race_flags1() const
{
    return this->old_race1;
}

void CreatureEntity::set_old_race_flags1(BIT_FLAGS value)
{
    this->old_race1 = value;
}

BIT_FLAGS CreatureEntity::get_old_race_flags2() const
{
    return this->old_race2;
}

void CreatureEntity::set_old_race_flags2(BIT_FLAGS value)
{
    this->old_race2 = value;
}

int16_t CreatureEntity::get_old_realm() const
{
    return this->old_realm;
}

void CreatureEntity::set_old_realm(int16_t value)
{
    this->old_realm = value;
}

int16_t CreatureEntity::get_old_spells() const
{
    return this->old_spells;
}

void CreatureEntity::set_old_spells(int16_t value)
{
    this->old_spells = value;
}

bool CreatureEntity::was_cumber_armor() const
{
    return this->old_cumber_armor;
}

void CreatureEntity::set_was_cumber_armor(bool value)
{
    this->old_cumber_armor = value;
}

bool CreatureEntity::was_cumber_glove() const
{
    return this->old_cumber_glove;
}

void CreatureEntity::set_was_cumber_glove(bool value)
{
    this->old_cumber_glove = value;
}

bool CreatureEntity::was_heavy_wield(int hand) const
{
    return this->old_heavy_wield[hand];
}

void CreatureEntity::set_was_heavy_wield(int hand, bool value)
{
    this->old_heavy_wield[hand] = value;
}

bool CreatureEntity::was_heavy_shoot() const
{
    return this->old_heavy_shoot;
}

void CreatureEntity::set_was_heavy_shoot(bool value)
{
    this->old_heavy_shoot = value;
}

bool CreatureEntity::was_icky_wield(int hand) const
{
    return this->old_icky_wield[hand];
}

void CreatureEntity::set_was_icky_wield(int hand, bool value)
{
    this->old_icky_wield[hand] = value;
}

bool CreatureEntity::was_icky_riding_wield(int hand) const
{
    return this->old_riding_wield[hand];
}

void CreatureEntity::set_was_icky_riding_wield(int hand, bool value)
{
    this->old_riding_wield[hand] = value;
}

bool CreatureEntity::was_riding_ryoute() const
{
    return this->old_riding_ryoute;
}

void CreatureEntity::set_was_riding_ryoute(bool value)
{
    this->old_riding_ryoute = value;
}

bool CreatureEntity::was_monlite() const
{
    return this->old_monlite;
}

void CreatureEntity::set_was_monlite(bool value)
{
    this->old_monlite = value;
}

byte CreatureEntity::get_action() const
{
    return this->action;
}

void CreatureEntity::set_action(byte value)
{
    this->action = value;
}

int16_t CreatureEntity::get_running() const
{
    return this->running;
}

void CreatureEntity::set_running(int16_t value)
{
    this->running = value;
}

GAME_TURN CreatureEntity::get_resting() const
{
    return this->resting;
}

void CreatureEntity::set_resting(GAME_TURN value)
{
    this->resting = value;
}

bool CreatureEntity::is_fired() const
{
    return this->fired;
}

void CreatureEntity::set_is_fired(bool value)
{
    this->fired = value;
}

bool CreatureEntity::has_level_up_message() const
{
    return this->level_up_message;
}

void CreatureEntity::set_level_up_message(bool value)
{
    this->level_up_message = value;
}

bool CreatureEntity::is_timewalking() const
{
    return this->timewalk;
}

void CreatureEntity::set_timewalking(bool value)
{
    this->timewalk = value;
}

bool CreatureEntity::is_now_damaged() const
{
    return this->now_damaged;
}

void CreatureEntity::set_now_damaged(bool value)
{
    this->now_damaged = value;
}

bool CreatureEntity::is_playing() const
{
    return this->playing;
}

void CreatureEntity::set_playing(bool value)
{
    this->playing = value;
}

bool CreatureEntity::is_leaving() const
{
    return this->leaving;
}

void CreatureEntity::set_leaving(bool value)
{
    this->leaving = value;
}

bool CreatureEntity::get_monk_notify_aux() const
{
    return this->monk_notify_aux;
}

void CreatureEntity::set_monk_notify_aux(bool value)
{
    this->monk_notify_aux = value;
}

bool CreatureEntity::is_teleport_town() const
{
    return this->teleport_town;
}

void CreatureEntity::set_teleport_town(bool value)
{
    this->teleport_town = value;
}

BIT_FLAGS CreatureEntity::get_yoiyami() const
{
    return this->yoiyami;
}

void CreatureEntity::set_yoiyami(BIT_FLAGS value)
{
    this->yoiyami = value;
}

bool CreatureEntity::is_sutemi() const
{
    return this->sutemi;
}

void CreatureEntity::set_sutemi(bool value)
{
    this->sutemi = value;
}

DIRECTION CreatureEntity::get_fishing_dir() const
{
    return this->fishing_dir;
}

void CreatureEntity::set_fishing_dir(DIRECTION value)
{
    this->fishing_dir = value;
}

int32_t CreatureEntity::get_dealt_damage() const
{
    return this->dealt_damage;
}

void CreatureEntity::set_dealt_damage(int32_t value)
{
    this->dealt_damage = value;
}

void CreatureEntity::add_dealt_damage(int32_t delta)
{
    this->dealt_damage += delta;
}

POSITION CreatureEntity::get_run_py() const
{
    return this->run_py;
}

void CreatureEntity::set_run_py(POSITION value)
{
    this->run_py = value;
}

POSITION CreatureEntity::get_run_px() const
{
    return this->run_px;
}

void CreatureEntity::set_run_px(POSITION value)
{
    this->run_px = value;
}

bool CreatureEntity::is_vanish_stairs_flag() const
{
    return this->vanish_stairs_flag;
}

void CreatureEntity::set_vanish_stairs_flag(bool value)
{
    this->vanish_stairs_flag = value;
}

bool CreatureEntity::is_suppress_multi_reward() const
{
    return this->suppress_multi_reward;
}

void CreatureEntity::set_suppress_multi_reward(bool value)
{
    this->suppress_multi_reward = value;
}

short CreatureEntity::get_tracking_bi_id() const
{
    return this->tracking_bi_id;
}

void CreatureEntity::set_tracking_bi_id(short value)
{
    this->tracking_bi_id = value;
}

ItemKindType CreatureEntity::get_tval_ammo() const
{
    return this->tval_ammo;
}

void CreatureEntity::set_tval_ammo(ItemKindType value)
{
    this->tval_ammo = value;
}

bool CreatureEntity::is_dtrap() const
{
    return this->dtrap;
}

void CreatureEntity::set_dtrap(bool value)
{
    this->dtrap = value;
}

bool CreatureEntity::is_autopick_autoregister() const
{
    return this->autopick_autoregister;
}

void CreatureEntity::set_autopick_autoregister(bool value)
{
    this->autopick_autoregister = value;
}

DungeonId CreatureEntity::get_recall_dungeon() const
{
    return this->recall_dungeon;
}

void CreatureEntity::set_recall_dungeon(DungeonId value)
{
    this->recall_dungeon = value;
}

ENERGY CreatureEntity::get_enchant_energy_need() const
{
    return this->enchant_energy_need;
}

void CreatureEntity::set_enchant_energy_need(ENERGY value)
{
    this->enchant_energy_need = value;
}

void CreatureEntity::add_enchant_energy_need(ENERGY delta)
{
    this->enchant_energy_need += delta;
}

void CreatureEntity::sub_enchant_energy_need(ENERGY delta)
{
    this->enchant_energy_need -= delta;
}

ENERGY CreatureEntity::get_energy_use() const
{
    return this->energy_use;
}

void CreatureEntity::set_energy_use(ENERGY value)
{
    this->energy_use = value;
}

void CreatureEntity::add_energy_use(ENERGY delta)
{
    this->energy_use += delta;
}

void CreatureEntity::sub_energy_use(ENERGY delta)
{
    this->energy_use -= delta;
}

void CreatureEntity::mul_energy_use(ENERGY factor)
{
    this->energy_use *= factor;
}

void CreatureEntity::div_energy_use(ENERGY divisor)
{
    this->energy_use /= divisor;
}

void CreatureEntity::set_age(int16_t value)
{
    this->age = value;
}

void CreatureEntity::add_age(int16_t delta)
{
    this->age += delta;
}

void CreatureEntity::set_ht(int16_t value)
{
    this->ht = value;
}

void CreatureEntity::set_wt(int16_t value)
{
    this->wt = value;
}

void CreatureEntity::set_prestige(int16_t value)
{
    this->prestige = value;
}

void CreatureEntity::add_prestige(int16_t delta)
{
    this->prestige += delta;
}

void CreatureEntity::divide_prestige(int divisor)
{
    if (divisor != 0) {
        this->prestige = static_cast<int16_t>(this->prestige / divisor);
    }
}

void CreatureEntity::set_ambush_flag(bool value)
{
    this->ambush_flag = value;
}

void CreatureEntity::set_food(int16_t value)
{
    this->food = value;
}

void CreatureEntity::set_town_num(int16_t value)
{
    this->town_num = value;
}

void CreatureEntity::set_level(int16_t value)
{
    this->level = value;
}

void CreatureEntity::set_max_plv(int16_t value)
{
    this->max_plv = value;
}

void CreatureEntity::set_mutant_regenerate_mod(PERCENTAGE value)
{
    this->mutant_regenerate_mod = value;
}

void CreatureEntity::set_learned_spells(int16_t value)
{
    this->learned_spells = value;
}

void CreatureEntity::set_add_spells(int16_t value)
{
    this->add_spells = value;
}

void CreatureEntity::set_easy_2weapon(BIT_FLAGS value)
{
    this->easy_2weapon = value;
}

void CreatureEntity::set_down_saving(BIT_FLAGS value)
{
    this->down_saving = value;
}

void CreatureEntity::set_max_mp(int value)
{
    this->max_mp = value;
}

void CreatureEntity::set_exp(EXP value)
{
    this->exp = value;
}

void CreatureEntity::set_max_exp(EXP value)
{
    this->max_exp = value;
}

void CreatureEntity::set_max_max_exp(EXP value)
{
    this->max_max_exp = value;
}

void CreatureEntity::set_au(int value)
{
    this->au = value;
}

void CreatureEntity::add_au(int delta)
{
    this->au += delta;
}

void CreatureEntity::sub_au(int delta)
{
    this->au -= delta;
}

void CreatureEntity::divide_au(int divisor)
{
    if (divisor != 0) {
        this->au /= divisor;
    }
}

void CreatureEntity::set_current_mp(int value)
{
    this->current_mp = value;
}

void CreatureEntity::add_current_mp(int delta)
{
    this->current_mp += delta;
}

void CreatureEntity::sub_current_mp(int delta)
{
    this->current_mp -= delta;
}

int CreatureEntity::get_au() const
{
    return this->au;
}

int CreatureEntity::get_current_mp() const
{
    return this->current_mp;
}

int16_t CreatureEntity::get_food() const
{
    return this->food;
}

int16_t CreatureEntity::get_town_num() const
{
    return this->town_num;
}

int16_t CreatureEntity::get_age() const
{
    return this->age;
}

int16_t CreatureEntity::get_ht() const
{
    return this->ht;
}

int16_t CreatureEntity::get_wt() const
{
    return this->wt;
}

int16_t CreatureEntity::get_prestige() const
{
    return this->prestige;
}

int16_t CreatureEntity::get_max_plv() const
{
    return this->max_plv;
}

PERCENTAGE CreatureEntity::get_mutant_regenerate_mod() const
{
    return this->mutant_regenerate_mod;
}

int16_t CreatureEntity::get_learned_spells() const
{
    return this->learned_spells;
}

int16_t CreatureEntity::get_add_spells() const
{
    return this->add_spells;
}

BIT_FLAGS CreatureEntity::get_easy_2weapon() const
{
    return this->easy_2weapon;
}

BIT_FLAGS CreatureEntity::get_down_saving() const
{
    return this->down_saving;
}

bool CreatureEntity::has_knowledge(BIT_FLAGS8 flag) const
{
    return (this->knowledge & flag) != 0;
}

void CreatureEntity::add_knowledge(BIT_FLAGS8 flag)
{
    this->knowledge |= flag;
}

void CreatureEntity::remove_knowledge(BIT_FLAGS8 flag)
{
    this->knowledge &= ~flag;
}

BIT_FLAGS8 CreatureEntity::get_knowledge() const
{
    return this->knowledge;
}

void CreatureEntity::set_knowledge(BIT_FLAGS8 value)
{
    this->knowledge = value;
}

int CreatureEntity::get_max_mp() const
{
    return this->max_mp;
}

EXP CreatureEntity::get_exp() const
{
    return this->exp;
}

// ==== 提案 E4: creature-entity.h からの inline virtual accessor 本体移設 ====

Pos2D CreatureEntity::get_position() const
{
    return Pos2D(this->y, this->x);
}

POSITION CreatureEntity::get_x() const
{
    return this->x;
}

POSITION CreatureEntity::get_y() const
{
    return this->y;
}

Pos2D CreatureEntity::get_old_position() const
{
    return Pos2D(this->oldpy, this->oldpx);
}

bool CreatureEntity::is_located_at(const Pos2D &pos) const
{
    return (this->y == pos.y) && (this->x == pos.x);
}

int CreatureEntity::get_current_hp() const
{
    return this->hp;
}

int CreatureEntity::get_max_hp() const
{
    return this->maxhp;
}

int CreatureEntity::get_max_maxhp() const
{
    return this->max_maxhp;
}

int CreatureEntity::get_maxhp_reduction() const
{
    return 0;
}

int CreatureEntity::get_speed() const
{
    return this->speed;
}

void CreatureEntity::set_speed(int new_speed)
{
    this->speed = new_speed;
}

bool CreatureEntity::is_dead() const
{
    return this->hp < 0;
}

FloorType *CreatureEntity::get_floor() const
{
    return this->current_floor_ptr;
}

player_sex CreatureEntity::get_psex() const
{
    return this->psex;
}

player_personality_type CreatureEntity::get_ppersonality() const
{
    return this->ppersonality;
}

PlayerRaceType CreatureEntity::get_prace() const
{
    return this->prace;
}

PlayerClassType CreatureEntity::get_pclass() const
{
    return this->pclass;
}

RealmType CreatureEntity::get_realm1() const
{
    return this->realm1;
}

RealmType CreatureEntity::get_realm2() const
{
    return this->realm2;
}

ElementRealmType CreatureEntity::get_element_realm() const
{
    return this->element_realm;
}

int16_t CreatureEntity::get_patron() const
{
    return this->patron;
}

void CreatureEntity::set_psex(player_sex value)
{
    this->psex = value;
}

void CreatureEntity::set_ppersonality(player_personality_type value)
{
    this->ppersonality = value;
}

void CreatureEntity::set_prace(PlayerRaceType value)
{
    this->prace = value;
}

void CreatureEntity::set_pclass(PlayerClassType value)
{
    this->pclass = value;
}

void CreatureEntity::set_realm1(RealmType value)
{
    this->realm1 = value;
}

void CreatureEntity::set_realm2(RealmType value)
{
    this->realm2 = value;
}

void CreatureEntity::set_element_realm(ElementRealmType value)
{
    this->element_realm = value;
}

void CreatureEntity::set_patron(int16_t value)
{
    this->patron = value;
}

ACTION_ENERGY CreatureEntity::get_energy_need() const
{
    return this->energy_need;
}

void CreatureEntity::set_energy_need(ACTION_ENERGY energy)
{
    this->energy_need = energy;
}

void CreatureEntity::add_energy_need(ACTION_ENERGY delta)
{
    this->energy_need += delta;
}

void CreatureEntity::sub_energy_need(ACTION_ENERGY delta)
{
    this->energy_need -= delta;
}

bool CreatureEntity::is_player() const
{
    return false;
}

void CreatureEntity::on_take_hit(int damage)
{
    this->dealt_damage += damage;
    if (this->dealt_damage > this->max_maxhp * 100) {
        this->dealt_damage = this->max_maxhp * 100;
    }
}

bool CreatureEntity::calc_damage_reduction(int &damage, [[maybe_unused]] int damage_type)
{
    (void)damage;
    return false;
}

bool CreatureEntity::should_skip_natural_regen() const
{
    return false;
}

int CreatureEntity::apply_state_regen_modifier(int amount) const
{
    return amount;
}

int CreatureEntity::apply_creature_specific_regen_modifier(int amount) const
{
    return amount;
}

void CreatureEntity::reset_chameleon_polymorph()
{
    const auto real_id = this->get_real_monrace_id();
    this->r_idx = real_id;
    this->ap_r_idx = real_id;
}

bool CreatureEntity::is_pet() const
{
    return this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::PET);
}

bool CreatureEntity::is_visible_on_map() const
{
    return this->has_monster_profile() && this->get_monster_profile().ml;
}

void CreatureEntity::set_visible_on_map(bool value)
{
    if (this->has_monster_profile()) {
        this->get_monster_profile().ml = value;
    }
}
