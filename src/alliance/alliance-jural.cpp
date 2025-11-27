#include "alliance/alliance-jural.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceJural::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 2, 40);
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }

    impression -= MonraceList::get_instance().get_monrace(MonraceId::ALIEN_JURAL).r_akills * 10;
    if (MonraceList::get_instance().get_monrace(MonraceId::JURAL_MONS).mob_num == 0) {
        impression -= 300;
    }
    if (MonraceList::get_instance().get_monrace(MonraceId::JURAL_WITCHKING).mob_num == 0) {
        impression -= 1230;
    }
    return impression;
}

bool AllianceJural::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::JURAL_WITCHKING).mob_num == 0;
}

void AllianceJural::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -40) {
        return;
    }

    if (one_in_(20)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 12, PROJECT_NONE);
        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, MonraceId::ALIEN_JURAL, PM_ALLOW_GROUP | PM_JURAL);
        if (m_idx) {
            msg_print(_("「おーい、行ってみよう！」ジュラル星人があなたに報復すべく追跡してきた！", "\"Hey, let's go!\" Alien Jurals is chasing you for revenge!"));
            disturb(&player_ptr, true, true);
            for (int k = 0; k < 4; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x, std::max(player_ptr.current_floor_ptr->monster_level, 5), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }

    return;
}

/*!
 * @brief 襲撃時に出現するモンスターのリストを取得する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param impression_point 印象値
 * @return ジュラル星人のモンスターIDのリスト（印象値が低い場合はジュラル星人）
 */
std::vector<MonraceId> AllianceJural::get_ambush_monsters([[maybe_unused]] PlayerType *player_ptr, int impression_point) const
{
    std::vector<MonraceId> monsters;

    // 印象値が低い場合のみ襲撃を行う（-40以下）
    if (impression_point >= -40) {
        return monsters;
    }

    // ジュラル星人による襲撃
    monsters.push_back(MonraceId::ALIEN_JURAL); // ジュラル星人
    monsters.push_back(MonraceId::ALIEN_JURAL); // 複数体を追加
    monsters.push_back(MonraceId::ALIEN_JURAL);

    // 印象値が非常に低い場合はリーダーも追加
    if (impression_point < -300) {
        monsters.push_back(MonraceId::JURAL_WITCHKING); // ジュラル魔王
        monsters.push_back(MonraceId::JURAL_MONS); // ジュラル星ボス
    }

    return monsters;
}

/*!
 * @brief 襲撃時のメッセージを取得する
 * @return ジュラル星人固有の襲撃メッセージ
 */
std::string AllianceJural::get_ambush_message() const
{
    return _("「おーい、行ってみよう！」ジュラル星人があなたに襲いかかってきた！",
        "\"Hey, let's go!\" Alien Jurals are attacking you!");
}
