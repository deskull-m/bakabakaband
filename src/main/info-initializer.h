#pragma once
/*!
 * @file info-initializer.h
 * @brief 馬鹿馬鹿蛮怒のゲームデータ解析処理ヘッダ
 */

#include "system/angband.h"

class PlayerType;
errr init_artifacts_info();
errr init_baseitems();
errr init_class_magics_info();
errr init_class_skills_info();
errr init_dungeons_info();
errr init_egos_info();
errr init_monrace_definitions();
errr init_terrains_info();
errr init_vaults_info();
bool init_wilderness();
