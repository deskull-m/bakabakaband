#include "system/player-type-definition.h"
#include "system/creature-entity.h"

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
