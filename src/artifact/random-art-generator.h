#pragma once
/*!
 * @file random-art-generator.h
 * @brief ランダムアーティファクトの生成メインヘッダ / Artifact code
 */

class ItemEntity;
class CreatureEntity;
bool become_random_artifact(CreatureEntity &creature, ItemEntity *o_ptr, bool a_scroll);
