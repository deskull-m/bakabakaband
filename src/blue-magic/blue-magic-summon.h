#pragma once
/*!
 * @file blue-magic-summon.h
 * @brief 青魔法の召喚系スペルヘッダ
 */

struct bmc_type;
class CreatureEntity;
bool cast_blue_summon_kin(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_cyber(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_monster(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_monsters(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_ant(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_spider(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_hound(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_hydra(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_fairy(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_ape(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_bird(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_angel(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_demon(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_undead(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_dragon(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_high_undead(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_high_dragon(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_amberite(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_choasian(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_unique(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_dead_unique(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_nasty(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_golem(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_cats(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_perverts(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_puyo(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_homo(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_wall(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_insect(CreatureEntity &creature, bmc_type *bmc_ptr);
bool cast_blue_summon_eldrazi(CreatureEntity &creature, bmc_type *bmc_ptr);
