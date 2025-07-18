#pragma once

enum class MonsterSpeakType {
    SPEAK_ALL = 0, /* SPEAK_BATTLE, SPEAK_FEAR, SPEAK_FRIEND, SPEAK_DEATH */
    SPEAK_BATTLE = 1,
    SPEAK_FEAR = 2,
    SPEAK_FRIEND = 3,
    SPEAK_DEATH = 4,
    SPEAK_SPAWN = 5,
    MAX
};

enum class MonsterMessageType {
    SPEAK_ALL = 0, /* SPEAK_BATTLE, SPEAK_FEAR, SPEAK_FRIEND, SPEAK_DEATH */
    SPEAK_BATTLE = 1,
    SPEAK_FEAR = 2,
    SPEAK_FRIEND = 3,
    SPEAK_DEATH = 4,
    SPEAK_SPAWN = 5,
    WALK_CLOSERANGE = 6,
    WALK_MIDDLERANGE = 7,
    WALK_LONGRANGE = 8,
    MESSAGE_STALKER = 9,
    MESSAGE_REFLECT = 10,
    MESSAGE_TIMESTOP = 11,
    MESSAGE_TIMESTART = 12,
    MESSAGE_BREATH_SOUND = 13,
    MESSAGE_BREATH_SHARDS = 14,
    MESSAGE_BREATH_FORCE = 15,
    MESSAGE_DETECT_UNIQUE = 16,
    MAX
};
