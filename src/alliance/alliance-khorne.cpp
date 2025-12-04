#include "alliance/alliance-khorne.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
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

#include "game-option/birth-options.h"
int AllianceKhorne::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }
    // プレイヤーの戦闘力を評価（コーンは戦闘を重視）
    impression += Alliance::calcPlayerPower(*creature_ptr, 3, 50);

    /*

    // コーン配下のモンスター討伐による好感度減少
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KHORNE_BERSERKER).r_akills * 15;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::BLOODLETTER).r_akills * 12;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KHORNE_DAEMON).r_akills * 20;

    // コーンの主要配下が全滅している場合の大幅減点
    if (MonraceList::get_instance().get_monrace(MonraceId::KHORNE_DAEMON_PRINCE).mob_num == 0) {
        impression -= 500;
    }

    // コーン自身が倒されている場合の最大減点
    if (MonraceList::get_instance().get_monrace(MonraceId::KHORNE_GOD).mob_num == 0) {
        impression -= 2000;
    }

    // 魔法使用による減点（コーンは魔法を嫌う）
    if (creature_ptr->realm1 != REALM_NONE || creature_ptr->realm2 != REALM_NONE) {
        impression -= 100;
    }
    */
    return impression;
}

bool AllianceKhorne::isAnnihilated()
{
    const auto &monrace_list = MonraceList::get_instance();
    return monrace_list.get_monrace(MonraceId::KHORNE).mob_num == 0;
}

void AllianceKhorne::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -60) {
        return;
    }
    /*
    if (one_in_(15)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 15, PROJECT_NONE);

        // コーンの怒りレベルに応じて異なる復讐者を派遣
        MonraceId avenger_id;
        if (impression < -500) {
            avenger_id = MonraceId::KHORNE_DAEMON_PRINCE;
            msg_print(_("「血の神コーンの怒りを受けよ！」デーモンプリンスがあなたを血祭りにあげるため現れた！",
                "\"Feel the wrath of Khorne, the Blood God!\" A Daemon Prince appears to offer you as a blood sacrifice!"));
        } else if (impression < -200) {
            avenger_id = MonraceId::KHORNE_DAEMON;
            msg_print(_("「血を！血を！」コーンデーモンがあなたの血を求めて現れた！",
                "\"Blood! Blood!\" A Khorne Daemon appears, thirsting for your blood!"));
        } else {
            avenger_id = MonraceId::KHORNE_BERSERKER;
            msg_print(_("「コーンのために！」狂戦士があなたに復讐すべく襲いかかってきた！",
                "\"For Khorne!\" Berserkers charge at you for revenge!"));
        }

        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, avenger_id, PM_ALLOW_GROUP | PM_KHORNE);
        if (m_idx) {
            disturb(&player_ptr, true, true);

            // 追加の配下召喚（血と戦いの混沌）
            for (int k = 0; k < 3 + (impression < -300 ? 2 : 0); k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x,
                    std::max(player_ptr.current_floor_ptr->monster_level, 10),
                    SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }


    // 極度に嫌われている場合の追加ペナルティ
    if (impression < -800 && one_in_(30)) {
        msg_print(_("コーンの怒りが戦場を血に染める！", "Khorne's rage stains the battlefield with blood!"));
        // 追加の効果（出血ダメージなど）をここに実装可能
    }
    */

    if (one_in_(20)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 12, PROJECT_NONE);
        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, MonraceId::KHORNE_CHOSEN, PM_ALLOW_GROUP);
        if (m_idx) {
            msg_print(_("コーンの選ばれし者があなたを誅すべく追跡してきた！", "Khorne's Chosen is chasing you for revenge!"));
            disturb(&player_ptr, true, true);
            for (int k = 0; k < 3; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x, std::max(player_ptr.current_floor_ptr->monster_level, 5), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }

    return;
}