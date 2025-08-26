#pragma once

enum class MonsterTemporaryFlagType {
    VIEW = 0, /* Monster is in line of sight */
    LOS = 1, /* Monster is marked for project_all_los(player_ptr, ) */
    ESP = 2, /* Monster is being sensed by ESP */
    PRESENT_AT_TURN_START = 3, /* Monster on level at start of player's or monster's turn */
    PREVENT_MAGIC = 4, /* Monster is still being no-magic */
    SANITY_BLAST = 5, /* Monster gives sanity blast effects to player */
    MAX,
};

enum class MonsterConstantFlagType {
    KAGE = 0, /* Monster is kage */
    NOPET = 1, /* Cannot make monster pet */
    NOGENO = 2, /* Cannot genocide */
    CHAMELEON = 3, /* Monster is chameleon */
    NOFLOW = 4, /* Monster is in no_flow_by_smell mode */
    SHOW = 5, /* Monster is recently memorized */
    MARK = 6, /* Monster is currently memorized */
    FRIENDLY = 7, /*!< 友好的である / Friendly */
    PET = 8, /*!< ペットである / Pet */
    CLONED = 9, /*!< クローンである / Cloned */
    SANTA = 10, /*!< サンタと化した / Santa turned */
    LARGE = 11, /*!< 大型 / Large */
    RIDING = 12, /*!< 乗馬中である / riding */
    DEFECATED = 13, /*!< 脱糞した / Defecated */
    VOMITED = 14, /*!< 嘔吐した / Vomited */
    ANGER = 15, /*!< 怒れる / Anger */
    QUYLTHLUG_BORN = 16, /*!< クイルスルグの落とし仔 / Quylthlug born */
    WAIFUIZED = 17, /*!< 美少女化した / waifuized */
    FAT = 18, /*!< 肥満した / Fat */
    MAX,
};
