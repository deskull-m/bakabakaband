#include "alliance/alliance-diabolique.h"
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

int AllianceDiabolique::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;

    // プレイヤーレベルによる基本印象値
    impression += creature_ptr->lev * 10;

    // 邪悪な行為への評価
    // TODO: 邪悪な行為、デーモン召喚、闇の魔法使用などでボーナス

    // 善なる行為への減点
    // TODO: 善行、聖なる魔法使用、デーモン退治などでペナルティ

    return impression;
}

bool AllianceDiabolique::isAnnihilated()
{
    return false; // TODO: デアボリカの指導者モンスターが全滅した場合
}

void AllianceDiabolique::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -40) {
        return;
    }

    // 25分の1の確率で復讐者を派遣
    if (one_in_(25)) {

        /*
        auto m_pos = player_ptr.get_position();
        m_pos = scatter(&player_ptr, m_pos, 10, PROJECT_NONE);
        MonraceId avenger_id;
        if (impression < -400) {
            // 極度に嫌われている場合：強力なデーモン
            avenger_id = MonraceId::GREATER_DEMON;
            msg_print(_("「デアボリカの怒りを思い知るがよい！」上位デーモンが闇から現れた！",
                "\"Feel the wrath of Diabolique!\" A Greater Demon emerges from the darkness!"));
        } else if (impression < -150) {
            // 中程度に嫌われている場合：中級デーモン
            avenger_id = MonraceId::DEMON;
            msg_print(_("「闇の力があなたを狩りに来る！」デーモンが地獄から召喚された！",
                "\"The dark forces come to hunt you!\" A Demon has been summoned from Hell!"));
        } else {
            // 軽度に嫌われている場合：下級デーモン
            avenger_id = MonraceId::IMP;
            msg_print(_("「デアボリカの名において！」インプがあなたを襲うため現れた！",
                "\"In the name of Diabolique!\" An Imp appears to attack you!"));
        }

        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, avenger_id, PM_ALLOW_GROUP);
        if (m_idx) {
            msg_print(_("デアボリカの復讐者があなたを狙っている！", "Diabolique's avenger is targeting you!"));
            disturb(&player_ptr, true, true);

            // 復讐者の仲間を呼ぶ（低確率）
            for (int k = 0; k < 2; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x,
                    std::max(player_ptr.current_floor_ptr->monster_level, 3),
                    SUMMON_DEMON, PM_ALLOW_GROUP, m_idx);
            }
        }
        */
    }
}
