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
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "wizard/wizard-messages.h"

ObjectThrowHitMonster::ObjectThrowHitMonster(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid({ y, x });
    if (!grid.has_monster() || std::cmp_greater_equal(grid.m_idx, floor.m_list.size())) {
        THROW_EXCEPTION(std::logic_error, "Invalid monster index");
    }

    this->m_idx = grid.m_idx;
    this->m_ptr = &floor.m_list[grid.m_idx];
    this->m_name = monster_name(player_ptr, grid.m_idx);
}

ObjectThrowEntity::ObjectThrowEntity(PlayerType *player_ptr, ItemEntity *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken)
    : q_ptr(q_ptr)
    , player_ptr(player_ptr)
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

    if (this->o_ptr->is_cursed() && (this->i_idx >= INVEN_MAIN_HAND)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return false;
    }

    const auto is_spike = this->o_ptr->bi_key.tval() == ItemKindType::SPIKE;
    if (this->player_ptr->current_floor_ptr->inside_arena && !this->boomerang && !is_spike) {
        msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
        msg_erase();
        return false;
    }

    return true;
}

void ObjectThrowEntity::calc_throw_range()
{
    *this->q_ptr = this->o_ptr->clone();
    this->obj_flags = this->q_ptr->get_flags();
    torch_flags(this->q_ptr, this->obj_flags);
    distribute_charges(this->o_ptr, this->q_ptr, 1);
    this->q_ptr->number = 1;
    this->o_name = describe_flavor(this->player_ptr, *this->q_ptr, OD_OMIT_PREFIX);
    if (this->player_ptr->mighty_throw) {
        this->mult += 3;
    }

    auto mul = 10 + 2 * (this->mult - 1);
    auto div = ((this->q_ptr->weight > 10) ? this->q_ptr->weight : 10);
    if ((this->obj_flags.has(TR_THROW)) || this->boomerang) {
        div /= 2;
    }

    this->tdis = (adj_str_blow[this->player_ptr->stat_index[A_STR]] + 20) * mul / div;
    if (this->tdis > mul) {
        this->tdis = mul;
    }
}

bool ObjectThrowEntity::calc_throw_grid()
{
    if (this->shuriken >= 0) {
        this->ty = randint0(101) - 50 + this->player_ptr->y;
        this->tx = randint0(101) - 50 + this->player_ptr->x;
        return true;
    }

    project_length = this->tdis + 1;
    const auto dir = get_aim_dir(this->player_ptr);
    if (!dir) {
        return false;
    }

    const auto pos_target = dir.get_target_position(this->player_ptr->get_position(), 99);
    this->tx = pos_target.x;
    this->ty = pos_target.y;

    project_length = 0;
    return true;
}

void ObjectThrowEntity::reflect_inventory_by_throw()
{
    if (this->q_ptr->is_specific_artifact(FixedArtifactId::MJOLLNIR) || this->q_ptr->is_specific_artifact(FixedArtifactId::AEGISFANG) || this->boomerang) {
        this->return_when_thrown = true;
    }

    if (this->i_idx < 0) {
        floor_item_increase(this->player_ptr, 0 - this->i_idx, -1);
        floor_item_optimize(this->player_ptr, 0 - this->i_idx);
        return;
    }

    inven_item_increase(this->player_ptr, this->i_idx, -1);
    if (!this->return_when_thrown) {
        inven_item_describe(this->player_ptr, this->i_idx);
    }

    inven_item_optimize(this->player_ptr, this->i_idx);
}

void ObjectThrowEntity::set_class_specific_throw_params()
{
    PlayerEnergy energy(this->player_ptr);
    energy.set_player_turn_energy(100);
    PlayerClass pc(this->player_ptr);
    if (pc.equals(PlayerClassType::ROGUE) || pc.equals(PlayerClassType::NINJA)) {
        energy.sub_player_turn_energy(this->player_ptr->lev);
    }

    this->y = this->player_ptr->y;
    this->x = this->player_ptr->x;
    handle_stuff(this->player_ptr);
    const auto tval = this->q_ptr->bi_key.tval();
    const auto is_spike = tval == ItemKindType::SPIKE;
    const auto is_sword = tval == ItemKindType::SWORD;
    this->shuriken = pc.equals(PlayerClassType::NINJA) && (is_spike || ((this->obj_flags.has(TR_THROW)) && is_sword));
}

void ObjectThrowEntity::set_racial_chance()
{
    auto compensation = this->obj_flags.has(TR_THROW) ? this->q_ptr->to_h : 0;
    this->chance = this->player_ptr->skill_tht + (this->player_ptr->to_h_b + compensation) * BTH_PLUS_ADJ;
    if (this->shuriken != 0) {
        this->chance *= 2;
    }
}

void ObjectThrowEntity::exe_throw()
{
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

        this->hit_monster = ObjectThrowHitMonster(this->player_ptr, this->y, this->x);
        this->attack_racial_power();
        break;
    }
}

void ObjectThrowEntity::display_figurine_throw()
{
    if ((this->q_ptr->bi_key.tval() != ItemKindType::FIGURINE) || this->player_ptr->current_floor_ptr->inside_arena) {
        return;
    }

    this->corruption_possibility = 100;
    auto figure_r_idx = i2enum<MonraceId>(this->q_ptr->pval);
    if (!(summon_named_creature(this->player_ptr, 0, this->y, this->x, figure_r_idx, !(this->q_ptr->is_cursed()) ? PM_FORCE_PET : PM_NONE))) {
        msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
        return;
    }

    if (this->q_ptr->is_cursed()) {
        msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));
    }
}

void ObjectThrowEntity::display_potion_throw()
{
    if (!this->q_ptr->is_potion()) {
        return;
    }

    if (!this->hit_monster && !this->hit_wall && (randint1(100) >= this->corruption_possibility)) {
        this->corruption_possibility = 0;
        return;
    }

    msg_format(_("%sは砕け散った！", "The %s shatters!"), this->o_name.data());
    this->do_drop = false;
    if (!potion_smash_effect(this->player_ptr, 0, this->y, this->x, this->q_ptr->bi_id)) {
        return;
    }

    if (!this->hit_monster) {
        return;
    }

    auto &monster = *this->hit_monster->m_ptr;
    if (!monster.is_friendly() || monster.is_invulnerable()) {
        return;
    }

    const auto angry_m_name = monster_desc(this->player_ptr, monster, 0);
    msg_format(_("%sは怒った！", "%s^ gets angry!"), angry_m_name.data());
    monster.set_hostile();
}

void ObjectThrowEntity::check_boomerang_throw()
{
    if (!this->return_when_thrown) {
        return;
    }

    this->back_chance = randint1(30) + 20 + ((int)(adj_dex_th[this->player_ptr->stat_index[A_DEX]]) - 128);
    this->super_boomerang = ((this->q_ptr->is_specific_artifact(FixedArtifactId::MJOLLNIR) || this->q_ptr->is_specific_artifact(FixedArtifactId::AEGISFANG)) && this->boomerang);
    this->corruption_possibility = -1;
    if (this->boomerang) {
        this->back_chance += 4 + randint1(5);
    }

    if (this->super_boomerang) {
        this->back_chance += 100;
    }

    this->o2_name = describe_flavor(this->player_ptr, *this->q_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    this->process_boomerang_throw();
}

void ObjectThrowEntity::process_boomerang_back()
{
    if (this->come_back) {
        if ((this->i_idx != INVEN_MAIN_HAND) && (this->i_idx != INVEN_SUB_HAND)) {
            store_item_to_inventory(this->player_ptr, this->q_ptr);
            this->do_drop = false;
            return;
        }

        this->o_ptr = player_ptr->inventory[this->i_idx].get();
        *this->o_ptr = this->q_ptr->clone();
        this->player_ptr->equip_cnt++;
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
        verify_equip_slot(this->player_ptr, this->i_idx);
        calc_android_exp(this->player_ptr);
    }
}

void ObjectThrowEntity::drop_thrown_item()
{
    if (!this->do_drop) {
        return;
    }

    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto has_terrain_projection = floor.has_terrain_characteristics({ this->y, this->x }, TerrainCharacteristics::PROJECTION);
    const auto drop_y = has_terrain_projection ? this->y : this->prev_y;
    const auto drop_x = has_terrain_projection ? this->x : this->prev_x;
    drop_ammo_near(this->player_ptr, *this->q_ptr, { drop_y, drop_x }, this->corruption_possibility);
}

bool ObjectThrowEntity::has_hit_monster() const
{
    return this->hit_monster.has_value();
}

bool ObjectThrowEntity::check_what_throw()
{
    if (this->shuriken >= 0) {
        this->i_idx = this->shuriken;
        this->o_ptr = this->player_ptr->inventory[this->i_idx].get();
        return true;
    }

    if (this->boomerang) {
        return this->check_throw_boomerang();
    }

    constexpr auto q = _("どのアイテムを投げますか? ", "Throw which item? ");
    constexpr auto s = _("投げるアイテムがない。", "You have nothing to throw.");
    this->o_ptr = choose_object(this->player_ptr, &this->i_idx, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!this->o_ptr) {
        flush();
        return false;
    }

    return true;
}

bool ObjectThrowEntity::check_throw_boomerang()
{
    if (has_melee_weapon(this->player_ptr, INVEN_MAIN_HAND) && has_melee_weapon(this->player_ptr, INVEN_SUB_HAND)) {
        concptr q, s;
        q = _("どの武器を投げますか? ", "Throw which item? ");
        s = _("投げる武器がない。", "You have nothing to throw.");
        this->o_ptr = choose_object(this->player_ptr, &this->i_idx, q, s, USE_EQUIP, FuncItemTester(&ItemEntity::is_throwable));
        if (!this->o_ptr) {
            flush();
            return false;
        }

        return true;
    }

    if (has_melee_weapon(this->player_ptr, INVEN_SUB_HAND)) {
        this->i_idx = INVEN_SUB_HAND;
        this->o_ptr = this->player_ptr->inventory[this->i_idx].get();
        return true;
    }

    this->i_idx = INVEN_MAIN_HAND;
    this->o_ptr = this->player_ptr->inventory[this->i_idx].get();
    return true;
}

bool ObjectThrowEntity::check_racial_target_bold()
{
    const auto pos = mmove2({ this->y, this->x }, this->player_ptr->get_position(), { this->ty, this->tx });
    this->ny[this->cur_dis] = pos.y;
    this->nx[this->cur_dis] = pos.x;
    const auto &floor = *this->player_ptr->current_floor_ptr;
    if (floor.has_terrain_characteristics({ this->ny[this->cur_dis], this->nx[this->cur_dis] }, TerrainCharacteristics::PROJECTION)) {
        return false;
    }

    this->hit_wall = true;
    const auto is_figurine = this->q_ptr->bi_key.tval() == ItemKindType::FIGURINE;
    return is_figurine || this->q_ptr->is_potion() || (floor.grid_array[this->ny[this->cur_dis]][this->nx[this->cur_dis]].m_idx == 0);
}

void ObjectThrowEntity::check_racial_target_seen()
{
    if (!panel_contains(this->ny[this->cur_dis], this->nx[this->cur_dis]) || !player_can_see_bold(this->player_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis])) {
        term_xtra(TERM_XTRA_DELAY, this->msec);
        return;
    }

    if (this->msec <= 0) {
        return;
    }

    const auto symbol = this->q_ptr->get_symbol();
    print_rel(this->player_ptr, symbol, this->ny[this->cur_dis], this->nx[this->cur_dis]);
    move_cursor_relative(this->ny[this->cur_dis], this->nx[this->cur_dis]);
    term_fresh();
    term_xtra(TERM_XTRA_DELAY, this->msec);
    lite_spot(this->player_ptr, { this->ny[this->cur_dis], this->nx[this->cur_dis] });
    term_fresh();
}

bool ObjectThrowEntity::check_racial_target_monster()
{
    this->prev_y = this->y;
    this->prev_x = this->x;
    this->x = this->nx[this->cur_dis];
    this->y = this->ny[this->cur_dis];
    this->cur_dis++;
    return this->player_ptr->current_floor_ptr->grid_array[this->y][this->x].m_idx == 0;
}

void ObjectThrowEntity::attack_racial_power()
{
    if (!this->hit_monster) {
        return;
    }

    auto &monster = *this->hit_monster->m_ptr;
    if (!test_hit_fire(this->player_ptr, this->chance - this->cur_dis, monster, monster.ml, this->o_name)) {
        return;
    }

    this->display_attack_racial_power();
    this->calc_racial_power_damage();
    msg_format_wizard(this->player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), this->tdam,
        monster.hp - this->tdam, monster.maxhp, monster.max_maxhp);

    auto fear = false;
    AttributeFlags attribute_flags{};
    attribute_flags.set(AttributeType::PLAYER_SHOOT);
    if (is_active_torch(this->o_ptr)) {
        attribute_flags.set(AttributeType::FIRE);
    }

    MonsterDamageProcessor mdp(this->player_ptr, this->hit_monster->m_idx, this->tdam, &fear, attribute_flags);
    if (mdp.mon_take_hit(monster.get_died_message())) {
        return;
    }
    const auto pain_message = monster.get_pain_message(this->hit_monster->m_name, this->tdam);
    if (pain_message) {
        msg_print(*pain_message);
    }

    if ((this->tdam > 0) && !this->q_ptr->is_potion()) {
        anger_monster(this->player_ptr, monster);
    }

    if (fear && monster.ml) {
        sound(SoundKind::FLEE);
        msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), this->hit_monster->m_name.data());
    }
}

void ObjectThrowEntity::display_attack_racial_power()
{
    if (!this->hit_monster) {
        return;
    }

    if (!this->hit_monster->m_ptr->ml) {
        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), this->o_name.data());
        return;
    }

    msg_format(_("%sが%sに命中した。", "The %s hits %s."), this->o_name.data(), this->hit_monster->m_name.data());

    if (!this->player_ptr->effects()->hallucination().is_hallucinated()) {
        LoreTracker::get_instance().set_trackee(this->hit_monster->m_ptr->ap_r_idx);
    }

    health_track(this->player_ptr, this->hit_monster->m_idx);
}

void ObjectThrowEntity::calc_racial_power_damage()
{
    if (!this->hit_monster) {
        return;
    }

    const auto damage_dice = is_active_torch(this->o_ptr) ? Dice(1, 6) : this->q_ptr->damage_dice;
    this->tdam = damage_dice.roll();
    this->tdam = calc_attack_damage_with_slay(this->player_ptr, this->q_ptr, this->tdam, *this->hit_monster->m_ptr, HISSATSU_NONE, true);
    this->tdam = critical_shot(this->player_ptr, this->q_ptr->weight, this->q_ptr->to_h, 0, this->tdam);
    this->tdam += (this->q_ptr->to_d > 0 ? 1 : -1) * this->q_ptr->to_d;
    if (this->boomerang) {
        this->tdam *= (this->mult + this->player_ptr->num_blow[this->i_idx - INVEN_MAIN_HAND]);
        this->tdam += this->player_ptr->to_d_m;
    } else if (this->obj_flags.has(TR_THROW)) {
        this->tdam *= (3 + this->mult);
        this->tdam += this->player_ptr->to_d_m;
    } else {
        this->tdam *= this->mult;
    }

    if (this->shuriken != 0) {
        this->tdam += ((this->player_ptr->lev + 30) * (this->player_ptr->lev + 30) - 900) / 55;
    }

    if (this->tdam < 0) {
        this->tdam = 0;
    }

    this->tdam = mon_damage_mod(this->player_ptr, *this->hit_monster->m_ptr, this->tdam, false);
}

void ObjectThrowEntity::process_boomerang_throw()
{
    if ((this->back_chance <= 30) || (one_in_(100) && !this->super_boomerang)) {
        msg_format(_("%sが返ってこなかった！", "%s doesn't come back!"), this->o2_name.data());
        return;
    }

    for (auto i = this->cur_dis - 1; i > 0; i--) {
        if (!panel_contains(this->ny[i], this->nx[i]) || !player_can_see_bold(this->player_ptr, this->ny[i], this->nx[i])) {
            term_xtra(TERM_XTRA_DELAY, this->msec);
            continue;
        }

        if (this->msec <= 0) {
            continue;
        }

        const auto symbol = this->q_ptr->get_symbol();
        print_rel(this->player_ptr, symbol, this->ny[i], this->nx[i]);
        move_cursor_relative(this->ny[i], this->nx[i]);
        term_fresh();
        term_xtra(TERM_XTRA_DELAY, this->msec);
        lite_spot(this->player_ptr, { this->ny[i], this->nx[i] });
        term_fresh();
    }

    this->display_boomerang_throw();
}

void ObjectThrowEntity::display_boomerang_throw()
{
    const auto is_blind = this->player_ptr->effects()->blindness().is_blind();
    if ((this->back_chance > 37) && !is_blind && (this->i_idx >= 0)) {
        msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), this->o2_name.data());
        this->come_back = true;
        return;
    }

    auto back_message = this->i_idx >= 0 ? _("%sを受け損ねた！", "%s comes back, but you can't catch!") : _("%sが返ってきた。", "%s comes back.");
    msg_format(back_message, this->o2_name.data());
    this->y = this->player_ptr->y;
    this->x = this->player_ptr->x;
}
