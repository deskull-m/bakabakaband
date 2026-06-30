#include "birth/birth-stat.h"
#include "birth/auto-roller.h"
#include "combat/martial-arts-style.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-personality.h"
#include "player/player-skill.h"
#include "spell/spells-status.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/creature-entity.h"
#include "system/inner-game-data.h"
#include "system/redrawing-flags-updater.h"
#include <array>

namespace {

constexpr auto random_distribution = 60;

/*! オートロール能力値の乱数分布 (1d3, 1d4, 1d5 を3 * 4 * 5 = 60個で表現) */
/*! 新形式: 8-17 -> 80-170 (3.0-17.0表示) */
constexpr std::array<short, random_distribution> auto_roller_distribution = {
    {
        80, 90, 90, 90, 100, 100, 100, 100, 100, 100, /*00-09*/
        110, 110, 110, 110, 110, 110, 110, 110, 110, 120, /*10-19*/
        120, 120, 120, 120, 120, 120, 120, 120, 120, 120, /*20-29*/
        130, 130, 130, 130, 130, 130, 130, 130, 130, 130, /*30-49*/
        130, 140, 140, 140, 140, 140, 140, 140, 140, 140, /*40-49*/
        150, 150, 150, 150, 150, 150, 160, 160, 160, 170 /*50-59*/
    }
};
}

/*!
 * @brief プレイヤーの能力値表現に基づいて加減算を行う。
 * @param value 現在の能力値 (30-400の範囲)
 * @param amount 加減算する値 (10単位)
 * @return 加減算の結果
 */
int adjust_stat(int value, int amount)
{
    value += amount * 10;

    if (value < STAT_MIN_VALUE) {
        value = STAT_MIN_VALUE;
    } else if (value > STAT_MAX_VALUE) {
        value = STAT_MAX_VALUE;
    }

    return value;
}

/*!
 * @brief クリーチャー（プレイヤーやモンスター等）の能力値を一通りロールする。 / Roll for a creature's stats
 * @param creature クリーチャーへの参照
 * @details
 * プレイヤーのキャラメイクと同じロジックで能力値を生成する。
 * calc_bonuses()による、独立ステータスからの副次ステータス算出も行っている。
 * For efficiency, we include a chunk of "calc_bonuses()".\n
 */
void get_stats(CreatureEntity &creature)
{
    while (true) {
        auto sum = 0;
        for (auto i = 0; i < 2; i++) {
            auto tmp = randint0(random_distribution * random_distribution * random_distribution);
            for (auto j = 0; j < 3; j++) {
                auto stat = i * 3 + j;
                auto val = auto_roller_distribution[tmp % random_distribution];
                sum += val;
                creature.set_stat_max(stat, val);
                creature.set_stat_cur(stat, val);
                tmp /= random_distribution;
            }
        }

        // 新形式: 42*10 + 5*6*10 = 720, 57*10 + 5*6*10 = 870
        if ((sum > 720) && (sum < 870)) {
            break;
        }
    }

    // stat_max_max は初期化、stat_use はロール結果と同値を初期表示にする。
    // プレイヤーは後段の calc_bonuses() で stat_use を再計算するが、
    // モンスターは calc_bonuses() を呼ばないので stat_use がここで確定する。
    for (auto i = 0; i < A_MAX; i++) {
        creature.set_stat_max_max(i, creature.get_stat_max(i));
        creature.set_stat_use(i, creature.get_stat_max(i));
    }
}

/*!
 * @brief クリーチャーの初期所持金を決める（プレイヤーと同様のロジック）
 * @param creature クリーチャーへの参照
 * @details プレイヤーのget_money()と同等の処理を行う
 */
void get_money_for_creature(CreatureEntity &creature)
{
    int gold = (creature.get_prestige() * 6) + randint1(100) + 300;

    // 能力値に応じて所持金を調整
    for (int i = 0; i < A_MAX; i++) {
        if (creature.get_stat_max(i) >= 680) {
            gold -= 300;
        } else if (creature.get_stat_max(i) >= 380) {
            gold -= 200;
        } else if (creature.get_stat_max(i) > 180) {
            gold -= 150;
        } else {
            gold -= (creature.get_stat_max(i) / 10 - 8) * 10;
        }
    }

    const int minimum_deposit = 100;
    if (gold < minimum_deposit) {
        gold = minimum_deposit;
    }

    creature.set_au(gold);
}

/*!
 * @brief 経験値修正の合計値を計算
 */
uint16_t get_expfact(CreatureEntity &creature)
{
    uint16_t expfact = creature.get_race_info()->r_exp;

    CreatureRace pr(&creature);
    if (!pr.equals(PlayerRaceType::ANDROID)) {
        expfact += (*creature.get_class_info()).c_exp;
    }

    auto is_race_gaining_additional_speed = pr.equals(PlayerRaceType::KLACKON) || pr.equals(PlayerRaceType::SPRITE);
    auto is_class_gaining_additional_speed = CreatureClass(creature).has_additional_speed();
    if (is_race_gaining_additional_speed && is_class_gaining_additional_speed) {
        expfact -= 15;
    }

    return expfact;
}

/*!
 * @brief その他「オートローラ中は算出の対象にしない」副次ステータスを処理する / Roll for some info that the auto-roller ignores
 */
void get_extra(CreatureEntity &creature, bool roll_hitdie)
{
    creature.expfact = get_expfact(creature);

    /* Reset record of race/realm changes */
    InnerGameData::get_instance().set_start_race(creature.prace);
    creature.set_old_race_flags1(0L);
    creature.set_old_race_flags2(0L);
    creature.set_old_realm(0);

    CreatureClass pc(creature);
    auto is_sorcerer = pc.equals(PlayerClassType::SORCERER);
    for (int i = 0; i < 64; i++) {
        if (is_sorcerer) {
            creature.set_spell_exp(i, PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER));
        } else if (pc.equals(PlayerClassType::RED_MAGE)) {
            creature.set_spell_exp(i, PlayerSkill::spell_exp_at(PlayerSkillRank::SKILLED));
        } else {
            creature.set_spell_exp(i, PlayerSkill::spell_exp_at(PlayerSkillRank::UNSKILLED));
        }
    }

    auto pclass = enum2i(creature.pclass);
    creature.weapon_exp = class_skills_info[pclass].w_start;
    creature.weapon_exp_max = class_skills_info[pclass].w_max;

    if (creature.ppersonality == PERSONALITY_SEXY) {
        auto &whip_exp = creature.weapon_exp[ItemKindType::HAFTED][SV_WHIP];
        whip_exp = std::max(whip_exp, PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER));
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        creature.set_skill_exp(i, class_skills_info[pclass].s_start[i]);
    }

    // 武術スタイルの初期設定
    creature.martial_arts_style = MartialArtsStyleType::TRADITIONAL;

    const auto r_mhp = is_sorcerer ? creature.get_race_info()->r_mhp / 2 : creature.get_race_info()->r_mhp;
    creature.hit_dice = Dice(1, r_mhp + (*creature.get_class_info()).c_mhp + (*creature.get_personality_info()).a_mhp);
    if (roll_hitdie) {
        roll_hitdice(creature, SPOP_NO_UPDATE);
    }

    creature.maxhp = creature.hp_table[0];
}

/*!
 * @brief プレイヤーの限界ステータスを決める。
 * @param creature クリーチャーへの参照
 * @details 新生の薬やステータスシャッフルでもこの関数が呼ばれる
 */
void get_max_stats(CreatureEntity &creature)
{
    int dice[6]{};
    while (true) {
        auto j = 0;
        for (auto i = 0; i < A_MAX; i++) {
            dice[i] = randint1(7);
            j += dice[i];
        }

        if (j == 24) {
            break;
        }
    }

    for (auto i = 0; i < A_MAX; i++) {
        // 新形式: 180 + 60 + dice[i] * 10 = 240 + 10~70 = 250~310 (25.0~31.0)
        short max_max = 180 + 60 + dice[i] * 10;
        creature.set_stat_max_max(i, max_max);
        if (creature.get_stat_max(i) > max_max) {
            creature.set_stat_max(i, max_max);
        }
        if (creature.get_stat_cur(i) > max_max) {
            creature.set_stat_cur(i, max_max);
        }
    }

    creature.remove_knowledge(KNOW_STAT);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ABILITY_SCORE);
}
