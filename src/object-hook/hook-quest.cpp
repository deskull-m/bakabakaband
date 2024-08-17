#include "object-hook/hook-quest.h"
#include "artifact/fixed-art-types.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/trg-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "world/world.h"

/*!
 * @brief オブジェクトが賞金首の報酬対象になるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが報酬対象になるならTRUEを返す
 */
bool object_is_bounty(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (o_ptr->bi_key.tval() != ItemKindType::CORPSE) {
        return false;
    }

    if (vanilla_town) {
        return false;
    }

    const auto corpse_monrace_id = i2enum<MonsterRaceId>(o_ptr->pval);
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(corpse_monrace_id);
    if (player_ptr->knows_daily_bounty && (monrace.name == monraces_info[w_ptr->today_mon].name)) {
        return true;
    }

    if (corpse_monrace_id == MonsterRaceId::TSUCHINOKO) {
        return true;
    }

    return monrace.is_bounty(true);
}

/*!
 * @brief オブジェクトがクエストの達成目的か否かを返す。
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 */
bool object_is_quest_target(QuestId quest_idx, const ItemEntity *o_ptr)
{
    if (!inside_quest(quest_idx)) {
        return false;
    }

    const auto &quest = QuestList::get_instance().get_quest(quest_idx);
    if (!quest.has_reward()) {
        return false;
    }

    const auto &artifact = quest.get_reward();
    if (artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return o_ptr->bi_key == artifact.bi_key;
}
