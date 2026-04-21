#include "player/player-status.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-magiceat.h"
#include "combat/attack-power-table.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-leaver.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-calculator.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "pet/pet-util.h"
#include "player-ability/player-charisma.h"
#include "player-ability/player-constitution.h"
#include "player-ability/player-dexterity.h"
#include "player-ability/player-intelligence.h"
#include "player-ability/player-strength.h"
#include "player-ability/player-wisdom.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/alignment.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/mimic-info-table.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-basic-statistics.h"
#include "player-status/player-hand-types.h"
#include "player-status/player-infravision.h"
#include "player-status/player-speed.h"
#include "player-status/player-stealth.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/patron.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personality-types.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-spell-status.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-view.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "specific-object/torch.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/range-calc.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "status/base-status.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/services/dungeon-service.h"
#include "system/terrain/terrain-definition.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

static bool is_martial_arts_mode(CreatureEntity &creature);

static ACTION_SKILL_POWER calc_disarming(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_device_ability(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_saving_throw(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_search(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_search_freq(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_to_hit_melee(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_to_hit_shoot(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_to_hit_throw(CreatureEntity &creature);
static ACTION_SKILL_POWER calc_skill_dig(CreatureEntity &creature);
static bool is_heavy_wield(CreatureEntity &creature, int i);
static int16_t calc_num_blow(CreatureEntity &creature, int i);
static int16_t calc_to_magic_chance(CreatureEntity &creature);
static ARMOUR_CLASS calc_base_ac(CreatureEntity &creature);
static ARMOUR_CLASS calc_to_ac(CreatureEntity &creature, bool is_real_value);
static int16_t calc_double_weapon_penalty(CreatureEntity &creature, INVENTORY_IDX slot);
static bool is_riding_two_hands(CreatureEntity &creature);
static int16_t calc_riding_bow_penalty(CreatureEntity &creature);
static void put_equipment_warning(CreatureEntity &creature);

static short calc_to_damage(CreatureEntity &creature, INVENTORY_IDX slot, bool is_real_value);
static int16_t calc_to_hit(CreatureEntity &creature, INVENTORY_IDX slot, bool is_real_value);

static int16_t calc_to_hit_bow(CreatureEntity &creature, bool is_real_value);

static int16_t calc_to_damage_misc(CreatureEntity &creature);
static int16_t calc_to_hit_misc(CreatureEntity &creature);

static int calc_to_weapon_dice_num(CreatureEntity &creature, INVENTORY_IDX slot);
static player_hand main_attack_hand(CreatureEntity &creature);

/*** Player information ***/

/*!
 * @brief 遅延描画更新 / Delayed visual update
 * @details update_view(), update_lite(), update_mon_lite() においてのみ更新すること / Only used if update_view(), update_lite() or update_mon_lite() was called
 * @param creature 主観となるクリーチャーへの参照
 * @todo 将来独自インターフェース実装にはz-term系に追い出すべきか？
 */
static void delayed_visual_update(CreatureEntity &creature)
{
    auto &floor = *creature.get_floor();
    const auto points = floor.collect_redraw_points();
    for (const auto &pos : points) {
        auto &grid = floor.get_grid(pos);
        if (any_bits(grid.info, CAVE_NOTE)) {
            note_spot(creature, pos);
        }

        lite_spot(creature, pos);
        if (grid.has_monster()) {
            update_monster(creature, grid.m_idx, false);
        }

        reset_bits(grid.info, (CAVE_NOTE | CAVE_REDRAW));
    }
}

/*!
 * @brief 射撃武器がプレイヤーにとって重すぎるかどうかの判定 /
 * @param o_ptr 判定する射撃武器のアイテム情報参照ポインタ
 * @return 重すぎるならばTRUE
 */
static bool is_heavy_shoot(CreatureEntity &creature, const ItemEntity *o_ptr)
{
    return calc_bow_weight_limit(creature) < (o_ptr->weight / 10);
}

/*!
 * @brief 所持品総重量を計算する
 * @param creature クリーチャーへの参照
 * @return 総重量
 */
int calc_inventory_weight(CreatureEntity &creature)
{
    auto weight = 0;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        const auto &item = *creature.inventory[i];
        if (!item.is_valid()) {
            continue;
        }

        weight += item.weight * item.number;
    }

    return weight;
}

static void update_ability_scores(CreatureEntity &creature)
{
    PlayerStrength player_str(creature);
    PlayerIntelligence player_int(creature);
    PlayerWisdom player_wis(creature);
    PlayerDexterity player_dex(creature);
    PlayerConstitution player_con(creature);
    PlayerCharisma player_chr(creature);
    PlayerBasicStatistics *player_stats[] = { &player_str, &player_int, &player_wis, &player_dex, &player_con, &player_chr };
    for (auto i = 0; i < A_MAX; ++i) {
        creature.stat_add[i] = player_stats[i]->modification_value();
        player_stats[i]->update_value();
    }
}

/*!
 * @brief プレイヤーの全ステータスを更新する /
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 * @details
 * <pre>
 * See also update_max_mana() and update_max_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * creature will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the creature, hurts
 * because in the old days a creature could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 * </pre>
 * @todo ここで計算していた各値は一部の状態変化メッセージ処理を除き、今後必要な時に適示計算する形に移行するためほぼすべて削られる。
 */
static void update_bonuses(CreatureEntity &creature)
{
    auto empty_hands_status = empty_hands(creature, true);
    ItemEntity *o_ptr;

    /* Save the old vision stuff */
    BIT_FLAGS old_telepathy = creature.telepathy;
    BIT_FLAGS old_esp_animal = creature.esp_animal;
    BIT_FLAGS old_esp_undead = creature.esp_undead;
    BIT_FLAGS old_esp_demon = creature.esp_demon;
    BIT_FLAGS old_esp_orc = creature.esp_orc;
    BIT_FLAGS old_esp_troll = creature.esp_troll;
    BIT_FLAGS old_esp_giant = creature.esp_giant;
    BIT_FLAGS old_esp_dragon = creature.esp_dragon;
    BIT_FLAGS old_esp_human = creature.esp_human;
    BIT_FLAGS old_esp_evil = creature.esp_evil;
    BIT_FLAGS old_esp_good = creature.esp_good;
    BIT_FLAGS old_esp_nonliving = creature.esp_nonliving;
    BIT_FLAGS old_esp_unique = creature.esp_unique;
    BIT_FLAGS old_see_inv = creature.see_inv;
    BIT_FLAGS old_mighty_throw = creature.mighty_throw;
    int16_t old_speed = creature.get_speed();

    ARMOUR_CLASS old_dis_ac = creature.dis_ac;
    ARMOUR_CLASS old_dis_to_a = creature.dis_to_a;

    creature.xtra_might = has_xtra_might(creature);
    creature.esp_evil = has_esp_evil(creature);
    creature.esp_animal = has_esp_animal(creature);
    creature.esp_nasty = has_esp_nasty(creature);
    creature.esp_homo = has_esp_homo(creature);
    creature.esp_undead = has_esp_undead(creature);
    creature.esp_demon = has_esp_demon(creature);
    creature.esp_orc = has_esp_orc(creature);
    creature.esp_troll = has_esp_troll(creature);
    creature.esp_giant = has_esp_giant(creature);
    creature.esp_dragon = has_esp_dragon(creature);
    creature.esp_human = has_esp_human(creature);
    creature.esp_good = has_esp_good(creature);
    creature.esp_nonliving = has_esp_nonliving(creature);
    creature.esp_unique = has_esp_unique(creature);
    creature.telepathy = has_esp_telepathy(creature);
    creature.bless_blade = has_bless_blade(creature);
    creature.easy_2weapon = has_easy2_weapon(creature);
    creature.down_saving = has_down_saving(creature);
    creature.yoiyami = has_no_ac(creature);
    creature.mighty_throw = has_mighty_throw(creature);
    creature.dec_mana = has_dec_mana(creature);
    creature.see_nocto = has_see_nocto(creature);
    creature.warning = has_warning(creature);
    creature.anti_magic = has_anti_magic(creature);
    creature.anti_tele = has_anti_tele(creature);
    creature.easy_spell = has_easy_spell(creature);
    creature.hard_spell = has_hard_spell(creature);
    creature.hold_exp = has_hold_exp(creature);
    creature.see_inv = has_see_inv(creature);
    creature.free_act = has_free_act(creature);
    creature.levitation = has_levitation(creature);
    creature.can_swim = has_can_swim(creature);
    creature.slow_digest = has_slow_digest(creature);
    creature.regenerate = has_regenerate(creature);
    update_curses(creature);
    creature.impact = has_impact(creature);
    creature.earthquake = has_earthquake(creature);
    update_extra_blows(creature);

    creature.lite = has_lite(creature);

    if (!CreatureClass(creature).monk_stance_is(MonkStanceType::NONE)) {
        if (none_bits(empty_hands_status, EMPTY_HAND_MAIN)) {
            set_action(creature, ACTION_NONE);
        }
    }

    update_ability_scores(creature);
    o_ptr = creature.inventory[INVEN_BOW].get();
    if (o_ptr->is_valid()) {
        creature.tval_ammo = o_ptr->get_arrow_kind();
        creature.num_fire = calc_num_fire(creature, o_ptr);
    }

    for (int i = 0; i < 2; i++) {
        creature.is_icky_wield[i] = is_wielding_icky_weapon(creature, i);
        creature.is_icky_riding_wield[i] = is_wielding_icky_riding_weapon(creature, i);
        creature.heavy_wield[i] = is_heavy_wield(creature, i);
        creature.num_blow[i] = calc_num_blow(creature, i);
        creature.damage_dice_bonus[i].num = calc_to_weapon_dice_num(creature, INVEN_MAIN_HAND + i);
        creature.damage_dice_bonus[i].sides = 0;
    }

    creature.set_speed(PlayerSpeed(creature).get_value());
    creature.see_infra = PlayerInfravision(creature).get_value();
    creature.skill_stl = PlayerStealth(creature).get_value();
    creature.skill_dis = calc_disarming(creature);
    creature.skill_dev = calc_device_ability(creature);
    creature.skill_sav = calc_saving_throw(creature);
    creature.skill_srh = calc_search(creature);
    creature.skill_fos = calc_search_freq(creature);
    creature.skill_thn = calc_to_hit_melee(creature);
    creature.skill_thb = calc_to_hit_shoot(creature);
    creature.skill_tht = calc_to_hit_throw(creature);
    creature.riding_ryoute = is_riding_two_hands(creature);
    creature.to_d[0] = calc_to_damage(creature, INVEN_MAIN_HAND, true);
    creature.to_d[1] = calc_to_damage(creature, INVEN_SUB_HAND, true);
    creature.dis_to_d[0] = calc_to_damage(creature, INVEN_MAIN_HAND, false);
    creature.dis_to_d[1] = calc_to_damage(creature, INVEN_SUB_HAND, false);
    creature.to_h[0] = calc_to_hit(creature, INVEN_MAIN_HAND, true);
    creature.to_h[1] = calc_to_hit(creature, INVEN_SUB_HAND, true);
    creature.dis_to_h[0] = calc_to_hit(creature, INVEN_MAIN_HAND, false);
    creature.dis_to_h[1] = calc_to_hit(creature, INVEN_SUB_HAND, false);
    creature.to_h_b = calc_to_hit_bow(creature, true);
    creature.dis_to_h_b = calc_to_hit_bow(creature, false);
    creature.to_d_m = calc_to_damage_misc(creature);
    creature.to_h_m = calc_to_hit_misc(creature);
    creature.skill_dig = calc_skill_dig(creature);
    creature.to_m_chance = calc_to_magic_chance(creature);
    creature.ac = calc_base_ac(creature);
    creature.to_a = calc_to_ac(creature, true);
    creature.dis_ac = calc_base_ac(creature);
    creature.dis_to_a = calc_to_ac(creature, false);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (old_mighty_throw != creature.mighty_throw) {
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
    }

    if (creature.telepathy != old_telepathy) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    auto is_esp_updated = creature.esp_animal != old_esp_animal;
    is_esp_updated |= creature.esp_undead != old_esp_undead;
    is_esp_updated |= creature.esp_demon != old_esp_demon;
    is_esp_updated |= creature.esp_orc != old_esp_orc;
    is_esp_updated |= creature.esp_troll != old_esp_troll;
    is_esp_updated |= creature.esp_giant != old_esp_giant;
    is_esp_updated |= creature.esp_dragon != old_esp_dragon;
    is_esp_updated |= creature.esp_human != old_esp_human;
    is_esp_updated |= creature.esp_evil != old_esp_evil;
    is_esp_updated |= creature.esp_good != old_esp_good;
    is_esp_updated |= creature.esp_nonliving != old_esp_nonliving;
    is_esp_updated |= creature.esp_unique != old_esp_unique;
    if (is_esp_updated) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    if (creature.see_inv != old_see_inv) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    if (creature.get_speed() != old_speed) {
        rfu.set_flag(MainWindowRedrawingFlag::SPEED);
    }

    if ((creature.dis_ac != old_dis_ac) || (creature.dis_to_a != old_dis_to_a)) {
        rfu.set_flag(MainWindowRedrawingFlag::AC);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    }

    if (AngbandWorld::get_instance().character_xtra) {
        return;
    }

    put_equipment_warning(creature);
    check_no_flowed(creature);
}

/*!
 * @brief プレイヤーの最大HPを更新する /
 * Update the players maximal hit points
 * Adjust current hitpoints if necessary
 * @details
 */
static void update_max_hitpoints(CreatureEntity &creature)
{
    int bonus = ((int)(adj_con_mhp[creature.stat_index[A_CON]]) - 128) * creature.level / 4;
    int mhp = creature.player_hp[creature.level - 1];

    CreatureClass pc(creature);
    auto is_sorcerer = pc.equals(PlayerClassType::SORCERER);
    if (creature.get_mimic_form() != MimicKindType::NONE) {
        auto r_mhp = mimic_info.at(creature.get_mimic_form()).r_mhp;
        const auto mimic_hit_dice = Dice(1, (is_sorcerer ? r_mhp / 2 : r_mhp) + (*creature.pclass_ref).c_mhp + (*creature.personality).a_mhp);
        mhp = mhp * mimic_hit_dice.maxroll() / creature.hit_dice.maxroll();
    }

    if (is_sorcerer) {
        if (creature.level < 30) {
            mhp = (mhp * (45 + creature.level) / 100);
        } else {
            mhp = (mhp * 75 / 100);
        }
        bonus = (bonus * 65 / 100);
    }

    mhp += bonus;

    if (pc.equals(PlayerClassType::BERSERKER)) {
        mhp = mhp * (110 + (((creature.level + 40) * (creature.level + 40) - 1550) / 110)) / 100;
    }

    if (mhp < creature.level + 1) {
        mhp = creature.level + 1;
    }
    if (creature.is_hero()) {
        mhp += 10;
    }
    if (creature.is_shero()) {
        mhp += 30;
    }
    if (creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI)) {
        mhp += 50;
    }
    if (SpellHex(creature).is_spelling_specific(HEX_XTRA_MIGHT)) {
        mhp += 15;
    }
    if (SpellHex(creature).is_spelling_specific(HEX_BUILDING)) {
        mhp += 60;
    }
    if (creature.maxhp == mhp) {
        return;
    }

    if (creature.hp >= mhp) {
        creature.hp = mhp;
        creature.hp_frac = 0;
    }

#ifdef JP
    if (creature.level_up_message && (mhp > creature.maxhp)) {
        msg_format("最大ヒット・ポイントが %d 増加した！", (mhp - creature.maxhp));
    }
#endif
    creature.maxhp = mhp;

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::HP);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
}

/*!
 * @brief プレイヤーの現在学習可能な魔法数を計算し、増減に応じて魔法の忘却、再学習を処置する。 /
 * Calculate number of spells creature should have, and forget,
 * or remember, spells until that number is properly reflected.
 * @details
 * Note that this function induces various "status" messages,
 * which must be bypasses until the character is created.
 */
static void update_num_of_spells(CreatureEntity &creature)
{
    const auto &world = AngbandWorld::get_instance();
    if ((mp_ptr->spell_book == ItemKindType::NONE) || !world.character_generated || world.character_xtra) {
        return;
    }

    CreatureClass pc(creature);
    if (pc.is_every_magic()) {
        creature.new_spells = 0;
        return;
    }

    const auto spell_category = spell_category_name(mp_ptr->spell_book);
    int levels = creature.level - mp_ptr->spell_first + 1;
    if (levels < 0) {
        levels = 0;
    }

    int num_allowed = (adj_mag_study[creature.stat_index[mp_ptr->spell_stat]] * levels / 2);
    int bonus = 0;
    if (!pc.equals(PlayerClassType::SAMURAI) && (mp_ptr->spell_book != ItemKindType::LIFE_BOOK)) {
        bonus = 4;
    }

    PlayerRealm pr(creature);
    if (pc.equals(PlayerClassType::SAMURAI)) {
        num_allowed = 32;
    } else if (!pr.realm2().is_available()) {
        num_allowed = (num_allowed + 1) / 2;
        if (num_allowed > (32 + bonus)) {
            num_allowed = 32 + bonus;
        }
    } else if (pc.equals(PlayerClassType::MAGE) || pc.equals(PlayerClassType::PRIEST)) {
        if (num_allowed > (96 + bonus)) {
            num_allowed = 96 + bonus;
        }
    } else {
        if (num_allowed > (80 + bonus)) {
            num_allowed = 80 + bonus;
        }
    }

    PlayerSpellStatus pss(creature);

    auto num_forgotten = 0;
    for (const auto &realm_status : { pss.realm1(), pss.realm2() }) {
        for (auto spell_id = 0; spell_id < 32; spell_id++) {
            if (realm_status.is_forgotten(spell_id)) {
                num_forgotten++;
            }
        }
    }

    creature.new_spells = num_allowed + creature.add_spells + num_forgotten - creature.learned_spells;

    for (auto it = creature.spell_order_learned.crbegin(); it != creature.spell_order_learned.crend(); ++it) {
        const auto is_realm1 = *it < 32;
        const auto spell_id = *it % 32;
        const auto &realm = is_realm1 ? pr.realm1() : pr.realm2();
        const auto &spell = realm.get_spell_info(spell_id);

        if (spell.slevel <= creature.level) {
            continue;
        }

        auto realm_status = is_realm1 ? pss.realm1() : pss.realm2();
        if (!realm_status.is_learned(spell_id)) {
            continue;
        }

        realm_status.set_forgotten(spell_id);
        realm_status.set_learned(spell_id, false);

        const auto &spell_name = realm.get_spell_name(spell_id);
#ifdef JP
        msg_format("%sの%sを忘れてしまった。", spell_name.data(), spell_category.data());
#else
        msg_format("You have forgotten the %s of %s.", spell_category.data(), spell_name.data());
#endif
        creature.new_spells++;
    }

    /* Forget spells if we know too many spells */
    for (auto it = creature.spell_order_learned.crbegin(); it != creature.spell_order_learned.crend(); ++it) {
        if (creature.new_spells >= 0) {
            break;
        }

        const auto is_realm1 = *it < 32;
        const auto spell_id = *it % 32;

        auto realm_status = is_realm1 ? pss.realm1() : pss.realm2();
        if (!realm_status.is_learned(spell_id)) {
            continue;
        }

        realm_status.set_forgotten(spell_id);
        realm_status.set_learned(spell_id, false);

        const auto &realm = is_realm1 ? pr.realm1() : pr.realm2();
        const auto &spell_name = realm.get_spell_name(spell_id);
#ifdef JP
        msg_format("%sの%sを忘れてしまった。", spell_name.data(), spell_category.data());
#else
        msg_format("You have forgotten the %s of %s.", spell_category.data(), spell_name.data());
#endif
        creature.new_spells++;
    }

    /* Check for spells to remember */
    for (const auto j : creature.spell_order_learned) {
        if (creature.new_spells <= 0) {
            break;
        }

        const auto is_realm1 = j < 32;
        const auto spell_id = j % 32;

        const auto &realm = is_realm1 ? pr.realm1() : pr.realm2();
        const auto &spell = realm.get_spell_info(spell_id);

        if (spell.slevel > creature.level) {
            continue;
        }

        auto realm_status = is_realm1 ? pss.realm1() : pss.realm2();
        if (!realm_status.is_forgotten(spell_id)) {
            continue;
        }

        realm_status.set_forgotten(spell_id, false);
        realm_status.set_learned(spell_id);

        const auto &spell_name = realm.get_spell_name(spell_id);
#ifdef JP
        msg_format("%sの%sを思い出した。", spell_name.data(), spell_category.data());
#else
        msg_format("You have remembered the %s of %s.", spell_category.data(), spell_name.data());
#endif
        creature.new_spells--;
    }

    if (!pr.realm2().is_available()) {
        int k = 0;
        for (int j = 0; j < 32; j++) {
            const auto &spell = pr.realm1().get_spell_info(j);

            if (spell.slevel > creature.level) {
                continue;
            }

            if (pss.realm1().is_learned(j)) {
                continue;
            }

            k++;
        }

        if (k > 32) {
            k = 32;
        }
        if ((creature.new_spells > k) && ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) || (mp_ptr->spell_book == ItemKindType::HISSATSU_BOOK))) {
            creature.new_spells = (int16_t)k;
        }
    }

    if (creature.new_spells < 0) {
        creature.new_spells = 0;
    }

    if (creature.old_spells == creature.new_spells) {
        return;
    }

    if (creature.new_spells) {
#ifdef JP
        if (creature.new_spells < 10) {
            msg_format("あと %d つの%sを学べる。", creature.new_spells, spell_category.data());
        } else {
            msg_format("あと %d 個の%sを学べる。", creature.new_spells, spell_category.data());
        }
#else
        msg_format("You can learn %d more %s%s.", creature.new_spells, spell_category.data(), (creature.new_spells != 1) ? "s" : "");
#endif
    }

    creature.old_spells = creature.new_spells;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::STUDY);
    rfu.set_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
}

/*!
 * @brief プレイヤーの最大MPを更新する /
 * Update maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor.
 * @details
 * This function induces status messages.
 */
static void update_max_mana(CreatureEntity &creature)
{
    if ((mp_ptr->spell_book == ItemKindType::NONE) && mp_ptr->spell_first == SPELL_FIRST_NO_SPELL) {
        return;
    }

    int levels;
    CreatureClass pc(creature);
    auto use_direct_level = pc.equals(PlayerClassType::MINDCRAFTER);
    use_direct_level |= pc.equals(PlayerClassType::MIRROR_MASTER);
    use_direct_level |= pc.equals(PlayerClassType::BLUE_MAGE);
    use_direct_level |= pc.equals(PlayerClassType::ELEMENTALIST);
    if (use_direct_level) {
        levels = creature.level;
    } else {
        if (mp_ptr->spell_first > creature.level) {
            creature.msp = 0;
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
            return;
        }

        levels = (creature.level - mp_ptr->spell_first) + 1;
    }

    int msp;
    if (pc.equals(PlayerClassType::SAMURAI)) {
        msp = (adj_mag_mana[creature.stat_index[mp_ptr->spell_stat]] + 10) * 2;
        if (msp) {
            msp += (msp * creature.race->r_adj[mp_ptr->spell_stat] / 20);
        }
    } else {
        msp = adj_mag_mana[creature.stat_index[mp_ptr->spell_stat]] * (levels + 3) / 4;
        if (msp) {
            msp++;
        }
        if (msp) {
            msp += (msp * creature.race->r_adj[mp_ptr->spell_stat] / 20);
        }
        if (msp && (creature.ppersonality == PERSONALITY_MUNCHKIN)) {
            msp += msp / 2;
        }
        if (msp && pc.equals(PlayerClassType::HIGH_MAGE)) {
            msp += msp / 4;
        }
        if (msp && pc.equals(PlayerClassType::SORCERER)) {
            msp += msp * (25 + creature.level) / 100;
        }
    }

    if (mp_ptr->has_glove_mp_penalty) {
        creature.cumber_glove = false;
        const auto *o_ptr = creature.inventory[INVEN_ARMS].get();
        const auto flags = o_ptr->get_flags();
        auto should_mp_decrease = o_ptr->is_valid();
        should_mp_decrease &= flags.has_not(TR_FREE_ACT);
        should_mp_decrease &= flags.has_not(TR_DEC_MANA);
        should_mp_decrease &= flags.has_not(TR_EASY_SPELL);
        should_mp_decrease &= flags.has_not(TR_MAGIC_MASTERY) || (o_ptr->pval <= 0);
        should_mp_decrease &= flags.has_not(TR_DEX) || (o_ptr->pval <= 0);
        if (should_mp_decrease) {
            creature.cumber_glove = true;
            msp = (3 * msp) / 4;
        }
    }

    creature.cumber_armor = false;

    auto cur_wgt = 0;
    const auto &item_main_hand = *creature.inventory[INVEN_MAIN_HAND];
    const auto tval_main = item_main_hand.bi_key.tval();
    if (tval_main > ItemKindType::SWORD) {
        cur_wgt += item_main_hand.weight;
    }

    const auto &item_sub_hand = *creature.inventory[INVEN_SUB_HAND];
    const auto tval_sub = item_sub_hand.bi_key.tval();
    if (item_sub_hand.bi_key.tval() > ItemKindType::SWORD) {
        cur_wgt += item_sub_hand.weight;
    }

    cur_wgt += creature.inventory[INVEN_BODY]->weight;
    cur_wgt += creature.inventory[INVEN_HEAD]->weight;
    cur_wgt += creature.inventory[INVEN_OUTER]->weight;
    cur_wgt += creature.inventory[INVEN_ARMS]->weight;
    cur_wgt += creature.inventory[INVEN_FEET]->weight;

    switch (creature.pclass) {
    case PlayerClassType::MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::BLUE_MAGE:
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
    case PlayerClassType::SORCERER:
    case PlayerClassType::ELEMENTALIST:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight;
        }

        break;
    case PlayerClassType::PRIEST:
    case PlayerClassType::BARD:
    case PlayerClassType::TOURIST:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight * 2 / 3;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight * 2 / 3;
        }

        break;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::BEASTMASTER:
    case PlayerClassType::MIRROR_MASTER:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight / 2;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight / 2;
        }

        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::RANGER:
    case PlayerClassType::RED_MAGE:
    case PlayerClassType::WARRIOR_MAGE:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight / 3;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight / 3;
        }

        break;
    case PlayerClassType::PALADIN:
    case PlayerClassType::CHAOS_WARRIOR:
        if (tval_main <= ItemKindType::SWORD) {
            cur_wgt += item_main_hand.weight / 5;
        }

        if (tval_sub <= ItemKindType::SWORD) {
            cur_wgt += item_sub_hand.weight / 5;
        }

        break;
    default:
        break;
    }

    int max_wgt = mp_ptr->spell_weight;
    if ((cur_wgt - max_wgt) > 0) {
        creature.cumber_armor = true;
        switch (creature.pclass) {
        case PlayerClassType::MAGE:
        case PlayerClassType::HIGH_MAGE:
        case PlayerClassType::BLUE_MAGE:
        case PlayerClassType::ELEMENTALIST: {
            msp -= msp * (cur_wgt - max_wgt) / 600;
            break;
        }
        case PlayerClassType::PRIEST:
        case PlayerClassType::MINDCRAFTER:
        case PlayerClassType::BEASTMASTER:
        case PlayerClassType::BARD:
        case PlayerClassType::FORCETRAINER:
        case PlayerClassType::TOURIST:
        case PlayerClassType::MIRROR_MASTER: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        case PlayerClassType::SORCERER: {
            msp -= msp * (cur_wgt - max_wgt) / 900;
            break;
        }
        case PlayerClassType::ROGUE:
        case PlayerClassType::RANGER:
        case PlayerClassType::MONK:
        case PlayerClassType::RED_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1000;
            break;
        }
        case PlayerClassType::PALADIN:
        case PlayerClassType::CHAOS_WARRIOR:
        case PlayerClassType::WARRIOR_MAGE: {
            msp -= msp * (cur_wgt - max_wgt) / 1200;
            break;
        }
        case PlayerClassType::SAMURAI: {
            creature.cumber_armor = false;
            break;
        }
        default: {
            msp -= msp * (cur_wgt - max_wgt) / 800;
            break;
        }
        }
    }

    if (msp < 0) {
        msp = 0;
    }

    if (creature.msp != msp) {
        if ((creature.csp >= msp) && !pc.equals(PlayerClassType::SAMURAI)) {
            creature.csp = msp;
            creature.csp_frac = 0;
        }

#ifdef JP
        if (creature.level_up_message && (msp > creature.msp)) {
            msg_format("最大マジック・ポイントが %d 増加した！", (msp - creature.msp));
        }
#endif
        creature.msp = msp;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::MP);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags);
    }

    if (AngbandWorld::get_instance().character_xtra) {
        return;
    }

    if (creature.old_cumber_glove != creature.cumber_glove) {
        if (creature.cumber_glove) {
            msg_print(_("手が覆われて呪文が唱えにくい感じがする。", "Your covered hands feel unsuitable for spellcasting."));
        } else {
            msg_print(_("この手の状態なら、ぐっと呪文が唱えやすい感じだ。", "Your hands feel more suitable for spellcasting."));
        }

        creature.old_cumber_glove = creature.cumber_glove;
    }

    if (creature.old_cumber_armor == creature.cumber_armor) {
        return;
    }

    if (creature.cumber_armor) {
        msg_print(_("装備の重さで動きが鈍くなってしまっている。", "The weight of your equipment encumbers your movement."));
    } else {
        msg_print(_("ぐっと楽に体を動かせるようになった。", "You feel able to move more freely."));
    }

    creature.old_cumber_armor = creature.cumber_armor;
}

/*!
 * @brief 装備中の射撃武器の威力倍率を返す /
 * calcurate the fire rate of target object
 * @param o_ptr 計算する射撃武器のアイテム情報参照ポインタ
 * @return 射撃倍率の値(100で1.00倍)
 */
short calc_num_fire(CreatureEntity &creature, const ItemEntity *o_ptr)
{
    int extra_shots = 0;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *q_ptr;
        q_ptr = creature.inventory[i].get();
        if (!q_ptr->is_valid()) {
            continue;
        }

        if (i == INVEN_BOW) {
            continue;
        }

        if (q_ptr->get_flags().has(TR_XTRA_SHOTS)) {
            extra_shots++;
        }
    }

    if (o_ptr->get_flags().has(TR_XTRA_SHOTS)) {
        extra_shots++;
    }

    int num = 0;
    if (!o_ptr->is_valid()) {
        return (int16_t)num;
    }

    num = 100;
    num += (extra_shots * 100);

    if (is_heavy_shoot(creature, o_ptr)) {
        return (int16_t)num;
    }

    const auto tval_ammo = o_ptr->get_arrow_kind();
    CreatureClass pc(creature);
    if (pc.equals(PlayerClassType::RANGER) && (tval_ammo == ItemKindType::ARROW)) {
        num += (creature.level * 4);
    }

    if (pc.equals(PlayerClassType::CAVALRY) && (tval_ammo == ItemKindType::ARROW)) {
        num += (creature.level * 3);
    }

    if (pc.equals(PlayerClassType::ARCHER)) {
        if (tval_ammo == ItemKindType::ARROW) {
            num += ((creature.level * 5) + 50);
        } else if ((tval_ammo == ItemKindType::BOLT) || (tval_ammo == ItemKindType::SHOT)) {
            num += (creature.level * 4);
        }
    }

    if (pc.equals(PlayerClassType::WARRIOR) && (tval_ammo <= ItemKindType::BOLT) && (tval_ammo >= ItemKindType::SHOT)) {
        num += (creature.level * 2);
    }

    if (pc.equals(PlayerClassType::ROGUE) && (tval_ammo == ItemKindType::SHOT)) {
        num += (creature.level * 4);
    }

    return (int16_t)num;
}

/*!
 * @brief 解除能力計算
 * @param creature クリーチャーへの参照
 * @return 解除能力
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 器用さに応じたadj_dex_disテーブルによる加算
 * * 知力に応じたadj_int_disテーブルによる加算
 */
static ACTION_SKILL_POWER calc_disarming(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_race_ptr;

    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];

    pow = tmp_race_ptr->r_dis + player_class.c_dis + player_personality.a_dis;
    pow += (((*creature.pclass_ref).x_dis * creature.level / 10) + ((*creature.personality).a_dis * creature.level / 50));
    pow += adj_dex_dis[creature.stat_index[A_DEX]];
    pow += adj_int_dis[creature.stat_index[A_INT]];
    return pow;
}

/*!
 * @brief 魔道具使用能力計算
 * @param creature クリーチャーへの参照
 * @return 魔道具使用能力
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 装備による加算(TR_MAGIC_MASTERYを持っていたら+pval*8)
 * * 知力に応じたadj_int_devテーブルによる加算
 * * 狂戦士化による減算(-20)
 */
static ACTION_SKILL_POWER calc_device_ability(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_race_ptr;

    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];

    pow = tmp_race_ptr->r_dev + player_class.c_dev + player_personality.a_dev;
    pow += ((player_class.x_dev * creature.level / 10) + ((*creature.personality).a_dev * creature.level / 50));

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_MAGIC_MASTERY)) {
            pow += 8 * o_ptr->pval;
        }
    }

    pow += adj_int_dev[creature.stat_index[A_INT]];

    if (creature.is_shero()) {
        pow -= 20;
    }
    return pow;
}

/*!
 * @brief 魔法防御計算
 * @param creature クリーチャーへの参照
 * @return 魔法防御
 * @details
 * * 種族/職業/性格による加算
 * * 職業と性格とレベルによる追加加算
 * * 変異MUT3_MAGIC_RESによる加算(15 + レベル / 5)
 * * 呪力耐性の装備による加算(30)
 * * 祝福された装備による加算(5 + レベル / 10)
 * * 賢さによるadj_wis_savテーブル加算
 * * 呪力弱点の装備による減算(-10)
 * * 呪力弱点の装備が強力に呪われているときさらに減算(-20)
 * * 狂戦士化による減算(-30)
 * * 反魔法持ちで大なり上書き(90+レベル未満ならその値に上書き)
 * * クターのつぶれ状態なら(10に上書き)
 * * 生命の「究極の耐性」や regist_magic,magicdef持ちなら大なり上書き(95+レベル未満ならその値に上書き)
 * * 呪いのdown_savingがかかっているなら半減
 */
static ACTION_SKILL_POWER calc_saving_throw(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_race_ptr;

    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];

    pow = tmp_race_ptr->r_sav + player_class.c_sav + player_personality.a_sav;
    pow += (((*creature.pclass_ref).x_sav * creature.level / 10) + ((*creature.personality).a_sav * creature.level / 50));

    if (creature.muta.has(PlayerMutationType::MAGIC_RES)) {
        pow += (15 + (creature.level / 5));
    }

    if (has_resist_curse(creature)) {
        pow += 30;
    }

    if (creature.bless_blade) {
        pow += 6 + (creature.level - 1) / 10;
    }

    pow += adj_wis_sav[creature.stat_index[A_WIS]];

    if (has_vuln_curse(creature)) {
        pow -= 10;
    }

    if (has_heavy_vuln_curse(creature)) {
        pow -= 20;
    }

    if (creature.is_shero()) {
        pow -= 30;
    }

    if (creature.anti_magic && (pow < (90 + creature.level))) {
        pow = 90 + creature.level;
    }

    if (creature.get_timed_effect(CreatureTimedEffect::TSUBURERU)) {
        pow = 10;
    }

    if ((creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE) || creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC) || creature.get_timed_effect(CreatureTimedEffect::MAGICDEF)) && (pow < (95 + creature.level))) {
        pow = 95 + creature.level;
    }

    if (creature.down_saving) {
        pow /= 2;
    }

    return pow;
}

/*!
 * @brief 探索深度計算
 * @param creature クリーチャーへの参照
 * @return 探索深度
 * @details
 * * 種族/職業/性格による加算
 * * 職業とレベルによる追加加算
 * * 各装備による加算(TR_SEARCHがあれば+pval*5)
 * * 狂戦士化による減算(-15)
 * * 変異(MUT3_XTRA_EYES)による加算(+15)
 */
static ACTION_SKILL_POWER calc_search(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_race_ptr;

    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];
    pow = tmp_race_ptr->r_srh + player_class.c_srh + player_personality.a_srh;
    pow += (player_class.x_srh * creature.level / 10);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_SEARCH)) {
            pow += (o_ptr->pval * 5);
        }
    }

    if (creature.muta.has(PlayerMutationType::XTRA_EYES)) {
        pow += 15;
    }

    if (creature.is_shero()) {
        pow -= 15;
    }

    return pow;
}

/*!
 * @brief 探索頻度計算
 * @param creature クリーチャーへの参照
 * @return 探索頻度
 * @details
 * * 種族/職業/性格による加算
 * * 職業とレベルによる追加加算
 * * 各装備による加算(TR_SEARCHがあれば+pval*5)
 * * 狂戦士化による減算(-15)
 * * 変異(MUT3_XTRA_EYES)による加算(+15)
 */
static ACTION_SKILL_POWER calc_search_freq(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const player_race_info *tmp_race_ptr;
    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];
    pow = tmp_race_ptr->r_fos + player_class.c_fos + player_personality.a_fos;
    pow += (player_class.x_fos * creature.level / 10);

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_SEARCH)) {
            pow += (o_ptr->pval * 5);
        }
    }

    if (creature.is_shero()) {
        pow -= 15;
    }

    if (creature.muta.has(PlayerMutationType::XTRA_EYES)) {
        pow += 15;
    }

    return pow;
}

/*!
 * @brief 打撃命中能力計算
 * @param creature クリーチャーへの参照
 * @return 打撃命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 */
static ACTION_SKILL_POWER calc_to_hit_melee(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];
    const player_race_info *tmp_race_ptr;
    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    pow = tmp_race_ptr->r_thn + player_class.c_thn + player_personality.a_thn;
    pow += ((player_class.x_thn * creature.level / 10) + (player_personality.a_thn * creature.level / 50));
    return pow;
}

/*!
 * @brief 射撃命中能力計算
 * @param creature クリーチャーへの参照
 * @return 射撃命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 */
static ACTION_SKILL_POWER calc_to_hit_shoot(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];
    const player_race_info *tmp_race_ptr;
    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    pow = tmp_race_ptr->r_thb + player_class.c_thb + player_personality.a_thb;
    pow += ((player_class.x_thb * creature.level / 10) + (player_personality.a_thb * creature.level / 50));
    return pow;
}

/*!
 * @brief 投擲命中能力計算
 * @param creature クリーチャーへの参照
 * @return 投擲命中能力
 * @details
 * * 種族/職業/性格による加算とレベルによる追加加算
 * * 狂戦士による減算(-20)
 */
static ACTION_SKILL_POWER calc_to_hit_throw(CreatureEntity &creature)
{
    ACTION_SKILL_POWER pow;
    const auto &player_class = class_info.at(creature.pclass);
    const auto &player_personality = personality_info[creature.ppersonality];
    const player_race_info *tmp_race_ptr;
    if (creature.get_mimic_form() != MimicKindType::NONE) {
        tmp_race_ptr = &mimic_info.at(creature.get_mimic_form());
    } else {
        tmp_race_ptr = &race_info[enum2i(creature.prace)];
    }

    pow = tmp_race_ptr->r_thb + player_class.c_thb + player_personality.a_thb;
    pow += ((player_class.x_thb * creature.level / 10) + (player_personality.a_thb * creature.level / 50));

    if (creature.is_shero()) {
        pow -= 20;
    }

    return pow;
}

/*!
 * @brief 掘削能力計算
 * @param creature クリーチャーへの参照
 * @return 掘削能力値
 * @details
 * * エントが素手の場合のプラス修正
 * * 狂戦士化時のプラス修正
 * * 腕力によるテーブルプラス修正
 * * 職業狂戦士のプラス修正
 * * 装備の特性によるプラス修正
 * * 武器重量によるプラス修正
 * * 最終算出値に1を保証
 */
static ACTION_SKILL_POWER calc_skill_dig(CreatureEntity &creature)
{
    ItemEntity *o_ptr;

    ACTION_SKILL_POWER pow;

    pow = 0;

    if (CreatureRace(&creature).equals(PlayerRaceType::ENT) && !creature.inventory[INVEN_MAIN_HAND]->is_valid()) {
        pow += creature.level * 10;
    }

    if (creature.is_shero()) {
        pow += 30;
    }

    pow += adj_str_dig[creature.stat_index[A_STR]];

    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        pow += (100 + creature.level * 8);
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->get_flags().has(TR_TUNNEL)) {
            pow += (o_ptr->pval * 20);
        }
    }

    for (int i = 0; i < 2; i++) {
        o_ptr = creature.inventory[INVEN_MAIN_HAND + i].get();
        if (has_melee_weapon(creature, INVEN_MAIN_HAND + i) && !creature.heavy_wield[i]) {
            pow += (o_ptr->weight / 10);
        }
    }

    if (creature.is_shero()) {
        pow += 30;
    }

    if (pow < 1) {
        pow = 1;
    }

    return pow;
}

static bool is_martial_arts_mode(CreatureEntity &creature)
{
    CreatureClass pc(creature);
    auto has_martial_arts = pc.equals(PlayerClassType::MONK);
    has_martial_arts |= pc.equals(PlayerClassType::FORCETRAINER);
    has_martial_arts |= pc.equals(PlayerClassType::BERSERKER);
    return has_martial_arts && any_bits(empty_hands(creature, true), EMPTY_HAND_MAIN) && !can_attack_with_sub_hand(creature);
}

static bool is_heavy_wield(CreatureEntity &creature, int i)
{
    const auto *o_ptr = creature.inventory[INVEN_MAIN_HAND + i].get();

    return has_melee_weapon(creature, INVEN_MAIN_HAND + i) && (calc_weapon_weight_limit(creature) < o_ptr->weight / 10);
}

static int16_t calc_num_blow(CreatureEntity &creature, int i)
{
    int16_t num_blow = 1;

    const auto *o_ptr = creature.inventory[INVEN_MAIN_HAND + i].get();
    CreatureClass pc(creature);
    if (has_melee_weapon(creature, INVEN_MAIN_HAND + i)) {
        if (o_ptr->is_valid() && !creature.heavy_wield[i]) {
            int str_index, dex_index;
            int num = 0, wgt = 0, mul = 0, div = 0;

            auto &player_class = class_info.at(creature.pclass);
            num = player_class.num;
            wgt = player_class.wgt;
            mul = player_class.mul;

            if (pc.equals(PlayerClassType::CAVALRY) && creature.riding && o_ptr->get_flags().has(TR_RIDING)) {
                num = 5;
                wgt = 70;
                mul = 4;
            }

            if (SpellHex(creature).is_spelling_specific(HEX_XTRA_MIGHT) || SpellHex(creature).is_spelling_specific(HEX_BUILDING)) {
                num++;
                wgt /= 2;
                mul += 2;
            }

            div = ((o_ptr->weight < wgt) ? wgt : o_ptr->weight);
            str_index = (adj_str_blow[creature.stat_index[A_STR]] * mul / div);

            if (has_two_handed_weapons(creature) && !has_disable_two_handed_bonus(creature, 0)) {
                str_index += pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::BERSERKER) ? (creature.level / 23 + 1) : 1;
            }
            if (pc.equals(PlayerClassType::NINJA)) {
                str_index = std::max(0, str_index - 1);
            }
            if (str_index > 11) {
                str_index = 11;
            }

            dex_index = (adj_dex_blow[creature.stat_index[A_DEX]]);
            if (dex_index > 11) {
                dex_index = 11;
            }

            num_blow = blows_table[str_index][dex_index];
            if (num_blow > num) {
                num_blow = (int16_t)num;
            }

            num_blow += (int16_t)creature.extra_blows[i];
            if (pc.equals(PlayerClassType::WARRIOR)) {
                num_blow += (creature.level / 40);
            } else if (pc.equals(PlayerClassType::BERSERKER)) {
                num_blow += (creature.level / 23);
            } else if (pc.equals(PlayerClassType::ROGUE) && (o_ptr->weight < 50) && (creature.stat_index[A_DEX] >= 30)) {
                num_blow++;
            }

            if (CreatureClass(creature).samurai_stance_is(SamuraiStanceType::FUUJIN)) {
                num_blow -= 1;
            }

            if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
                num_blow = 1;
            }

            if (has_not_ninja_weapon(creature, i)) {
                num_blow /= 2;
            }

            if (num_blow < 1) {
                num_blow = 1;
            }
        }
    }

    if (i != 0) {
        return num_blow;
    }
    /* Different calculation for monks with empty hands */
    if (is_martial_arts_mode(creature)) {
        int blow_base = creature.level + adj_dex_blow[creature.stat_index[A_DEX]];
        num_blow = 0;

        if (pc.equals(PlayerClassType::FORCETRAINER)) {
            if (blow_base > 18) {
                num_blow++;
            }
            if (blow_base > 31) {
                num_blow++;
            }
            if (blow_base > 44) {
                num_blow++;
            }
            if (blow_base > 58) {
                num_blow++;
            }
        } else {
            if (blow_base > 12) {
                num_blow++;
            }
            if (blow_base > 22) {
                num_blow++;
            }
            if (blow_base > 31) {
                num_blow++;
            }
            if (blow_base > 39) {
                num_blow++;
            }
            if (blow_base > 46) {
                num_blow++;
            }
            if (blow_base > 53) {
                num_blow++;
            }
            if (blow_base > 59) {
                num_blow++;
            }
        }

        if (heavy_armor(creature) && !pc.equals(PlayerClassType::BERSERKER)) {
            num_blow /= 2;
        }

        if (pc.monk_stance_is(MonkStanceType::GENBU)) {
            num_blow -= 2;
            if (pc.equals(PlayerClassType::MONK) && (creature.level > 42)) {
                num_blow--;
            }
            if (num_blow < 0) {
                num_blow = 0;
            }
        } else if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
            num_blow /= 2;
        }

        num_blow += 1 + creature.extra_blows[0];
    }

    return num_blow;
}

/*!
 * @brief 魔法失敗値計算
 * @param creature クリーチャーへの参照
 * @return 魔法失敗値
 * @details
 * * 性格なまけものなら加算(+10)
 * * 性格きれものなら減算(-3)
 * * 性格ちからじまんとがまんづよいなら加算(+1)
 * * 性格チャージマンなら加算(+5)
 * * 装備品にTRC::HARD_SPELLがあるなら加算(軽い呪いなら+3/重い呪いなら+10)
 */
static int16_t calc_to_magic_chance(CreatureEntity &creature)
{
    int16_t chance = 0;

    if (creature.ppersonality == PERSONALITY_LAZY) {
        chance += 10;
    }
    if (creature.ppersonality == PERSONALITY_SHREWD) {
        chance -= 3;
    }
    if ((creature.ppersonality == PERSONALITY_PATIENT) || (creature.ppersonality == PERSONALITY_MIGHTY)) {
        chance++;
    }
    if (creature.ppersonality == PERSONALITY_CHARGEMAN) {
        chance += 5;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->curse_flags.has(CurseTraitType::HARD_SPELL)) {
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                chance += 10;
            } else {
                chance += 3;
            }
        }
    }
    return chance;
}

static ARMOUR_CLASS calc_base_ac(CreatureEntity &creature)
{
    ARMOUR_CLASS ac = 0;
    if (creature.yoiyami) {
        return 0;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }
        ac += o_ptr->ac;
    }

    const auto o_ptr_mh = creature.inventory[INVEN_MAIN_HAND].get();
    const auto o_ptr_sh = creature.inventory[INVEN_SUB_HAND].get();
    if (o_ptr_mh->is_protector() || o_ptr_sh->is_protector()) {
        ac += creature.skill_exp[PlayerSkillKindType::SHIELD] * (1 + creature.level / 22) / 2000;
    }

    // 装甲技能による防御力ボーナス（鎧装備時）
    const auto o_ptr_body = creature.inventory[INVEN_BODY].get();
    if (o_ptr_body->is_valid() && (o_ptr_body->bi_key.tval() == ItemKindType::HARD_ARMOR)) {
        ac += creature.skill_exp[PlayerSkillKindType::ARMOR] * (1 + creature.level / 25) / 2500;
    }

    // 回避技能による防御力ボーナス（軽装備時）
    auto equipment_weight = 0;
    equipment_weight += creature.inventory[INVEN_BODY]->weight;
    equipment_weight += creature.inventory[INVEN_HEAD]->weight;
    equipment_weight += creature.inventory[INVEN_OUTER]->weight;
    equipment_weight += creature.inventory[INVEN_ARMS]->weight;
    equipment_weight += creature.inventory[INVEN_FEET]->weight;

    // 装備重量が軽い（300ポンド以下）時に回避技能ボーナス
    if (equipment_weight <= 300) {
        auto evasion_bonus = creature.skill_exp[PlayerSkillKindType::EVASION] * (1 + creature.level / 20) / 3000;
        // 装備がより軽いほどボーナスが大きくなる（最大で2倍）
        auto weight_ratio = (300 - equipment_weight) / 300.0;
        evasion_bonus = static_cast<int>(evasion_bonus * (1.0 + weight_ratio));
        ac += evasion_bonus;
    }

    return ac;
}

static ARMOUR_CLASS calc_to_ac(CreatureEntity &creature, bool is_real_value)
{
    ARMOUR_CLASS ac = 0;
    if (creature.yoiyami) {
        return 0;
    }

    ac += ((int)(adj_dex_ta[creature.stat_index[A_DEX]]) - 128);

    switch (creature.get_mimic_form()) {
    case MimicKindType::NONE:
        break;
    case MimicKindType::DEMON:
        ac += 10;
        break;
    case MimicKindType::DEMON_LORD:
        ac += 20;
        break;
    case MimicKindType::VAMPIRE:
        ac += 10;
        break;
    case MimicKindType::ANGEL:
        ac += 10;
        break;
    case MimicKindType::DEMIGOD:
        ac += 20;
        break;
    default:
        break;
    }

    CreatureClass pc(creature);
    if (pc.equals(PlayerClassType::BERSERKER)) {
        ac += 10 + creature.level / 2;
    }

    if (pc.equals(PlayerClassType::SORCERER)) {
        ac -= 50;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto *o_ptr = creature.inventory[i].get();
        const auto flags = o_ptr->get_flags();
        if (!o_ptr->is_valid()) {
            continue;
        }
        if (is_real_value || o_ptr->is_known()) {
            ac += o_ptr->to_a;
        }

        if (o_ptr->curse_flags.has(CurseTraitType::LOW_AC) || flags.has(TR_LOW_AC)) {
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                if (is_real_value || o_ptr->is_fully_known()) {
                    ac -= 30;
                }
            } else {
                if (is_real_value || o_ptr->is_fully_known()) {
                    ac -= 10;
                }
            }
        }

        if ((i == INVEN_SUB_HAND) && flags.has(TR_SUPPORTIVE)) {
            ac += 5;
        }
    }

    CreatureRace pr(&creature);
    if (pr.equals(PlayerRaceType::GOLEM) || pr.equals(PlayerRaceType::ANDROID)) {
        ac += 10 + (creature.level * 2 / 5);
    }

    if (set_quick_and_tiny(creature)) {
        ac += 10;
    }

    if (set_musasi(creature)) {
        ac += 10;
    }

    if (set_icing_and_twinkle(creature)) {
        ac += 5;
    }

    if (creature.muta.has(PlayerMutationType::WART_SKIN)) {
        ac += 5;
    }

    if (creature.muta.has(PlayerMutationType::SCALES)) {
        ac += 10;
    }

    if (creature.muta.has(PlayerMutationType::IRON_SKIN)) {
        ac += 25;
    }

    if (pc.is_martial_arts_pro() && !heavy_armor(creature)) {
        if (!creature.inventory[INVEN_BODY]->is_valid()) {
            ac += (creature.level * 3) / 2;
        }
        if (!creature.inventory[INVEN_OUTER]->is_valid() && (creature.level > 15)) {
            ac += ((creature.level - 13) / 3);
        }
        if (!creature.inventory[INVEN_SUB_HAND]->is_valid() && (creature.level > 10)) {
            ac += ((creature.level - 8) / 3);
        }
        if (!creature.inventory[INVEN_HEAD]->is_valid() && (creature.level > 4)) {
            ac += (creature.level - 2) / 3;
        }
        if (!creature.inventory[INVEN_ARMS]->is_valid()) {
            ac += (creature.level / 2);
        }
        if (!creature.inventory[INVEN_FEET]->is_valid()) {
            ac += (creature.level / 3);
        }
    }

    if (PlayerRealm(creature).is_realm_hex()) {
        if (SpellHex(creature).is_spelling_specific(HEX_ICE_ARMOR)) {
            ac += 30;
        }

        for (int i = INVEN_MAIN_HAND; i <= INVEN_FEET; i++) {
            auto *o_ptr = creature.inventory[i].get();
            if (!o_ptr->is_valid()) {
                continue;
            }
            if (!o_ptr->is_protector()) {
                continue;
            }
            if (!o_ptr->is_cursed()) {
                continue;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
                ac += 5;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                ac += 7;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                ac += 13;
            }
        }
    }

    if (pc.monk_stance_is(MonkStanceType::GENBU)) {
        ac += (creature.level * creature.level) / 50;
    } else if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        ac -= 40;
    } else if (pc.monk_stance_is(MonkStanceType::SEIRYU)) {
        ac -= 50;
    } else if (pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        ac -= 50;
    }

    if (creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE) || (pc.samurai_stance_is(SamuraiStanceType::MUSOU))) {
        ac += 100;
    } else if (creature.get_timed_effect(CreatureTimedEffect::TSUBURERU) || creature.get_timed_effect(CreatureTimedEffect::SHIELD) || creature.get_timed_effect(CreatureTimedEffect::MAGICDEF)) {
        ac += 50;
    }

    if (creature.is_blessed()) {
        ac += 5;
    }

    if (creature.is_shero()) {
        ac -= 10;
    }

    if (pc.equals(PlayerClassType::NINJA)) {
        const auto bi_id_main = creature.inventory[INVEN_MAIN_HAND]->bi_id;
        const auto bi_id_sub = creature.inventory[INVEN_SUB_HAND]->bi_id;
        if (((bi_id_main == 0) || can_attack_with_main_hand(creature)) && ((bi_id_sub == 0) || can_attack_with_sub_hand(creature))) {
            ac += creature.level / 2 + 5;
        }
    }

    return ac;
}

/*!
 * @brief 二刀流ペナルティ量計算
 * @param creature クリーチャーへの参照
 * @param slot ペナルティ量を計算する武器スロット
 * @return 二刀流ペナルティ量
 * @details
 * * 二刀流にしていなければ0
 * * 特別セットによる軽減
 * * EASY2_WEAPONによる軽減
 * * SUPPORTIVEを左に装備した場合の軽減
 * * 武蔵セットによる免除
 * * 竿状武器による増加
 */
int16_t calc_double_weapon_penalty(CreatureEntity &creature, INVENTORY_IDX slot)
{
    int penalty = 0;

    if (has_melee_weapon(creature, INVEN_MAIN_HAND) && has_melee_weapon(creature, INVEN_SUB_HAND)) {
        const auto flags = creature.inventory[INVEN_SUB_HAND]->get_flags();

        penalty = ((100 - creature.skill_exp[PlayerSkillKindType::TWO_WEAPON] / 160) - (130 - creature.inventory[slot]->weight) / 8);
        if (set_quick_and_tiny(creature) || set_icing_and_twinkle(creature) || set_anubis_and_chariot(creature)) {
            penalty = penalty / 2 - 5;
        }

        for (uint i = FLAG_CAUSE_INVEN_MAIN_HAND; i < FLAG_CAUSE_MAX; i <<= 1) {
            if (penalty > 0 && any_bits(creature.easy_2weapon, i)) {
                penalty /= 2;
            }
        }

        if (flags.has(TR_SUPPORTIVE)) {
            penalty = std::max(0, penalty - 10);
        }

        if (set_musasi(creature)) {
            penalty = std::min(0, penalty);
        }

        if (creature.inventory[slot]->bi_key.tval() == ItemKindType::POLEARM) {
            penalty += 10;
        }
    }
    return (int16_t)penalty;
}

static bool is_riding_two_hands(CreatureEntity &creature)
{
    if (!creature.riding) {
        return false;
    }

    if (has_two_handed_weapons(creature) || (empty_hands(creature, false) == EMPTY_HAND_NONE)) {
        return true;
    }

    if (any_bits(creature.pet_extra_flags, PF_TWO_HANDS)) {
        switch (creature.pclass) {
        case PlayerClassType::MONK:
        case PlayerClassType::FORCETRAINER:
        case PlayerClassType::BERSERKER:
            return (empty_hands(creature, false) != EMPTY_HAND_NONE) && !has_melee_weapon(creature, INVEN_MAIN_HAND) && !has_melee_weapon(creature, INVEN_SUB_HAND);
        default:
            break;
        }
    }

    return false;
}

static int16_t calc_riding_bow_penalty(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    if (!creature.riding) {
        return 0;
    }

    int16_t penalty = 0;

    if (CreatureClass(creature).is_tamer()) {
        if (creature.tval_ammo != ItemKindType::ARROW) {
            penalty = 5;
        }
    } else {
        penalty = floor.get_monster(creature.riding).get_monrace().level - creature.skill_exp[PlayerSkillKindType::RIDING] / 80;
        penalty += 30;
        if (penalty < 30) {
            penalty = 30;
        }
    }

    if (creature.tval_ammo == ItemKindType::BOLT) {
        penalty *= 2;
    }

    return penalty;
}

void put_equipment_warning(CreatureEntity &creature)
{
    bool heavy_shoot = is_heavy_shoot(creature, creature.inventory[INVEN_BOW].get());
    if (creature.old_heavy_shoot != heavy_shoot) {
        if (heavy_shoot) {
            msg_print(_("こんな重い弓を装備しているのは大変だ。", "You have trouble wielding such a heavy bow."));
        } else if (creature.inventory[INVEN_BOW]->is_valid()) {
            msg_print(_("この弓なら装備していても辛くない。", "You have no trouble wielding your bow."));
        } else {
            msg_print(_("重い弓を装備からはずして体が楽になった。", "You feel relieved to put down your heavy bow."));
        }
        creature.old_heavy_shoot = heavy_shoot;
    }

    for (int i = 0; i < 2; i++) {
        if (creature.old_heavy_wield[i] != creature.heavy_wield[i]) {
            if (creature.heavy_wield[i]) {
                msg_print(_("こんな重い武器を装備しているのは大変だ。", "You have trouble wielding such a heavy weapon."));
            } else if (has_melee_weapon(creature, INVEN_MAIN_HAND + i)) {
                msg_print(_("これなら装備していても辛くない。", "You have no trouble wielding your weapon."));
            } else if (creature.heavy_wield[1 - i]) {
                msg_print(_("まだ武器が重い。", "You still have trouble wielding a heavy weapon."));
            } else {
                msg_print(_("重い武器を装備からはずして体が楽になった。", "You feel relieved to put down your heavy weapon."));
            }

            creature.old_heavy_wield[i] = creature.heavy_wield[i];
        }

        if (creature.old_riding_wield[i] != creature.is_icky_riding_wield[i]) {
            if (creature.is_icky_riding_wield[i]) {
                msg_print(_("この武器は乗馬中に使うにはむかないようだ。", "This weapon is not suitable for use while riding."));
            } else if (!creature.riding) {
                msg_print(_("この武器は徒歩で使いやすい。", "This weapon is suitable for use on foot."));
            } else if (has_melee_weapon(creature, INVEN_MAIN_HAND + i)) {
                msg_print(_("これなら乗馬中にぴったりだ。", "This weapon is suitable for use while riding."));
            }

            creature.old_riding_wield[i] = creature.is_icky_riding_wield[i];
        }

        if (creature.old_icky_wield[i] == creature.is_icky_wield[i]) {
            continue;
        }

        if (creature.is_icky_wield[i]) {
            msg_print(_("今の装備はどうも自分にふさわしくない気がする。", "You do not feel comfortable with your weapon."));
            if (AngbandWorld::get_instance().is_loading_now) {
                chg_virtue(creature, Virtue::FAITH, -1);
            }
        } else if (has_melee_weapon(creature, INVEN_MAIN_HAND + i)) {
            msg_print(_("今の装備は自分にふさわしい気がする。", "You feel comfortable with your weapon."));
        } else {
            msg_print(_("装備をはずしたら随分と気が楽になった。", "You feel more comfortable after removing your weapon."));
        }

        creature.old_icky_wield[i] = creature.is_icky_wield[i];
    }

    if (creature.riding && (creature.old_riding_ryoute != creature.riding_ryoute)) {
        if (creature.riding_ryoute) {
#ifdef JP
            msg_format("%s馬を操れない。", (empty_hands(creature, false) == EMPTY_HAND_NONE) ? "両手がふさがっていて" : "");
#else
            msg_print("You are using both hand for fighting, and you can't control the pet you're riding.");
#endif
        } else {
#ifdef JP
            msg_format("%s馬を操れるようになった。", (empty_hands(creature, false) == EMPTY_HAND_NONE) ? "手が空いて" : "");
#else
            msg_print("You began to control the pet you're riding with one hand.");
#endif
        }

        creature.old_riding_ryoute = creature.riding_ryoute;
    }

    CreatureClass pc(creature);
    if ((pc.is_martial_arts_pro() || pc.equals(PlayerClassType::NINJA)) && (heavy_armor(creature) != creature.monk_notify_aux)) {
        if (heavy_armor(creature)) {
            msg_print(_("装備が重くてバランスを取れない。", "The weight of your armor disrupts your balance."));
            if (AngbandWorld::get_instance().is_loading_now) {
                chg_virtue(creature, Virtue::HARMONY, -1);
            }
        } else {
            msg_print(_("バランスがとれるようになった。", "You regain your balance."));
        }

        creature.monk_notify_aux = heavy_armor(creature);
    }
}

static bool is_bare_knuckle(CreatureEntity &creature)
{
    auto bare_knuckle = is_martial_arts_mode(creature);
    bare_knuckle &= empty_hands(creature, false) == (EMPTY_HAND_MAIN | EMPTY_HAND_SUB);
    return bare_knuckle;
}

static short calc_to_damage(CreatureEntity &creature, INVENTORY_IDX slot, bool is_real_value)
{
    const auto *o_ptr = creature.inventory[slot].get();
    player_hand calc_hand = PLAYER_HAND_OTHER;
    if (slot == INVEN_MAIN_HAND) {
        calc_hand = PLAYER_HAND_MAIN;
    }
    if (slot == INVEN_SUB_HAND) {
        calc_hand = PLAYER_HAND_SUB;
    }

    auto damage = 0;
    damage += ((int)(adj_str_td[creature.stat_index[A_STR]]) - 128);

    if (creature.is_shero()) {
        damage += 3 + (creature.level / 5);
    }

    damage -= creature.effects()->stun().get_damage_penalty();
    CreatureClass pc(creature);
    const auto tval = o_ptr->bi_key.tval();
    if (pc.equals(PlayerClassType::PRIEST) && (o_ptr->get_flags().has_not(TR_BLESSED)) && ((tval == ItemKindType::SWORD) || (tval == ItemKindType::POLEARM))) {
        damage -= 2;
    } else if (pc.equals(PlayerClassType::BERSERKER)) {
        damage += creature.level / 6;
        if (((calc_hand == PLAYER_HAND_MAIN) && !can_attack_with_sub_hand(creature)) || has_two_handed_weapons(creature)) {
            damage += creature.level / 6;
        }
    } else if (pc.equals(PlayerClassType::SORCERER)) {
        auto is_suitable = o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_WIZSTAFF);
        is_suitable |= o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_NAMAKE_HAMMER);
        if (!is_suitable) {
            damage -= 200;
        } else {
            damage -= 10;
        }
    } else if (pc.equals(PlayerClassType::FORCETRAINER)) {
        // 練気術師は格闘ダメージに (気)/5 の修正を得る。
        if (is_martial_arts_mode(creature) && calc_hand == PLAYER_HAND_MAIN) {
            damage += get_current_ki(creature) / 5;
        }
    }

    if (PlayerRealm(creature).is_realm_hex() && o_ptr->is_cursed()) {
        if (SpellHex(creature).is_spelling_specific(HEX_RUNESWORD)) {
            if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
                damage += 5;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                damage += 7;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                damage += 13;
            }
        }
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int bonus_to_d = 0;
        o_ptr = creature.inventory[i].get();
        const auto has_melee = has_melee_weapon(creature, i);
        if (!o_ptr->is_valid() || (o_ptr->bi_key.tval() == ItemKindType::CAPTURE)) {
            continue;
        }

        if (((i == INVEN_MAIN_HAND) && has_melee) || ((i == INVEN_SUB_HAND) && has_melee) || (i == INVEN_BOW)) {
            continue;
        }

        if (!o_ptr->is_known() && !is_real_value) {
            continue;
        }
        bonus_to_d = o_ptr->to_d;

        if (pc.equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_d > 0) {
                bonus_to_d = (o_ptr->to_d + 1) / 2;
            }
        }

        switch (player_melee_type(creature)) {
        case MELEE_TYPE_BAREHAND_TWO:
        case MELEE_TYPE_WEAPON_TWOHAND:
            if (calc_hand == main_attack_hand(creature)) {
                damage += (int16_t)bonus_to_d;
            }
            break;

        case MELEE_TYPE_BAREHAND_MAIN:
        case MELEE_TYPE_WEAPON_MAIN:
            if ((calc_hand == PLAYER_HAND_MAIN) && (i != INVEN_SUB_RING)) {
                damage += (int16_t)bonus_to_d;
            }
            break;

        case MELEE_TYPE_BAREHAND_SUB:
        case MELEE_TYPE_WEAPON_SUB:
            if ((calc_hand == PLAYER_HAND_SUB) && (i != INVEN_MAIN_RING)) {
                damage += (int16_t)bonus_to_d;
            }
            break;

        case MELEE_TYPE_WEAPON_DOUBLE:
            if (calc_hand == PLAYER_HAND_MAIN) {
                if (i == INVEN_MAIN_RING) {
                    damage += (int16_t)bonus_to_d;
                } else if (i != INVEN_SUB_RING) {
                    damage += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
                }
            }
            if (calc_hand == PLAYER_HAND_SUB) {
                if (i == INVEN_SUB_RING) {
                    damage += (int16_t)bonus_to_d;
                } else if (i != INVEN_MAIN_RING) {
                    damage += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;
                }
            }
            break;

        case MELEE_TYPE_SHIELD_DOUBLE:
            break;

        default:
            break;
        }
    }

    if (main_attack_hand(creature) == calc_hand) {
        if (is_bare_knuckle(creature) || !has_disable_two_handed_bonus(creature, calc_hand)) {
            int bonus_to_d = 0;
            bonus_to_d = ((int)(adj_str_td[creature.stat_index[A_STR]]) - 128) / 2;
            damage += std::max<int>(bonus_to_d, 1);
        }
    }

    if (is_martial_arts_mode(creature) && (!heavy_armor(creature) || !pc.equals(PlayerClassType::BERSERKER))) {
        damage += (creature.level / 6);
    }

    // 朱雀の構えをとっているとき、格闘ダメージに -(レベル)/6 の修正を得る。
    if (CreatureClass(creature).monk_stance_is(MonkStanceType::SUZAKU)) {
        if (is_martial_arts_mode(creature) && calc_hand == PLAYER_HAND_MAIN) {
            damage -= (creature.level / 6);
        }
    }

    return static_cast<short>(damage);
}

/*!
 * @brief 武器の命中修正を計算する。 / Calculate hit bonus from a wielded weapon.
 * @details
 * 'slot' MUST be INVEN_MAIN_HAND or INVEM_SUB_HAND.
 */
static short calc_to_hit(CreatureEntity &creature, INVENTORY_IDX slot, bool is_real_value)
{
    auto hit = 0;

    /* Base bonuses */
    hit += ((int)(adj_dex_th[creature.stat_index[A_DEX]]) - 128);
    hit += ((int)(adj_str_th[creature.stat_index[A_STR]]) - 128);

    /* Temporary bonuses */
    if (creature.is_blessed()) {
        hit += 10;
    }

    if (creature.is_hero()) {
        hit += 12;
    }

    if (creature.is_shero()) {
        hit += 12;
    }

    hit -= creature.effects()->stun().get_damage_penalty();
    player_hand calc_hand = PLAYER_HAND_OTHER;
    if (slot == INVEN_MAIN_HAND) {
        calc_hand = PLAYER_HAND_MAIN;
    }
    if (slot == INVEN_SUB_HAND) {
        calc_hand = PLAYER_HAND_SUB;
    }

    /* Default hand bonuses */
    if (main_attack_hand(creature) == calc_hand) {
        switch (player_melee_type(creature)) {
        case MELEE_TYPE_BAREHAND_MAIN:
            if (creature.riding) {
                break;
            }
            [[fallthrough]];
        case MELEE_TYPE_BAREHAND_SUB:
            if (creature.riding) {
                break;
            }
            [[fallthrough]];
        case MELEE_TYPE_BAREHAND_TWO:
            hit += (creature.skill_exp[PlayerSkillKindType::MARTIAL_ARTS] - PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER)) / 200;
            break;

        default:
            break;
        }

        if (is_bare_knuckle(creature) || !has_disable_two_handed_bonus(creature, calc_hand)) {
            int bonus_to_h = 0;
            bonus_to_h = ((int)(adj_str_th[creature.stat_index[A_STR]]) - 128) + ((int)(adj_dex_th[creature.stat_index[A_DEX]]) - 128);
            hit += std::max<int>(bonus_to_h, 1);
        }
    }

    /* Bonuses and penalties by weapon */
    CreatureClass pc(creature);
    if (has_melee_weapon(creature, slot)) {
        const auto *o_ptr = creature.inventory[slot].get();

        /* Traind bonuses */
        const auto tval = o_ptr->bi_key.tval();
        const auto sval = *o_ptr->bi_key.sval();
        hit += (creature.weapon_exp[tval][sval] - PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER)) / 200;

        /* Weight penalty */
        if (calc_weapon_weight_limit(creature) < o_ptr->weight / 10) {
            hit += 2 * (calc_weapon_weight_limit(creature) - o_ptr->weight / 10);
        }

        /* Low melee penalty */
        if ((o_ptr->is_fully_known() || is_real_value) && o_ptr->curse_flags.has(CurseTraitType::LOW_MELEE)) {
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                hit -= 15;
            } else {
                hit -= 5;
            }
        }

        /* Riding bonus and penalty */
        const auto flags = o_ptr->get_flags();
        if (creature.riding > 0) {
            if (o_ptr->is_lance()) {
                hit += 15;
            } else if (flags.has_not(TR_RIDING)) {
                short penalty;
                if (CreatureClass(creature).is_tamer()) {
                    penalty = 5;
                } else {
                    penalty = creature.get_floor()->get_monster(creature.riding).get_monrace().level - creature.skill_exp[PlayerSkillKindType::RIDING] / 80;
                    penalty += 30;
                    if (penalty < 30) {
                        penalty = 30;
                    }
                }

                hit -= penalty;
            }
        }

        /* Class penalties */
        if (pc.equals(PlayerClassType::PRIEST) && (flags.has_not(TR_BLESSED)) && ((tval == ItemKindType::SWORD) || (tval == ItemKindType::POLEARM))) {
            hit -= 2;
        } else if (pc.equals(PlayerClassType::BERSERKER)) {
            hit += creature.level / 5;
            if (((calc_hand == PLAYER_HAND_MAIN) && !can_attack_with_sub_hand(creature)) || has_two_handed_weapons(creature)) {
                hit += creature.level / 5;
            }
        } else if (pc.equals(PlayerClassType::SORCERER)) {
            auto is_suitable = o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_WIZSTAFF);
            is_suitable |= o_ptr->bi_key == BaseitemKey(ItemKindType::HAFTED, SV_NAMAKE_HAMMER);
            if (!is_suitable) {
                hit -= 200;
            } else {
                hit -= 30;
            }
        }

        if (has_not_ninja_weapon(creature, (int)calc_hand) || has_not_monk_weapon(creature, (int)calc_hand)) {
            hit -= 40;
        }

        /* Hex realm bonuses */
        if (PlayerRealm(creature).is_realm_hex() && o_ptr->is_cursed()) {
            if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
                hit += 5;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                hit += 7;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                hit += 13;
            }
            if (o_ptr->curse_flags.has(CurseTraitType::TY_CURSE)) {
                hit += 5;
            }
        }
    }

    /* Bonuses from inventory */
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = creature.inventory[i].get();

        /* Ignore empty hands, handed weapons, bows and capture balls */
        const auto has_melee = has_melee_weapon(creature, i);
        if (!o_ptr->is_valid() || o_ptr->bi_key.tval() == ItemKindType::CAPTURE) {
            continue;
        }

        if (((i == INVEN_MAIN_HAND) && has_melee) || ((i == INVEN_SUB_HAND) && has_melee) || (i == INVEN_BOW)) {
            continue;
        }

        /* Fake value does not include unknown objects' value */
        if (!o_ptr->is_known() && !is_real_value) {
            continue;
        }

        int bonus_to_h = o_ptr->to_h;

        /* When wields only a weapon */
        if (pc.equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_h > 0) {
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            }
        }

        switch (player_melee_type(creature)) {
        case MELEE_TYPE_BAREHAND_TWO:
        case MELEE_TYPE_WEAPON_TWOHAND:
            if (calc_hand == main_attack_hand(creature)) {
                hit += (int16_t)bonus_to_h;
            }
            break;

        case MELEE_TYPE_BAREHAND_MAIN:
        case MELEE_TYPE_WEAPON_MAIN:
            if ((calc_hand == PLAYER_HAND_MAIN) && (i != INVEN_SUB_RING)) {
                hit += (int16_t)bonus_to_h;
            }
            break;

        case MELEE_TYPE_BAREHAND_SUB:
        case MELEE_TYPE_WEAPON_SUB:
            if ((calc_hand == PLAYER_HAND_SUB) && (i != INVEN_MAIN_RING)) {
                hit += (int16_t)bonus_to_h;
            }
            break;

        case MELEE_TYPE_WEAPON_DOUBLE:
            if (calc_hand == PLAYER_HAND_MAIN) {
                if (i == INVEN_MAIN_RING) {
                    hit += (int16_t)bonus_to_h;
                } else if (i != INVEN_SUB_RING) {
                    hit += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
                }
            }
            if (calc_hand == PLAYER_HAND_SUB) {
                if (i == INVEN_SUB_RING) {
                    hit += (int16_t)bonus_to_h;
                } else if (i != INVEN_MAIN_RING) {
                    hit += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
                }
            }
            break;

        case MELEE_TYPE_SHIELD_DOUBLE:
            break;

        default:
            break;
        }
    }

    /* Martial arts bonus */
    if (is_martial_arts_mode(creature) && (!heavy_armor(creature) || !pc.equals(PlayerClassType::BERSERKER))) {
        hit += (creature.level / 3);
    }

    /* Two handed combat penalty */
    hit -= calc_double_weapon_penalty(creature, slot);

    // 朱雀の構えをとっているとき、格闘命中に -(レベル)/3 の修正を得る。
    if (CreatureClass(creature).monk_stance_is(MonkStanceType::SUZAKU)) {
        if (is_martial_arts_mode(creature) && calc_hand == PLAYER_HAND_MAIN) {
            hit -= (creature.level / 3);
        }
    }

    return static_cast<short>(hit);
}

static int16_t calc_to_hit_bow(CreatureEntity &creature, bool is_real_value)
{
    int16_t pow = 0;

    pow += ((int)(adj_dex_th[creature.stat_index[A_DEX]]) - 128);
    pow += ((int)(adj_str_th[creature.stat_index[A_STR]]) - 128);

    {
        ItemEntity *o_ptr;
        o_ptr = creature.inventory[INVEN_BOW].get();
        if (o_ptr->is_valid()) {
            if (o_ptr->curse_flags.has(CurseTraitType::LOW_MELEE)) {
                if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                    pow -= 15;
                } else {
                    pow -= 5;
                }
            }
        }
    }

    pow -= creature.effects()->stun().get_damage_penalty();
    if (creature.is_blessed()) {
        pow += 10;
    }

    if (creature.is_hero()) {
        pow += 12;
    }

    if (creature.is_shero()) {
        pow -= 12;
    }

    auto *o_ptr = creature.inventory[INVEN_BOW].get();

    if (is_heavy_shoot(creature, o_ptr)) {
        pow += 2 * (calc_bow_weight_limit(creature) - o_ptr->weight / 10);
    }

    if (o_ptr->is_valid()) {
        if (!is_heavy_shoot(creature, creature.inventory[INVEN_BOW].get())) {
            if (CreatureClass(creature).equals(PlayerClassType::SNIPER) && (creature.tval_ammo == ItemKindType::BOLT)) {
                pow += (10 + (creature.level / 5));
            }
        }
    }

    // 武器以外の装備による修正
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int bonus_to_h;
        o_ptr = creature.inventory[i].get();
        const auto has_melee = has_melee_weapon(creature, i);
        if (!o_ptr->is_valid() || (o_ptr->bi_key.tval() == ItemKindType::CAPTURE)) {
            continue;
        }

        if (((i == INVEN_MAIN_HAND) && has_melee) || ((i == INVEN_SUB_HAND) && has_melee) || (i == INVEN_BOW)) {
            continue;
        }

        bonus_to_h = o_ptr->to_h;

        if (CreatureClass(creature).equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_h > 0) {
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            }
        }

        if (is_real_value || o_ptr->is_known()) {
            pow += (int16_t)bonus_to_h;
        }
    }

    pow -= calc_riding_bow_penalty(creature);

    return pow;
}

static int16_t calc_to_damage_misc(CreatureEntity &creature)
{
    ItemEntity *o_ptr;

    int16_t to_dam = 0;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        int bonus_to_d = o_ptr->to_d;
        if (CreatureClass(creature).equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_d > 0) {
                bonus_to_d = (o_ptr->to_d + 1) / 2;
            }
        }
        to_dam += (int16_t)bonus_to_d;
    }

    if (creature.is_shero()) {
        to_dam += 3 + (creature.level / 5);
    }

    to_dam -= creature.effects()->stun().get_damage_penalty();
    to_dam += ((int)(adj_str_td[creature.stat_index[A_STR]]) - 128);
    return to_dam;
}

static int16_t calc_to_hit_misc(CreatureEntity &creature)
{
    ItemEntity *o_ptr;

    int16_t to_hit = 0;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        int bonus_to_h = o_ptr->to_h;
        if (CreatureClass(creature).equals(PlayerClassType::NINJA)) {
            if (o_ptr->to_h > 0) {
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            }
        }
        to_hit += (int16_t)bonus_to_h;
    }

    if (creature.is_blessed()) {
        to_hit += 10;
    }

    if (creature.is_hero()) {
        to_hit += 12;
    }

    if (creature.is_shero()) {
        to_hit += 12;
    }

    to_hit -= creature.effects()->stun().get_damage_penalty();
    to_hit += ((int)(adj_dex_th[creature.stat_index[A_DEX]]) - 128);
    to_hit += ((int)(adj_str_th[creature.stat_index[A_STR]]) - 128);

    return to_hit;
}

static int calc_to_weapon_dice_num(CreatureEntity &creature, INVENTORY_IDX slot)
{
    auto *o_ptr = creature.inventory[slot].get();
    return (creature.riding > 0) && o_ptr->is_lance() ? 2 : 0;
}

/*!
 * @brief プレイヤーの所持重量制限を計算する /
 * Computes current weight limit.
 * @return 制限重量(ポンド)
 */
int calc_weight_limit(CreatureEntity &creature)
{
    auto i = adj_str_wgt[creature.stat_index[A_STR]] * 50;
    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        i = i * 3 / 2;
    }

    return i;
}

/*!
 * @brief update のフラグに応じた更新をまとめて行う / Handle "update"
 * @details 更新処理の対象はプレイヤーの能力修正/光源寿命/HP/MP/魔法の学習状態、他多数の外界の状態判定。
 */
void update_creature(CreatureEntity &creature)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (!rfu.any_stats()) {
        return;
    }

    auto &floor = *creature.get_floor();
    if (rfu.has(StatusRecalculatingFlag::AUTO_DESTRUCTION)) {
        rfu.reset_flag(StatusRecalculatingFlag::AUTO_DESTRUCTION);
        autopick_delayed_alter(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::COMBINATION)) {
        rfu.reset_flag(StatusRecalculatingFlag::COMBINATION);
        combine_pack(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::REORDER)) {
        rfu.reset_flag(StatusRecalculatingFlag::REORDER);
        reorder_pack(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::BONUS)) {
        rfu.reset_flag(StatusRecalculatingFlag::BONUS);
        PlayerAlignment(creature).update_alignment();
        PlayerSkill ps(creature);
        ps.apply_special_weapon_skill_max_values();
        ps.limit_weapon_skills_by_max_value();
        update_bonuses(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::TORCH)) {
        rfu.reset_flag(StatusRecalculatingFlag::TORCH);
        update_lite_radius(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::HP)) {
        rfu.reset_flag(StatusRecalculatingFlag::HP);
        update_max_hitpoints(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::MP)) {
        rfu.reset_flag(StatusRecalculatingFlag::MP);
        update_max_mana(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::SPELLS)) {
        rfu.reset_flag(StatusRecalculatingFlag::SPELLS);
        update_num_of_spells(creature);
    }

    const auto &world = AngbandWorld::get_instance();
    if (!world.character_generated || (world.character_icky_depth > 0)) {
        return;
    }

    if (rfu.has(StatusRecalculatingFlag::UN_LITE)) {
        rfu.reset_flag(StatusRecalculatingFlag::UN_LITE);
        floor.forget_lite();
    }

    if (rfu.has(StatusRecalculatingFlag::UN_VIEW)) {
        rfu.reset_flag(StatusRecalculatingFlag::UN_VIEW);
        floor.forget_view();
    }

    if (rfu.has(StatusRecalculatingFlag::VIEW)) {
        rfu.reset_flag(StatusRecalculatingFlag::VIEW);
        update_view(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::LITE)) {
        rfu.reset_flag(StatusRecalculatingFlag::LITE);
        update_lite(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::FLOW)) {
        rfu.reset_flag(StatusRecalculatingFlag::FLOW);
        update_flow(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::DISTANCE)) {
        rfu.reset_flag(StatusRecalculatingFlag::DISTANCE);
        update_monsters(creature, true);
    }

    if (rfu.has(StatusRecalculatingFlag::MONSTER_LITE)) {
        rfu.reset_flag(StatusRecalculatingFlag::MONSTER_LITE);
        update_mon_lite(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::DELAY_VISIBILITY)) {
        rfu.reset_flag(StatusRecalculatingFlag::DELAY_VISIBILITY);
        delayed_visual_update(creature);
    }

    if (rfu.has(StatusRecalculatingFlag::MONSTER_STATUSES)) {
        rfu.reset_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        update_monsters(creature, false);
    }
}

/*!
 * @brief プレイヤーが魔道書を一冊も持っていないかを判定する
 * @return 魔道書を一冊も持っていないならTRUEを返す
 */
bool player_has_no_spellbooks(CreatureEntity &creature)
{
    for (int i = 0; i < INVEN_PACK; i++) {
        const auto *o_ptr = creature.inventory[i].get();
        if (o_ptr->is_valid() && check_book_realm(creature, o_ptr->bi_key)) {
            return false;
        }
    }

    const auto &floor = *creature.get_floor();
    for (const auto this_o_idx : floor.grid_array[creature.y][creature.x].o_idx_list) {
        const auto *o_ptr = floor.o_list[this_o_idx].get();
        if (o_ptr->is_valid() && o_ptr->marked.has(OmType::FOUND) && check_book_realm(creature, o_ptr->bi_key)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 種族アンバライトが出血時パターンの上に乗った際のペナルティ処理
 */
void wreck_the_pattern(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    const auto &terrain = floor.get_grid(p_pos).get_terrain();
    if (terrain.subtype == PATTERN_TILE_WRECKED) {
        return;
    }

    msg_print(_("パターンを血で汚してしまった！", "You bleed on the Pattern!"));
    msg_print(_("何か恐ろしい事が起こった！", "Something terrible happens!"));
    if (!creature.is_invulnerable()) {
        take_hit(creature, DAMAGE_NOESCAPE, Dice::roll(10, 8), _("パターン損壊", "corrupting the Pattern"));
    }

    auto to_ruin = randint1(45) + 35;
    while (to_ruin--) {
        const auto pos = scatter(floor, p_pos, 4, PROJECT_NONE);
        if (floor.has_terrain_characteristics(pos, TerrainCharacteristics::PATTERN) && (floor.get_grid(pos).get_terrain().subtype != PATTERN_TILE_WRECKED)) {
            set_terrain_id_to_grid(creature, pos, TerrainTag::PATTERN_CORRUPTED);
        }
    }

    set_terrain_id_to_grid(creature, p_pos, TerrainTag::PATTERN_CORRUPTED);
}

/*!
 * @brief プレイヤーの経験値について整合性のためのチェックと調整を行う /
 * Advance experience levels and print experience
 */
void check_experience(CreatureEntity &creature)
{
    if (!creature.is_player()) {
        return;
    }
    if (creature.exp < 0) {
        creature.exp = 0;
    }
    if (creature.max_exp < 0) {
        creature.max_exp = 0;
    }
    if (creature.max_max_exp < 0) {
        creature.max_max_exp = 0;
    }

    if (creature.exp > PY_MAX_EXP) {
        creature.exp = PY_MAX_EXP;
    }
    if (creature.max_exp > PY_MAX_EXP) {
        creature.max_exp = PY_MAX_EXP;
    }
    if (creature.max_max_exp > PY_MAX_EXP) {
        creature.max_max_exp = PY_MAX_EXP;
    }

    if (creature.exp > creature.max_exp) {
        creature.max_exp = creature.exp;
    }
    if (creature.max_exp > creature.max_max_exp) {
        creature.max_max_exp = creature.max_exp;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::EXP);
    handle_stuff(creature);

    CreatureRace pr(&creature);
    bool android = pr.equals(PlayerRaceType::ANDROID);
    PLAYER_LEVEL old_lev = creature.level;
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    while ((creature.level > 1) && (creature.exp < ((android ? player_exp_a : player_exp)[creature.level - 2] * creature.expfact / 100L))) {
        creature.level--;
        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::TITLE,
        };
        rfu.set_flags(flags_mwrf);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        handle_stuff(creature);
    }

    bool level_reward = false;
    bool level_mutation = false;
    bool level_inc_stat = false;
    while ((creature.level < PY_MAX_LEVEL) && (creature.exp >= ((android ? player_exp_a : player_exp)[creature.level - 1] * creature.expfact / 100L))) {
        creature.level++;
        if (creature.level > creature.max_plv) {
            creature.max_plv = creature.level;

            if (CreatureClass(creature).equals(PlayerClassType::CHAOS_WARRIOR) || creature.muta.has(PlayerMutationType::CHAOS_GIFT)) {
                level_reward = true;
            }
            if (pr.equals(PlayerRaceType::BEASTMAN)) {
                if (one_in_(5)) {
                    level_mutation = true;
                }
            }
            level_inc_stat = true;

            exe_write_diary(*creature.get_floor(), DiaryKind::LEVELUP, creature.level);
        }

        sound(SoundKind::LEVEL);
        msg_format(_("レベル %d にようこそ。", "Welcome to level %d."), creature.level);
        rfu.set_flags(flags_srf);
        const auto flags_mwrf_levelup = {
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::TITLE,
            MainWindowRedrawingFlag::EXP,
        };
        rfu.set_flags(flags_mwrf_levelup);
        const auto &flags_swrf_levelup = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
            SubWindowRedrawingFlag::INVENTORY,
        };
        rfu.set_flags(flags_swrf_levelup);
        creature.level_up_message = true;
        handle_stuff(creature);

        creature.level_up_message = false;
        if (level_inc_stat) {
            if (!(creature.max_plv % 10)) {
                int choice;
                screen_save();
                while (true) {
                    int n;

                    prt(format(_("        a) 腕力 (現在値 %s)", "        a) Str (cur %s)"), cnv_stat(creature.stat_max[0]).data()), 2, 14);
                    prt(format(_("        b) 知能 (現在値 %s)", "        b) Int (cur %s)"), cnv_stat(creature.stat_max[1]).data()), 3, 14);
                    prt(format(_("        c) 賢さ (現在値 %s)", "        c) Wis (cur %s)"), cnv_stat(creature.stat_max[2]).data()), 4, 14);
                    prt(format(_("        d) 器用 (現在値 %s)", "        d) Dex (cur %s)"), cnv_stat(creature.stat_max[3]).data()), 5, 14);
                    prt(format(_("        e) 耐久 (現在値 %s)", "        e) Con (cur %s)"), cnv_stat(creature.stat_max[4]).data()), 6, 14);
                    prt(format(_("        f) 魅力 (現在値 %s)", "        f) Chr (cur %s)"), cnv_stat(creature.stat_max[5]).data()), 7, 14);

                    prt("", 8, 14);
                    prt(_("        どの能力値を上げますか？", "        Which stat do you want to raise?"), 1, 14);

                    while (true) {
                        choice = inkey();
                        if ((choice >= 'a') && (choice <= 'f')) {
                            break;
                        }
                    }
                    for (n = 0; n < A_MAX; n++) {
                        if (n != choice - 'a') {
                            prt("", n + 2, 14);
                        }
                    }
                    if (input_check(_("よろしいですか？", "Are you sure? "))) {
                        break;
                    }
                }
                do_inc_stat(creature, choice - 'a');
                screen_load();
            } else if (!(creature.max_plv % 2)) {
                do_inc_stat(creature, randint0(6));
            }
        }

        if (level_mutation) {
            msg_print(_("あなたは変わった気がする...", "You feel different..."));
            (void)gain_mutation(creature, 0);
            level_mutation = false;
        }

        /*
         * 報酬でレベルが上ると再帰的に check_experience(creature) が
         * 呼ばれるので順番を最後にする。
         */
        if (level_reward) {
            patron_list[creature.patron].gain_level_reward(creature, 0);
            level_reward = false;
        }

        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::TITLE,
        };
        rfu.set_flags(flags_mwrf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags_swrf);
        handle_stuff(creature);
    }

    if (old_lev != creature.level) {
        autopick_load_pref(creature, false);
    }
}

/*!
 * @brief 現在の修正後能力値を3.0～40.0形式に変換する / Converts stat num into a six-char (right justified) string
 * @param val 能力値 (30～400の範囲)
 * @return std::string 右詰め6文字で記述した能力値 (3.0～40.0表示)
 */
std::string cnv_stat(int val)
{
    // 30～400 -> 3.0～40.0に変換
    int integer_part = val / 10;
    int decimal_part = val % 10;

    if (val >= 400) {
        return " 40.0";
    } else if (integer_part >= 10) {
        return format(" %2d.%d", integer_part, decimal_part);
    } else {
        return format("  %d.%d", integer_part, decimal_part);
    }
}

/*!
 * @brief 能力値現在値から加減算を行う。
 * Modify a stat value by a "modifier", return new value
 * @param value 現在値 (30～400の範囲)
 * @param amount 加減算値 (10単位)
 * @return 加減算後の値
 * @details
 * <pre>
 * 新システム: 30,40,...,170,180,190,...,400 (表示: 3.0,4.0,...,17.0,18.0,19.0,...,40.0)
 * 最小値30、最大値400で制限
 * </pre>
 */
int16_t modify_stat_value(int value, int amount)
{
    value += amount * 10;

    if (value < 30) {
        value = 30;
    } else if (value > 400) {
        value = 400;
    }

    return (int16_t)value;
}

/*!
 * @brief スコアを計算する /
 * Hack -- Calculates the total number of points earned		-JWT-
 * @details
 */
uint32_t calc_score(CreatureEntity &creature)
{
    const auto &entries = ArenaEntryList::get_instance();
    const auto current_entry = entries.get_current_entry();
    const auto arena_win = std::min(current_entry, entries.get_max_entries());
    auto mult = 100;
    if (!preserve_mode) {
        mult += 10;
    }
    if (!autoroller) {
        mult += 10;
    }
    if (!smart_learn) {
        mult -= 20;
    }
    if (smart_cheat) {
        mult += 30;
    }
    if (ironman_shops) {
        mult += 50;
    }
    if (ironman_small_levels) {
        mult += 10;
    }
    if (ironman_empty_levels) {
        mult += 20;
    }
    if (!powerup_home) {
        mult += 50;
    }
    if (ironman_rooms) {
        mult += 100;
    }
    if (ironman_nightmare) {
        mult += 100;
    }

    if (mult < 5) {
        mult = 5;
    }

    const auto max_dungeon_level = DungeonService::find_max_level();
    uint32_t point_l = (creature.max_max_exp + (100 * max_dungeon_level));
    uint32_t point_h = point_l / 0x10000L;
    point_l = point_l % 0x10000L;
    point_h *= mult;
    point_l *= mult;
    point_h += point_l / 0x10000L;
    point_l %= 0x10000L;

    point_l += ((point_h % 100) << 16);
    point_h /= 100;
    point_l /= 100;

    uint32_t point = (point_h << 16) + (point_l);
    if (current_entry >= 0) {
        point += (arena_win * arena_win * (arena_win > 29 ? 1000 : 100));
    }

    if (ironman_downward) {
        point *= 2;
    }
    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        if (CreatureRace(&creature).equals(PlayerRaceType::SPECTRE)) {
            point = point / 5;
        }
    }

    if (creature.death_count > 0) {
        point /= (creature.death_count + 1);
    }

    if ((creature.ppersonality == PERSONALITY_MUNCHKIN) && point) {
        point = 1;
        if (AngbandWorld::get_instance().total_winner) {
            point = 2;
        }
    }

    return point;
}

/*!

 * @brief 口を使う継続的な処理を中断する
 * @param creature クリーチャーへの参照
 */
void stop_mouth(CreatureEntity &creature)
{
    if (music_singing_any(creature)) {
        stop_singing(creature);
    }

    if (SpellHex(creature).is_spelling_any()) {
        (void)SpellHex(creature).stop_all_spells();
    }
}

int calc_weapon_weight_limit(CreatureEntity &creature)
{
    auto weight = adj_str_hold[creature.stat_index[A_STR]];
    if (has_two_handed_weapons(creature)) {
        weight *= 2;
    }

    return weight;
}

int calc_bow_weight_limit(CreatureEntity &creature)
{
    auto weight = adj_str_hold[creature.stat_index[A_STR]];
    return weight;
}

static player_hand main_attack_hand(CreatureEntity &creature)
{
    switch (player_melee_type(creature)) {
    case MELEE_TYPE_BAREHAND_TWO:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_BAREHAND_MAIN:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_BAREHAND_SUB:
        return PLAYER_HAND_SUB;
    case MELEE_TYPE_WEAPON_MAIN:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_WEAPON_SUB:
        return PLAYER_HAND_SUB;
    case MELEE_TYPE_WEAPON_TWOHAND:
        return has_melee_weapon(creature, INVEN_MAIN_HAND) ? PLAYER_HAND_MAIN : PLAYER_HAND_SUB;
    case MELEE_TYPE_WEAPON_DOUBLE:
        return PLAYER_HAND_MAIN;
    case MELEE_TYPE_SHIELD_DOUBLE:
        return PLAYER_HAND_MAIN;
    }
    return PLAYER_HAND_MAIN;
}

bool set_quick_and_tiny(CreatureEntity &creature)
{
    auto is_quickly_tiny = creature.inventory[INVEN_MAIN_HAND]->is_specific_artifact(FixedArtifactId::QUICKTHORN);
    is_quickly_tiny &= creature.inventory[INVEN_SUB_HAND]->is_specific_artifact(FixedArtifactId::TINYTHORN);
    return is_quickly_tiny;
}

bool set_musasi(CreatureEntity &creature)
{
    auto is_musasi = creature.inventory[INVEN_MAIN_HAND]->is_specific_artifact(FixedArtifactId::MUSASI_KATANA);
    is_musasi &= creature.inventory[INVEN_SUB_HAND]->is_specific_artifact(FixedArtifactId::MUSASI_WAKIZASI);
    return is_musasi;
}

bool set_icing_and_twinkle(CreatureEntity &creature)
{
    auto can_call_ice_wind_saga = creature.inventory[INVEN_MAIN_HAND]->is_specific_artifact(FixedArtifactId::ICINGDEATH);
    can_call_ice_wind_saga &= creature.inventory[INVEN_SUB_HAND]->is_specific_artifact(FixedArtifactId::TWINKLE);
    return can_call_ice_wind_saga;
}

bool set_anubis_and_chariot(CreatureEntity &creature)
{
    auto is_anubis_chariot = creature.inventory[INVEN_MAIN_HAND]->is_specific_artifact(FixedArtifactId::ANUBIS);
    is_anubis_chariot &= creature.inventory[INVEN_SUB_HAND]->is_specific_artifact(FixedArtifactId::SILVER_CHARIOT);
    return is_anubis_chariot;
}
