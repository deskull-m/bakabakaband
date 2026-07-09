#include "system/player-type-definition.h"
#include "locale/language-switcher.h"
#include "player-info/class-info.h"
#include "system/creature-entity.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief プレイヤー構造体実体 / Static player info record
 * @note get_instance() を通してのみアクセスすること。直接参照しないこと。
 */
static PlayerType p_body;

PlayerType::PlayerType() = default;

PlayerType &PlayerType::get_instance()
{
    return p_body;
}

PlayerType *PlayerType::get_instance_ptr()
{
    return &p_body;
}

bool PlayerType::is_valid() const
{
    return true; // プレイヤーは常に有効
}

bool PlayerType::is_dead() const
{
    return this->is_dead_;
}

bool PlayerType::is_player() const
{
    return true;
}

/*!
 * @brief プレイヤー状態を空の初期値にリセットする
 * @details game-play-initializer 等からのキャラクター再初期化時に使われる。
 *          名前やフロア情報等は呼び出し側が事前退避・事後復元する前提。
 */
void PlayerType::wipe()
{
    *this = {};
}

/*!
 * @brief 表示用の称号を取得する (提案 E5, プレイヤー override)
 * @details wizard モード / 勝利者状態を最優先し、通常時は職業・レベル別称号を返す。
 * 旧 print_title() の is_player() 分岐をここへ集約 (基底は "なし" を返す)。
 */
std::string PlayerType::get_title() const
{
    const auto &world = AngbandWorld::get_instance();
    if (world.wizard) {
        return _("［ウィザード］", "[=-WIZARD-=]");
    }

    if (world.total_winner) {
        return world.is_player_true_winner() ? _("*真・勝利者*", "*TRUEWINNER*") : _("***勝利者***", "***WINNER***");
    }

    const auto &titles = player_titles.at(this->get_pclass());
    const auto title_index = std::min(static_cast<size_t>((this->get_level() - 1) / 5), titles.size() - 1);
    return titles.at(title_index);
}
