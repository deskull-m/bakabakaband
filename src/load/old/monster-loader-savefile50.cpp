#include "load/old/monster-loader-savefile50.h"
#include "load/item/item-loader-base.h"
#include "load/item/item-loader-factory.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/race-info-table.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief モンスターを読み込む(v3.0.0 Savefile ver50まで)
 */
void MonsterLoader50::rd_monster(CreatureEntity &monster)
{
    auto flags = rd_u32b();
    monster.set_r_idx(i2enum<MonraceId>(rd_s16b()));

    if (loading_savefile_version_is_older_than(16)) {
        MonraceDefinition *r_ptr = &MonraceList::get_instance().get_monrace(monster.r_idx);
        monster.set_alliance_idx(r_ptr->alliance_idx);
    } else {
        monster.set_alliance_idx(i2enum<AllianceType>(rd_s32b()));
    }

    monster.y = rd_byte();
    monster.x = rd_byte();

    if (loading_savefile_version_is_older_than(46)) {
        monster.hp = rd_s16b();
        monster.maxhp = rd_s16b();
        monster.max_maxhp = rd_s16b();
    } else {
        monster.hp = rd_s32b();
        monster.maxhp = rd_s32b();
        monster.max_maxhp = rd_s32b();
    }

    monster.dealt_damage = rd_s32b();

    monster.set_ap_r_idx(any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX) ? i2enum<MonraceId>(rd_s16b()) : monster.r_idx);
    monster.set_sub_align(any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN) ? rd_byte() : 0);
    monster.set_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS, any_bits(flags, SaveDataMonsterFlagType::SLEEP) ? rd_s16b() : 0);
    monster.speed = rd_byte();
    monster.energy_need = rd_s16b();
    if (loading_savefile_version_is_older_than(38)) {
        MonraceDefinition *r_ptr = &MonraceList::get_instance().get_monrace(monster.r_idx);
        monster.ac = r_ptr->ac;
    } else {
        monster.ac = rd_s16b();
    }
    monster.set_timed_effect(CreatureTimedEffect::ACCELERATION, any_bits(flags, SaveDataMonsterFlagType::FAST) ? rd_byte() : 0);
    monster.set_timed_effect(CreatureTimedEffect::DECELERATION, any_bits(flags, SaveDataMonsterFlagType::SLOW) ? rd_byte() : 0);
    monster.set_timed_effect(CreatureTimedEffect::STUN, any_bits(flags, SaveDataMonsterFlagType::STUNNED) ? rd_byte() : 0);
    monster.set_timed_effect(CreatureTimedEffect::CONFUSION, any_bits(flags, SaveDataMonsterFlagType::CONFUSED) ? rd_byte() : 0);
    monster.set_timed_effect(CreatureTimedEffect::FEAR, any_bits(flags, SaveDataMonsterFlagType::MONFEAR) ? rd_byte() : 0);
    monster.target.y = any_bits(flags, SaveDataMonsterFlagType::TARGET_Y) ? rd_s16b() : 0;
    monster.target.x = any_bits(flags, SaveDataMonsterFlagType::TARGET_X) ? rd_s16b() : 0;
    monster.set_timed_effect(CreatureTimedEffect::INVULNERABILITY, any_bits(flags, SaveDataMonsterFlagType::INVULNER) ? rd_byte() : 0);
    monster.clear_temporary_flags();
    monster.clear_constant_flags();
    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(monster.get_monster_profile().smart, tmp32u);

            // 3.0.0Alpha10以前のSM_CLONED(ビット位置22)、SM_PET(23)、SM_FRIEDLY(28)をMFLAG2に移行する
            // ビット位置の定義はなくなるので、ビット位置の値をハードコードする。
            std::bitset<32> rd_bits(tmp32u);
            monster.assign_constant_flag(MonsterConstantFlagType::CLONED, rd_bits[22]);
            monster.assign_constant_flag(MonsterConstantFlagType::PET, rd_bits[23]);
            monster.assign_constant_flag(MonsterConstantFlagType::FRIENDLY, rd_bits[28]);
            monster.get_monster_profile().smart.reset(i2enum<MonsterSmartLearnType>(22)).reset(i2enum<MonsterSmartLearnType>(23)).reset(i2enum<MonsterSmartLearnType>(28));
        } else {
            rd_FlagGroup(monster.get_monster_profile().smart, rd_byte);
        }
    } else {
        monster.clear_smart_flags();
    }

    monster.exp = any_bits(flags, SaveDataMonsterFlagType::EXP) ? rd_u32b() : 0;
    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        if (loading_savefile_version_is_older_than(2)) {
            auto tmp8u = rd_byte();
            constexpr auto base = enum2i(MonsterConstantFlagType::KAGE);
            migrate_bitflag_to_flaggroup(monster.get_monster_profile().mflag2, tmp8u, base, 7);
        } else {
            rd_FlagGroup(monster.get_monster_profile().mflag2, rd_byte);
        }
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        monster.name = rd_string();
    } else {
        monster.name.clear();
    }

    monster.set_parent_m_idx(any_bits(flags, SaveDataMonsterFlagType::PARENT) ? rd_s16b() : 0);

    // バージョン40以降: 所持金、身長、体重の読み込み
    if (loading_savefile_version_is_older_than(40)) {
        monster.au = 0;
        monster.set_ht(0);
        monster.set_wt(0);
    } else {
        monster.au = any_bits(flags, SaveDataMonsterFlagType::GOLD) ? rd_s32b() : 0;
        if (any_bits(flags, SaveDataMonsterFlagType::HEIGHT_WEIGHT)) {
            monster.set_ht(rd_s16b());
            monster.set_wt(rd_s16b());
        } else {
            monster.set_ht(0);
            monster.set_wt(0);
        }
    }

    // バージョン41以降: 種族・職業の読み込み
    if (loading_savefile_version_is_older_than(41)) {
        monster.prace = PlayerRaceType::NONE;
        monster.pclass = PlayerClassType::NONE;
        monster.race = nullptr;
        monster.pclass_ref = nullptr;
    } else {
        // 種族の読み込み
        if (any_bits(flags, SaveDataMonsterFlagType::RACE)) {
            monster.prace = i2enum<PlayerRaceType>(rd_byte());
            // ポインタを復元
            if (monster.prace != PlayerRaceType::NONE && enum2i(monster.prace) < MAX_RACES) {
                monster.race = &race_info[enum2i(monster.prace)];
            } else {
                monster.race = nullptr;
            }
        } else {
            monster.prace = PlayerRaceType::NONE;
            monster.race = nullptr;
        }

        // 職業の読み込み
        if (any_bits(flags, SaveDataMonsterFlagType::CLASS)) {
            monster.pclass = i2enum<PlayerClassType>(rd_s16b());
            // ポインタを復元
            if (monster.pclass != PlayerClassType::NONE) {
                monster.pclass_ref = &class_info.at(monster.pclass);
            } else {
                monster.pclass_ref = nullptr;
            }
        } else {
            monster.pclass = PlayerClassType::NONE;
            monster.pclass_ref = nullptr;
        }
    }

    // バージョン43以降: 変身情報の読み込み
    if (loading_savefile_version_is_older_than(43)) {
        monster.set_transform_r_idx(MonraceId::PLAYER);
        monster.set_transform_hp_threshold(0);
        monster.set_has_transformed(false);
    } else {
        if (any_bits(flags, SaveDataMonsterFlagType::TRANSFORM)) {
            monster.set_transform_r_idx(i2enum<MonraceId>(rd_s16b()));
            monster.set_transform_hp_threshold(rd_byte());
            monster.set_has_transformed(rd_byte() != 0);
        } else {
            monster.set_transform_r_idx(MonraceId::PLAYER);
            monster.set_transform_hp_threshold(0);
            monster.set_has_transformed(false);
        }
    }

    // バージョン45以降: インベントリの読み込み
    if (loading_savefile_version_is_older_than(45)) {
        // 古いバージョンではインベントリは空
        for (auto &item : monster.inventory) {
            if (item) {
                item->wipe();
            }
        }
        monster.set_inven_cnt(0);
        monster.set_equip_cnt(0);
    } else {
        if (any_bits(flags, SaveDataMonsterFlagType::INVENTORY)) {
            // PlayerTypeと同じ形式で読み込み
            auto item_loader = ItemLoaderFactory::create_loader();
            monster.set_inven_cnt(0);
            monster.set_equip_cnt(0);

            while (true) {
                auto n = rd_u16b();
                if (n == 0xFFFF) {
                    break;
                }

                if (n >= monster.inventory.size()) {
                    continue; // 範囲外は無視
                }

                if (!monster.inventory[n]) {
                    monster.inventory[n] = std::make_shared<ItemEntity>();
                }

                item_loader->rd_item(monster.inventory[n].get());

                if (monster.inventory[n]->is_valid()) {
                    monster.increment_inven_cnt();
                }
            }
        } else {
            // フラグが立っていない場合は空
            for (auto &item : monster.inventory) {
                if (item) {
                    item->wipe();
                }
            }
            monster.set_inven_cnt(0);
            monster.set_equip_cnt(0);
        }
    }
}
