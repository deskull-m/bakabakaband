#include "player/player-sex.h"

/*
 * Pointer to the player tables
 * (sex, race, class, magic)
 */
const player_sex_type *sp_ptr;

/*!
 * @brief 性別表記 /
 * Player Sexes
 * @details
 * <pre>
 *      Title,
 *      Winner
 * </pre>
 * 配列の並びは player_sex enum (SEX_FEMALE=0, SEX_MALE=1,
 * SEX_BISEXUAL=2, SEX_ASEXUAL=3, SEX_NONE=4) と必ず一致させること。
 * SEX_NONE はモンスター生成過程の中間値で、UI 表示等では明示的に
 * 「未設定」として扱う。
 */
const player_sex_type sex_info[MAX_SEXES] = {
    {
        { "女性", "Female" },
        { "クイーン", "Queen" },
    },
    {
        { "男性", "Male" },
        { "キング", "King" },
    },
    {
        { "両性", "Bisexual" },
        { "ロード", "Lord" },
    },
    {
        { "無性", "Asexual" },
        { "ロード", "Lord" },
    },
    {
        { "未設定", "Unset" },
        { "未設定", "Unset" },
    },
};
