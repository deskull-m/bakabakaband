#pragma once
#include "system/angband.h"
#include "util/flag-group.h"
#include <map>
#include <string>

typedef int ALLIANCE_ID;
class PlayerType;

enum class AllianceType : int {
    NONE = 0, //!< 無所属
    AMBER = 1, //!< アンバー
    COCHAOS = 2, //!< 混沌の宮廷
    VALINOR = 3, //!< ヴァリノール
    UTUMNO = 4, //!< ウトゥムノ
    JURAL = 5, //!< ジュラル星人
    CHINCHINTEI = 6, //!< ちんちん亭
    ODIO = 7, //!< オディオ
    KENOHGUN = 8, //!< 拳王軍
    FANG_FAMILY = 9, //!< 牙一族
    KOGAN_RYU = 10, //!< 虎眼流
    ELDRAZI = 11, //!< エルドラージ
    UNGOLIANT = 12, //!< ウンゴリアント一族
    SHITTO_DAN = 13, //!< しっと団
    GE_ORLIC = 14, //!< オーリック朝銀河帝国（超人ロック）
    TURBAN_KIDS = 15, //!< ターバンのガキ共
    NAKED_KNIGHTS = 16, //!< 全裸騎士団
    NUMENOR = 17, //!< ヌメノール王国
    GO = 18, //!< GO教
    THE_SHIRE = 19, //!< ホビット庄
    HAKUSIN_KARATE = 20, //!< 迫真空手部
    DOKACHANS = 21, //!< 岡山中高年男児糞尿愛好会
    KETHOLDETH = 22, //!< ケツホルデス
    MELDOR = 23, //!< メルドール
    ANGARTHA = 24, //!< アンガルタ
    GETTER = 25, //!< ゲッター
    PURE_MIRRODIN = 26, //!< 清純なるミラディン
    KING = 27, //!< KING
    PHYREXIA = 28, //!< ファイレクシア
    AVARIN_LORDS = 29, //!< アヴァリ諸侯
    GOLAN = 30, //!< GOLAN
    BINJO_BUDDHISM = 31, //!< 便乗仏教
    ASHINA_CLAN = 32, //!< 葦名一門
    SUREN = 33, //!< スレン王国
    FEANOR_NOLDOR = 34, //!< フェアノール統ノルドール
    GAICHI = 35, //!< ガイチ帝国
    LEGEND_OF_SAVIOR = 36, //!< 世紀末救世主伝説
    TOPHAMHATT = 37, //!< トップハム・ハット一族
    TRIOTHEPANCH = 38, //!< トリオ・ザ・パンチ
    MEGADETH = 39, //!< 秘密結社メガデス
    KHORNE = 40, //!< 血の神コーン
    SLAANESH = 41, //!< 快楽神スラーネッシュ
    HAFU = 42, //!< 覇府
    TZEENTCH = 43, //!< 変幻の神ティーンチ
    NIBELUNG = 44, //!< ニーベルングの王国
    SEXY_COMMANDO_CLUB = 45, //!< セクシーコマンドー部
    NURGLE = 46, //!< 腐敗神ナーグル
    NANMAN = 47, //!< 南蛮
    COOKIE_GRANDMA = 48, //!< クッキーババア
    HIDE = 49, //!< ひで
    GONDOR = 50, //!< ゴンドール
    VALVERDE = 51, //!< バルベルデ共和国
    FINGOLFIN_NOLDOR = 52, //!< フィンゴルフィン統ノルドール
    INCUBETOR = 53, //!< インキュベーター
    FRIEZA_CLAN = 54, //!< フリーザ一族
    SILVAN_ELF = 55, //!< シルヴァン・エルフ
    ARYAN_FAMILY = 56, //!< アーリアン・ファミリー
    BASAM_EMPIRE = 57, //!< バサム帝国
    HIONHURN = 58, //!< ハイオンハーン
    CHARDROS = 59, //!< チャードロス
    ARIOCH = 60, //!< アリオッチ
    XIOMBARG = 61, //!< キシオムバーグ
    MABELODE = 62, //!< マベロード
    KHAINE = 63, //!< カイン
    ANOR_LONDO = 64, //!< アノール・ロンド
    BOLETARIA = 65, //!< ボーレタリア
    IDE = 66, //!< イデ
    MAX,
};

enum alliance_flags {
    ALLF_ORDER, //!< 秩序の陣営
    ALLF_CHAOS, //!< 混沌の陣営
    MAX,
};

class Alliance {
public:
    AllianceType id; //!< ID
    std::string tag; //!< タグ
    std::string name; //!< 陣営名
    int64_t base_power; //!< 基本勢力指数
    Alliance(AllianceType id, std::string tag, std::string name, int64_t base_power);
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int64_t calcCurrentPower();
    virtual bool isAnnihilated();
    virtual bool isFriendly(PlayerType *creature_ptr) const;
    virtual int calcImpressionPoint(PlayerType *creature_ptr) const = 0;
    virtual ~Alliance() = default;
    int64_t AnihilatedPowerdownDiv = 1000; //!< 壊滅時戦力指数除算
    virtual void panishment(PlayerType &player_ptr);

protected:
    static int calcPlayerPower(PlayerType const &player_ptr, const int bias, const int base_level);
};

class AllianceNone : public Alliance {
public:
    using Alliance::Alliance;
    AllianceNone() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const override;
    virtual ~AllianceNone() = default;
};

class AllianceGEOrlic : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGEOrlic() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGEOrlic() = default;
};

class AllianceGO : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGO() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGO() = default;
};

class AllianceHakushinKarate : public Alliance {
public:
    using Alliance::Alliance;
    AllianceHakushinKarate() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceHakushinKarate() = default;
};

class AllianceDokachans : public Alliance {
public:
    using Alliance::Alliance;
    AllianceDokachans() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceDokachans() = default;
};

class AllianceMeldor : public Alliance {
public:
    using Alliance::Alliance;
    AllianceMeldor() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceMeldor() = default;
};

class AllianceAngartha : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAngartha() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceAngartha() = default;
};

class AllianceGetter : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGetter() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGetter() = default;
};

class AlliancePureMirrodin : public Alliance {
public:
    using Alliance::Alliance;
    AlliancePureMirrodin() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AlliancePureMirrodin() = default;
};

class AllianceKING : public Alliance {
public:
    using Alliance::Alliance;
    AllianceKING() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceKING() = default;
};

class AlliancePhyrexia : public Alliance {
public:
    using Alliance::Alliance;
    AlliancePhyrexia() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AlliancePhyrexia() = default;
};

class AllianceAvarinLords : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAvarinLords() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceAvarinLords() = default;
};

class AllianceGOLAN : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGOLAN() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGOLAN() = default;
};

class AllianceBinzyouBuddhism : public Alliance {
public:
    using Alliance::Alliance;
    AllianceBinzyouBuddhism() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceBinzyouBuddhism() = default;
};

class AllianceAshinaClan : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAshinaClan() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceAshinaClan() = default;
};

class AllianceSuren : public Alliance {
public:
    using Alliance::Alliance;
    AllianceSuren() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceSuren() = default;
};

class AllianceGaichi : public Alliance {
public:
    using Alliance::Alliance;
    AllianceGaichi() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceGaichi() = default;
};

extern const std::map<AllianceType, std::shared_ptr<Alliance>> alliance_list;
extern const std::map<std::tuple<AllianceType, AllianceType>, int> each_alliance_impression;
