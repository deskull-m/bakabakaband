#include "alliance/alliance.h"
#include "alliance/alliance-jural.h"
#include "alliance/alliance-megadeth.h"
#include "alliance/alliance-shire.h"
#include "alliance/alliance-tophamhatt.h"
#include "alliance/alliance-triothepunch.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/race-flags1.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

const std::map<AllianceType, std::shared_ptr<Alliance>> alliance_list = {
    { AllianceType::NONE, std::make_unique<AllianceNone>(AllianceType::NONE, "NONE", _("無所属", "None"), 0) },
    { AllianceType::AMBER, std::make_unique<AllianceAmber>(AllianceType::AMBER, "AMBER", _("アンバー", "Amber"), 350000000L) },
    { AllianceType::COCHAOS, std::make_unique<AllianceCourtOfChaos>(AllianceType::COCHAOS, "COCHAOS", _("混沌の宮廷", "Court of Chaos"), 200000000L) },
    { AllianceType::VALINOR, std::make_unique<AllianceValinor>(AllianceType::VALINOR, "VALINOR", _("ヴァリノール", "Valinor"), 4000000L) },
    { AllianceType::UTUMNO, std::make_unique<AllianceCourtOfChaos>(AllianceType::UTUMNO, "UTUMNO", _("ウトゥムノ", "Utumno"), 3000000L) },
    { AllianceType::JURAL, std::make_unique<AllianceJural>(AllianceType::JURAL, "JURAL", _("ジュラル星人", "Jural"), 5500L) },
    { AllianceType::CHINCHINTEI, std::make_unique<AllianceChinChinTei>(AllianceType::CHINCHINTEI, "CHINCHINTEI", _("ちんちん亭", "Chin-Chin-Tei"), 191919L) },
    { AllianceType::ODIO, std::make_unique<AllianceOdio>(AllianceType::ODIO, "ODIO", _("オディオ", "Odio"), 300000L) },
    { AllianceType::KENOHGUN, std::make_unique<AllianceKenohgun>(AllianceType::KENOHGUN, "KENOHGUN", _("拳王軍", "Kenohgun"), 100000L) },
    { AllianceType::FANG_FAMILY, std::make_unique<AllianceFangFamily>(AllianceType::FANG_FAMILY, "FANG-FAMILY", _("牙一族", "Fang Family"), 4000L) },
    { AllianceType::KOGAN_RYU, std::make_unique<AllianceKoganRyu>(AllianceType::KOGAN_RYU, "KOGAN-RYU", _("虎眼流", "Kogan Ryu"), 10000L) },
    { AllianceType::ELDRAZI, std::make_unique<AllianceEldrazi>(AllianceType::ELDRAZI, "ELDRAZI", _("エルドラージ", "Eldrazi"), 120000000L) },
    { AllianceType::UNGOLIANT, std::make_unique<AllianceUngoliant>(AllianceType::UNGOLIANT, "UNGOLIANT", _("ウンゴリアント一族", "Ungoliant's Family"), 1500000L) },
    { AllianceType::SHITTO_DAN, std::make_unique<AllianceShittoDan>(AllianceType::SHITTO_DAN, "SHITTO-DAN", _("しっと団", "Sitto-Dan"), 1500L) },
    { AllianceType::GE_ORLIC, std::make_unique<AllianceGEOrlic>(AllianceType::GE_ORLIC, "GE-ORLIC", _("オーリック朝銀河帝国", "Galactic Empire of Orlic"), 2000000L) },
    { AllianceType::TURBAN_KIDS, std::make_unique<AllianceTurbanKids>(AllianceType::TURBAN_KIDS, "TURBAN-KIDS", _("ターバンのガキ共", "Turban Kids"), 20000L) },
    { AllianceType::NAKED_KNIGHTS, std::make_unique<AllianceNakedKnights>(AllianceType::NAKED_KNIGHTS, "NAKED-KNIGHTS", _("全裸騎士団", "Naked Nights"), 3000L) },
    { AllianceType::NUMENOR, std::make_unique<AllianceNumenor>(AllianceType::NUMENOR, "NUMENOR", _("ヌメノール王国", "Numenor Kingdom"), 1500000L) },
    { AllianceType::GO, std::make_unique<AllianceGO>(AllianceType::GO, "GO", _("GO教", "GO"), 1800000L) },
    { AllianceType::THE_SHIRE, std::make_unique<AllianceTheShire>(AllianceType::THE_SHIRE, "THE-SHIRE", _("ホビット庄", "The Shire"), 5000L) },
    { AllianceType::HAKUSIN_KARATE, std::make_unique<AllianceTheShire>(AllianceType::HAKUSIN_KARATE, "HAKUSIN-KARATE", _("迫真空手部", "Hakusin Karate"), 5000L) },
    { AllianceType::DOKACHANS, std::make_unique<AllianceDokachans>(AllianceType::DOKACHANS, "DOKACHANS", _("岡山中高年男児糞尿愛好会", "Dokachans"), 69L) },
    { AllianceType::KETHOLDETH, std::make_unique<AllianceKetholdeth>(AllianceType::KETHOLDETH, "KETHOLDETH", _("ケツホルデス家", "Kethholdeth House"), 1919L) },
    { AllianceType::MELDOR, std::make_unique<AllianceMeldor>(AllianceType::MELDOR, "MELDOR", _("メルドール", "Meldor"), 5000000L) },
    { AllianceType::ANGARTHA, std::make_unique<AllianceAngartha>(AllianceType::ANGARTHA, "ANGARTHA", _("アンガルタ", "Angartha"), 900000L) },
    { AllianceType::GETTER, std::make_unique<AllianceGetter>(AllianceType::GETTER, "GETTER", _("ゲッター", "Getter"), 200000000L) },
    { AllianceType::PURE_MIRRODIN, std::make_unique<AlliancePureMirrodin>(AllianceType::PURE_MIRRODIN, "PURE-MIRRODIN", _("清純なるミラディン", "Pure Mirrodin"), 200000L) },
    { AllianceType::KING, std::make_unique<AllianceKING>(AllianceType::KING, "KING", _("KING", "KING"), 150000L) },
    { AllianceType::PHYREXIA, std::make_unique<AlliancePhyrexia>(AllianceType::PHYREXIA, "PHYREXIA", _("ファイレクシア", "Phyrexia"), 2000000L) },
    { AllianceType::AVARIN_LORDS, std::make_unique<AllianceAvarinLords>(AllianceType::AVARIN_LORDS, "AVARIN-LORDS", _("アヴァリ諸侯同盟", "Avarin Lords"), 1000000L) },
    { AllianceType::GOLAN, std::make_unique<AllianceGOLAN>(AllianceType::GOLAN, "GOLAN", _("GOLAN", "GOLAN"), 100000L) },
    { AllianceType::BINJO_BUDDHISM, std::make_unique<AllianceBinzyouBuddhism>(AllianceType::BINJO_BUDDHISM, "BINJO-BUDDHISM", _("便乗仏教", "Binjo Buddhism"), 80000L) },
    { AllianceType::ASHINA_CLAN, std::make_unique<AllianceBinzyouBuddhism>(AllianceType::ASHINA_CLAN, "ASHINA-CLAN", _("葦名一門", "Ashina Clan"), 180000L) },
    { AllianceType::SUREN, std::make_unique<AllianceSuren>(AllianceType::SUREN, "SUREN", _("スレン王国", "Suren Kingdom"), 100000L) },
    { AllianceType::FEANOR_NOLDOR, std::make_unique<AllianceFeanorNoldor>(AllianceType::FEANOR_NOLDOR, "FEANOR-NOLDOR", _("フェアノール統一ノルドール", "Feanor Noldor"), 3500000L) },
    { AllianceType::GAICHI, std::make_unique<AllianceGaichi>(AllianceType::GAICHI, "GAICHI", _("ガイチ帝国", "Gaichi Empire"), 1100000L) },
    { AllianceType::LEGEND_OF_SAVIOR, std::make_unique<AllianceLegendOfSavior>(AllianceType::LEGEND_OF_SAVIOR, "LEGEND-OF-SAVIOR", _("世紀末救世主伝説", "Legend of the Latter-day Savior"), 0L) },
    { AllianceType::TOPHAMHATT, std::make_unique<AllianceTophamHatt>(AllianceType::TOPHAMHATT, "TOPHAMHATT", _("トップハムハット一族", "Topham Hatt Family"), 1400000L) },
    { AllianceType::TRIOTHEPANCH, std::make_unique<AllianceTrioThePunch>(AllianceType::TRIOTHEPANCH, "TRIOTHEPANCH", _("トリオ・ザ・パンチ", "Trio The Panch"), 50000L) },
    { AllianceType::MEGADETH, std::make_unique<AllianceMegadeth>(AllianceType::MEGADETH, "MEGADETH", _("秘密結社メガデス", "Secret Society Megadeth"), 4000L) },
};

const std::map<std::tuple<AllianceType, AllianceType>, int> each_alliance_impression = {
    { std::make_tuple(AllianceType::VALINOR, AllianceType::UTUMNO), -5000 },
    { std::make_tuple(AllianceType::UTUMNO, AllianceType::VALINOR), -1000 },
};

Alliance::Alliance(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : id(id)
    , tag(tag)
    , name(name)
    , base_power(base_power)
{
}

/*!
 * @brief プレイヤーのレベル自体を印象値に加減算する処理
 * @param player_ptr 評価対象とするプレイヤー
 * @param bias 倍率
 * @param min_level 評価基準最低レベル
 */
int Alliance::calcPlayerPower(PlayerType const &player_ptr, const int bias, const int min_level)
{
    if (min_level > player_ptr.lev) {
        return 0;
    }
    return (2000 + 10 * (player_ptr.lev - min_level + 1) * (player_ptr.lev - min_level + 1)) * bias / 100;
}

void Alliance::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    return;
}

int64_t Alliance::calcCurrentPower()
{
    const auto &monraces = MonraceList::get_instance();
    int64_t res = this->base_power;
    for (auto &[r_idx, r_ref] : monraces_info) {
        if (r_ref.alliance_idx == this->id) {
            if (r_ref.mob_num > 0) {
                res += monraces.get_monrace(r_idx).calc_power() * r_ref.mob_num;
            }
        }
    }
    if (this->isAnnihilated()) {
        res /= this->AnihilatedPowerdownDiv;
    }
    return res;
}

bool Alliance::isAnnihilated()
{
    return false;
}

int AllianceNone::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceAmber::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);
    return impression;
}

int AllianceCourtOfChaos::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);
    return impression;
}

int AllianceValinor::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += (creature_ptr->alignment > 0) ? creature_ptr->alignment : -creature_ptr->alignment * 3;
    impression += Alliance::calcPlayerPower(*creature_ptr, -16, 30);
    return impression;
}

int AllianceUtumno::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 30);
    return impression;
}

int AllianceChinChinTei::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceOdio::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceKenohgun::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 20);
    return impression;
}

bool AllianceKenohgun::isAnnihilated()
{
    return monraces_info[MonsterRaceId::RAOU].mob_num == 0;
}

bool AllianceFangFamily::isAnnihilated()
{
    return monraces_info[MonsterRaceId::KING_FANG_FAMILY].mob_num == 0;
}

int AllianceFangFamily::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 5, 10);
    impression -= monraces_info[MonsterRaceId::FANG_FAMILY].r_akills * 5;
    if (monraces_info[MonsterRaceId::KING_FANG_FAMILY].mob_num == 0) {
        impression -= 300;
    }
    return impression;
}

int AllianceKoganRyu::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 18);
    return impression;
}

int AllianceEldrazi::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceUngoliant::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 8, 30);
    return impression;
}

int AllianceShittoDan::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceShittoDan::isAnnihilated()
{
    return monraces_info[MonsterRaceId::SHITTO_MASK].mob_num == 0;
}

int AllianceGEOrlic::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 30);
    return impression;
}

int AllianceTurbanKids::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceNakedKnights::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceNumenor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 20);
    return impression;
}

int AllianceGO::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceDokachans::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    auto impression = 0;
    if (monraces_info[MonsterRaceId::DOKACHAN].mob_num == 0) {
        impression -= 1000;
    }
    if (monraces_info[MonsterRaceId::OLDMAN_60TY].mob_num == 0) {
        impression -= 1000;
    }
    if (monraces_info[MonsterRaceId::BROTHER_45TH].mob_num == 0) {
        impression -= 1000;
    }
    return impression;
}

bool AllianceDokachans::isAnnihilated()
{
    return monraces_info[MonsterRaceId::DOKACHAN].mob_num == 0;
}

int AllianceKetholdeth::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceKetholdeth::isAnnihilated()
{
    return monraces_info[MonsterRaceId::PRINCESS_KETHOLDETH].mob_num == 0;
}

int AllianceMeldor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 13, 28);
    return impression;
}

int AllianceAngartha::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 11, 20);
    return impression;
}

int AllianceGetter::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 20, 1);
    return impression;
}

int AlliancePureMirrodin::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 12);
    return impression;
}

int AllianceKING::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 15);
    return impression;
}

int AlliancePhyrexia::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 21);
    return impression;
}

int AllianceAvarinLords::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 23);
    return impression;
}

int AllianceGOLAN::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 12);
    impression -= monraces_info[MonsterRaceId::GOLAN_COLONEL].r_pkills * 1200;
    impression -= monraces_info[MonsterRaceId::GOLAN_MAD].r_pkills * 100;
    impression -= monraces_info[MonsterRaceId::GOLAN_RED_BELET].r_pkills * 60;
    impression -= monraces_info[MonsterRaceId::GOLAN_OFFICER].r_pkills * 10;
    impression -= monraces_info[MonsterRaceId::GOLAN_SOLDIER].r_pkills * 2;
    return impression;
}

int AllianceBinzyouBuddhism::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceBinzyouBuddhism::isAnnihilated()
{
    return monraces_info[MonsterRaceId::BINZYOU_MUR].mob_num == 0;
}

int AllianceAshinaClan::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

int AllianceSuren::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceSuren::isAnnihilated()
{
    return monraces_info[MonsterRaceId::SUREN].mob_num == 0;
}

int AllianceFeanorNoldor::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 19, 26);
    return impression;
}

int AllianceGaichi::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 4, 10);
    return impression;
}

int AllianceLegendOfSavior::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    auto impression = 0;
    if (monraces_info[MonsterRaceId::MISUMI].mob_num == 0) {
        impression = -10000;
    }
    return impression;
}

bool AllianceLegendOfSavior::isAnnihilated()
{
    return monraces_info[MonsterRaceId::KENSHIROU].mob_num == 0;
}

void AllianceLegendOfSavior::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -100) {
        return;
    }

    if (one_in_(30)) {
        int cy, cx;
        scatter(&player_ptr, &cy, &cx, player_ptr.y, player_ptr.x, 6, PROJECT_NONE);
        if (summon_named_creature(&player_ptr, 0, cy, cx, MonsterRaceId::KENSHIROU, 0)) {
            msg_print(_("「てめえに今日を生きる資格はねえ！」", "You don't deserve to live today!"));
            msg_print(_("ケンシロウはあなたがミスミ老人を殺したことに義憤を覚えて襲ってきた！", "Kenshiro attacked you because you killed old man Misumi!"));
        }
    }

    return;
}
