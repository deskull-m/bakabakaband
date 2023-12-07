﻿#pragma once
/*!
 * @file info-initializer.h
 * @brief 馬鹿馬鹿蛮怒のゲームデータ解析処理ヘッダ
 */

#include "system/angband.h"

class PlayerType;
errr init_artifacts_info();
errr init_dungeons_info();
errr init_misc(PlayerType *player_ptr);
errr init_f_info();
errr init_k_info();
errr init_e_info();
errr init_r_info();
errr init_v_info();
errr init_s_info();
errr init_m_info();
