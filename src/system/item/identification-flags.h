#pragma once

#include <cstdint>

/*!
 * @brief アイテムの鑑定状態フラグ (旧 IDENT_* ビットフラグ。ビット位置は
 *        special-object-flags.h の旧定数と一致させてある (セーブ互換のため))
 */
enum class IdentificationFlag : uint8_t {
    SENSE = 0, //!< 簡易鑑定 (旧 IDENT_SENSE = 0x01)
    XXX01 = 1, //!< 予約 (旧 IDENT_XXX02 = 0x02)
    EMPTY = 2, //!< 魔道具の空判定 (旧 IDENT_EMPTY = 0x04)
    KNOWN = 3, //!< 鑑定済 (旧 IDENT_KNOWN = 0x08)
    STORE = 4, //!< 店頭購入 (注: 宝物庫報酬で使い回している) (旧 IDENT_STORE = 0x10)
    FULL_KNOWN = 5, //!< *鑑定* 済 (旧 IDENT_FULL_KNOWN = 0x20)
    XXX06 = 6, //!< 予約 (旧 0x40)
    BROKEN = 7, //!< 呪い・マイナスエゴなど無価値品 (旧 IDENT_BROKEN = 0x80)
    MAX,
};
