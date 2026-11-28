#include "view/screen-state.h"
#include "term/screen-processor.h"
#include "world/world.h"

namespace {
/*!
 * @brief 地図の上で 1 行で答える入力か
 */
bool is_single_line_prompt(InputKind kind)
{
    switch (kind) {
    case InputKind::CONFIRM:
    case InputKind::COMMAND_CHAR:
    case InputKind::STRING:
    case InputKind::QUANTITY:
    case InputKind::MORE:
    case InputKind::DIRECTION:
    case InputKind::ANY_KEY:
    case InputKind::ITEM:
    case InputKind::SPELL:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 画面全体を使う表示の入力か
 */
bool is_fullscreen_input(const InputContext &context)
{
    switch (context.kind) {
    case InputKind::FILE_VIEW:
    case InputKind::BROWSER:
    case InputKind::OPTION:
    case InputKind::AUTOPICK_EDITOR:
    case InputKind::BUILDING:
    case InputKind::BIRTH:
        return true;
    case InputKind::COMMAND: {
        const auto *details = std::get_if<CommandInputDetails>(&context.details);
        return (details != nullptr) && details->shopping;
    }
    default:
        return false;
    }
}

tl::optional<Pos2D> get_cursor_pos(const InputContext &context)
{
    if (const auto *target = std::get_if<TargetInputDetails>(&context.details)) {
        return target->pos;
    }
    if (const auto *point = std::get_if<PointInputDetails>(&context.details)) {
        return point->pos;
    }

    return tl::nullopt;
}
}

/*!
 * @brief 画面の状態を分類する
 * @details
 * - フロアが無ければ NO_GAME
 * - 店・知識・ヘルプ・オプションなど画面全体を使う入力なら FULLSCREEN_TEXT
 * - ターゲットの選択・場所の指定なら TARGETING (説明のために重ねた画面の上でも、カーソルは地図の上にある)
 * - 重ねた画面が無ければ、コマンド待ちと入力待ち以外は MAP、1 行のプロンプトなら MAP_WITH_PROMPT
 * - 重ねた画面があれば、開いてから画面全体を消したなら FULLSCREEN_TEXT、消していなければ OVERLAY_MENU
 */
ScreenStateInfo classify_screen_state(const ScreenStateInputs &inputs)
{
    ScreenStateInfo info;
    if (inputs.input_context) {
        info.input_kind = inputs.input_context->kind;
    }

    if (!inputs.has_dungeon) {
        info.state = ScreenState::NO_GAME;
        return info;
    }

    const auto &context = inputs.input_context;
    if (context && is_fullscreen_input(*context)) {
        info.state = ScreenState::FULLSCREEN_TEXT;
        return info;
    }

    if (context && ((context->kind == InputKind::TARGET) || (context->kind == InputKind::POINT))) {
        info.state = ScreenState::TARGETING;
        info.cursor_pos = get_cursor_pos(*context);
        return info;
    }

    if (inputs.icky_depth <= 0) {
        info.state = (context && is_single_line_prompt(context->kind)) ? ScreenState::MAP_WITH_PROMPT : ScreenState::MAP;
        return info;
    }

    info.state = inputs.cleared_since_save ? ScreenState::FULLSCREEN_TEXT : ScreenState::OVERLAY_MENU;
    return info;
}

ScreenStateInputs collect_screen_state_inputs()
{
    const auto &world = AngbandWorld::get_instance();
    ScreenStateInputs inputs;
    inputs.has_dungeon = world.character_dungeon;
    inputs.icky_depth = world.character_icky_depth;
    inputs.cleared_since_save = is_main_screen_cleared_since_save();
    inputs.input_context = get_input_context();
    return inputs;
}
