/*!
 * @file win-exception-tracer.cpp
 * @brief Godot(Windows) ビルド用 C++ 例外バックトレース出力
 *
 * Godot ビルドでは main-win.cpp (stack-trace-win.cpp) を除外しているため、
 * 未捕捉例外でクラッシュしても投げ場所が分からない。Vectored Exception
 * Handler を登録し、C++ 例外がスローされた瞬間 (スタック巻き戻し前) に
 * バックトレースを stderr へ出力する。診断専用。
 */

#if defined(_WIN32) && defined(USE_GODOT)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// DbgHelp.h は windows.h より後にインクルードする必要がある
#include <DbgHelp.h>

#include <cstdio>
#include <cstring>

namespace hengband_godot {

namespace {

LONG WINAPI cpp_exception_tracer(EXCEPTION_POINTERS *info)
{
    // MSVC の C++ 例外は ExceptionCode 0xE06D7363 ('msc' | 0xE0000000)
    constexpr DWORD CPP_EXCEPTION_CODE = 0xE06D7363u;
    if (info == nullptr || info->ExceptionRecord == nullptr
        || info->ExceptionRecord->ExceptionCode != CPP_EXCEPTION_CODE) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // 本 DLL (bakabakaband) のモジュールハンドルを取得 (= ベースアドレス)。
    static HMODULE self_module = []() -> HMODULE {
        HMODULE h = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&cpp_exception_tracer), &h);
        return h;
    }();

    void *frames[64];
    const USHORT n = CaptureStackBackTrace(0, 64, frames, nullptr);

    // 自モジュール (bakabakaband) のフレームを含む例外のみ対象とする。
    // Windows のシステム DLL (uiautomationcore 等) がウィンドウ操作中に内部で
    // スロー＆キャッチする良性の C++ 例外まで出力するとノイズになるため除外する。
    bool from_self = false;
    for (USHORT i = 0; i < n; ++i) {
        HMODULE frame_module = nullptr;
        if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(frames[i]), &frame_module)
            && frame_module == self_module) {
            from_self = true;
            break;
        }
    }
    if (!from_self) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // 正常動作中に投げて捕捉される例外もあり得るため出力回数を制限する
    static int dump_count = 0;
    if (dump_count >= 8) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    ++dump_count;

    HANDLE process = GetCurrentProcess();
    static bool sym_inited = false;
    if (!sym_inited) {
        // シンボル検索パスに本 DLL のあるディレクトリ (PDB の置き場所) を含める。
        // これを指定しないと PDB が読まれず、エクスポート関数名までしか解決できない。
        char dll_dir[MAX_PATH] = {};
        if (self_module != nullptr && GetModuleFileNameA(self_module, dll_dir, MAX_PATH) > 0) {
            char *last_sep = std::strrchr(dll_dir, '\\');
            if (last_sep != nullptr) {
                *last_sep = '\0';
            }
        }

        // SYMOPT_DEFERRED_LOADS は外し、PDB を即時ロードさせる。
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        SymInitialize(process, dll_dir[0] != '\0' ? dll_dir : nullptr, TRUE);
        // 動的ロードされた GDExtension DLL のシンボルも確実に取り込む。
        SymRefreshModuleList(process);
        sym_inited = true;
    }

    alignas(SYMBOL_INFO) char sym_buf[sizeof(SYMBOL_INFO) + 256] = {};
    auto *symbol = reinterpret_cast<SYMBOL_INFO *>(sym_buf);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 255;

    std::fprintf(stderr, "=== Hengband C++ exception backtrace (#%d) ===\n", dump_count);
    for (USHORT i = 0; i < n; ++i) {
        const DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);
        DWORD64 disp = 0;
        const char *name = "(no symbol)";
        if (SymFromAddr(process, addr, &disp, symbol)) {
            name = symbol->Name;
        }

        // シンボルが無くても解析できるよう module 名 + RVA(オフセット) を併記する。
        IMAGEHLP_MODULE64 mod = {};
        mod.SizeOfStruct = sizeof(mod);
        const char *mod_name = "?";
        DWORD64 mod_base = 0;
        if (SymGetModuleInfo64(process, addr, &mod)) {
            mod_name = mod.ModuleName;
            mod_base = mod.BaseOfImage;
        }
        const DWORD64 rva = (mod_base != 0) ? (addr - mod_base) : addr;

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD ldisp = 0;
        if (SymGetLineFromAddr64(process, addr, &ldisp, &line)) {
            std::fprintf(stderr, "#%u %s!%s+0x%llx at %s:%lu\n",
                i, mod_name, name, static_cast<unsigned long long>(disp), line.FileName, line.LineNumber);
        } else {
            std::fprintf(stderr, "#%u %s!%s  (%s+0x%llx)\n",
                i, mod_name, name, mod_name, static_cast<unsigned long long>(rva));
        }
    }
    std::fflush(stderr);

    // 通常の例外処理 (catch) を妨げない
    return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void install_cpp_exception_tracer()
{
    // 第 1 引数 1 = 最初に呼ばれるハンドラとして登録
    AddVectoredExceptionHandler(1, cpp_exception_tracer);
}

} // namespace hengband_godot

#else

namespace hengband_godot {
void install_cpp_exception_tracer() {}
} // namespace hengband_godot

#endif
