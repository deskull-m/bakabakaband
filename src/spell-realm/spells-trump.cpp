#include "spell-realm/spells-trump.h"
#include "avatar/avatar.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "mutation/mutation-investor-remover.h"
#include "player-base/player-class.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief トランプ領域の「シャッフル」の効果をランダムに決めて処理する。
 * @param creature クリーチャーへの参照
 */
void cast_shuffle(CreatureEntity &creature)
{
    PLAYER_LEVEL plev = creature.level;
    int die;
    int vir = virtue_number(creature, Virtue::CHANCE);
    int i;

    CreatureClass pc(creature);
    auto is_good_shuffle = pc.equals(PlayerClassType::ROGUE);
    is_good_shuffle |= pc.equals(PlayerClassType::HIGH_MAGE);
    is_good_shuffle |= pc.equals(PlayerClassType::SORCERER);
    if (is_good_shuffle) {
        die = (randint1(110)) + plev / 5;
    } else {
        die = randint1(120);
    }

    if (vir) {
        auto it = creature.virtues.find(Virtue::CHANCE);
        if (it != creature.virtues.end()) {
            if (it->second > 0) {
                while (randint1(400) < it->second) {
                    die++;
                }
            } else {
                while (randint1(400) < (0 - it->second)) {
                    die--;
                }
            }
        }
    }

    msg_print(_("あなたはカードを切って一枚引いた...", "You shuffle the deck and draw a card..."));

    if (die < 30) {
        chg_virtue(creature, Virtue::CHANCE, 1);
    }

    const auto &floor = *creature.current_floor_ptr;
    if (die < 7) {
        msg_print(_("なんてこった！《死》だ！", "Oh no! It's Death!"));

        for (i = 0; i < randint1(3); i++) {
            activate_hi_summon(creature, creature.y, creature.x, false);
        }

        return;
    }

    if (die < 14) {
        msg_print(_("なんてこった！《悪魔》だ！", "Oh no! It's the Devil!"));
        summon_specific(creature, creature.y, creature.x, floor.dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
        return;
    }

    if (die < 18) {
        int count = 0;
        msg_print(_("なんてこった！《吊られた男》だ！", "Oh no! It's the Hanged Man."));
        activate_ty_curse(creature, false, &count);
        return;
    }

    if (die < 22) {
        msg_print(_("《不調和の剣》だ。", "It's the swords of discord."));
        aggravate_monsters(creature, 0);
        return;
    }

    if (die < 26) {
        msg_print(_("《愚者》だ。", "It's the Fool."));
        do_dec_stat(creature, A_INT);
        do_dec_stat(creature, A_WIS);
        return;
    }

    if (die < 30) {
        msg_print(_("奇妙なモンスターの絵だ。", "It's the picture of a strange monster."));
        trump_summoning(creature, 1, false, creature.y, creature.x, (floor.dun_level * 3 / 2),
            SUMMON_UNIQUE + randint1(6), PM_ALLOW_GROUP | PM_ALLOW_UNIQUE);
        return;
    }

    if (die < 33) {
        msg_print(_("《月》だ。", "It's the Moon."));
        unlite_area(creature, 10, 3);
        return;
    }

    if (die < 38) {
        msg_print(_("《運命の輪》だ。", "It's the Wheel of Fortune."));
        wild_magic(creature, randint0(32));
        return;
    }

    if (die < 40) {
        msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
        teleport_player(creature, 10, TELEPORT_PASSIVE);
        return;
    }

    if (die < 42) {
        msg_print(_("《正義》だ。", "It's Justice."));
        set_blessed(creature, creature.level, false);
        return;
    }

    if (die < 47) {
        msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
        teleport_player(creature, 100, TELEPORT_PASSIVE);
        return;
    }

    if (die < 52) {
        msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
        teleport_player(creature, 200, TELEPORT_PASSIVE);
        return;
    }

    if (die < 60) {
        msg_print(_("《塔》だ。", "It's the Tower."));
        wall_breaker(creature);
        return;
    }

    if (die < 72) {
        msg_print(_("《節制》だ。", "It's Temperance."));
        sleep_monsters_touch(creature);
        return;
    }

    if (die < 80) {
        msg_print(_("《塔》だ。", "It's the Tower."));
        earthquake(creature, creature.get_position(), 5);
        return;
    }

    if (die < 82) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(creature, 1, true, creature.y, creature.x, (floor.dun_level * 3 / 2), SUMMON_MOLD, 0L);
        return;
    }

    if (die < 84) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(creature, 1, true, creature.y, creature.x, (floor.dun_level * 3 / 2), SUMMON_BAT, 0L);
        return;
    }

    if (die < 86) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(creature, 1, true, creature.y, creature.x, (floor.dun_level * 3 / 2), SUMMON_VORTEX, 0L);
        return;
    }

    if (die < 88) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(creature, 1, true, creature.y, creature.x, (floor.dun_level * 3 / 2), SUMMON_COIN_MIMIC, 0L);
        return;
    }

    if (die < 96) {
        msg_print(_("《恋人》だ。", "It's the Lovers."));

        if (const auto dir = get_aim_dir(creature)) {
            charm_monster(creature, dir, std::min<short>(creature.level, 20));
        }

        return;
    }

    if (die < 101) {
        msg_print(_("《隠者》だ。", "It's the Hermit."));
        wall_stone(creature);
        return;
    }

    if (die < 111) {
        msg_print(_("《審判》だ。", "It's Judgement."));
        roll_hitdice(creature, SPOP_NONE);
        lose_all_mutations(creature);
        return;
    }

    if (die < 120) {
        msg_print(_("《太陽》だ。", "It's the Sun."));
        chg_virtue(creature, Virtue::KNOWLEDGE, 1);
        chg_virtue(creature, Virtue::ENLIGHTEN, 1);
        wiz_lite(creature, false);
        return;
    }

    msg_print(_("《世界》だ。", "It's the World."));
    if (creature.exp >= PY_MAX_EXP) {
        return;
    }

    int32_t ee = (creature.exp / 25) + 1;
    if (ee > 5000) {
        ee = 5000;
    }
    msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
    gain_exp(creature, ee);
}

void become_living_trump(CreatureEntity &creature)
{
    /* 1/7 Teleport control and 6/7 Random teleportation (uncontrolled) */
    MUTATION_IDX mutation = one_in_(7) ? 12 : 77;
    if (gain_mutation(creature, mutation)) {
        msg_print(_("あなたは生きているカードに変わった。", "You have turned into a Living Trump."));
    }
}
