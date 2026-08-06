#pragma once
/*!
 * @file blue-magic-breath.h
 * @brief 青魔法のブレス系呪文ヘッダ
 */

struct bmc_type;
class CreatureEntity;
bool cast_blue_magic_breath(CreatureEntity &creature, bmc_type *bmc_ptr);
