﻿/*!
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

#ifndef INCLUDED_H_SYSTEM_H
#define INCLUDED_H_SYSTEM_H

#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <errno.h>
#include <stddef.h>
# include <stdlib.h>

#ifdef SET_UID

# include <sys/types.h>

# if defined(Pyramid) || defined(NCR3K) || defined(ibm032) || \
     defined(__osf__) || defined(ISC) || defined(linux)
#  include <sys/time.h>
# endif

#  include <sys/timeb.h>

#endif /* SET_UID */

#include <time.h>

#if defined(WINDOWS)
# include <io.h>
#endif

#if !defined(VM)
# if defined(__TURBOC__) || defined(__WATCOMC__)
#  include <mem.h>
# else
#  include <memory.h>
# endif
#endif

#include <fcntl.h>

#ifdef SET_UID

#include <sys/param.h>
#include <sys/file.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

#endif /* SET_UID */

#include <string.h>

#include <stdarg.h>

#endif /* INCLUDED_H_SYSTEM_H */
