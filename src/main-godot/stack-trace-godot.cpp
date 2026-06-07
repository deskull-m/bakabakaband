/*!
 * @file stack-trace-godot.cpp
 * @brief Godot 版 StackTrace スタブ実装
 *
 * Godot GDExtension ビルドでは詳細なスタックトレース取得は行わない。
 * クラッシュ時のデバッグは Godot エディタのデバッガに委ねる。
 */

#include "util/stack-trace.h"

namespace util {

struct StackTrace::Frame {
    // スタブ: フレームデータなし
};

StackTrace::StackTrace() = default;
StackTrace::~StackTrace() = default;

std::string StackTrace::dump() const
{
    return "(stack trace not available in Godot build)\n";
}

} // namespace util
