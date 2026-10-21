#include "io/pref-file-expressor.h"
#include "game-option/runtime-arguments.h"
#include "io/condition-expression.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "player/process-name.h"
#include "system/creature-entity.h"
#include "system/system-variables.h"
#include "term/z-form.h"

/*!
 * @brief 設定ファイルと町のマップの条件式に共通の変数の値を返す
 * @param creature クリーチャーへの参照
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。共通の変数でなければ tl::nullopt
 */
tl::optional<std::string> resolve_common_expression_variable(CreatureEntity &creature, std::string_view name)
{
    if (name == "SYS") {
        return std::string(ANGBAND_SYS);
    }
    if (name == "GRAF") {
        return std::string(ANGBAND_GRAF);
    }
    if (name == "MONOCHROME") {
        return arg_monochrome ? "ON" : "OFF";
    }
    if (name == "RACE") {
        return creature.get_race_info()->title.en_string();
    }
    if (name == "CLASS") {
        return (*creature.get_class_info()).title.en_string();
    }
    if (name == "PLAYER") {
        return make_player_name_for_expression(creature.name);
    }
    if (name == "REALM1") {
        return PlayerRealm(creature).realm1().get_name().en_string();
    }
    if (name == "REALM2") {
        return PlayerRealm(creature).realm2().get_name().en_string();
    }

    return tl::nullopt;
}

/*!
 * @brief 設定ファイルの条件式の変数の値を返す
 * @param creature クリーチャーへの参照
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。知らない変数なら tl::nullopt
 */
static tl::optional<std::string> resolve_pref_file_variable(CreatureEntity &creature, std::string_view name)
{
    if (name == "KEYBOARD") {
        return std::string(ANGBAND_KEYBOARD);
    }
    if (name == "LEVEL") {
        return format("%02d", creature.get_level());
    }
    if (name == "AUTOREGISTER") {
        return creature.is_autopick_autoregister() ? "1" : "0";
    }
    if (name == "MONEY") {
        return format("%09ld", (long int)creature.get_au());
    }

    return resolve_common_expression_variable(creature, name);
}

/*!
 * @brief 設定ファイルの条件式 (「?:」の行) を評価する
 * @param creature クリーチャーへの参照
 * @param expr 条件式
 * @return 評価の結果。条件を満たさなければ "0"
 * @details 書式と返す値は evaluate_condition_expression() を参照。
 */
std::string process_pref_file_expr(CreatureEntity &creature, std::string_view expr)
{
    return process_pref_file_expr_checked(creature, expr).value_or("0");
}

/*!
 * @brief 設定ファイルの条件式を評価し、読込側へエラーを返す
 * @details 書式とエラー分類は evaluate_condition_expression_checked() を参照
 */
tl::expected<std::string, ConditionExpressionError> process_pref_file_expr_checked(CreatureEntity &creature, std::string_view expr)
{
    return evaluate_condition_expression_checked(expr, [&creature](std::string_view name) { return resolve_pref_file_variable(creature, name); });
}
