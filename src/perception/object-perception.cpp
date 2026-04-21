#include "perception/object-perception.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

/*!
 * @brief オブジェクトを＊鑑定＊済にする
 * @param creature クリーチャーへの参照
 * @param item ＊鑑定＊済にするアイテムへの参照
 */
void object_aware(CreatureEntity &creature, const ItemEntity &item)
{
    const bool is_already_awared = item.is_aware();
    auto &baseitem = item.get_baseitem();
    baseitem.mark_awareness(true);

    // 以下、playrecordに記録しない場合はreturnする
    if (!record_ident) {
        return;
    }

    if (is_already_awared || creature.is_dead()) {
        return;
    }

    // アーティファクト専用ベースアイテムは記録しない
    if (baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return;
    }

    if (!item.has_unidentified_name()) {
        return;
    }

    // playrecordに識別したアイテムを記録
    const auto item_name = describe_flavor(creature, item, OD_NAME_ONLY | OD_OMIT_PREFIX);
    exe_write_diary(*creature.get_floor(), DiaryKind::FOUND, 0, item_name);
}
