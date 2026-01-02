#include "alliance/alliance-nurgle.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
/*!
 * @brief ナーグル神アライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details 腐敗、病気、耐久力を重視し、美しさや清浄さを嫌う
 */
int AllianceNurgle::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;

    impression += calcIronmanHostilityPenalty();
    // 基本的な戦力による評価（控えめ）
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 25);

    // 耐久力を最重視（病気に耐える力）
    impression += (creature_ptr->stat_use[A_CON] - 10) * 6;

    // 筋力も評価（腐敗した肉体でも力強く）
    impression += (creature_ptr->stat_use[A_STR] - 10) * 3;

    // 魅力は逆に低い方が好まれる（醜さは美徳）
    impression -= (creature_ptr->stat_use[A_CHR] - 10) * 4;

    // 種族による評価
    switch (creature_ptr->prace) {
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SKELETON:
        impression += 200; // アンデッドは最高評価
        break;
    case PlayerRaceType::HALF_ORC:
    case PlayerRaceType::HALF_TROLL:
        impression += 120; // 醜い種族を好む
        break;
    case PlayerRaceType::DWARF:
    case PlayerRaceType::GNOME:
        impression += 80; // 頑健さを評価
        break;
    case PlayerRaceType::HUMAN:
        impression += 50; // 病気になりやすい人間
        break;
    case PlayerRaceType::ELF:
    case PlayerRaceType::HIGH_ELF:
        impression -= 100; // 美しすぎる
        break;
    case PlayerRaceType::SPECTRE:
    case PlayerRaceType::VAMPIRE:
        impression += 150; // 不死系
        break;
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ANDROID:
        impression -= 150; // 病気にならない
        break;
    default:
        break;
    }

    // 職業による評価
    switch (creature_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::BERSERKER:
        impression += 100; // 戦場で傷つく者
        break;
    case PlayerClassType::CHAOS_WARRIOR:
        impression += 150; // 堕落した騎士
        break;
    case PlayerClassType::PRIEST:
        impression -= 80; // 清浄すぎる
        break;
    case PlayerClassType::PALADIN:
        impression -= 120; // 聖なる力
        break;
    case PlayerClassType::MONK:
        impression += 60; // 肉体修行で傷つく
        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA:
        impression += 70; // 影に潜む者
        break;
    case PlayerClassType::BEASTMASTER:
        impression += 80; // 野生の生き物
        break;
    default:
        break;
    }

    // 現在のHP状況（傷ついているほど好まれる）
    /*
    int hp_ratio = (creature_ptr->hp * 100) / creature_ptr->maxhp;
    if (hp_ratio <= 25)
        impression += 60;
    else if (hp_ratio <= 50)
        impression += 30;
    else if (hp_ratio >= 90)
        impression -= 20;
    */

    // 状態異常持ちを評価
    /*
    if (creature_ptr->poisoned)
        impression += 50;
    if (creature_ptr->diseased)
        impression += 80;
    if (creature_ptr->cut)
        impression += 30;
    if (creature_ptr->stun)
        impression += 20;
    */

    return impression;
}

/*!
 * @brief ナーグル神アライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 段階的な疫病と腐敗の制裁
 */
void AllianceNurgle::panishment(PlayerType &player_ptr)
{
    // 印象に応じて段階的な制裁
    if (this->calcImpressionPoint(&player_ptr) <= -50) {
        // 軽微な制裁：軽い病気
        msg_print("あなたの体に軽い不調を感じる...");
        msg_print("「ナーグルの小さな贈り物だ」");

        (void)BadStatusSetter(&player_ptr).set_poison(randint1(20) + 10);
        if (one_in_(3)) {
            msg_print("体がだるく重い...");
            (void)BadStatusSetter(&player_ptr).set_stun(randint1(10) + 10);
        }
        return;
    }

    if (this->calcImpressionPoint(&player_ptr) <= -100) {
        // 中程度の制裁：疫病の使者
        msg_print("腐敗の悪臭が漂ってきた...");
        msg_print("「ナーグルの慈悲深い疫病を受けるがよい」");

        (void)BadStatusSetter(&player_ptr).set_poison(randint1(40) + 20);
        if (one_in_(2)) {
            msg_print("体の傷口が膿み始めた...");
            (void)BadStatusSetter(&player_ptr).set_cut(randint1(50) + 25);
        }

        // 疫病の使者召喚
        /*
        for (int i = 0; i < 2; i++) {
            MONSTER_IDX m_idx = summon_specific(&player_ptr, 0, player_ptr.y, player_ptr.x,
                player_ptr.current_floor_ptr->dun_level + 5,
                SUMMON_DEMON, PM_FORCE_PET | PM_ALLOW_GROUP);
            if (m_idx) {
                MonsterEntity *m_ptr = &player_ptr.current_floor_ptr->m_list[m_idx];
                set_monster_hostile(m_ptr);
            }
        }
        return;
        */
    }

    if (this->calcImpressionPoint(&player_ptr) <= -150) {
        // 重い制裁：大悪疫
        msg_print("恐ろしい疫病があなたを襲う！");
        msg_print("「ナーグルの偉大なる慈悲を味わうがよい！」");

        (void)BadStatusSetter(&player_ptr).set_poison(randint1(80) + 40);
        (void)BadStatusSetter(&player_ptr).set_cut(randint1(100) + 50);
        (void)BadStatusSetter(&player_ptr).set_stun(randint1(50) + 25);

        if (one_in_(2)) {
            msg_print("あなたの体が腐敗し始めた...");
            project(&player_ptr, 0, 3, player_ptr.y, player_ptr.x,
                player_ptr.level * 2, AttributeType::POIS,
                PROJECT_KILL | PROJECT_ITEM);
        }

        if (one_in_(3)) {
            msg_print("意識が朦朧としてきた...");
            (void)BadStatusSetter(&player_ptr).set_paralysis(randint1(10) + 5);
        }

        // より強力な悪魔軍団
        /*
        for (int i = 0; i < 4; i++) {
            MONSTER_IDX m_idx = summon_specific(&player_ptr, 0, player_ptr.y, player_ptr.x,
                player_ptr.current_floor_ptr->dun_level + 10,
                SUMMON_DEMON, PM_FORCE_PET | PM_ALLOW_GROUP);
            if (m_idx) {
                MonsterEntity *m_ptr = &player_ptr.current_floor_ptr->m_list[m_idx];
                set_monster_hostile(m_ptr);
            }
        }
        return;
        */
    }

    if (this->calcImpressionPoint(&player_ptr) <= -250) {
        // 最重の制裁：腐敗の庭園
        msg_print("周囲の世界が腐敗し始めた！");
        msg_print("「我が庭園へようこそ...永遠の腐敗と再生の世界へ」");

        // 極限状態異常
        (void)BadStatusSetter(&player_ptr).set_poison(randint1(200) + 100);
        (void)BadStatusSetter(&player_ptr).set_cut(randint1(200) + 100);
        (void)BadStatusSetter(&player_ptr).set_stun(randint1(100) + 50);

        // 大ダメージ（腐敗エリア攻撃）
        project(&player_ptr, 0, 5, player_ptr.y, player_ptr.x,
            player_ptr.level * 4, AttributeType::POIS,
            PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID);

        if (one_in_(2)) {
            msg_print("あなたの魂まで腐敗の影響を受けている...");
            (void)BadStatusSetter(&player_ptr).set_paralysis(randint1(20) + 10);
        }
    }

    // 大軍団召喚
    /*
    for (int i = 0; i < 8; i++) {
        MONSTER_IDX m_idx = summon_specific(&player_ptr, 0, player_ptr.y, player_ptr.x,
            player_ptr.current_floor_ptr->dun_level + 15,
            SUMMON_DEMON, PM_FORCE_PET | PM_ALLOW_GROUP);
        if (m_idx) {
            MonsterEntity *m_ptr = &player_ptr.current_floor_ptr->m_list[m_idx];
            set_monster_hostile(m_ptr);
        }
    }
    */
}

/*!
 * @brief ナーグル神アライアンスの壊滅判定
 * @return 壊滅しているかどうか
 * @details 腐敗と再生の神なので滅びにくい
 */
bool AllianceNurgle::isAnnihilated()
{
    /*
    // ナーグルは腐敗と再生の神なので非常に滅びにくい
    if (this->calcImpressionPoint(nullptr) <= -300) {
        if (one_in_(50)) { // 2%の確率で一時的な眠り
            msg_print("ナーグルの腐敗の庭園に静寂が訪れた...");
            msg_print("しかし、腐敗は永遠に続く。やがて再び目覚めるだろう。");
            return true;
        }
    }
    */
    return false;
}
