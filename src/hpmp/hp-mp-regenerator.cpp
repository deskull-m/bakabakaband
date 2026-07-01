#include "hpmp/hp-mp-regenerator.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-slot-types.h"
#include "monster/monster-status.h"
#include "object-enchant/trc-types.h"
#include "player-base/player-class.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-status-table.h"
#include "player/special-defense-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "tracking/health-bar-tracker.h"

/*!<広域マップ移動時の自然回復処理カウンタ（広域マップ1マス毎に20回処理を基本とする）*/
int wild_regen = 20;

/*!
 * @brief 共通の自然回復量を算出する。プレイヤー基準の計算に揃え、モンスターでも同形で
 *        評価できる係数 (regen_amount) を返す。
 *
 * - 構え・行動による完全停止判定は should_skip_natural_regen() (virtual)
 * - ベース回復量は get_base_natural_regen_amount() (virtual)。プレイヤーは満腹度に応じた値を返す
 * - 構え・呪いによる減衰は apply_state_regen_modifier() (virtual)
 * - ミュータント体質等のクリーチャー固有要因は apply_creature_specific_regen_modifier() (virtual)
 * - 毒・切り傷・再生種族フラグ・行動 (search/rest)・地形衛生はプレイヤー・モンスター共通で適用
 * - regenhp() / regenmana() / 表示用 calculate_*_regen_rate() で同じ値を使うことで
 *   プレイヤー・モンスター・c画面表示を一貫させる。
 */
int compute_regen_amount(CreatureEntity &creature)
{
    if (creature.should_skip_natural_regen()) {
        return 0;
    }

    int regen_amount = creature.get_base_natural_regen_amount();

    if (creature.is_poisoned() || creature.is_cut()) {
        regen_amount = 0;
    }

    if (creature.has_regen_flag()) {
        regen_amount = regen_amount * 2;
    }

    regen_amount = creature.apply_state_regen_modifier(regen_amount);

    if ((creature.get_action() == ACTION_SEARCH) || (creature.get_action() == ACTION_REST)) {
        regen_amount = regen_amount * 2;
    }

    const auto &grid = creature.get_floor()->get_grid(creature.get_position());
    const auto &terrain = grid.get_terrain();
    if (regen_amount > 0 && terrain.hygiene != 0) {
        const int hygiene_modifier = 100 + terrain.hygiene;
        regen_amount = (regen_amount * hygiene_modifier) / 100;
        if (regen_amount < 0) {
            regen_amount = 0;
        }
    }

    return creature.apply_creature_specific_regen_modifier(regen_amount);
}

bool PlayerType::should_skip_natural_regen() const
{
    CreatureClass pc(const_cast<PlayerType &>(*this));
    if (pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        return true;
    }
    return this->get_action() == ACTION_HAYAGAKE;
}

int PlayerType::get_base_natural_regen_amount() const
{
    if (this->get_food() >= PY_FOOD_WEAK) {
        return PY_REGEN_NORMAL;
    }
    if (this->get_food() < PY_FOOD_STARVE) {
        return 0;
    }
    if (this->get_food() < PY_FOOD_FAINT) {
        return PY_REGEN_FAINT;
    }
    return PY_REGEN_WEAK;
}

int PlayerType::apply_state_regen_modifier(int amount) const
{
    CreatureClass pc(const_cast<PlayerType &>(*this));
    if (!pc.monk_stance_is(MonkStanceType::NONE) || !pc.samurai_stance_is(SamuraiStanceType::NONE)) {
        amount /= 2;
    }
    if (this->get_cursed_flags().has(CurseTraitType::SLOW_REGEN)) {
        amount /= 5;
    }
    return amount;
}

int PlayerType::apply_creature_specific_regen_modifier(int amount) const
{
    return (amount * this->get_mutant_regenerate_mod()) / 100;
}

/*!
 * @brief プレイヤーのHP自然回復処理 / Regenerate hit points -RAK-
 * @param percent 回復比率
 */
void regenhp(CreatureEntity &creature, int percent)
{
    if (CreatureClass(creature).samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        return;
    }
    if (creature.get_action() == ACTION_HAYAGAKE) {
        return;
    }

    int old_chp = creature.hp;

    /*
     * Extract the new hitpoints
     *
     * 'percent' is the Regen factor in unit (1/2^16)
     */
    int new_chp = 0;
    uint32_t new_chp_frac = (creature.maxhp * percent + PY_REGEN_HPBASE);
    s64b_lshift(&new_chp, &new_chp_frac, 16);
    s64b_add(&(creature.hp), &(creature.hp_frac), new_chp, new_chp_frac);
    if (0 < s64b_cmp(creature.hp, creature.hp_frac, creature.maxhp, 0)) {
        creature.hp = creature.maxhp;
        creature.hp_frac = 0;
    }

    if (old_chp != creature.hp) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::HP);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        wild_regen = 20;
    }
}

/*!
 * @brief プレイヤーのMP自然回復処理(regen_magic()のサブセット) / Regenerate mana points
 * @param upkeep_factor ペット維持によるMPコスト量
 * @param regen_amount 回復量
 */
void regenmana(CreatureEntity &creature, MANA_POINT upkeep_factor, MANA_POINT regen_amount)
{
    MANA_POINT old_current_mp = creature.get_current_mp();
    int32_t regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

    /*
     * Excess mana will decay 32 times faster than normal
     * regeneration rate.
     */
    if (creature.get_current_mp() > creature.get_max_mp()) {
        int32_t decay = 0;
        uint32_t decay_frac = (creature.get_max_mp() * 32 * PY_REGEN_NORMAL + PY_REGEN_MNBASE);
        s64b_lshift(&decay, &decay_frac, 16);
        creature.sub_current_mp_with_frac(decay, decay_frac);
        if (creature.get_current_mp() < creature.get_max_mp()) {
            creature.set_current_mp(creature.get_max_mp());
            creature.current_mp_frac = 0;
        }
    }

    /* Regenerating mana (unless the creature has excess mana) */
    else if (regen_rate > 0) {
        MANA_POINT new_mana = 0;
        uint32_t new_mana_frac = (creature.get_max_mp() * regen_rate / 100 + PY_REGEN_MNBASE);
        s64b_lshift(&new_mana, &new_mana_frac, 16);
        creature.add_current_mp_with_frac(new_mana, new_mana_frac);
        if (creature.get_current_mp() >= creature.get_max_mp()) {
            creature.set_current_mp(creature.get_max_mp());
            creature.current_mp_frac = 0;
        }
    }

    /* Reduce mana (even when the creature has excess mana) */
    if (regen_rate < 0) {
        int32_t reduce_mana = 0;
        uint32_t reduce_mana_frac = (creature.get_max_mp() * (-1) * regen_rate / 100 + PY_REGEN_MNBASE);
        s64b_lshift(&reduce_mana, &reduce_mana_frac, 16);
        creature.sub_current_mp_with_frac(reduce_mana, reduce_mana_frac);
        if (creature.get_current_mp() < 0) {
            creature.set_current_mp(0);
            creature.current_mp_frac = 0;
        }
    }

    if (old_current_mp != creature.get_current_mp()) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::MP);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags);
        wild_regen = 20;
    }
}

/*!
 * @brief 取り込んだ魔道具の自然回復処理 / Regenerate magic regen_amount: PY_REGEN_NORMAL * 2 (if resting) * 2 (if having regenarate)
 * @param regen_amount 回復量
 */
void regenmagic(CreatureEntity &creature, int regen_amount)
{
    auto magic_eater_data = CreatureClass(creature).get_specific_data<MagicEaterDataList>();
    if (!magic_eater_data) {
        return;
    }

    const int dev = 30;
    const int mult = (dev + adj_mag_mana[creature.get_stat_index(A_INT)]); /* x1 to x2 speed bonus for recharging */

    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND }) {
        for (auto &item : magic_eater_data->get_item_group(tval)) {
            const int maximum_charge = item.count * EATER_CHARGE;
            if (item.count == 0 || item.charge == maximum_charge) {
                continue;
            }

            /* Increase remaining charge number like float value */
            auto new_mana = (regen_amount * mult * (item.count + 13)) / (dev * 8);
            item.charge += new_mana;

            /* Check maximum charge */
            item.charge = std::min(item.charge, maximum_charge);
        }
    }

    for (auto &item : magic_eater_data->get_item_group(ItemKindType::ROD)) {
        if (item.count == 0 || item.charge == 0) {
            continue;
        }

        /* Decrease remaining period for charging */
        auto new_mana = (regen_amount * mult * (item.count + 10) * EATER_ROD_CHARGE) / (dev * 16 * PY_REGEN_NORMAL);
        item.charge -= new_mana;

        /* Check minimum remaining period for charging */
        item.charge = std::max(item.charge, 0);
    }

    wild_regen = 20;
}

/*!
 * @brief 100ゲームターン毎のモンスターのHP/MP自然回復処理 / Regenerate the monsters (once per 100 game turns)
 * @param creature クリーチャーへの参照
 * @note Should probably be done during monster turns.
 * @details
 * プレイヤー側 (process_player_hp_mp) と同じ compute_regen_amount() で
 * regen_amount を算出し、regenhp() / regenmana() に流す。
 * 本処理は 100 ターン毎、プレイヤー側は 10 ターン毎に走るため
 * 1 回あたりの強度を 10 倍してプレイヤーの 10 ティック分を一度に補正する
 * (1 ターン平均の回復速度はプレイヤー基準と一致)。
 */
void regenerate_monsters(CreatureEntity &creature)
{
    constexpr int monster_regen_tick_scale = 10;
    auto &tracker = HealthBarTracker::get_instance();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    for (short i = 1; i < creature.get_floor()->m_max; i++) {
        auto &monster = creature.get_floor()->get_monster(i);
        if (!monster.is_valid()) {
            continue;
        }

        const auto regen_amount = compute_regen_amount(monster) * monster_regen_tick_scale;
        if (regen_amount <= 0) {
            continue;
        }

        const auto old_hp = monster.hp;
        if (monster.hp < monster.maxhp) {
            regenhp(monster, regen_amount);
        }
        if (monster.get_current_mp() < monster.get_max_mp()) {
            regenmana(monster, 0, regen_amount);
        }

        if (old_hp != monster.hp) {
            tracker.set_flag_if_tracking(i);
            if (monster.is_riding()) {
                rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
            }
        }
    }
}

/*!
 * @brief 30ゲームターン毎のボール中モンスターのHP自然回復処理 / Regenerate the captured monsters (once per 30 game turns)
 * @param creature クリーチャーへの参照
 * @note Should probably be done during monster turns.
 */
void regenerate_captured_monsters(CreatureEntity &creature)
{
    bool heal = false;
    for (const auto i_idx : INVEN_ALL_SLOTS) {
        auto *o_ptr = creature.inventory[i_idx].get();
        if (!o_ptr->is_valid()) {
            continue;
        }
        if (o_ptr->bi_key.tval() != ItemKindType::CAPTURE) {
            continue;
        }
        if (!o_ptr->pval) {
            continue;
        }

        heal = true;
        const auto &monrace = o_ptr->get_monrace();
        if (o_ptr->captured_monster_current_hp < o_ptr->captured_monster_max_hp) {
            short frac = o_ptr->captured_monster_max_hp / 100;
            if (!frac) {
                if (one_in_(2)) {
                    frac = 1;
                }
            }

            if (monrace.misc_flags.has(MonsterMiscType::REGENERATE)) {
                frac *= 2;
            }

            // Apply hygiene-based regeneration modifier (based on creature's current terrain)
            const auto &grid = creature.get_floor()->get_grid(creature.get_position());
            const auto &terrain = grid.get_terrain();
            if (terrain.hygiene != 0) {
                const int hygiene_modifier = 100 + terrain.hygiene;
                frac = (frac * hygiene_modifier) / 100;
                if (frac < 0) {
                    frac = 0;
                }
            }

            o_ptr->captured_monster_current_hp += frac;
            if (o_ptr->captured_monster_current_hp > o_ptr->captured_monster_max_hp) {
                o_ptr->captured_monster_current_hp = o_ptr->captured_monster_max_hp;
            }
        }
    }

    if (heal) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::COMBINATION);

        /*!
         * @todo FIXME 広域マップ移動で1歩毎に何度も再描画されて重くなる.
         * 現在はボール中モンスターのHP回復でボールの表示は変わらないためコメントアウトする.
         */
        // rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        // rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
        wild_regen = 20;
    }
}
