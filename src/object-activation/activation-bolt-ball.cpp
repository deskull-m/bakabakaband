#include "object-activation/activation-bolt-ball.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-util.h"
#include "hpmp/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-specific-bolt.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_missile_1(CreatureEntity &creature)
{
    msg_print(_("それは眩しいくらいに明るく輝いている...", "It glows extremely brightly..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::MISSILE, dir, Dice::roll(2, 6));
    return true;
}

bool activate_missile_2(CreatureEntity &creature)
{
    msg_print(_("魔法のトゲが現れた...", "It grows magical spikes..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::MISSILE, dir, 150);
    return true;
}

bool activate_missile_3(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
    fire_breath(creature, AttributeType::MISSILE, dir, 300, 4);
    return true;
}

bool activate_bolt_acid_1(CreatureEntity &creature)
{
    msg_print(_("それは酸に覆われた...", "It is covered in acid..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::ACID, dir, Dice::roll(5, 8));
    return true;
}

bool activate_bolt_elec_1(CreatureEntity &creature)
{
    msg_print(_("それは火花に覆われた...", "It is covered in sparks..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::ELEC, dir, Dice::roll(4, 8));
    return true;
}

bool activate_bolt_fire_1(CreatureEntity &creature)
{
    msg_print(_("それは炎に覆われた...", "It is covered in fire..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::FIRE, dir, Dice::roll(9, 8));
    return true;
}

bool activate_bolt_cold_1(CreatureEntity &creature)
{
    msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::COLD, dir, Dice::roll(6, 8));
    return true;
}

bool activate_bolt_hypodynamia_1(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("あなたは%sに敵を締め殺すよう命じた。", "You order the %s to strangle your opponent."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    hypodynamic_bolt(creature, dir, 100);
    return true;
}

bool activate_bolt_hypodynamia_2(CreatureEntity &creature)
{
    msg_print(_("黒く輝いている...", "It glows black..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    hypodynamic_bolt(creature, dir, 120);
    return true;
}

bool activate_bolt_drain_1(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    for (int dummy = 0; dummy < 3; dummy++) {
        if (hypodynamic_bolt(creature, dir, 50)) {
            hp_player(creature, 50);
        }
    }

    return true;
}

bool activate_bolt_drain_2(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    for (int dummy = 0; dummy < 3; dummy++) {
        if (hypodynamic_bolt(creature, dir, 100)) {
            hp_player(creature, 100);
        }
    }

    return true;
}

bool activate_bolt_mana(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("% sに魔法のトゲが現れた...", "The %s grows magical spikes..."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_bolt(creature, AttributeType::MISSILE, dir, 150);
    return true;
}

bool activate_ball_pois_1(CreatureEntity &creature)
{
    msg_print(_("それは濃緑色に脈動している...", "It throbs deep green..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::POIS, dir, 12, 3);
    return true;
}

bool activate_ball_cold_1(CreatureEntity &creature)
{
    msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::COLD, dir, 48, 2);
    return true;
}

bool activate_ball_cold_2(CreatureEntity &creature)
{
    msg_print(_("それは青く激しく輝いた...", "It glows an intense blue..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::COLD, dir, 100, 2);
    return true;
}

bool activate_ball_cold_3(CreatureEntity &creature)
{
    msg_print(_("明るく白色に輝いている...", "It glows bright white..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::COLD, dir, 400, 3);
    return true;
}

bool activate_ball_fire_1(CreatureEntity &creature)
{
    msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::FIRE, dir, 72, 2);
    return true;
}

bool activate_ball_fire_2(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("% sから炎が吹き出した...", "The %s rages in fire..."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::FIRE, dir, 120, 3);
    return true;
}

bool activate_ball_fire_3(CreatureEntity &creature)
{
    msg_print(_("深赤色に輝いている...", "It glows deep red..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::FIRE, dir, 300, 3);
    return true;
}

bool activate_ball_fire_4(CreatureEntity &creature)
{
    msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::FIRE, dir, 100, 2);
    return true;
}

bool activate_ball_elec_2(CreatureEntity &creature)
{
    msg_print(_("電気がパチパチ音を立てた...", "It crackles with electricity..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::ELEC, dir, 100, 3);
    return true;
}

bool activate_ball_elec_3(CreatureEntity &creature)
{
    msg_print(_("深青色に輝いている...", "It glows deep blue..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::ELEC, dir, 500, 3);
    return true;
}

bool activate_ball_acid_1(CreatureEntity &creature)
{
    msg_print(_("それは黒く激しく輝いた...", "It glows an intense black..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::ACID, dir, 100, 2);
    return true;
}

bool activate_ball_nuke_1(CreatureEntity &creature)
{
    msg_print(_("それは緑に激しく輝いた...", "It glows an intense green..."));
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::NUKE, dir, 100, 2);
    return true;
}

bool activate_rocket(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    msg_print(_("ロケットを発射した！", "You launch a rocket!"));
    (void)fire_ball(creature, AttributeType::ROCKET, dir, 250 + creature.level * 3, 2);
    return true;
}

bool activate_ball_water(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("% sが深い青色に鼓動している...", "The %s throbs deep blue..."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::WATER, dir, 200, 3);
    return true;
}

bool activate_ball_lite(CreatureEntity &creature, std::string_view name)
{
    const auto num = Dice::roll(5, 3);
    msg_format(_("% sが稲妻で覆われた...", "The %s is surrounded by lightning..."), name.data());
    const auto p_pos = creature.get_position();
    const auto &floor = *creature.get_floor();
    for (auto k = 0; k < num; k++) {
        auto attempts = 1000;
        Pos2D pos(0, 0);
        while (attempts--) {
            pos = scatter(floor, p_pos, 4, PROJECT_NONE);
            if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
                continue;
            }

            if (pos != p_pos) {
                break;
            }
        }

        constexpr uint32_t flags = PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        project(creature, 0, 3, pos.y, pos.x, 150, AttributeType::ELEC, flags);
    }

    return true;
}

bool activate_ball_dark(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("% sが深い闇に覆われた...", "The %s is covered in pitch-darkness..."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::DARK, dir, 250, 4);
    return true;
}

bool activate_ball_mana(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("% sが青白く光った．．．", "The %s becomes pale..."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::MANA, dir, 250, 4);
    return true;
}
