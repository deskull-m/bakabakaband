#include "info-reader/monster-flag-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/language-switcher.h"
#include "view/display-messages.h"

/*!
 * @brief 基本フラグの格納先を、era_flags を除く 14 個すべてを受け取って束ねる
 * @details 引数は MonsterBasicFlagGroups の各メンバーと同じ順・同じ名前で、それぞれのフラグの格納先を表す。
 * era_flags のみ、モンスターの定義だけが持つため末尾のポインタ (既定 nullptr) で受け取る。
 */
MonsterBasicFlagGroups::MonsterBasicFlagGroups(
    EnumClassFlagGroup<MonsterFeedType> &meat_feed_flags,
    EnumClassFlagGroup<MonsterResistanceType> &resistance_flags,
    EnumClassFlagGroup<MonsterAuraType> &aura_flags,
    EnumClassFlagGroup<MonsterBehaviorType> &behavior_flags,
    EnumClassFlagGroup<MonsterVisualType> &visual_flags,
    EnumClassFlagGroup<MonsterKindType> &kind_flags,
    EnumClassFlagGroup<MonsterDropType> &drop_flags,
    EnumClassFlagGroup<MonsterWildernessType> &wilderness_flags,
    EnumClassFlagGroup<MonsterFeatureType> &feature_flags,
    EnumClassFlagGroup<MonsterPopulationType> &population_flags,
    EnumClassFlagGroup<MonsterSpeakType> &speak_flags,
    EnumClassFlagGroup<MonsterBrightnessType> &brightness_flags,
    EnumClassFlagGroup<MonsterSpecialType> &special_flags,
    EnumClassFlagGroup<MonsterMiscType> &misc_flags,
    EnumClassFlagGroup<MonsterEraType> *era_flags)
    : meat_feed_flags(meat_feed_flags)
    , resistance_flags(resistance_flags)
    , aura_flags(aura_flags)
    , behavior_flags(behavior_flags)
    , visual_flags(visual_flags)
    , kind_flags(kind_flags)
    , drop_flags(drop_flags)
    , wilderness_flags(wilderness_flags)
    , feature_flags(feature_flags)
    , population_flags(population_flags)
    , speak_flags(speak_flags)
    , brightness_flags(brightness_flags)
    , special_flags(special_flags)
    , misc_flags(misc_flags)
    , era_flags(era_flags)
{
}

/*!
 * @brief フラグ名をモンスターの基本フラグの表で探し、見つかった表のフラグを1つ立てる
 * @details 食性・耐性・オーラ・習性・シンボル・種族・時代・ドロップ・荒野・地形・出現数・セリフ・明暗・特殊・その他の順に探す。
 * 見つからなかったときはどのフラグも変えず、未知のモンスター・フラグとして表示する。
 * 性別はモンスターの定義では別の項目で読み込み、ダンジョンの出現条件では呼び出し側で先に探すため、ここでは扱わない。
 * 時代フラグはモンスターの定義のみが持つため、ダンジョンの出現条件では era_flags が null で読み飛ばされる。
 * @param groups 基本フラグの格納先
 * @param what フラグ名
 * @return 見つけたら true
 */
bool grab_one_monster_basic_flag(const MonsterBasicFlagGroups &groups, std::string_view what)
{
    if (info_grab_one_flag(groups.meat_feed_flags, r_info_meat_feed, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.aura_flags, r_info_aura_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if ((groups.era_flags != nullptr) && info_grab_one_flag(*groups.era_flags, r_info_era_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.special_flags, r_info_special_flags, what)) {
        return true;
    }

    if (info_grab_one_flag(groups.misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}

/*!
 * @brief フラグ名を能力フラグ(魔法・ブレス)の表で探し、見つかったフラグを1つ立てる
 * @details 見つからなかったときはフラグを変えず、未知のモンスター・フラグとして表示する。
 * @param ability_flags 能力フラグの格納先
 * @param what フラグ名
 * @return 見つけたら true
 */
bool grab_one_monster_spell_flag(EnumClassFlagGroup<MonsterAbilityType> &ability_flags, std::string_view what)
{
    if (info_grab_one_flag(ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}
