#include "birth/birth-body-spec.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "system/player-type-definition.h"

/*!
 * @brief プレイヤーの身長体重を決める / Get character's height and weight
 */
void get_height_weight(PlayerType *player_ptr)
{
    int deviation;
    switch (player_ptr->psex) {
    case SEX_MALE:
        player_ptr->ht = randnor(player_ptr->race->m_b_ht, player_ptr->race->m_m_ht);
        deviation = (int)(player_ptr->ht) * 100 / (int)(player_ptr->race->m_b_ht);
        player_ptr->wt = randnor((int)(player_ptr->race->m_b_wt) * deviation / 100, (int)(player_ptr->race->m_m_wt) * deviation / 300);
        return;
    case SEX_FEMALE:
        player_ptr->ht = randnor(player_ptr->race->f_b_ht, player_ptr->race->f_m_ht);
        deviation = (int)(player_ptr->ht) * 100 / (int)(player_ptr->race->f_b_ht);
        player_ptr->wt = randnor((int)(player_ptr->race->f_b_wt) * deviation / 100, (int)(player_ptr->race->f_m_wt) * deviation / 300);
        return;
    default:
        return;
    }
}

/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 */
void get_ahw(PlayerType *player_ptr)
{
    player_ptr->age = player_ptr->race->b_age + randint1(player_ptr->race->m_age);
    get_height_weight(player_ptr);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void get_money(PlayerType *player_ptr)
{
    int gold = (player_ptr->prestige * 6) + randint1(100) + 300;
    if (PlayerClass(player_ptr).equals(PlayerClassType::TOURIST)) {
        gold += 2000;
    }

    for (int i = 0; i < A_MAX; i++) {
        // 新形式: 180+50*10=680, 180+20*10=380, 180
        if (player_ptr->stat_max[i] >= 680) { // 旧18/50 -> 新68.0以上
            gold -= 300;
        } else if (player_ptr->stat_max[i] >= 380) { // 旧18/20 -> 新38.0以上
            gold -= 200;
        } else if (player_ptr->stat_max[i] > 180) { // 旧18超 -> 新18.0超
            gold -= 150;
        } else {
            gold -= (player_ptr->stat_max[i] / 10 - 8) * 10; // 新形式での計算
        }
    }

    const int minimum_deposit = 100;
    if (gold < minimum_deposit) {
        gold = minimum_deposit;
    }

    if (player_ptr->ppersonality == PERSONALITY_LAZY) {
        gold /= 2;
    } else if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        gold = 10000000;
    }
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        gold /= 5;
    }

    player_ptr->au = gold;
}
