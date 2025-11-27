#include "alliance/alliance-slaanesh.h"
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
int AllianceSlaanesh::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;

    // 鉄人モード: 全てのアライアンスから猛烈に敵対される
    if (ironman_alliance_hostility) {
        impression -= 10000;
    }
    // プレイヤーの魅力と魔法力を評価（スラーネッシュは快楽と魔法を重視）
    impression += Alliance::calcPlayerPower(*creature_ptr, 2, 30);

    // 魅力による追加ボーナス
    impression += (creature_ptr->stat_use[A_CHR] - 10) * 5;

    // 魔法使用による好感度向上（コーンとは逆）
    // if (creature_ptr->realm1 != REALM_NONE || creature_ptr->realm2 != REALM_NONE) {
    //     impression += 50;
    // }

    /*
    // スラーネッシュ配下のモンスター討伐による好感度減少
    impression -= MonraceList::get_instance().get_monrace(MonraceId::SLAANESH_DAEMON).r_akills * 18;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::KEEPER_OF_SECRETS).r_akills * 25;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::DAEMONETTE).r_akills * 10;

    // スラーネッシュの主要配下が全滅している場合の大幅減点
    if (MonraceList::get_instance().get_monrace(MonraceId::SLAANESH_DAEMON_PRINCE).mob_num == 0) {
        impression -= 600;
    }

    // スラーネッシュ自身が倒されている場合の最大減点
    if (MonraceList::get_instance().get_monrace(MonraceId::SLAANESH_GOD).mob_num == 0) {
        impression -= 2500;
    }
    */

    return impression;
}

bool AllianceSlaanesh::isAnnihilated()
{
    return false; // TODO: MonraceList::get_instance().get_monrace(MonraceId::SLAANESH_GOD).mob_num == 0;
}

void AllianceSlaanesh::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -40) {
        return;
    }

    /*
    if (one_in_(20)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 12, PROJECT_NONE);

        // スラーネッシュの怒りレベルに応じて異なる復讐者を派遣
        MonraceId avenger_id;
        if (impression < -400) {
            avenger_id = MonraceId::KEEPER_OF_SECRETS;
            msg_print(_("「快楽の神スラーネッシュの怒りを知るがよい！」秘密の番人があなたを誘惑しに現れた！",
                       "\"Know the wrath of Slaanesh, the God of Pleasure!\" A Keeper of Secrets appears to seduce you!"));
        } else if (impression < -150) {
            avenger_id = MonraceId::SLAANESH_DAEMON;
            msg_print(_("「快楽を！感覚を！」スラーネッシュデーモンがあなたの魂を求めて現れた！",
                       "\"Pleasure! Sensation!\" A Slaanesh Daemon appears, craving your soul!"));
        } else {
            avenger_id = MonraceId::DAEMONETTE;
            msg_print(_("「美しき破滅を！」デーモネットがあなたを誘惑すべく踊りながら現れた！",
                       "\"Beautiful destruction!\" Daemonettes appear, dancing to seduce you!"));
        }

        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, avenger_id, PM_ALLOW_GROUP | PM_SLAANESH);
        if (m_idx) {
            disturb(&player_ptr, true, true);

            // 追加の配下召喚（快楽と誘惑の混沌）
            for (int k = 0; k < 2 + (impression < -250 ? 3 : 0); k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x,
                               std::max(player_ptr.current_floor_ptr->monster_level, 8),
                               SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }

    // 極度に嫌われている場合の追加ペナルティ
    if (impression < -600 && one_in_(25)) {
        msg_print(_("スラーネッシュの誘惑が現実を歪める！", "Slaanesh's temptation distorts reality!"));
        // 追加の効果（混乱、魅惑効果など）をここに実装可能
    }
    */

    if (one_in_(25)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 10, PROJECT_NONE);
        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, MonraceId::SLAANESH_CHOSEN, PM_ALLOW_GROUP);
        if (m_idx) {
            msg_print(_("スラーネッシュの選ばれし者があなたを誘惑すべく現れた！", "Slaanesh's Chosen appears to seduce you!"));
            disturb(&player_ptr, true, true);
            for (int k = 0; k < 2; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x, std::max(player_ptr.current_floor_ptr->monster_level, 3), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }

    return;
}
