#include "combat/martial-arts-table.h"

/*!
 * @brief マーシャルアーツの技を定義する
 * @param desc_fmt 打撃の説明の書式 ({} に対象の名前が入る)
 * @param min_lev 使える最低のレベル
 * @param success_chance 成功しやすさ
 * @param dice ダメージのダイス
 * @param special_effect 特殊効果のID
 * @details 打撃の説明の書式 (fmt::format_string) は既定のコンストラクタを持たないので、
 * 集成体のままだと既定のコンストラクタが暗黙に削除され MSVC の警告 C4623 になる。
 */
martial_arts::martial_arts(fmt::format_string<std::string_view> desc_fmt, PLAYER_LEVEL min_lev, int success_chance, const Dice &dice, int special_effect)
    : desc(desc_fmt)
    , min_level(min_lev)
    , chance(success_chance)
    , damage_dice(dice)
    , effect(special_effect)
{
}

/*!
 * @brief マーシャルアーツ打撃テーブル（伝統派）
 */
const martial_arts ma_blows[MAX_MA] = {
    { _("{}を殴った。", "You punch {}."), 1, 0, Dice(1, 5), 0 },
    { _("{}を蹴った。", "You kick {}."), 2, 0, Dice(1, 7), 0 },
    { _("{}に正拳突きをくらわした。", "You strike {}."), 3, 0, Dice(1, 9), 0 },
    { _("{}に膝蹴りをくらわした。", "You hit {} with your knee."), 5, 5, Dice(2, 4), MA_KNEE },
    { _("{}に肘打ちをくらわした。", "You hit {} with your elbow."), 7, 5, Dice(1, 12), 0 },
    { _("{}に体当りした。", "You butt {}."), 9, 10, Dice(2, 6), 0 },
    { _("{}を蹴った。", "You kick {}."), 11, 10, Dice(3, 6), MA_SLOW },
    { _("{}にアッパーをくらわした。", "You uppercut {}."), 13, 12, Dice(5, 5), 6 },
    { _("{}に二段蹴りをくらわした。", "You double-kick {}."), 16, 15, Dice(5, 6), 8 },
    { _("{}に猫爪撃をくらわした。", "You hit {} with a Cat's Claw."), 20, 20, Dice(5, 8), 0 },
    { _("{}に跳空脚をくらわした。", "You hit {} with a jump kick."), 24, 25, Dice(6, 8), 10 },
    { _("{}に鷲爪襲をくらわした。", "You hit {} with an Eagle's Claw."), 28, 25, Dice(7, 9), 0 },
    { _("{}に回し蹴りをくらわした。", "You hit {} with a circle kick."), 32, 30, Dice(8, 10), 10 },
    { _("{}に鉄拳撃をくらわした。", "You hit {} with an Iron Fist."), 35, 35, Dice(8, 11), 10 },
    { _("{}に飛空脚をくらわした。", "You hit {} with a flying kick."), 39, 35, Dice(8, 12), 12 },
    { _("{}に昇龍拳をくらわした。", "You hit {} with a Dragon Fist."), 43, 35, Dice(9, 12), 16 },
    { _("{}に石破天驚拳をくらわした。", "You hit {} with a Crushing Blow."), 48, 40, Dice(10, 13), 18 },
};

/*!
 * @brief ストリートファイト流派の技テーブル
 */
const martial_arts ma_street_fighting[MAX_MA] = {
    { _("%sを殴った。", "You punch %s."), 1, 0, Dice(1, 6), 0 },
    { _("%sを蹴り飛ばした。", "You kick %s hard."), 2, 0, Dice(1, 8), 0 },
    { _("%sに頭突きをかました。", "You headbutt %s."), 3, 5, Dice(1, 10), 0 },
    { _("%sの顔面を殴った。", "You punch %s in the face."), 5, 10, Dice(2, 5), 0 },
    { _("%sに肘鉄をくらわした。", "You elbow %s brutally."), 7, 10, Dice(2, 7), 0 },
    { _("%sを投げ飛ばした。", "You throw %s down."), 9, 15, Dice(2, 8), 0 },
    { _("%sの急所を蹴った。", "You kick %s in a vital spot."), 11, 15, Dice(3, 7), MA_KNEE },
    { _("%sをボコボコにした。", "You pummel %s."), 13, 20, Dice(4, 6), 0 },
    { _("%sにコンボ攻撃をした。", "You combo attack %s."), 16, 20, Dice(4, 8), 0 },
    { _("%sを叩きのめした。", "You beat %s down."), 20, 25, Dice(5, 8), 0 },
    { _("%sにブルータル攻撃をした。", "You brutally attack %s."), 24, 30, Dice(6, 9), 0 },
    { _("%sをフルボッコにした。", "You completely destroy %s."), 28, 30, Dice(7, 10), 0 },
    { _("%sに地獄の連打をした。", "You unleash hell on %s."), 32, 35, Dice(8, 11), 0 },
    { _("%sを完全に破壊した。", "You completely annihilate %s."), 35, 40, Dice(9, 12), 0 },
    { _("%sに必殺の一撃をした。", "You deliver a killing blow to %s."), 39, 40, Dice(10, 13), 0 },
    { _("%sを完膚なきまでに叩きのめした。", "You utterly demolish %s."), 43, 40, Dice(11, 14), 0 },
    { _("%sに最終奥義をかました。", "You unleash your ultimate technique on %s."), 48, 45, Dice(12, 15), 0 },
};

/*!
 * @brief 空手流派の技テーブル
 */
const martial_arts ma_karate[MAX_MA] = {
    { _("%sに直突きをした。", "You straight punch %s."), 1, 0, Dice(1, 5), 0 },
    { _("%sに前蹴りをした。", "You front kick %s."), 2, 0, Dice(1, 7), 0 },
    { _("%sに逆突きをした。", "You reverse punch %s."), 3, 0, Dice(1, 9), 0 },
    { _("%sに中段蹴りをした。", "You middle kick %s."), 5, 5, Dice(2, 5), 0 },
    { _("%sに肘撃ちをした。", "You elbow strike %s."), 7, 5, Dice(2, 6), 0 },
    { _("%sに横蹴りをした。", "You side kick %s."), 9, 10, Dice(2, 7), 0 },
    { _("%sに回し蹴りをした。", "You roundhouse kick %s."), 11, 10, Dice(3, 7), MA_SLOW },
    { _("%sに裏拳をくらわした。", "You backfist %s."), 13, 12, Dice(4, 6), 0 },
    { _("%sに上段蹴りをした。", "You high kick %s."), 16, 15, Dice(4, 8), 8 },
    { _("%sに手刀打ちをした。", "You chop %s."), 20, 20, Dice(5, 8), 0 },
    { _("%sに後ろ蹴りをした。", "You back kick %s."), 24, 25, Dice(6, 9), 10 },
    { _("%sに膝蹴り上げをした。", "You knee strike %s."), 28, 25, Dice(7, 10), MA_KNEE },
    { _("%sに連続突きをした。", "You rapid punch %s."), 32, 30, Dice(7, 11), 0 },
    { _("%sに三段蹴りをした。", "You triple kick %s."), 35, 35, Dice(8, 12), 12 },
    { _("%sに正拳二段突きをした。", "You double punch %s."), 39, 35, Dice(9, 13), 0 },
    { _("%sに昇龍撃をした。", "You rising dragon strike %s."), 43, 35, Dice(10, 14), 16 },
    { _("%sに秘奥義をくらわした。", "You unleash secret technique on %s."), 48, 40, Dice(11, 15), 20 },
};

/*!
 * @brief カンフー流派の技テーブル
 */
const martial_arts ma_kung_fu[MAX_MA] = {
    { _("%sに虎拳をした。", "You tiger punch %s."), 1, 0, Dice(1, 4), 0 },
    { _("%sに蛇拳をした。", "You snake strike %s."), 2, 0, Dice(1, 6), 0 },
    { _("%sに猿拳をした。", "You monkey strike %s."), 3, 0, Dice(1, 8), 0 },
    { _("%sに鶴拳をした。", "You crane strike %s."), 5, 5, Dice(2, 4), 0 },
    { _("%sに龍拳をした。", "You dragon punch %s."), 7, 5, Dice(2, 5), 0 },
    { _("%sに豹拳をした。", "You leopard strike %s."), 9, 10, Dice(2, 6), 0 },
    { _("%sに螳螂拳をした。", "You mantis strike %s."), 11, 10, Dice(3, 5), 0 },
    { _("%sに鷹爪功をした。", "You eagle claw %s."), 13, 12, Dice(3, 7), 0 },
    { _("%sに酔拳をした。", "You drunken fist %s."), 16, 15, Dice(4, 7), 0 },
    { _("%sに白鶴拳をした。", "You white crane %s."), 20, 20, Dice(4, 9), 0 },
    { _("%sに黒虎拳をした。", "You black tiger %s."), 24, 25, Dice(5, 9), 0 },
    { _("%sに金剛拳をした。", "You diamond fist %s."), 28, 25, Dice(6, 10), 0 },
    { _("%sに太極拳をした。", "You tai chi %s."), 32, 30, Dice(7, 11), 0 },
    { _("%sに少林拳をした。", "You shaolin fist %s."), 35, 35, Dice(8, 12), 0 },
    { _("%sに武当拳をした。", "You wudang fist %s."), 39, 35, Dice(9, 13), 0 },
    { _("%sに天龍八部をした。", "You heavenly dragon %s."), 43, 35, Dice(10, 14), 18 },
    { _("%sに九陰真経をくらわした。", "You Nine Yin Manual %s."), 48, 40, Dice(12, 16), 22 },
};
