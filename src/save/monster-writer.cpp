#include "save/monster-writer.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "save/item-writer.h"
#include "save/save-util.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-list.h"
#include "util/enum-converter.h"

MonsterWriter::MonsterWriter(const CreatureEntity &monster)
    : monster(monster)
{
}

/*!
 * @brief モンスター情報をセーブデータに書き込む
 */
void MonsterWriter::write_to_savedata() const
{
    const auto flags = this->write_monster_flags();

    wr_s16b(enum2i(this->monster.get_r_idx()));
    wr_s32b(enum2i(this->monster.get_alliance_idx()));

    wr_byte((byte)this->monster.y);
    wr_byte((byte)this->monster.x);
    wr_s32b(this->monster.hp);
    wr_s32b(this->monster.maxhp);
    wr_s32b(this->monster.max_maxhp);
    wr_u32b(this->monster.get_dealt_damage());

    if (any_bits(flags, SaveDataMonsterFlagType::AP_R_IDX)) {
        wr_s16b(enum2i(this->monster.get_ap_r_idx()));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN)) {
        wr_byte(this->monster.get_sub_align());
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SLEEP)) {
        wr_s16b(this->monster.get_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS));
    }

    wr_byte((byte)this->monster.speed);
    wr_s16b(this->monster.energy_need);
    wr_s16b(this->monster.ac);
    this->write_monster_info(flags);
}

uint32_t MonsterWriter::write_monster_flags() const
{
    uint32_t flags = 0x00000000;
    if (!this->monster.is_original_ap()) {
        set_bits(flags, SaveDataMonsterFlagType::AP_R_IDX);
    }

    if (this->monster.get_sub_align()) {
        set_bits(flags, SaveDataMonsterFlagType::SUB_ALIGN);
    }

    if (this->monster.is_asleep()) {
        set_bits(flags, SaveDataMonsterFlagType::SLEEP);
    }

    if (this->monster.is_accelerated()) {
        set_bits(flags, SaveDataMonsterFlagType::FAST);
    }

    if (this->monster.is_decelerated()) {
        set_bits(flags, SaveDataMonsterFlagType::SLOW);
    }

    if (this->monster.is_stunned()) {
        set_bits(flags, SaveDataMonsterFlagType::STUNNED);
    }

    if (this->monster.is_confused()) {
        set_bits(flags, SaveDataMonsterFlagType::CONFUSED);
    }

    if (this->monster.is_fearful()) {
        set_bits(flags, SaveDataMonsterFlagType::MONFEAR);
    }

    if (this->monster.target.y) {
        set_bits(flags, SaveDataMonsterFlagType::TARGET_Y);
    }

    if (this->monster.target.x) {
        set_bits(flags, SaveDataMonsterFlagType::TARGET_X);
    }

    if (this->monster.is_invulnerable()) {
        set_bits(flags, SaveDataMonsterFlagType::INVULNER);
    }

    if (this->monster.get_smart_flags().any()) {
        set_bits(flags, SaveDataMonsterFlagType::SMART);
    }

    if (this->monster.get_exp()) {
        set_bits(flags, SaveDataMonsterFlagType::EXP);
    }

    if (this->monster.get_monster_profile().mflag2.any()) {
        set_bits(flags, SaveDataMonsterFlagType::MFLAG2);
    }

    if (this->monster.is_named()) {
        set_bits(flags, SaveDataMonsterFlagType::NICKNAME);
    }

    if (this->monster.has_parent()) {
        set_bits(flags, SaveDataMonsterFlagType::PARENT);
    }

    if (this->monster.get_au()) {
        set_bits(flags, SaveDataMonsterFlagType::GOLD);
    }

    if (this->monster.get_ht() || this->monster.get_wt()) {
        set_bits(flags, SaveDataMonsterFlagType::HEIGHT_WEIGHT);
    }

    if (this->monster.prace != PlayerRaceType::NONE) {
        set_bits(flags, SaveDataMonsterFlagType::RACE);
    }

    if (this->monster.pclass != PlayerClassType::NONE) {
        set_bits(flags, SaveDataMonsterFlagType::CLASS);
    }

    if (MonraceList::is_valid(this->monster.get_transform_r_idx()) || this->monster.has_transformed()) {
        set_bits(flags, SaveDataMonsterFlagType::TRANSFORM);
    }

    // インベントリがあるかチェック
    bool has_inventory = false;
    for (const auto &item : this->monster.inventory) {
        if (item && item->is_valid()) {
            has_inventory = true;
            break;
        }
    }
    if (has_inventory) {
        set_bits(flags, SaveDataMonsterFlagType::INVENTORY);
    }

    // [Phase 2] 拡張装備スロットの有効アイテムがあるかチェック
    bool has_extended = false;
    for (const auto &item : this->monster.extended_inventory) {
        if (item && item->is_valid()) {
            has_extended = true;
            break;
        }
    }
    if (has_extended) {
        set_bits(flags, SaveDataMonsterFlagType::EXTENDED_INVENTORY);
    }

    if (!this->monster.get_materials().empty()) {
        set_bits(flags, SaveDataMonsterFlagType::MATERIALS);
    }

    wr_u32b(flags);
    return flags;
}

void MonsterWriter::write_monster_info(uint32_t flags) const
{
    byte tmp8u;
    if (any_bits(flags, SaveDataMonsterFlagType::FAST)) {
        tmp8u = (byte)this->monster.get_timed_effect(CreatureTimedEffect::ACCELERATION);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SLOW)) {
        tmp8u = (byte)this->monster.get_timed_effect(CreatureTimedEffect::DECELERATION);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::STUNNED)) {
        tmp8u = (byte)this->monster.get_timed_effect(CreatureTimedEffect::STUN);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CONFUSED)) {
        tmp8u = (byte)this->monster.get_timed_effect(CreatureTimedEffect::CONFUSION);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MONFEAR)) {
        tmp8u = (byte)this->monster.get_timed_effect(CreatureTimedEffect::FEAR);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_Y)) {
        wr_s16b((int16_t)this->monster.target.y);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TARGET_X)) {
        wr_s16b((int16_t)this->monster.target.x);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::INVULNER)) {
        tmp8u = (byte)this->monster.get_timed_effect(CreatureTimedEffect::INVULNERABILITY);
        wr_byte(tmp8u);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::SMART)) {
        wr_FlagGroup(this->monster.get_smart_flags(), wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::EXP)) {
        wr_u32b(this->monster.get_exp());
    }

    if (any_bits(flags, SaveDataMonsterFlagType::MFLAG2)) {
        wr_FlagGroup(this->monster.get_monster_profile().mflag2, wr_byte);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::NICKNAME)) {
        wr_string(this->monster.name);
    }

    if (any_bits(flags, SaveDataMonsterFlagType::PARENT)) {
        wr_s16b(this->monster.get_parent_m_idx());
    }

    if (any_bits(flags, SaveDataMonsterFlagType::GOLD)) {
        wr_s32b(this->monster.get_au());
    }

    if (any_bits(flags, SaveDataMonsterFlagType::HEIGHT_WEIGHT)) {
        wr_s16b(this->monster.get_ht());
        wr_s16b(this->monster.get_wt());
    }

    if (any_bits(flags, SaveDataMonsterFlagType::RACE)) {
        wr_byte(static_cast<byte>(enum2i(this->monster.prace)));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::CLASS)) {
        wr_s16b(enum2i(this->monster.pclass));
    }

    if (any_bits(flags, SaveDataMonsterFlagType::TRANSFORM)) {
        wr_s16b(static_cast<int16_t>(enum2i(this->monster.get_transform_r_idx())));
        wr_byte(static_cast<byte>(this->monster.get_transform_hp_threshold()));
        wr_byte(this->monster.has_transformed() ? 1 : 0);
    }

    // インベントリの保存（PlayerTypeと同じ形式）
    if (any_bits(flags, SaveDataMonsterFlagType::INVENTORY)) {
        for (size_t i = 0; i < this->monster.inventory.size(); i++) {
            const auto &item = this->monster.inventory[i];
            if (!item || !item->is_valid()) {
                continue;
            }

            wr_u16b(static_cast<uint16_t>(i));
            wr_item(*item);
        }
        wr_u16b(0xFFFF); // 終端マーカー
    }

    // [Phase 2] 拡張装備スロットの保存。各エントリは
    //   u16b インデックス (0xFFFF で終端) + wr_item
    // の繰り返し。インベントリと同じ形式。
    if (any_bits(flags, SaveDataMonsterFlagType::EXTENDED_INVENTORY)) {
        for (size_t i = 0; i < this->monster.extended_inventory.size(); i++) {
            const auto &item = this->monster.extended_inventory[i];
            if (!item || !item->is_valid()) {
                continue;
            }
            wr_u16b(static_cast<uint16_t>(i));
            wr_item(*item);
        }
        wr_u16b(0xFFFF); // 終端マーカー
    }

    // 材質 (副種族) の保存。個数 u16b に続けて各材質 ID を s16b で書き出す。
    if (any_bits(flags, SaveDataMonsterFlagType::MATERIALS)) {
        const auto &materials = this->monster.get_materials();
        wr_u16b(static_cast<uint16_t>(materials.size()));
        for (const auto material : materials) {
            wr_s16b(static_cast<int16_t>(enum2i(material)));
        }
    }
}
