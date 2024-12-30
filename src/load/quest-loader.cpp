#include "load/quest-loader.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "load/savedata-old-flag-types.h"
#include "object-enchant/trg-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

errr load_town(void)
{
    auto max_towns_load = rd_u16b();
    if (max_towns_load <= towns_info.size()) {
        return 0;
    }

    load_note(format(_("町が多すぎる(%u)！", "Too many (%u) towns!"), max_towns_load));
    return 23;
}

std::tuple<uint16_t, byte> load_quest_info()
{
    auto max_quests_load = rd_u16b();
    byte max_rquests_load;
    max_rquests_load = rd_byte();

    return std::make_tuple(max_quests_load, max_rquests_load);
}

static void load_quest_completion(QuestType *q_ptr)
{
    q_ptr->status = i2enum<QuestStatusType>(rd_s16b());
    q_ptr->level = rd_s16b();
    q_ptr->complev = rd_byte();
    q_ptr->comptime = rd_u32b();
}

static void load_quest_details(PlayerType *player_ptr, QuestType *q_ptr, const QuestId loading_quest_id)
{
    q_ptr->cur_num = rd_s16b();
    q_ptr->max_num = rd_s16b();
    q_ptr->type = i2enum<QuestKindType>(rd_s16b());

    q_ptr->r_idx = i2enum<MonraceId>(rd_s16b());
    if ((q_ptr->type == QuestKindType::RANDOM) && !q_ptr->get_bounty().is_valid()) {
        auto &quests = QuestList::get_instance();
        determine_random_questor(player_ptr, quests.get_quest(loading_quest_id));
    }
    q_ptr->reward_fa_id = i2enum<FixedArtifactId>(rd_s16b());
    if (q_ptr->has_reward()) {
        q_ptr->get_reward().gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    }

    q_ptr->flags = rd_byte();
}

static bool is_loadable_quest(const QuestId q_idx, const byte max_rquests_load)
{
    const auto &quests = QuestList::get_instance();
    if (quests.find(q_idx) != quests.end()) {
        return true;
    }

    auto status = i2enum<QuestStatusType>(rd_s16b());

    strip_bytes(7);

    auto is_quest_running = (status == QuestStatusType::TAKEN);
    is_quest_running |= (status == QuestStatusType::COMPLETED);
    is_quest_running |= ((enum2i(q_idx) >= MIN_RANDOM_QUEST) && (enum2i(q_idx) <= (MIN_RANDOM_QUEST + max_rquests_load)));
    if (!is_quest_running) {
        return false;
    }

    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(1);
    return false;
}

void analyze_quests(PlayerType *player_ptr, const uint16_t max_quests_load, const byte max_rquests_load)
{
    for (auto i = 0; i < max_quests_load; i++) {
        QuestId quest_id;
        if (loading_savefile_version_is_older_than(17)) {
            quest_id = i2enum<QuestId>(i);
        } else {
            quest_id = i2enum<QuestId>(rd_s16b());
        }
        if (!is_loadable_quest(quest_id, max_rquests_load)) {
            continue;
        }

        auto &quests = QuestList::get_instance();
        auto &quest = quests.get_quest(quest_id);

        load_quest_completion(&quest);
        auto is_quest_running = (quest.status == QuestStatusType::TAKEN);
        is_quest_running |= (quest.status == QuestStatusType::COMPLETED);
        is_quest_running |= (enum2i(quest_id) >= MIN_RANDOM_QUEST) && (enum2i(quest_id) <= (MIN_RANDOM_QUEST + max_rquests_load));
        if (!is_quest_running) {
            continue;
        }

        load_quest_details(player_ptr, &quest, quest_id);
        quest.dungeon = rd_byte();

        if (quest.status == QuestStatusType::TAKEN || quest.status == QuestStatusType::UNTAKEN) {
            auto &monrace = quest.get_bounty();
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                monrace.misc_flags.set(MonsterMiscType::QUESTOR);
            }
        }
    }
}
