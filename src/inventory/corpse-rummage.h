/*!
 * @file corpse-rummage.h
 * @brief フロアに遺された死体を漁ってアイテムを得る処理
 */

#pragma once

class CreatureEntity;

bool rummage_corpses_here(CreatureEntity &creature, bool pickup);
