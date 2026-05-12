/*!
 * @file mutation-execution.cpp
 * @brief プレイヤーの変異能力実行定義
 */

#include "action/mutation-execution.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-mage.h"
#include "mind/mind-warrior.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-techniques.h"
#include "object-enchant/item-feeling.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "racial/racial-vampire.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 突然変異のレイシャル効果実装
 * @param creature クリーチャーへの参照
 * @param power 発動させる突然変異レイシャルのID
 * @return レイシャルを実行した場合TRUE、キャンセルした場合FALSEを返す
 */
bool exe_mutation_power(CreatureEntity &creature, PlayerMutationType power)
{
    PLAYER_LEVEL lvl = creature.level;
    auto &floor = *creature.get_floor();
    switch (power) {
    case PlayerMutationType::SPIT_ACID: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        stop_mouth(creature);
        msg_print(_("酸を吐きかけた...", "You spit acid..."));
        fire_ball(creature, AttributeType::ACID, dir, lvl, 1 + (lvl / 30));
        return true;
    }
    case PlayerMutationType::BR_FIRE: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        stop_mouth(creature);
        msg_print(_("あなたは火炎のブレスを吐いた...", "You breathe fire..."));
        fire_breath(creature, AttributeType::FIRE, dir, lvl * 2, 1 + (lvl / 20));
        return true;
    }
    case PlayerMutationType::HYPN_GAZE: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("あなたの目は幻惑的になった...", "Your eyes look mesmerizing..."));
        (void)charm_monster(creature, dir, lvl);
        return true;
    }
    case PlayerMutationType::TELEKINES: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("集中している...", "You concentrate..."));
        fetch_item(creature, dir, lvl * 10, true);
        return true;
    }
    case PlayerMutationType::VTELEPORT:
        msg_print(_("集中している...", "You concentrate..."));
        teleport_player(creature, 10 + 4 * lvl, TELEPORT_SPONTANEOUS);
        return true;
    case PlayerMutationType::MIND_BLST: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("集中している...", "You concentrate..."));
        fire_bolt(creature, AttributeType::PSI, dir, Dice::roll(3 + ((lvl - 1) / 5), 3));
        return true;
    }
    case PlayerMutationType::RADIATION:
        msg_print(_("体から放射能が発生した！", "Radiation flows from your body!"));
        fire_ball(creature, AttributeType::NUKE, Direction::self(), (lvl * 2), 3 + (lvl / 20));
        return true;
    case PlayerMutationType::VAMPIRISM:
        vampirism(creature);
        return true;
    case PlayerMutationType::SMELL_MET:
        stop_mouth(creature);
        (void)detect_treasure(creature, DETECT_RAD_DEFAULT);
        return true;
    case PlayerMutationType::SMELL_MON:
        stop_mouth(creature);
        (void)detect_monsters_normal(creature, DETECT_RAD_DEFAULT);
        return true;
    case PlayerMutationType::BLINK:
        teleport_player(creature, 10, TELEPORT_SPONTANEOUS);
        return true;
    case PlayerMutationType::EAT_ROCK:
        return eat_rock(creature);
    case PlayerMutationType::SWAP_POS: {
        project_length = -1;
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            project_length = 0;
            return false;
        }

        (void)teleport_swap(creature, dir);
        project_length = 0;
        return true;
    }
    case PlayerMutationType::SHRIEK:
        stop_mouth(creature);
        (void)fire_ball(creature, AttributeType::SOUND, Direction::self(), 2 * lvl, 8);
        (void)aggravate_monsters(creature, 0);
        return true;
    case PlayerMutationType::ILLUMINE:
        (void)lite_area(creature, Dice::roll(2, (lvl / 2)), (lvl / 10) + 1);
        return true;
    case PlayerMutationType::DET_CURSE:
        for (int i = 0; i < INVEN_TOTAL; i++) {
            auto *o_ptr = creature.inventory[i].get();
            if (!o_ptr->is_valid() || !o_ptr->is_cursed()) {
                continue;
            }

            o_ptr->feeling = FEEL_CURSED;
        }

        return true;
    case PlayerMutationType::BERSERK:
        (void)berserk(creature, randint1(25) + 25);
        return true;
    case PlayerMutationType::POLYMORPH:
        if (!input_check(_("変身します。よろしいですか？", "You will polymorph your self. Are you sure? "))) {
            return false;
        }

        do_poly_self(creature);
        return true;
    case PlayerMutationType::MIDAS_TCH:
        return alchemy(creature);
    case PlayerMutationType::GROW_MOLD:
        for (DIRECTION i = 0; i < 8; i++) {
            summon_specific(creature, creature.y, creature.x, lvl, SUMMON_MOLD, PM_FORCE_PET);
        }

        return true;
    case PlayerMutationType::RESIST: {
        int num = lvl / 10;
        TIME_EFFECT dur = randint1(20) + 20;
        if (randint0(5) < num) {
            (void)set_oppose_acid(creature, dur, false);
            num--;
        }

        if (randint0(4) < num) {
            (void)set_oppose_elec(creature, dur, false);
            num--;
        }

        if (randint0(3) < num) {
            (void)set_oppose_fire(creature, dur, false);
            num--;
        }

        if (randint0(2) < num) {
            (void)set_oppose_cold(creature, dur, false);
            num--;
        }

        if (num != 0) {
            (void)set_oppose_pois(creature, dur, false);
            num--;
        }

        return true;
    }
    case PlayerMutationType::EARTHQUAKE:
        (void)earthquake(creature, creature.get_position(), 10);
        return true;
    case PlayerMutationType::EAT_MAGIC:
        return eat_magic(creature, creature.level * 2);
    case PlayerMutationType::WEIGH_MAG:
        report_magics(creature);
        return true;
    case PlayerMutationType::STERILITY:
        msg_print(_("突然頭が痛くなった！", "You suddenly have a headache!"));
        take_hit(creature, DAMAGE_LOSELIFE, randint1(17) + 17, _("禁欲を強いた疲労", "the strain of forcing abstinence"));
        floor.num_repro += MAX_REPRODUCTION;
        return true;
    case PlayerMutationType::HIT_AND_AWAY:
        return hit_and_away(creature);
    case PlayerMutationType::DAZZLE:
        stun_monsters(creature, lvl * 4);
        confuse_monsters(creature, lvl * 4);
        turn_monsters(creature, lvl * 4);
        return true;
    case PlayerMutationType::LASER_EYE: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_beam(creature, AttributeType::LITE, dir, 2 * lvl);
        return true;
    }
    case PlayerMutationType::RECALL:
        return recall_player(creature, randint0(21) + 15);
    case PlayerMutationType::BANISH: {
        const auto dir = get_direction(creature);
        if (!dir) {
            return false;
        }

        const auto pos = creature.get_neighbor(dir);
        const auto &grid = floor.get_grid(pos);
        if (!grid.has_monster()) {
            msg_print(_("邪悪な存在を感じとれません！", "You sense no evil there!"));
            return true;
        }

        auto &monster = floor.get_monster(grid.m_idx);
        const auto &monrace = monster.get_monrace();
        auto can_banish = monrace.kind_flags.has(MonsterKindType::EVIL);
        can_banish &= monrace.misc_flags.has_not(MonsterMiscType::QUESTOR);
        can_banish &= monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
        can_banish &= !floor.inside_arena;
        can_banish &= !floor.is_in_quest();
        can_banish &= (monrace.level < randint1(creature.level + 50));
        can_banish &= !monster.is_nogeno();
        if (can_banish) {
            if (record_named_pet && monster.is_named_pet()) {
                const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
                exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
            }

            delete_monster_idx(creature, grid.m_idx);
            msg_print(_("その邪悪なモンスターは硫黄臭い煙とともに消え去った！", "The evil creature vanishes in a puff of sulfurous smoke!"));
            return true;
        }

        msg_print(_("祈りは効果がなかった！", "Your invocation is ineffectual!"));
        if (one_in_(13)) {
            monster.get_monster_profile().mflag2.set(MonsterConstantFlagType::NOGENO);
        }

        return true;
    }
    case PlayerMutationType::COLD_TOUCH: {
        const auto dir = get_direction(creature);
        if (!dir) {
            return false;
        }

        const auto pos = creature.get_neighbor(dir);
        const auto &grid = floor.get_grid(pos);
        if (!grid.has_monster()) {
            msg_print(_("あなたは何もない場所で手を振った。", "You wave your hands in the air."));
            return true;
        }

        fire_bolt(creature, AttributeType::COLD, dir, 2 * lvl);
        return true;
    }
    case PlayerMutationType::LAUNCHER:
        return ThrowCommand(creature).do_cmd_throw(2 + lvl / 40, false, -1);
    default:
        PlayerEnergy(creature).reset_player_turn();
        msg_format(_("能力 %s は実装されていません。", "Power %s not implemented. Oops."), power);
        return true;
    }
}
