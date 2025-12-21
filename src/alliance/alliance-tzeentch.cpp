#include "alliance/alliance-tzeentch.h"
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
int AllianceTzeentch::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();
    // ティーンチは知識と魔法の力を重視
    impression += Alliance::calcPlayerPower(*creature_ptr, 2, 35);

    // 知力と魔法能力による追加評価
    impression += (creature_ptr->stat_use[A_INT] - 10) * 4;
    impression += (creature_ptr->stat_use[A_WIS] - 10) * 2;

    /*
    // 魔法使用による大幅な好感度向上
    if (creature_ptr->realm1 != REALM_NONE || creature_ptr->realm2 != REALM_NONE) {
        impression += 100;
    }

    // 特に混沌魔法やソーサリーを好む
    if (creature_ptr->realm1 == REALM_CHAOS || creature_ptr->realm2 == REALM_CHAOS) {
        impression += 150;
    }
    if (creature_ptr->realm1 == REALM_SORCERY || creature_ptr->realm2 == REALM_SORCERY) {
        impression += 100;
    }
    */

    /*
    // ティーンチ配下のモンスター討伐による減点
    impression -= MonraceList::get_instance().get_monrace(MonraceId::TZEENTCH_DAEMON).r_akills * 20;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::LORD_OF_CHANGE).r_akills * 30;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::HORROR_BLUE).r_akills * 12;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::HORROR_PINK).r_akills * 15;

    // ティーンチの主要配下が全滅している場合の大幅減点
    if (MonraceList::get_instance().get_monrace(MonraceId::TZEENTCH_DAEMON_PRINCE).mob_num == 0) {
        impression -= 700;
    }

    // ティーンチ自身が倒されている場合の最大減点
    if (MonraceList::get_instance().get_monrace(MonraceId::TZEENTCH_GOD).mob_num == 0) {
        impression -= 3000;
    }
    */

    return impression;
}

bool AllianceTzeentch::isAnnihilated()
{
    return false; // TODO: MonraceList::get_instance().get_monrace(MonraceId::TZEENTCH_GOD).mob_num == 0;
}

void AllianceTzeentch::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -60) {
        return;
    }

    if (one_in_(22)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 15, PROJECT_NONE);

        // ティーンチの知識と変幻レベルに応じて異なる復讐者を派遣
        /*
        MonraceId avenger_id;
        if (impression < -600) {
            avenger_id = MonraceId::LORD_OF_CHANGE;
            msg_print(_("「変幻の神ティーンチの無限の知識を愚弄するか！」チェンジロードがあなたの運命を書き換えるために現れた！",
                "\"You dare mock the infinite knowledge of Tzeentch, the Changer of Ways!\" A Lord of Change appears to rewrite your fate!"));
        } else if (impression < -300) {
            avenger_id = MonraceId::TZEENTCH_DAEMON;
            msg_print(_("「変化こそが真理なり！」ティーンチデーモンがあなたの現実を歪めるため現れた！",
                "\"Change is the only truth!\" A Tzeentch Daemon appears to warp your reality!"));
        } else if (impression < -100) {
            avenger_id = MonraceId::HORROR_BLUE;
            msg_print(_("「知識の探求者よ、誤った道を歩むな！」青きホラーがあなたを正しい道へ導くため現れた！",
                "\"Seeker of knowledge, do not walk the wrong path!\" Blue Horrors appear to guide you to the right way!"));
        } else {
            avenger_id = MonraceId::HORROR_PINK;
            msg_print(_("「ティーンチの恵みを受けよ！」桃色ホラーがあなたに変化の祝福を与えるため現れた！",
                "\"Receive Tzeentch's blessing!\" Pink Horrors appear to bestow the blessing of change upon you!"));
        }

        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, avenger_id, PM_ALLOW_GROUP | PM_TZEENTCH);
        if (m_idx) {
            disturb(&player_ptr, true, true);

            // 追加の配下召喚（知識と変幻の混沌）
            int summon_count = 1;
            if (impression < -600) {
                summon_count = 3 + randint1(3); // 強力な場合は多数召喚
            } else if (impression < -300) {
                summon_count = 2 + randint1(2); // 中程度
            } else {
                summon_count = 1 + one_in_(2); // 軽微な場合は少数
            }

            for (int k = 0; k < summon_count; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x,
                    std::max(player_ptr.current_floor_ptr->monster_level, 12),
                    SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
        */
    }

    // 極度に嫌われている場合の追加ペナルティ（現実改変効果）
    if (impression < -800 && one_in_(35)) {
        msg_print(_("ティーンチの力が現実そのものを歪める！", "Tzeentch's power warps reality itself!"));
        // 追加の効果（テレポート、変身、混乱効果など）をここに実装可能

        // ランダムテレポート効果
        /*
        if (one_in_(3)) {
            msg_print(_("空間が歪み、あなたは別の場所に飛ばされた！", "Space warps and you are teleported elsewhere!"));
            teleport_player(&player_ptr, 50, TELEPORT_NONMAGICAL);
        }
        */
    }
}
