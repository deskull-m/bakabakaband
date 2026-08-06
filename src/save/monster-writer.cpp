#include "save/monster-writer.h"
#include "save/creature-common-writer.h"
#include "save/item-writer.h"
#include "save/save-util.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/enum-converter.h"

MonsterWriter::MonsterWriter(const CreatureEntity &monster)
    : monster(monster)
{
}

/*!
 * @brief モンスター情報をセーブデータに書き込む (セーブデータバージョン 50 以降の統合フォーマット)
 * @details
 * 共通基底 (CreatureEntity) フィールドは wr_creature_common() に委譲し、
 * モンスター固有フィールドのみをここで書き出す。読み込みは
 * MonsterLoader50::rd_monster() の v50 経路と完全対称であること。
 * 旧バージョン (<50) の読み込みは MonsterLoader50 のレガシー経路が担う。
 */
void MonsterWriter::write_to_savedata() const
{
    // --- 共通基底 (CreatureEntity) フィールド ---
    wr_creature_common(this->monster);

    // --- モンスター固有フィールド ---
    wr_s16b(enum2i(this->monster.get_r_idx()));
    wr_s16b(enum2i(this->monster.get_ap_r_idx()));
    wr_s32b(enum2i(this->monster.get_alliance_idx()));
    wr_byte(this->monster.get_sub_align());

    wr_FlagGroup(this->monster.get_smart_flags(), wr_byte);
    wr_FlagGroup(this->monster.get_monster_profile().mflag2, wr_byte);

    wr_s16b(this->monster.get_parent_m_idx());

    wr_s16b(static_cast<int16_t>(enum2i(this->monster.get_transform_r_idx())));
    wr_byte(static_cast<byte>(this->monster.get_transform_hp_threshold()));
    wr_byte(this->monster.has_transformed() ? 1 : 0);

    // prace / pclass は wr_creature_common() (v54 拡張) に集約済み

    // 通常インベントリ (u16b スロット番号 + アイテム、0xFFFF 終端)
    for (size_t i = 0; i < this->monster.inventory.size(); i++) {
        const auto &item = this->monster.inventory[i];
        if (!item || !item->is_valid()) {
            continue;
        }
        wr_u16b(static_cast<uint16_t>(i));
        wr_item(*item);
    }
    wr_u16b(0xFFFF);

    // 拡張装備スロット (同形式)
    for (size_t i = 0; i < this->monster.get_extended_inventory_size(); i++) {
        const auto &item = this->monster.get_extended_item(i);
        if (!item || !item->is_valid()) {
            continue;
        }
        wr_u16b(static_cast<uint16_t>(i));
        wr_item(*item);
    }
    wr_u16b(0xFFFF);
}
