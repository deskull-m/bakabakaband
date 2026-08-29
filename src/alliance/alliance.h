#pragma once
#include "system/angband.h"
#include "util/flag-group.h"
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

enum class MonraceId : int16_t;
class CreatureEntity;
class MonraceDefinition;
class CreatureEntity;

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
    NANTO_ORTHODOX = 67, //!< 南斗正統派
    SEITEI = 68, //!< 聖帝軍
    DIABOLIQUE = 69, //!< デアボリカ
    SOUKAIYA = 70, //!< ソウカイヤ
    YEEK_KINGDOM = 71, //!< イークの王国
    EAGLE_CLAN = 72, //!< 大鷲の一族
    BOLAS = 73, //!< ボーラス
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
    int64_t natural_recovery; //!< 自然回復量（10ターンごと）
    Alliance(AllianceType id, std::string tag, std::string name, int64_t base_power, int64_t natural_recovery = 0);
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int64_t calcCurrentPower();
    virtual bool isAnnihilated();
    virtual bool isFriendly(const CreatureEntity &creature) const;
    virtual int calcImpressionPoint(const CreatureEntity &creature) const = 0;
    virtual ~Alliance() = default;
    int64_t AnnihilatedPowerdownDiv = 1000; //!< 壊滅時戦力指数除算
    virtual void panishment(CreatureEntity &creature);
    virtual std::vector<MonraceId> get_ambush_monsters(CreatureEntity &creature, int impression_point) const;
    virtual std::string get_ambush_message() const;
    virtual bool is_hostile_to(const CreatureEntity &creature_other, const MonraceDefinition &monrace) const;

    // base_powerを変更するメソッド
    void set_base_power(int64_t new_power)
    {
        base_power = new_power;
    }
    int64_t get_base_power() const
    {
        return base_power;
    }

    // natural_recoveryを変更するメソッド
    void set_natural_recovery(int64_t new_recovery)
    {
        natural_recovery = new_recovery;
    }
    int64_t get_natural_recovery() const
    {
        return natural_recovery;
    }

protected:
    static int calcPlayerPower(const CreatureEntity &creature, const int bias, const int base_level);
    static int calcIronmanHostilityPenalty();
    //! 指定した全モンレースが絶滅 (mob_num == 0) しているかを返す (isAnnihilated の定型集約)
    static bool all_monraces_extinct(std::initializer_list<MonraceId> monrace_ids);
};

// 分離されたアライアンスクラスのインクルード
#include "alliance/alliance-angartha.h"
#include "alliance/alliance-ashina-clan.h"
#include "alliance/alliance-avarin-lords.h"
#include "alliance/alliance-binzyou-buddhism.h"
#include "alliance/alliance-diabolique.h"
#include "alliance/alliance-dokachans.h"
#include "alliance/alliance-gaichi.h"
#include "alliance/alliance-ge-orlic.h"
#include "alliance/alliance-getter.h"
#include "alliance/alliance-go.h"
#include "alliance/alliance-golan.h"
#include "alliance/alliance-hakushin-karate.h"
#include "alliance/alliance-king.h"
#include "alliance/alliance-meldor.h"
#include "alliance/alliance-naked-knights.h"
#include "alliance/alliance-none.h"
#include "alliance/alliance-phyrexia.h"
#include "alliance/alliance-pure-mirrodin.h"
#include "alliance/alliance-suren.h"
#include "alliance/alliance-turban-kids.h"
#include "alliance/alliance-yeek-kingdom.h"

extern const std::map<AllianceType, std::shared_ptr<Alliance>> alliance_list;
extern const std::map<std::tuple<AllianceType, AllianceType>, int> each_alliance_impression;

std::string get_alliance_type_tag(AllianceType alliance_type);
