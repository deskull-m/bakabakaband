/*!
 * @file h-system.h
 * @brief 馬鹿馬鹿蛮怒用システムヘッダーファイル /
 * The most basic "include" file. This file simply includes other low level header files.
 * @date 2014/08/16
 * @author
 * 不明(馬鹿馬鹿蛮怒スタッフ？)
 * @details
 * Include the basic "system" files.
 * Make sure all "system" constants/macros are defined.
 * Make sure all "system" functions have "extern" declarations.
 * This file is a big hack to make other files less of a hack.
 * This file has been rebuilt -- it may need a little more work.
 *
 * It is (very) unlikely that VMS will work without help, primarily
 * because VMS does not use the "ASCII" character set.
 */

#pragma once

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwctype>
#include <fcntl.h>
#include <memory.h>

// clang-format off

#ifdef WINDOWS
  #include <io.h>
#else
  #ifdef SET_UID
    #include <pwd.h>
    #include <sys/file.h>
    #include <sys/param.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
  #endif
#endif

// clang-format on
