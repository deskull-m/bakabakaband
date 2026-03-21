#pragma once
/*!
 * @file angband-initializer.h
 * @brief 馬鹿馬鹿蛮怒のシステム初期化処理ヘッダファイル
 * @date 2015/01/02
 * @author
 * Copyright (c) 2000 Robert Ruehlmann
 * @details
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#include <filesystem>

class CreatureEntity;
void init_angband(CreatureEntity &creature, bool no_term);
void init_file_paths(const std::filesystem::path &libpath);
