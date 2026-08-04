#pragma once

class CreatureEntity;
enum class StoreSaleType;

enum class BotKnowledgeCategory {
    ARTIFACTS_KNOWN,
    ARTIFACTS_IDENTIFIED,
    OBJECTS_KNOWN,
    UNIQUES_ALIVE,
    UNIQUES_DEAD,
    BOUNTY,
    HOME,
    EQUIP_RESISTANCES,
    FEATURES,
    SELF_INFO,
    MUTATIONS,
    WEAPON_EXP,
    SPELL_EXP,
    SKILL_EXP,
    VIRTUES,
    DUNGEONS,
    QUESTS,
    PETS,
    AUTOPICK,
    MAX,
};

void output_bot_json_snapshot(CreatureEntity &creature);
void output_bot_json_store_snapshot(CreatureEntity &creature, StoreSaleType store_num);
void output_bot_json_character_snapshot(CreatureEntity &creature);
void output_bot_json_knowledge_snapshot(CreatureEntity &creature, BotKnowledgeCategory category);
void output_bot_json_look_snapshot(CreatureEntity &creature);
