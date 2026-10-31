#pragma once

#include "object-enchant/object-ego.h"

namespace test {

/*!
 * @brief エゴ定義一覧 (egos_info) を空へ退避し、スコープを抜けるときに元の一覧へ復元する
 * @details エゴを読み書きするテストが各所で手書きしていた egos_info の退避・復元を共通化する。
 *          コンストラクタで egos_info を空にするため、テストは直後から自由に登録してよい。
 */
class ScopedEgoList {
public:
    ScopedEgoList()
    {
        this->saved_egos.swap(egos_info);
    }

    ~ScopedEgoList()
    {
        egos_info.swap(this->saved_egos);
    }

    ScopedEgoList(const ScopedEgoList &) = delete;
    ScopedEgoList &operator=(const ScopedEgoList &) = delete;

private:
    decltype(egos_info) saved_egos;
};

}
