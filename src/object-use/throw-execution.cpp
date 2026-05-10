/*!
 * @file throw-util.cpp
 * @brief 投擲処理関連クラス
 * @date 2021/08/20
 * @author Hourier
 */

#include "object-use/throw-execution.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "combat/slaying.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object/item-use-flags.h"
#include "object/object-broken.h"
#include "object/object-stack.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "racial/racial-android.h"
#include "specific-object/torch.h"
#include "system/angband-exceptions.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "wizard/wizard-messages.h"

ObjectThrowHitMonster::ObjectThrowHitMonster(CreatureEntity &creature, POSITION y, POSITION x)
{
    auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid({ y, x });
    if (!grid.has_monster() || std::cmp_greater_equal(grid.m_idx, floor.m_list.size())) {
        THROW_EXCEPTION(std::logic_error, "Invalid monster index");
    }

    this->m_idx = grid.m_idx;
    this->m_ptr = &floor.get_monster(grid.m_idx);
    this->m_name = monster_name(creature, grid.m_idx);
}

ObjectThrowEntity::ObjectThrowEntity(CreatureEntity &creature, ItemEntity *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken)
    : q_ptr(q_ptr)
    , creature_ptr(&creature)
    , shuriken(shuriken)
    , mult(mult)
    , msec(delay_factor_val)
    , boomerang(boomerang)
{
}

bool ObjectThrowEntity::check_can_throw()
{
    if (!this->check_what_throw()) {
        return false;
    }

    if (this->item->is_cursed() && (this->i_idx >= INVEN_MAIN_HAND)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return false;
    }

    const auto is_spike = this->item->bi_key.tval() == ItemKindType::SPIKE;
    if (this->creature_ptr->get_floor()->inside_arena && !this->boomerang && !is_spike) {
        msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
        msg_erase();
        return false;
    }

    return true;
}

void ObjectThrowEntity::calc_throw_range()
{
    auto &creature = *this->creature_ptr;
    *this->q_ptr = this->item->clone();
    this->obj_flags = this->q_ptr->get_flags();
    torch_flags(this->q_ptr, this->obj_flags);
    distribute_charges(this->item.get(), this->q_ptr, 1);
    this->q_ptr->number = 1;
    this->o_name = describe_flavor(creature, *this->q_ptr, OD_OMIT_PREFIX);
    if (creature.has_mighty_throw()) {
        this->mult += 3;
    }

    auto mul = 10 + 2 * (this->mult - 1);
    auto div = ((this->q_ptr->weight > 10) ? this->q_ptr->weight : 10);
    if ((this->obj_flags.has(TR_THROW)) || this->boomerang) {
        div /= 2;
    }

    this->tdis = (adj_str_blow[creature.stat_index[A_STR]] + 20) * mul / div;
    if (this->tdis > mul) {
        this->tdis = mul;
    }
}

bool ObjectThrowEntity::calc_throw_grid()
{
    auto &creature = *this->creature_ptr;
    if (this->shuriken >= 0) {
        this->ty = randint0(101) - 50 + creature.y;
        this->tx = randint0(101) - 50 + creature.x;
        return true;
    }

    project_length = this->tdis + 1;
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    const auto pos_target = dir.get_target_position(creature.get_position(), 99);
    this->tx = pos_target.x;
    this->ty = pos_target.y;

    project_length = 0;
    return true;
}

void ObjectThrowEntity::reflect_inventory_by_throw()
{
    auto &creature = *this->creature_ptr;
    if (this->q_ptr->is_specific_artifact(FixedArtifactId::MJOLLNIR) || this->q_ptr->is_specific_artifact(FixedArtifactId::AEGISFANG) || this->boomerang) {
        this->return_when_thrown = true;
    }

    if (this->i_idx < 0) {
        floor_item_increase(creature, 0 - this->i_idx, -1);
        floor_item_optimize(creature, 0 - this->i_idx);
        return;
    }

    inven_item_increase(creature, this->i_idx, -1);
    if (!this->return_when_thrown) {
        inven_item_describe(creature, this->i_idx);
    }

    inven_item_optimize(creature, this->i_idx);
}

void ObjectThrowEntity::set_class_specific_throw_params()
{
    auto &creature = *this->creature_ptr;
    PlayerEnergy energy(creature);
    energy.set_player_turn_energy(100);
    CreatureClass pc(creature);
    if (pc.equals(PlayerClassType::ROGUE) || pc.equals(PlayerClassType::NINJA)) {
        energy.sub_player_turn_energy(creature.level);
    }

    this->y = creature.y;
    this->x = creature.x;
    handle_stuff(creature);
    const auto tval = this->q_ptr->bi_key.tval();
    const auto is_spike = tval == ItemKindType::SPIKE;
    const auto is_sword = tval == ItemKindType::SWORD;
    this->shuriken = pc.equals(PlayerClassType::NINJA) && (is_spike || ((this->obj_flags.has(TR_THROW)) && is_sword));
}

void ObjectThrowEntity::set_racial_chance()
{
    auto &creature = *this->creature_ptr;
    auto compensation = this->obj_flags.has(TR_THROW) ? this->q_ptr->to_h : 0;
    this->chance = creature.skill_tht + (creature.to_h_b + compensation) * BTH_PLUS_ADJ;
    if (this->shuriken != 0) {
        this->chance *= 2;
    }
}

void ObjectThrowEntity::exe_throw()
{
    auto &creature = *this->creature_ptr;
    this->cur_dis = 0;
    while (this->cur_dis <= this->tdis) {
        if ((this->y == this->ty) && (this->x == this->tx)) {
            break;
        }

        if (this->check_racial_target_bold()) {
            break;
        }

        this->check_racial_target_seen();
        if (this->check_racial_target_monster()) {
            continue;
        }

        this->hit_monster = ObjectThrowHitMonster(creature, this->y, this->x);
        this->attack_racial_power();
        break;
    }
}

void ObjectThrowEntity::display_figurine_throw()
{
    auto &creature = *this->creature_ptr;
    if ((this->q_ptr->bi_key.tval() != ItemKindType::FIGURINE) || creature.get_floor()->inside_arena) {
        return;
    }

    this->corruption_possibility = 100;
    auto figure_r_idx = i2enum<MonraceId>(this->q_ptr->pval);
    if (!(summon_named_creature(creature, 0, this->y, this->x, figure_r_idx, !(this->q_ptr->is_cursed()) ? PM_FORCE_PET : PM_NONE))) {
        msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
        return;
    }

    if (this->q_ptr->is_cursed()) {
        msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));
    }
}

void ObjectThrowEntity::display_potion_throw()
{
    auto &creature = *this->creature_ptr;
    if (!this->q_ptr->is_potion()) {
        return;
    }

    if (!this->hit_monster && !this->hit_wall && (randint1(100) >= this->corruption_possibility)) {
        this->corruption_possibility = 0;
        return;
    }

    msg_format(_("%sは砕け散った！", "The %s shatters!"), this->o_name.data());
    this->do_drop = false;
    if (!potion_smash_effect(creature, 0, this->y, this->x, this->q_ptr->bi_id)) {
        return;
    }

    if (!this->hit_monster) {
        return;
    }

    auto &monster = *this->hit_monster->m_ptr;
    if (!monster.is_friendly() || monster.is_invulnerable()) {
        return;
    }

    const auto angry_m_name = monster_desc(creature, monster, 0);
    msg_format(_("%sは怒った！", "%s^ gets angry!"), angry_m_name.data());
    monster.set_hostile();
}

void ObjectThrowEntity::check_boomerang_throw()
{
    auto &creature = *this->creature_ptr;
    if (!this->return_when_thrown) {
        return;
    }

    this->back_chance = randint1(30) + 20 + ((int)(adj_dex_th[creature.stat_index[A_DEX]]) - 128);
    this->super_boomerang = ((this->q_ptr->is_specific_artifact(FixedArtifactId::MJOLLNIR) || this->q_ptr->is_specific_artifact(FixedArtifactId::AEGISFANG)) && this->boomerang);
    this->corruption_possibility = -1;
    if (this->boomerang) {
        this->back_chance += 4 + randint1(5);
    }

    if (this->super_boomerang) {
        this->back_chance += 100;
    }

    this->o2_name = describe_flavor(creature, *this->q_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    this->process_boomerang_throw();
}

void ObjectThrowEntity::process_boomerang_back()
{
    auto &creature = *this->creature_ptr;
    if (this->come_back) {
        if ((this->i_idx != INVEN_MAIN_HAND) && (this->i_idx != INVEN_SUB_HAND)) {
            creature.store_item(*this->q_ptr);
            this->do_drop = false;
            return;
        }

        this->item = creature.inventory[this->i_idx];
        *this->item = this->q_ptr->clone();
        this->creature_ptr->equip_cnt++;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        static constexpr auto flags = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::TORCH,
            StatusRecalculatingFlag::MP,
        };
        rfu.set_flags(flags);
        rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
        this->do_drop = false;
        return;
    }

    if (this->equiped_item) {
        verify_equip_slot(creature, this->i_idx);
        calc_android_exp(creature);
    }
}

void ObjectThrowEntity::drop_thrown_item()
{
    auto &creature = *this->creature_ptr;
    if (!this->do_drop) {
        return;
    }

    const auto &floor = *creature.get_floor();
    const auto has_terrain_projection = floor.has_terrain_characteristics({ this->y, this->x }, TerrainCharacteristics::PROJECTION);
    const auto drop_y = has_terrain_projection ? this->y : this->prev_y;
    const auto drop_x = has_terrain_projection ? this->x : this->prev_x;
    drop_ammo_near(creature, *this->q_ptr, { drop_y, drop_x }, this->corruption_possibility);
}

bool ObjectThrowEntity::has_hit_monster() const
{
    return this->hit_monster.has_value();
}

bool ObjectThrowEntity::check_what_throw()
{
    if (this->shuriken >= 0) {
        this->i_idx = this->shuriken;
        this->item = this->creature_ptr->inventory[this->i_idx];
        return true;
    }

    if (this->boomerang) {
        return this->check_throw_boomerang();
    }

    constexpr auto q = _("どのアイテムを投げますか? ", "Throw which item? ");
    constexpr auto s = _("投げるアイテムがない。", "You have nothing to throw.");
    std::tie(this->item, this->i_idx) = choose_item(*this->creature_ptr, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!this->item) {
        flush();
        return false;
    }

    return true;
}

bool ObjectThrowEntity::check_throw_boomerang()
{
    if (has_melee_weapon(*this->creature_ptr, INVEN_MAIN_HAND) && has_melee_weapon(*this->creature_ptr, INVEN_SUB_HAND)) {
        constexpr auto q = _("どの武器を投げますか? ", "Throw which item? ");
        constexpr auto s = _("投げる武器がない。", "You have nothing to throw.");
        std::tie(this->item, this->i_idx) = choose_item(*this->creature_ptr, q, s, USE_EQUIP, FuncItemTester(&ItemEntity::is_throwable));
        if (!this->item) {
            flush();
            return false;
        }

        return true;
    }

    if (has_melee_weapon(*this->creature_ptr, INVEN_SUB_HAND)) {
        this->i_idx = INVEN_SUB_HAND;
        this->item = this->creature_ptr->inventory[this->i_idx];
        return true;
    }

    this->i_idx = INVEN_MAIN_HAND;
    this->item = this->creature_ptr->inventory[this->i_idx];
    return true;
}

bool ObjectThrowEntity::check_racial_target_bold()
{
    auto &creature = *this->creature_ptr;
    const auto pos = mmove2({ this->y, this->x }, creature.get_position(), { this->ty, this->tx });
    this->ny[this->cur_dis] = pos.y;
    this->nx[this->cur_dis] = pos.x;
    const auto &floor = *creature.get_floor();
    if (floor.has_terrain_characteristics({ this->ny[this->cur_dis], this->nx[this->cur_dis] }, TerrainCharacteristics::PROJECTION)) {
        return false;
    }

    this->hit_wall = true;
    const auto is_figurine = this->q_ptr->bi_key.tval() == ItemKindType::FIGURINE;
    return is_figurine || this->q_ptr->is_potion() || (floor.grid_array[this->ny[this->cur_dis]][this->nx[this->cur_dis]].m_idx == 0);
}

void ObjectThrowEntity::check_racial_target_seen()
{
    auto &creature = *this->creature_ptr;
    if (!panel_contains({ this->ny[this->cur_dis], this->nx[this->cur_dis] }) || !player_can_see_bold(creature, this->ny[this->cur_dis], this->nx[this->cur_dis])) {
        term_xtra(TERM_XTRA_DELAY, this->msec);
        return;
    }

    if (this->msec <= 0) {
        return;
    }

    const auto symbol = this->q_ptr->get_symbol();
    print_rel(creature, symbol, { this->ny[this->cur_dis], this->nx[this->cur_dis] });
    move_cursor_relative(this->ny[this->cur_dis], this->nx[this->cur_dis]);
    term_fresh();
    term_xtra(TERM_XTRA_DELAY, this->msec);
    lite_spot(creature, { this->ny[this->cur_dis], this->nx[this->cur_dis] });
    term_fresh();
}

bool ObjectThrowEntity::check_racial_target_monster()
{
    auto &creature = *this->creature_ptr;
    this->prev_y = this->y;
    this->prev_x = this->x;
    this->x = this->nx[this->cur_dis];
    this->y = this->ny[this->cur_dis];
    this->cur_dis++;
    return creature.get_floor()->grid_array[this->y][this->x].m_idx == 0;
}

void ObjectThrowEntity::attack_racial_power()
{
    auto &creature = *this->creature_ptr;
    if (!this->hit_monster) {
        return;
    }

    auto &monster = *this->hit_monster->m_ptr;
    if (!test_hit_fire(creature, this->chance - this->cur_dis, monster, monster.is_visible_on_map(), this->o_name)) {
        return;
    }

    this->display_attack_racial_power();
    this->calc_racial_power_damage();
    msg_format_wizard(creature, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), this->tdam,
        monster.hp - this->tdam, monster.maxhp, monster.max_maxhp);

    auto fear = false;
    AttributeFlags attribute_flags{};
    attribute_flags.set(AttributeType::PLAYER_SHOOT);
    if (is_active_torch(this->item.get())) {
        attribute_flags.set(AttributeType::FIRE);
    }

    MonsterDamageProcessor mdp(creature, this->hit_monster->m_idx, this->tdam, &fear, attribute_flags);
    if (mdp.mon_take_hit(monster.get_died_message())) {
        return;
    }
    const auto pain_message = monster.get_pain_message(this->hit_monster->m_name, this->tdam);
    if (pain_message) {
        msg_print(*pain_message);
    }

    if ((this->tdam > 0) && !this->q_ptr->is_potion()) {
        anger_monster(creature, monster);
    }

    if (fear && monster.is_visible_on_map()) {
        sound(SoundKind::FLEE);
        msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), this->hit_monster->m_name.data());
    }
}

void ObjectThrowEntity::display_attack_racial_power()
{
    auto &creature = *this->creature_ptr;
    if (!this->hit_monster) {
        return;
    }

    if (!this->hit_monster->m_ptr->is_visible_on_map()) {
        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), this->o_name.data());
        return;
    }

    msg_format(_("%sが%sに命中した。", "The %s hits %s."), this->o_name.data(), this->hit_monster->m_name.data());

    if (!creature.is_hallucinated()) {
        LoreTracker::get_instance().set_trackee(this->hit_monster->m_ptr->ap_r_idx);
    }

    health_track(creature, this->hit_monster->m_idx);
}

void ObjectThrowEntity::calc_racial_power_damage()
{
    auto &creature = *this->creature_ptr;
    if (!this->hit_monster) {
        return;
    }

    const auto damage_dice = is_active_torch(this->item.get()) ? Dice(1, 6) : this->q_ptr->damage_dice;
    this->tdam = damage_dice.roll();
    this->tdam = calc_attack_damage_with_slay(creature, this->q_ptr, this->tdam, *this->hit_monster->m_ptr, HISSATSU_NONE, true);
    this->tdam = critical_shot(creature, this->q_ptr->weight, this->q_ptr->to_h, 0, this->tdam);
    this->tdam += (this->q_ptr->to_d > 0 ? 1 : -1) * this->q_ptr->to_d;
    if (this->boomerang) {
        this->tdam *= (this->mult + creature.num_blow[this->i_idx - INVEN_MAIN_HAND]);
        this->tdam += creature.to_d_m;
    } else if (this->obj_flags.has(TR_THROW)) {
        this->tdam *= (3 + this->mult);
        this->tdam += creature.to_d_m;
    } else {
        this->tdam *= this->mult;
    }

    if (this->shuriken != 0) {
        this->tdam += ((creature.level + 30) * (creature.level + 30) - 900) / 55;
    }

    if (this->tdam < 0) {
        this->tdam = 0;
    }

    this->tdam = mon_damage_mod(creature, *this->hit_monster->m_ptr, this->tdam, false);
}

void ObjectThrowEntity::process_boomerang_throw()
{
    auto &creature = *this->creature_ptr;
    if ((this->back_chance <= 30) || (one_in_(100) && !this->super_boomerang)) {
        msg_format(_("%sが返ってこなかった！", "%s doesn't come back!"), this->o2_name.data());
        return;
    }

    for (auto i = this->cur_dis - 1; i > 0; i--) {
        if (!panel_contains({ this->ny[i], this->nx[i] }) || !player_can_see_bold(creature, this->ny[i], this->nx[i])) {
            term_xtra(TERM_XTRA_DELAY, this->msec);
            continue;
        }

        if (this->msec <= 0) {
            continue;
        }

        const auto symbol = this->q_ptr->get_symbol();
        print_rel(creature, symbol, { this->ny[i], this->nx[i] });
        move_cursor_relative(this->ny[i], this->nx[i]);
        term_fresh();
        term_xtra(TERM_XTRA_DELAY, this->msec);
        lite_spot(creature, { this->ny[i], this->nx[i] });
        term_fresh();
    }

    this->display_boomerang_throw();
}

void ObjectThrowEntity::display_boomerang_throw()
{
    const auto is_blind = this->creature_ptr->is_blind();
    if ((this->back_chance > 37) && !is_blind && (this->i_idx >= 0)) {
        msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), this->o2_name.data());
        this->come_back = true;
        return;
    }

    auto back_message = this->i_idx >= 0 ? _("%sを受け損ねた！", "%s comes back, but you can't catch!") : _("%sが返ってきた。", "%s comes back.");
    msg_format(back_message, this->o2_name.data());
    this->y = this->creature_ptr->y;
    this->x = this->creature_ptr->x;
}
