#pragma once
#include "util/enum-converter.h"
/*!
 * @file init-error-messages-table.h
 * @brief 馬鹿馬鹿蛮怒のゲームデータ解析エラー名ヘッダ
 */

#include "info-reader/parse-error-types.h"
#include "system/angband.h"

extern concptr err_str[enum2i(ParseErrorType::MAX)];
