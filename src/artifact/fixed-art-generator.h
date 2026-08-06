#pragma once
/*!
 * @file fixed-art-generator.h
 * @brief 固定アーティファクトの生成処理ヘッダ
 */

#include "system/angband.h"

enum class FixedArtifactId : short;
class ItemEntity;
class CreatureEntity;
bool create_named_art(CreatureEntity &creature, FixedArtifactId a_idx, POSITION y, POSITION x);
void apply_artifact(CreatureEntity &creature, ItemEntity *o_ptr);
