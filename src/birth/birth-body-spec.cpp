#include "birth/birth-body-spec.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "system/creature-entity.h"

/*!
 * @brief クリーチャーの身長体重を決める / Get creature's height and weight
 * @param creature クリーチャーへの参照
 * @details 種族情報に基づいて身長・体重を設定する。PlayerTypeもCreatureEntityを継承しているため、プレイヤーにも使用可能。
 */
void get_height_weight(CreatureEntity &creature)
{
    // 種族情報が設定されていない場合は何もしない
    if (creature.race == nullptr) {
        return;
    }

    int deviation;
    switch (creature.psex) {
    case SEX_MALE:
        creature.ht = randnor(creature.race->m_b_ht, creature.race->m_m_ht);
        deviation = (int)(creature.ht) * 100 / (int)(creature.race->m_b_ht);
        creature.wt = randnor((int)(creature.race->m_b_wt) * deviation / 100, (int)(creature.race->m_m_wt) * deviation / 300);
        return;
    case SEX_FEMALE:
        creature.ht = randnor(creature.race->f_b_ht, creature.race->f_m_ht);
        deviation = (int)(creature.ht) * 100 / (int)(creature.race->f_b_ht);
        creature.wt = randnor((int)(creature.race->f_b_wt) * deviation / 100, (int)(creature.race->f_m_wt) * deviation / 300);
        return;
    default:
        return;
    }
}

/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 */
void get_ahw(CreatureEntity &creature)
{
    creature.age = creature.race->b_age + randint1(creature.race->m_age);
    get_height_weight(creature);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @param creature クリーチャーへの参照
 */
void get_money(CreatureEntity &creature)
{
    int gold = (creature.prestige * 6) + randint1(100) + 300;
    if (CreatureClass(creature).equals(PlayerClassType::TOURIST)) {
        gold += 2000;
    }

    for (int i = 0; i < A_MAX; i++) {
        // 新形式: 180+50*10=680, 180+20*10=380, 180
        if (creature.stat_max[i] >= 680) { // 旧18/50 -> 新68.0以上
            gold -= 300;
        } else if (creature.stat_max[i] >= 380) { // 旧18/20 -> 新38.0以上
            gold -= 200;
        } else if (creature.stat_max[i] > 180) { // 旧18超 -> 新18.0超
            gold -= 150;
        } else {
            gold -= (creature.stat_max[i] / 10 - 8) * 10; // 新形式での計算
        }
    }

    const int minimum_deposit = 100;
    if (gold < minimum_deposit) {
        gold = minimum_deposit;
    }

    if (creature.ppersonality == PERSONALITY_LAZY) {
        gold /= 2;
    } else if (creature.ppersonality == PERSONALITY_MUNCHKIN) {
        gold = 10000000;
    }
    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        gold /= 5;
    }

    creature.au = gold;
}
