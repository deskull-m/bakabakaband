#include "mind/mind-force-trainer.h"
#include "action/travel-execution.h"
#include "avatar/avatar.h"
#include "combat/damage-dispatcher.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-numbers.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/force-trainer-data-type.h"
#include "player/player-damage.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell/summon-types.h"
#include "status/temporary-resistance.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 練気術師が「練気」で溜めた気の量を返す
 * @param player_ptr プレイヤーの参照ポインタ
 * @return 現在溜まっている気の量
 */
int32_t get_current_ki(CreatureEntity &creature)
{
    auto data = CreatureClass(creature).get_specific_data<force_trainer_data_type>();

    return data ? data->ki : 0;
}

/*!
 * @brief 練気術師において、気を溜める
 * @param player_ptr プレイヤーの参照ポインタ
 * @param is_reset TRUEなら気の量をkiにセットし、FALSEなら加減算を行う
 * @param ki 気の量
 */
void set_current_ki(CreatureEntity &creature, bool is_reset, int32_t ki)
{
    auto data = CreatureClass(creature).get_specific_data<force_trainer_data_type>();
    if (!data) {
        return;
    }

    if (is_reset) {
        data->ki = ki;
        return;
    }

    data->ki += ki;
}

bool clear_mind(CreatureEntity &creature)
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return false;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    creature.csp += (3 + creature.level / 20);
    if (creature.csp >= creature.msp) {
        creature.csp = creature.msp;
        creature.csp_frac = 0;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
    return true;
}

/*!
 * @brief 光速移動の継続時間をセットする / Set "lightspeed", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 */
void set_lightspeed(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return;
    }

    if (AngbandWorld::get_instance().is_wild_mode()) {
        v = 0;
    }

    if (v) {
        if (creature.lightspeed && !do_dec) {
            if (creature.lightspeed > v) {
                return;
            }
        } else if (!creature.lightspeed) {
            msg_print(_("非常に素早く動けるようになった！", "You feel yourself moving extremely fast!"));
            notice = true;
            chg_virtue(creature, Virtue::PATIENCE, -1);
            chg_virtue(creature, Virtue::DILIGENCE, 1);
        }
    } else {
        if (creature.lightspeed) {
            msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            notice = true;
        }
    }

    creature.lightspeed = v;

    if (!notice) {
        return;
    }

    if (disturb_state) {
        disturb(creature, false, false);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
}

/*!
 * @brief 一時的闘気のオーラの継続時間をセットする / Set "tim_sh_touki", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_force(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.tim_sh_touki && !do_dec) {
            if (creature.tim_sh_touki > v) {
                return false;
            }
        } else if (!creature.tim_sh_touki) {
            msg_print(_("体が闘気のオーラで覆われた。", "You are enveloped by an aura of the Force!"));
            notice = true;
        }
    } else {
        if (creature.tim_sh_touki) {
            msg_print(_("闘気が消えた。", "The aura of the Force disappeared."));
            notice = true;
        }
    }

    creature.tim_sh_touki = v;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    handle_stuff(creature);
    return true;
}

/*!
 * @brief 衝波
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 命中したらTRUE
 */
bool shock_power(CreatureEntity &creature)
{
    auto boost = get_current_ki(creature);
    if (heavy_armor(creature)) {
        boost /= 2;
    }

    project_length = 1;
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    auto pos = creature.get_neighbor(dir);
    PLAYER_LEVEL plev = creature.level;
    const auto dam = Dice::roll(8 + ((plev - 5) / 4) + boost / 12, 8);
    fire_beam(creature, AttributeType::MISSILE, dir, dam);
    auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid(pos);
    if (!grid.has_monster()) {
        return true;
    }

    auto pos_target = pos;
    const auto pos_origin = pos;
    const auto m_idx = grid.m_idx;
    auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    const auto m_name = monster_desc(creature, monster, 0);

    if (randint1(monrace.level * 3 / 2) > randint0(dam / 2) + dam / 2) {
        msg_format(_("%sは飛ばされなかった。", "%s^ was not blown away."), m_name.data());
        return true;
    }

    const auto p_pos = creature.get_position();
    for (auto i = 0; i < 5; i++) {
        pos += dir.vec();
        if (floor.is_empty_at(pos) && (pos != p_pos)) {
            pos_target = pos;
        } else {
            break;
        }
    }

    if (pos_target == pos_origin) {
        return true;
    }

    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name.data());
    floor.get_grid(pos_origin).m_idx = 0;
    floor.get_grid(pos_target).m_idx = m_idx;
    monster.y = pos_target.y;
    monster.x = pos_target.x;

    update_monster(creature, m_idx, true);
    lite_spot(creature, pos_origin);
    lite_spot(creature, pos_target);

    if (monrace.brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    return true;
}

/*!
 * @brief 練気術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ForceTrainer'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_force_spell(CreatureEntity &creature, MindForceTrainerType spell)
{
    PLAYER_LEVEL plev = creature.level;
    int boost = get_current_ki(creature);
    if (heavy_armor(creature)) {
        boost /= 2;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    switch (spell) {
    case MindForceTrainerType::SMALL_FORCE_BALL: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_ball(creature, AttributeType::MISSILE, dir, Dice::roll(3 + ((plev - 1) / 5) + boost / 12, 4), 0);
        break;
    }
    case MindForceTrainerType::FLASH_LIGHT:
        (void)lite_area(creature, Dice::roll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case MindForceTrainerType::FLYING_TECHNIQUE:
        set_tim_levitation(creature, randint1(30) + 30 + boost / 5, false);
        break;
    case MindForceTrainerType::KAMEHAMEHA: {
        project_length = plev / 8 + 3;
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_beam(creature, AttributeType::MISSILE, dir, Dice::roll(5 + ((plev - 1) / 5) + boost / 10, 5));
        break;
    }
    case MindForceTrainerType::MAGIC_RESISTANCE:
        set_resist_magic(creature, randint1(20) + 20 + boost / 5, false);
        break;
    case MindForceTrainerType::IMPROVE_FORCE:
        msg_print(_("気を練った。", "You improved the Force."));
        set_current_ki(creature, false, 70 + plev);
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        if (randint1(get_current_ki(creature)) > (plev * 4 + 120)) {
            msg_print(_("気が暴走した！", "The Force exploded!"));
            fire_ball(creature, AttributeType::MANA, Direction::self(), get_current_ki(creature) / 2, 10);
            auto data = CreatureClass(creature).get_specific_data<force_trainer_data_type>();
            apply_damage_to_creature(creature, DAMAGE_LOSELIFE, data->ki / 2, _("気の暴走", "Explosion of the Force"));
        } else {
            return true;
        }

        break;
    case MindForceTrainerType::AURA_OF_FORCE:
        set_tim_sh_force(creature, randint1(plev / 2) + 15 + boost / 7, false);
        break;
    case MindForceTrainerType::SHOCK_POWER:
        return shock_power(creature);
        break;
    case MindForceTrainerType::LARGE_FORCE_BALL: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_ball(creature, AttributeType::MISSILE, dir, Dice::roll(10, 6) + plev * 3 / 2 + boost * 3 / 5, (plev < 30) ? 2 : 3);
        break;
    }
    case MindForceTrainerType::DISPEL_MAGIC: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        const auto &floor = *creature.get_floor();
        const auto &grid = floor.get_grid(*pos);
        const auto m_idx = grid.m_idx;
        const auto p_pos = creature.get_position();
        const auto is_projectable = projectable(floor, p_pos, *pos);
        if ((m_idx == 0) || !grid.has_los() || !is_projectable) {
            break;
        }

        dispel_monster_status(creature, m_idx);
        break;
    }
    case MindForceTrainerType::SUMMON_GHOST: {
        bool success = false;
        for (int i = 0; i < 1 + boost / 100; i++) {
            if (summon_specific(creature, creature.y, creature.x, plev, SUMMON_PHANTOM, PM_FORCE_PET)) {
                success = true;
            }
        }

        if (success) {
            msg_print(_("御用でございますが、御主人様？", "'Your wish, master?'"));
        } else {
            msg_print(_("何も現れなかった。", "Nothing happens."));
        }

        break;
    }
    case MindForceTrainerType::EXPLODING_FLAME:
        fire_ball(creature, AttributeType::FIRE, Direction::self(), 200 + (2 * plev) + boost * 2, 10);
        break;
    case MindForceTrainerType::SUPER_KAMEHAMEHA: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_beam(creature, AttributeType::MANA, dir, Dice::roll(10 + (plev / 2) + boost * 3 / 10, 15));
        break;
    }
    case MindForceTrainerType::LIGHT_SPEED:
        set_lightspeed(creature, randint1(16) + 16 + boost / 20, false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    set_current_ki(creature, true, 0);
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    return true;
}
