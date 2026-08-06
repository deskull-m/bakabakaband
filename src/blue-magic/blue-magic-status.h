#pragma once
/*!
 * @file blue-magic-status.h
 * @brief 青魔法の状態異常系スペルヘッダ
 */

struct bmc_type;
class CreatureEntity;
bool cast_blue_scare(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_blind(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_confusion(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_slow(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_sleep(CreatureEntity &creature, bmc_type *bmc_ptr);
