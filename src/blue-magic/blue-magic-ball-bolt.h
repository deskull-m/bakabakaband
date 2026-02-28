#pragma once
/*!
 * @file blue-magic-ball-bolt.h
 * @brief 青魔法のボール/ボルト系呪文ヘッダ
 */

struct bmc_type;
class CreatureEntity;
bool cast_blue_magic_ball(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_magic_bolt(CreatureEntity &creature, bmc_type *bmc_ptr);
