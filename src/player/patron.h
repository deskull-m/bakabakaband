#pragma once

#include "locale/localized-string.h"
#include "system/angband.h"
#include "system/enums/monrace/monrace-id.h"
#include <string>
#include <tl/optional.hpp>
#include <vector>

#define MAX_PATRON 18 /*!< パトロンの最大定義数 / The number of "patrons" available (for Chaos Warriors) */

enum class PatronType : int16_t {
    NONE = -1, //!< なし
    KHORNE = 11, //!< コーン
    GETTER = 16, //!< ゲッター
};

/* パトロンからの報酬種別定義 / Chaos Warrior: Reward types: */
enum patron_reward {
    REW_POLY_SLF = 1, /*!< パトロンからの報酬: 自己変容 */
    REW_GAIN_EXP = 2, /*!< パトロンからの報酬: 経験値増加 */
    REW_LOSE_EXP = 3, /*!< パトロンからの報酬: 経験値減少 */
    REW_GOOD_OBJ = 4, /*!< パトロンからの報酬: GOODなアイテム単体の下賜 */
    REW_GREA_OBJ = 5, /*!< パトロンからの報酬: GREATなアイテム単体の下賜 */
    REW_CHAOS_WP = 6, /*!< パトロンからの報酬: 混沌武器の下賜 */
    REW_GOOD_OBS = 7, /*!< パトロンからの報酬: GOODなアイテム複数の下賜 */
    REW_GREA_OBS = 8, /*!< パトロンからの報酬: GREATなアイテム複数の下賜 */
    REW_TY_CURSE = 9, /*!< パトロンからの報酬: 太古の怨念 */
    REW_SUMMON_M = 10, /*!< パトロンからの報酬: 敵対的なモンスターの召喚(通常) */
    REW_H_SUMMON = 11, /*!< パトロンからの報酬: 敵対的なモンスターの召喚(hi-summon) */
    REW_DO_HAVOC = 12, /*!< パトロンからの報酬: 混沌招来 */
    REW_GAIN_ABL = 13, /*!< パトロンからの報酬: 増強 */
    REW_LOSE_ABL = 14, /*!< パトロンからの報酬: 1能力低下 */
    REW_RUIN_ABL = 15, /*!< パトロンからの報酬: 全能力低下 */
    REW_AUGM_ABL = 16, /*!< パトロンからの報酬: 1能力上昇 */
    REW_POLY_WND = 17, /*!< パトロンからの報酬: 傷の変化 */
    REW_HEAL_FUL = 18, /*!< パトロンからの報酬: 完全回復 */
    REW_HURT_LOT = 19, /*!< パトロンからの報酬: 分解の球によるダメージ */
    REW_CURSE_WP = 20, /*!< パトロンからの報酬: 武器呪縛 */
    REW_CURSE_AR = 21, /*!< パトロンからの報酬: 防具呪縛 */
    REW_PISS_OFF = 22, /*!< パトロンからの報酬: 苛立ち */
    REW_WRATH = 23, /*!< パトロンからの報酬: 怒り */
    REW_DESTRUCT = 24, /*!< パトロンからの報酬: *破壊* */
    REW_GENOCIDE = 25, /*!< パトロンからの報酬: シンボル抹殺 */
    REW_MASS_GEN = 26, /*!< パトロンからの報酬: 周辺抹殺 */
    REW_DISPEL_C = 27, /*!< パトロンからの報酬: モンスター退散 */
    REW_GOOD_HAFTED = 28, /*!< パトロンからの報酬: 高級品の鈍器 */
    REW_UNUSED_2 = 29, /*!< パトロンからの報酬: 未使用 */
    REW_UNUSED_3 = 30, /*!< パトロンからの報酬: 未使用 */
    REW_UNUSED_4 = 31, /*!< パトロンからの報酬: 未使用 */
    REW_UNUSED_5 = 32, /*!< パトロンからの報酬: 未使用 */
    REW_IGNORE = 33, /*!< パトロンからの報酬: 無視 */
    REW_SER_UNDE = 34, /*!< パトロンからの報酬: アンデッドの下僕下賜 */
    REW_SER_DEMO = 35, /*!< パトロンからの報酬: 悪魔の下僕下賜 */
    REW_SER_MONS = 36, /*!< パトロンからの報酬: モンスターの下僕下賜 */
};

class PlayerType;
enum player_ability_type : int;

/*!
 * @brief パトロン情報の定義
 */
class Patron {
public:
    LocalizedString name; //!< パトロン名
    Patron(LocalizedString &&name, std::vector<patron_reward> reward_table, const player_ability_type boost_stat);
    Patron(LocalizedString &&name, std::vector<patron_reward> reward_table, const player_ability_type boost_stat, MonraceId monrace_id);

    // @note C4458 クラスメンバーの隠蔽 への対応として末尾に「_」を付ける.
    void gain_level_reward(PlayerType *player_ptr_, int chosen_reward);
    void admire(PlayerType *player_ptr_);

private:
    PlayerType *player_ptr = nullptr; //!< プレイヤー参照ポインタ
    std::vector<patron_reward> reward_table; //!< 報酬テーブル
    player_ability_type boost_stat; //!< 強化能力値傾向
    tl::optional<MonraceId> monrace_id = tl::nullopt; //!< パトロンのモンスターID
};

extern std::vector<Patron> patron_list;
