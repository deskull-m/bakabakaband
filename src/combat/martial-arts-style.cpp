#include "combat/martial-arts-style.h"
#include "combat/martial-arts-table.h"

/*!
 * @brief 武術流派名を取得する
 * @param style 武術流派
 * @return 流派名
 */
const char *get_martial_arts_style_name(MartialArtsStyleType style)
{
    switch (style) {
    case MartialArtsStyleType::TRADITIONAL:
        return _("伝統派", "Traditional");
    case MartialArtsStyleType::STREET_FIGHTING:
        return _("ストリートファイト", "Street Fighting");
    case MartialArtsStyleType::KARATE:
        return _("空手", "Karate");
    case MartialArtsStyleType::KUNG_FU:
        return _("カンフー", "Kung Fu");
    default:
        return _("不明", "Unknown");
    }
}

/*!
 * @brief 武術流派の説明を取得する
 * @param style 武術流派
 * @return 流派の説明
 */
const char *get_martial_arts_style_desc(MartialArtsStyleType style)
{
    switch (style) {
    case MartialArtsStyleType::TRADITIONAL:
        return _("バランスの取れた伝統的な武術。", "Balanced traditional martial arts.");
    case MartialArtsStyleType::STREET_FIGHTING:
        return _("実戦重視の荒々しい格闘術。", "Rough practical fighting style.");
    case MartialArtsStyleType::KARATE:
        return _("正確性と威力を重視する空手。", "Precise and powerful karate.");
    case MartialArtsStyleType::KUNG_FU:
        return _("流れるような動きの中国武術。", "Flowing Chinese martial arts.");
    default:
        return _("不明な流派。", "Unknown style.");
    }
}

/*!
 * @brief 武術流派に対応するテーブルを取得する
 * @param style 武術流派
 * @return 技テーブル
 */
const martial_arts *get_martial_arts_table(MartialArtsStyleType style)
{
    switch (style) {
    case MartialArtsStyleType::TRADITIONAL:
        return ma_blows;
    case MartialArtsStyleType::STREET_FIGHTING:
        return ma_street_fighting;
    case MartialArtsStyleType::KARATE:
        return ma_karate;
    case MartialArtsStyleType::KUNG_FU:
        return ma_kung_fu;
    default:
        return ma_blows;
    }
}

/*!
 * @brief 指定したレベルで使用可能な技を取得する
 * @param style 武術流派
 * @param level プレイヤーレベル
 * @return 使用可能な技のポインタ
 */
const martial_arts *get_martial_arts_technique(MartialArtsStyleType style, int level)
{
    const martial_arts *table = get_martial_arts_table(style);
    const martial_arts *best_technique = &table[0];

    for (int i = 0; i < MAX_MA; i++) {
        if (table[i].min_level <= level) {
            best_technique = &table[i];
        } else {
            break;
        }
    }

    return best_technique;
}