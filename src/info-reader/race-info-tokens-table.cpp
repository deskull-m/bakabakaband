#include "info-reader/race-info-tokens-table.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-population-flags.h"
#include "monster-race/race-sex-const.h"
#include "monster-race/race-speak-flags.h"
#include "monster-race/race-special-flags.h"
#include "monster-race/race-visual-flags.h"
#include "monster-race/race-wilderness-flags.h"
#include "system/material-type-definition.h"

/*!
 * モンスター特性トークンの定義9 /
 * Monster race flags
 */
const std::unordered_map<std::string_view, MonsterFeedType> r_info_meat_feed = {
    { "EAT_BLIND", MonsterFeedType::BLIND },
    { "EAT_CONF", MonsterFeedType::CONF },
    { "EAT_MANA", MonsterFeedType::MANA },
    { "EAT_NEXUS", MonsterFeedType::NEXUS },
    // { "EAT_BLINK", MonsterFeedType::BLINK }, //<! @note フラグ未定義
    { "EAT_SLEEP", MonsterFeedType::SLEEP },
    { "EAT_BERSERKER", MonsterFeedType::BERSERKER },
    { "EAT_ACIDIC", MonsterFeedType::ACIDIC },
    { "EAT_SPEED", MonsterFeedType::SPEED },
    { "EAT_CURE", MonsterFeedType::CURE },
    { "EAT_FIRE_RES", MonsterFeedType::FIRE_RES },
    { "EAT_COLD_RES", MonsterFeedType::COLD_RES },
    { "EAT_ACID_RES", MonsterFeedType::ACID_RES },
    { "EAT_ELEC_RES", MonsterFeedType::ELEC_RES },
    { "EAT_POIS_RES", MonsterFeedType::POIS_RES },
    { "EAT_INSANITY", MonsterFeedType::INSANITY },
    { "EAT_DRAIN_EXP", MonsterFeedType::DRAIN_EXP },
    { "EAT_POISONOUS", MonsterFeedType::POISONOUS },
    { "EAT_GIVE_STR", MonsterFeedType::GIVE_STR },
    { "EAT_GIVE_INT", MonsterFeedType::GIVE_INT },
    { "EAT_GIVE_WIS", MonsterFeedType::GIVE_WIS },
    { "EAT_GIVE_DEX", MonsterFeedType::GIVE_DEX },
    { "EAT_GIVE_CON", MonsterFeedType::GIVE_CON },
    { "EAT_GIVE_CHR", MonsterFeedType::GIVE_CHR },
    { "EAT_LOSE_STR", MonsterFeedType::LOSE_STR },
    { "EAT_LOSE_INT", MonsterFeedType::LOSE_INT },
    { "EAT_LOSE_WIS", MonsterFeedType::LOSE_WIS },
    { "EAT_LOSE_DEX", MonsterFeedType::LOSE_DEX },
    { "EAT_LOSE_CON", MonsterFeedType::LOSE_CON },
    { "EAT_LOSE_CHR", MonsterFeedType::LOSE_CHR },
    { "EAT_DRAIN_MANA", MonsterFeedType::DRAIN_MANA },
};

/*!
 * モンスターの打撃手段トークンの定義 /
 * Monster Blow Methods
 */
const std::unordered_map<std::string_view, RaceBlowMethodType> r_info_blow_method = {
    { "HIT", RaceBlowMethodType::HIT },
    { "TOUCH", RaceBlowMethodType::TOUCH },
    { "PUNCH", RaceBlowMethodType::PUNCH },
    { "KICK", RaceBlowMethodType::KICK },
    { "CLAW", RaceBlowMethodType::CLAW },
    { "BITE", RaceBlowMethodType::BITE },
    { "STING", RaceBlowMethodType::STING },
    { "SLASH", RaceBlowMethodType::SLASH },
    { "BUTT", RaceBlowMethodType::BUTT },
    { "CRUSH", RaceBlowMethodType::CRUSH },
    { "ENGULF", RaceBlowMethodType::ENGULF },
    { "CHARGE", RaceBlowMethodType::CHARGE },
    { "CRAWL", RaceBlowMethodType::CRAWL },
    { "DROOL", RaceBlowMethodType::DROOL },
    { "SPIT", RaceBlowMethodType::SPIT },
    { "EXPLODE", RaceBlowMethodType::EXPLODE },
    { "GAZE", RaceBlowMethodType::GAZE },
    { "WAIL", RaceBlowMethodType::WAIL },
    { "SPORE", RaceBlowMethodType::SPORE },
    { "XXX4", RaceBlowMethodType::XXX4 },
    { "BEG", RaceBlowMethodType::BEG },
    { "INSULT", RaceBlowMethodType::INSULT },
    { "MOAN", RaceBlowMethodType::MOAN },
    { "SHOW", RaceBlowMethodType::SHOW },
    { "ENEMA", RaceBlowMethodType::ENEMA },
    { "BIND", RaceBlowMethodType::BIND },
    { "WHISPER", RaceBlowMethodType::WHISPER },
    { "STAMP", RaceBlowMethodType::STAMP },
    { "FECES", RaceBlowMethodType::FECES },
    { "PUTAWAY", RaceBlowMethodType::PUTAWAY },
    { "CHOKE", RaceBlowMethodType::CHOKE },
};

/*!
 * モンスターの打撃属性トークンの定義 /
 * Monster Blow Effects
 */
const std::unordered_map<std::string_view, RaceBlowEffectType> r_info_blow_effect = {
    { "HURT", RaceBlowEffectType::HURT },
    { "POISON", RaceBlowEffectType::POISON },
    { "UN_BONUS", RaceBlowEffectType::UN_BONUS },
    { "UN_POWER", RaceBlowEffectType::UN_POWER },
    { "EAT_GOLD", RaceBlowEffectType::EAT_GOLD },
    { "EAT_ITEM", RaceBlowEffectType::EAT_ITEM },
    { "EAT_FOOD", RaceBlowEffectType::EAT_FOOD },
    { "EAT_LITE", RaceBlowEffectType::EAT_LITE },
    { "ACID", RaceBlowEffectType::ACID },
    { "ELEC", RaceBlowEffectType::ELEC },
    { "FIRE", RaceBlowEffectType::FIRE },
    { "COLD", RaceBlowEffectType::COLD },
    { "BLIND", RaceBlowEffectType::BLIND },
    { "CONFUSE", RaceBlowEffectType::CONFUSE },
    { "TERRIFY", RaceBlowEffectType::TERRIFY },
    { "PARALYZE", RaceBlowEffectType::PARALYZE },
    { "LOSE_STR", RaceBlowEffectType::LOSE_STR },
    { "LOSE_INT", RaceBlowEffectType::LOSE_INT },
    { "LOSE_WIS", RaceBlowEffectType::LOSE_WIS },
    { "LOSE_DEX", RaceBlowEffectType::LOSE_DEX },
    { "LOSE_CON", RaceBlowEffectType::LOSE_CON },
    { "LOSE_CHR", RaceBlowEffectType::LOSE_CHR },
    { "LOSE_ALL", RaceBlowEffectType::LOSE_ALL },
    { "SHATTER", RaceBlowEffectType::SHATTER },
    { "EXP_10", RaceBlowEffectType::EXP_10 },
    { "EXP_20", RaceBlowEffectType::EXP_20 },
    { "EXP_40", RaceBlowEffectType::EXP_40 },
    { "EXP_80", RaceBlowEffectType::EXP_80 },
    { "DISEASE", RaceBlowEffectType::DISEASE },
    { "TIME", RaceBlowEffectType::TIME },
    { "EXP_VAMP", RaceBlowEffectType::DR_LIFE },
    { "DR_MANA", RaceBlowEffectType::DR_MANA },
    { "SUPERHURT", RaceBlowEffectType::SUPERHURT },
    { "INERTIA", RaceBlowEffectType::INERTIA },
    { "STUN", RaceBlowEffectType::STUN },
    { "HUNGRY", RaceBlowEffectType::HUNGRY },
    { "CHAOS", RaceBlowEffectType::CHAOS },
    { "FLAVOR", RaceBlowEffectType::FLAVOR },
    { "DEFECATE", RaceBlowEffectType::DEFECATE },
    { "SANITY_BLAST", RaceBlowEffectType::SANITY_BLAST },
    { "LOCKUP", RaceBlowEffectType::LOCKUP },
    { "DESTROY_ASSHOLE", RaceBlowEffectType::DESTROY_ASSHOLE },
};

/*!
 * モンスター特性トークン (発動型能力) /
 * Monster race flags
 */
/* clang-format off */
const std::unordered_map<std::string_view, MonsterAbilityType> r_info_ability_flags = {
	{"SHRIEK", MonsterAbilityType::SHRIEK },
	{"XXX1", MonsterAbilityType::XXX1 },
	{"DISPEL", MonsterAbilityType::DISPEL },
	{"ROCKET", MonsterAbilityType::ROCKET },
	{"SHOOT", MonsterAbilityType::SHOOT },
	{"XXX2", MonsterAbilityType::XXX2 },
	{"XXX3", MonsterAbilityType::XXX3 },
	{"XXX4", MonsterAbilityType::XXX4 },
	{"BR_ACID", MonsterAbilityType::BR_ACID },
	{"BR_ELEC", MonsterAbilityType::BR_ELEC },
	{"BR_FIRE", MonsterAbilityType::BR_FIRE },
	{"BR_COLD", MonsterAbilityType::BR_COLD },
	{"BR_POIS", MonsterAbilityType::BR_POIS },
	{"BR_NETH", MonsterAbilityType::BR_NETH },
	{"BR_LITE", MonsterAbilityType::BR_LITE },
	{"BR_DARK", MonsterAbilityType::BR_DARK },
	{"BR_CONF", MonsterAbilityType::BR_CONF },
	{"BR_SOUN", MonsterAbilityType::BR_SOUN },
	{"BR_CHAO", MonsterAbilityType::BR_CHAO },
	{"BR_DISE", MonsterAbilityType::BR_DISE },
	{"BR_NEXU", MonsterAbilityType::BR_NEXU },
	{"BR_TIME", MonsterAbilityType::BR_TIME },
	{"BR_INER", MonsterAbilityType::BR_INER },
	{"BR_GRAV", MonsterAbilityType::BR_GRAV },
	{"BR_SHAR", MonsterAbilityType::BR_SHAR },
	{"BR_PLAS", MonsterAbilityType::BR_PLAS },
	{"BR_FORC", MonsterAbilityType::BR_FORC },
	{"BR_MANA", MonsterAbilityType::BR_MANA },
	{"BA_NUKE", MonsterAbilityType::BA_NUKE },
	{"BR_NUKE", MonsterAbilityType::BR_NUKE },
	{"BA_CHAO", MonsterAbilityType::BA_CHAO },
	{"BR_DISI", MonsterAbilityType::BR_DISI },
	{"BR_VOID", MonsterAbilityType::BR_VOID },
	{"BR_ABYSS", MonsterAbilityType::BR_ABYSS },
	{"BR_FECES", MonsterAbilityType::BR_FECES },
	{"BR_SPIDER_STRING", MonsterAbilityType::BR_SPIDER_STRING },
	{"BA_ACID", MonsterAbilityType::BA_ACID },
	{"BA_ELEC", MonsterAbilityType::BA_ELEC },
	{"BA_FIRE", MonsterAbilityType::BA_FIRE },
	{"BA_COLD", MonsterAbilityType::BA_COLD },
	{"BA_POIS", MonsterAbilityType::BA_POIS },
	{"BA_NETH", MonsterAbilityType::BA_NETH },
	{"BA_WATE", MonsterAbilityType::BA_WATE },
	{"BA_MANA", MonsterAbilityType::BA_MANA },
	{"BA_DARK", MonsterAbilityType::BA_DARK },
	{"BA_VOID", MonsterAbilityType::BA_VOID },
	{"BA_ABYSS", MonsterAbilityType::BA_ABYSS },
	{"BA_METEOR", MonsterAbilityType::BA_METEOR },
	{"BA_GRAVITY", MonsterAbilityType::BA_GRAVITY },
	{"DRAIN_MANA", MonsterAbilityType::DRAIN_MANA },
	{"MIND_BLAST", MonsterAbilityType::MIND_BLAST },
	{"BRAIN_SMASH", MonsterAbilityType::BRAIN_SMASH },
	{"CAUSE_1", MonsterAbilityType::CAUSE_1 },
	{"CAUSE_2", MonsterAbilityType::CAUSE_2 },
	{"CAUSE_3", MonsterAbilityType::CAUSE_3 },
	{"CAUSE_4", MonsterAbilityType::CAUSE_4 },
	{"BO_ACID", MonsterAbilityType::BO_ACID },
	{"BO_ELEC", MonsterAbilityType::BO_ELEC },
	{"BO_FIRE", MonsterAbilityType::BO_FIRE },
	{"BO_COLD", MonsterAbilityType::BO_COLD },
	{"BA_LITE", MonsterAbilityType::BA_LITE },
	{"BO_NETH", MonsterAbilityType::BO_NETH },
	{"BO_WATE", MonsterAbilityType::BO_WATE },
	{"BO_MANA", MonsterAbilityType::BO_MANA },
	{"BO_PLAS", MonsterAbilityType::BO_PLAS },
	{"BO_ICEE", MonsterAbilityType::BO_ICEE },
	{"BO_VOID", MonsterAbilityType::BO_VOID },
	{"BO_ABYSS", MonsterAbilityType::BO_ABYSS },
	{"BO_METEOR", MonsterAbilityType::BO_METEOR },
	{"BO_LITE", MonsterAbilityType::BO_LITE },
	{"MISSILE", MonsterAbilityType::MISSILE },
	{"SCARE", MonsterAbilityType::SCARE },
	{"BLIND", MonsterAbilityType::BLIND },
	{"CONF", MonsterAbilityType::CONF },
	{"SLOW", MonsterAbilityType::SLOW },
	{"HOLD", MonsterAbilityType::HOLD },

	{"HASTE", MonsterAbilityType::HASTE },
	{"HAND_DOOM", MonsterAbilityType::HAND_DOOM },
	{"HEAL", MonsterAbilityType::HEAL },
	{"INVULNER", MonsterAbilityType::INVULNER },
	{"BLINK", MonsterAbilityType::BLINK },
	{"TPORT", MonsterAbilityType::TPORT },
	{"WORLD", MonsterAbilityType::WORLD },
	{"SPECIAL", MonsterAbilityType::SPECIAL },
	{"TELE_TO", MonsterAbilityType::TELE_TO },
	{"TELE_AWAY", MonsterAbilityType::TELE_AWAY },
	{"TELE_LEVEL", MonsterAbilityType::TELE_LEVEL },
	{"PSY_SPEAR", MonsterAbilityType::PSY_SPEAR },
	{"DARKNESS", MonsterAbilityType::DARKNESS },
	{"TRAPS", MonsterAbilityType::TRAPS },
	{"FORGET", MonsterAbilityType::FORGET },
	{"ANIM_DEAD", MonsterAbilityType::RAISE_DEAD /* ToDo: Implement ANIM_DEAD */ },
	{"S_KIN", MonsterAbilityType::S_KIN },
	{"S_CYBER", MonsterAbilityType::S_CYBER },
	{"S_MONSTER", MonsterAbilityType::S_MONSTER },
	{"S_MONSTERS", MonsterAbilityType::S_MONSTERS },
	{"S_ANT", MonsterAbilityType::S_ANT },
	{"S_SPIDER", MonsterAbilityType::S_SPIDER },
	{"S_HOUND", MonsterAbilityType::S_HOUND },
	{"S_HYDRA", MonsterAbilityType::S_HYDRA },
	{"S_FAIRY", MonsterAbilityType::S_FAIRY },
	{"S_BIRD", MonsterAbilityType::S_BIRD },
	{"S_ANGEL", MonsterAbilityType::S_ANGEL },
	{"S_DEMON", MonsterAbilityType::S_DEMON },
	{"S_UNDEAD", MonsterAbilityType::S_UNDEAD },
	{"S_DRAGON", MonsterAbilityType::S_DRAGON },
	{"S_HI_UNDEAD", MonsterAbilityType::S_HI_UNDEAD },
	{"S_HI_DRAGON", MonsterAbilityType::S_HI_DRAGON },
	{"S_AMBERITES", MonsterAbilityType::S_AMBERITES },
	{"S_CHOASIANS", MonsterAbilityType::S_CHOASIANS },
	{"S_UNIQUE", MonsterAbilityType::S_UNIQUE },
	{"S_DEAD_UNIQUE", MonsterAbilityType::S_DEAD_UNIQUE },
	{"S_CAT", MonsterAbilityType::S_CAT },
	{"S_WALL", MonsterAbilityType::S_WALL },
	{"S_INSECT", MonsterAbilityType::S_INSECT },
	{"S_ELDRAZI", MonsterAbilityType::S_ELDRAZI },
	{"S_ROBOT", MonsterAbilityType::S_ROBOT },
};
/* clang-format on */

/*!
 * モンスター特性トークンの定義R(耐性) /
 * Monster race flags
 */
const std::unordered_map<std::string_view, MonsterResistanceType> r_info_flagsr = {
    { "RES_ALL", MonsterResistanceType::RESIST_ALL },
    { "HURT_ACID", MonsterResistanceType::HURT_ACID },
    { "RES_ACID", MonsterResistanceType::RESIST_ACID },
    { "IM_ACID", MonsterResistanceType::IMMUNE_ACID },
    { "HURT_ELEC", MonsterResistanceType::HURT_ELEC },
    { "RES_ELEC", MonsterResistanceType::RESIST_ELEC },
    { "IM_ELEC", MonsterResistanceType::IMMUNE_ELEC },
    { "HURT_FIRE", MonsterResistanceType::HURT_FIRE },
    { "RES_FIRE", MonsterResistanceType::RESIST_FIRE },
    { "IM_FIRE", MonsterResistanceType::IMMUNE_FIRE },
    { "HURT_COLD", MonsterResistanceType::HURT_COLD },
    { "RES_COLD", MonsterResistanceType::RESIST_COLD },
    { "IM_COLD", MonsterResistanceType::IMMUNE_COLD },
    { "HURT_POIS", MonsterResistanceType::HURT_POISON },
    { "RES_POIS", MonsterResistanceType::RESIST_POISON },
    { "IM_POIS", MonsterResistanceType::IMMUNE_POISON },
    { "HURT_LITE", MonsterResistanceType::HURT_LITE },
    { "RES_LITE", MonsterResistanceType::RESIST_LITE },
    { "HURT_DARK", MonsterResistanceType::HURT_DARK },
    { "RES_DARK", MonsterResistanceType::RESIST_DARK },
    { "HURT_NETH", MonsterResistanceType::HURT_NETHER },
    { "RES_NETH", MonsterResistanceType::RESIST_NETHER },
    { "HURT_WATE", MonsterResistanceType::HURT_WATER },
    { "RES_WATE", MonsterResistanceType::RESIST_WATER },
    { "HURT_PLAS", MonsterResistanceType::HURT_PLASMA },
    { "RES_PLAS", MonsterResistanceType::RESIST_PLASMA },
    { "HURT_SHAR", MonsterResistanceType::HURT_SHARDS },
    { "RES_SHAR", MonsterResistanceType::RESIST_SHARDS },
    { "HURT_SOUN", MonsterResistanceType::HURT_SOUND },
    { "RES_SOUN", MonsterResistanceType::RESIST_SOUND },
    { "HURT_CHAO", MonsterResistanceType::HURT_CHAOS },
    { "RES_CHAO", MonsterResistanceType::RESIST_CHAOS },
    { "HURT_NEXU", MonsterResistanceType::HURT_NEXUS },
    { "RES_NEXU", MonsterResistanceType::RESIST_NEXUS },
    { "HURT_DISE", MonsterResistanceType::HURT_DISENCHANT },
    { "RES_DISE", MonsterResistanceType::RESIST_DISENCHANT },
    { "HURT_WALL", MonsterResistanceType::HURT_FORCE },
    { "RES_WALL", MonsterResistanceType::RESIST_FORCE },
    { "HURT_INER", MonsterResistanceType::HURT_INERTIA },
    { "RES_INER", MonsterResistanceType::RESIST_INERTIA },
    { "HURT_TIME", MonsterResistanceType::HURT_TIME },
    { "RES_TIME", MonsterResistanceType::RESIST_TIME },
    { "HURT_GRAV", MonsterResistanceType::HURT_GRAVITY },
    { "RES_GRAV", MonsterResistanceType::RESIST_GRAVITY },
    { "RES_TELE", MonsterResistanceType::RESIST_TELEPORT },
    { "HURT_ROCK", MonsterResistanceType::HURT_ROCK },
    { "RES_ROCK", MonsterResistanceType::RESIST_ROCK },
    { "HURT_ABYSS", MonsterResistanceType::HURT_ABYSS },
    { "RES_ABYSS", MonsterResistanceType::RESIST_ABYSS },
    { "HURT_VOID", MonsterResistanceType::HURT_VOID_MAGIC },
    { "RES_VOID", MonsterResistanceType::RESIST_VOID_MAGIC },
    { "HURT_METEOR", MonsterResistanceType::HURT_METEOR },
    { "RES_METEOR", MonsterResistanceType::RESIST_METEOR },
    { "NO_FEAR", MonsterResistanceType::NO_FEAR },
    { "NO_STUN", MonsterResistanceType::NO_STUN },
    { "NO_CONF", MonsterResistanceType::NO_CONF },
    { "NO_SLEEP", MonsterResistanceType::NO_SLEEP },
    { "NO_INSTANTLY_DEATH", MonsterResistanceType::NO_INSTANTLY_DEATH },
    { "NO_DEFECATE", MonsterResistanceType::NO_DEFECATE },
    { "NO_VOMIT", MonsterResistanceType::NO_VOMIT }
};

const std::unordered_map<std::string_view, MonsterAuraType> r_info_aura_flags = {
    { "AURA_FIRE", MonsterAuraType::FIRE },
    { "AURA_COLD", MonsterAuraType::COLD },
    { "AURA_ELEC", MonsterAuraType::ELEC },
    { "AURA_ACID", MonsterAuraType::ACID },
    { "AURA_POISON", MonsterAuraType::POISON },
    { "AURA_NUKE", MonsterAuraType::NUKE },
    { "AURA_PLASMA", MonsterAuraType::PLASMA },
    { "AURA_WATER", MonsterAuraType::WATER },
    { "AURA_ICEE", MonsterAuraType::ICEE },
    { "AURA_LITE", MonsterAuraType::LITE },
    { "AURA_DARK", MonsterAuraType::DARK },
    { "AURA_SHARDS", MonsterAuraType::SHARDS },
    { "AURA_FORCE", MonsterAuraType::FORCE },
    { "AURA_MANA", MonsterAuraType::MANA },
    { "AURA_METEOR", MonsterAuraType::METEOR },
    { "AURA_CHAOS", MonsterAuraType::CHAOS },
    { "AURA_HOLINESS", MonsterAuraType::HOLINESS },
    { "AURA_NETHER", MonsterAuraType::NETHER },
    { "AURA_DISENCHANT", MonsterAuraType::DISENCHANT },
    { "AURA_NEXUS", MonsterAuraType::NEXUS },
    { "AURA_TIME", MonsterAuraType::TIME },
    { "AURA_GRAVITY", MonsterAuraType::GRAVITY },
    { "AURA_VOIDS", MonsterAuraType::VOIDS },
    { "AURA_ABYSS", MonsterAuraType::ABYSS },
    { "AURA_DIRT", MonsterAuraType::DIRT },
};

const std::unordered_map<std::string_view, MonsterBehaviorType> r_info_behavior_flags = {
    { "NEVER_MOVE", MonsterBehaviorType::NEVER_MOVE },
    { "NEVER_BLOW", MonsterBehaviorType::NEVER_BLOW },
    { "OPEN_DOOR", MonsterBehaviorType::OPEN_DOOR },
    { "BASH_DOOR", MonsterBehaviorType::BASH_DOOR },
    { "MOVE_BODY", MonsterBehaviorType::MOVE_BODY },
    { "KILL_BODY", MonsterBehaviorType::KILL_BODY },
    { "TAKE_ITEM", MonsterBehaviorType::TAKE_ITEM },
    { "KILL_ITEM", MonsterBehaviorType::KILL_ITEM },
    { "RAND_25", MonsterBehaviorType::RAND_MOVE_25 },
    { "RAND_50", MonsterBehaviorType::RAND_MOVE_50 },
    { "STUPID", MonsterBehaviorType::STUPID },
    { "SMART", MonsterBehaviorType::SMART },
    { "FRIENDLY", MonsterBehaviorType::FRIENDLY },
    { "FRIENDLY_STANDBY", MonsterBehaviorType::FRIENDLY_STANDBY },
    { "TIMID", MonsterBehaviorType::TIMID },
};

const std::unordered_map<std::string_view, MonsterVisualType> r_info_visual_flags = {
    { "CHAR_CLEAR", MonsterVisualType::CLEAR },
    { "SHAPECHANGER", MonsterVisualType::SHAPECHANGER },
    { "ATTR_CLEAR", MonsterVisualType::CLEAR_COLOR },
    { "ATTR_MULTI", MonsterVisualType::MULTI_COLOR },
    { "ATTR_SEMIRAND", MonsterVisualType::RANDOM_COLOR },
    { "ATTR_ANY", MonsterVisualType::ANY_COLOR },
};

const std::unordered_map<std::string_view, MonsterKindType> r_info_kind_flags = {
    { "UNIQUE", MonsterKindType::UNIQUE },
    { "HUMAN", MonsterKindType::HUMAN },
    { "QUANTUM", MonsterKindType::QUANTUM },
    { "ORC", MonsterKindType::ORC },
    { "TROLL", MonsterKindType::TROLL },
    { "GIANT", MonsterKindType::GIANT },
    { "DRAGON", MonsterKindType::DRAGON },
    { "DEMON", MonsterKindType::DEMON },
    { "UNDEAD", MonsterKindType::UNDEAD },
    { "EVIL", MonsterKindType::EVIL },
    { "ANIMAL", MonsterKindType::ANIMAL },
    { "AMBERITE", MonsterKindType::AMBERITE },
    { "CHOASIAN", MonsterKindType::CHOASIAN },
    { "GOOD", MonsterKindType::GOOD },
    { "NONLIVING", MonsterKindType::NONLIVING },
    { "ANGEL", MonsterKindType::ANGEL },
    { "NASTY", MonsterKindType::NASTY },
    { "ELF", MonsterKindType::ELF },
    { "DWARF", MonsterKindType::DWARF },
    { "HOBBIT", MonsterKindType::HOBBIT },
    { "ELDRAZI", MonsterKindType::ELDRAZI },
    { "QUYLTHLUG", MonsterKindType::QUYLTHLUG },
    { "SPIDER", MonsterKindType::SPIDER },
    { "WARRIOR", MonsterKindType::WARRIOR },
    { "SOLDIER", MonsterKindType::SOLDIER },
    { "ROGUE", MonsterKindType::ROGUE },
    { "MAGE", MonsterKindType::MAGE },
    { "PRIEST", MonsterKindType::PRIEST },
    { "PALADIN", MonsterKindType::PALADIN },
    { "RANGER", MonsterKindType::RANGER },
    { "SAMURAI", MonsterKindType::SAMURAI },
    { "NINJA", MonsterKindType::NINJA },
    { "SUMOU_WRESTLER", MonsterKindType::SUMOU_WRESTLER },
    { "YAKUZA", MonsterKindType::YAKUZA },
    { "KARATEKA", MonsterKindType::KARATEKA },
    { "JOKE", MonsterKindType::JOKE },
    { "HOMO_SEXUAL", MonsterKindType::HOMO_SEXUAL },
    { "TANK", MonsterKindType::TANK },
    { "HENTAI", MonsterKindType::HENTAI },
    { "ELEMENTAL", MonsterKindType::ELEMENTAL },
    { "GOLEM", MonsterKindType::GOLEM },
    { "PUYO", MonsterKindType::PUYO },
    { "INSECT", MonsterKindType::INSECT },
    { "ROBOT", MonsterKindType::ROBOT },
    { "YAZYU", MonsterKindType::YAZYU },
    { "SKELETON", MonsterKindType::SKELETON },
    { "CANCER", MonsterKindType::CANCER },
    { "FUNGAS", MonsterKindType::FUNGAS },
    { "TURTLE", MonsterKindType::TURTLE },
    { "MIMIC", MonsterKindType::MIMIC },
    { "IXITXACHITL", MonsterKindType::IXITXACHITL },
    { "NAGA", MonsterKindType::NAGA },
    { "PERVERT", MonsterKindType::PERVERT },
    { "ZOMBIE", MonsterKindType::ZOMBIE },
    { "DOG", MonsterKindType::DOG },
    { "CAT", MonsterKindType::CAT },
    { "RABBIT", MonsterKindType::RABBIT },
    { "PEASANT", MonsterKindType::PEASANT },
    { "RABBLE", MonsterKindType::RABBLE },
    { "NOBLE", MonsterKindType::NOBLE },
    { "BEAST", MonsterKindType::BEAST },
    { "LEECH", MonsterKindType::LEECH },
    { "JELLYFISH", MonsterKindType::JELLYFISH },
    { "CITIZEN", MonsterKindType::CITIZEN },
    { "TREEFOLK", MonsterKindType::TREEFOLK },
    { "VIRUS", MonsterKindType::VIRUS },
    { "SPHINX", MonsterKindType::SPHINX },
    { "SCORPION", MonsterKindType::SCORPION },
    { "MINDCRAFTER", MonsterKindType::MINDCRAFTER },
    { "TANUKI", MonsterKindType::TANUKI },
    { "CHAMELEON", MonsterKindType::CHAMELEON },
    { "MONKEY_SPACE", MonsterKindType::MONKEY_SPACE }, // 猿空間
    { "APE", MonsterKindType::APE }, // 類人猿
    { "HORSE", MonsterKindType::HORSE }, // 馬
    { "FROG", MonsterKindType::FROG }, // カエル
    { "BEHOLDER", MonsterKindType::BEHOLDER }, // ビホルダー
    { "YEEK", MonsterKindType::YEEK }, // イーク
    { "AQUATIC_MAMMAL", MonsterKindType::AQUATIC_MAMMAL }, // 水棲哺乳類
    { "FISH", MonsterKindType::FISH }, // 魚類
    { "BIRD", MonsterKindType::BIRD }, // 鳥類
    { "WALL", MonsterKindType::WALL }, // 壁
    { "PLANT", MonsterKindType::PLANT }, // 植物
    { "FUNGUS", MonsterKindType::FUNGUS }, // 菌類
    { "TURTLE", MonsterKindType::TURTLE }, // 亀
    { "SNAKE", MonsterKindType::SNAKE }, // 蛇
    { "FAIRY", MonsterKindType::FAIRY }, // 妖精
    { "VAMPIRE", MonsterKindType::VAMPIRE }, // 吸血鬼
    { "BEAR", MonsterKindType::BEAR }, // 熊
    { "VORTEX", MonsterKindType::VORTEX }, // ボルテックス
    { "OOZE", MonsterKindType::OOZE }, // ウーズ
    { "DINOSAUR", MonsterKindType::DINOSAUR }, // 恐竜
    { "LICH", MonsterKindType::LICH }, // リッチ
    { "GHOST", MonsterKindType::GHOST }, // 幽霊
    { "BERSERK", MonsterKindType::BERSERK }, // 狂戦士
    { "EXPLOSIVE", MonsterKindType::EXPLOSIVE }, // 爆発物
    { "RAT", MonsterKindType::RAT }, // ネズミ
    { "MINOTAUR", MonsterKindType::MINOTAUR }, // ミノタウロス
    { "SKAVEN", MonsterKindType::SKAVEN }, // スケイヴン
    { "KOBOLD", MonsterKindType::KOBOLD }, // コボルド
    { "OGRE", MonsterKindType::OGRE }, // オーガ
    { "BOVINE", MonsterKindType::BOVINE }, // 牛
    { "MERFOLK", MonsterKindType::MERFOLK }, // マーフォーク
    { "SHARK", MonsterKindType::SHARK }, // サメ
    { "MESUGAKI", MonsterKindType::MESUGAKI }, // メスガキ
    { "SAIYAN", MonsterKindType::SAIYAN }, // サイヤ人
    { "HYDRA", MonsterKindType::HYDRA }, // ヒドラ
    { "SHIP", MonsterKindType::SHIP }, // 船舶
    { "SLUG", MonsterKindType::SLUG }, // ナメクジ
    { "EYE", MonsterKindType::EYE }, // 目
    { "ALIEN", MonsterKindType::ALIEN }, // 異星人
    { "GRANDMA", MonsterKindType::GRANDMA }, // ババア
    { "PAPER", MonsterKindType::PAPER }, // 紙で出来た
    { "WOODEN", MonsterKindType::WOODEN }, // 木で出来た
    { "IRON", MonsterKindType::IRON }, // 鉄で出来た
    { "COPPER", MonsterKindType::COPPER }, // 銅で出来た
    { "STONE", MonsterKindType::STONE }, // 石で出来た
    { "SILVER", MonsterKindType::SILVER }, // 銀で出来た
    { "GOLD", MonsterKindType::GOLD }, // 金で出来た
    { "MITHRIL", MonsterKindType::MITHRIL }, // ミスリルで出来た
    { "ADAMANTITE", MonsterKindType::ADAMANTITE }, // アダマンタイトで出来た
    { "FECES", MonsterKindType::FECES }, // 糞で出来た
    { "FLESH", MonsterKindType::FLESH }, // 肉で出来た
    { "DARKSTEEL", MonsterKindType::DARKSTEEL }, // ダークスティールで出来た
    { "WARPSTONE", MonsterKindType::WARPSTONE }, // ワープストーンで出来た
    { "DEEPONE", MonsterKindType::DEEPONE }, // 深きもの
    { "PHYREXIAN", MonsterKindType::PHYREXIAN }, // ファイレクシア人
    { "HORROR", MonsterKindType::HORROR }, // ホラー
    { "WORM", MonsterKindType::WORM }, // ワーム
    { "OCTOPUS", MonsterKindType::OCTOPUS }, // タコ
    { "SQUID", MonsterKindType::SQUID }, // イカ
    { "FACE", MonsterKindType::FACE }, // 顔面
    { "HAND", MonsterKindType::HAND }, // 手
    { "MINDFLAYER", MonsterKindType::MINDFLAYER }, // マインドフレア
    { "NIBELUNG", MonsterKindType::NIBELUNG }, // ニーベルング
    { "GNOME", MonsterKindType::GNOME }, // ノーム
    { "KRAKEN", MonsterKindType::KRAKEN }, // クラーケン
    { "HARPY", MonsterKindType::HARPY }, // ハーピー
    { "ALARM", MonsterKindType::ALARM }, // 警報機
    { "DEER", MonsterKindType::DEER }, // 鹿
    { "ELEPHANT", MonsterKindType::ELEPHANT }, // 象
    { "LIZARD", MonsterKindType::LIZARD }, // トカゲ
    { "AVATAR", MonsterKindType::AVATAR }, // アヴァター
    { "NIGHTSHADE", MonsterKindType::NIGHTSHADE }, // ナイトシェード
    { "HIPPO", MonsterKindType::HIPPO }, // カバ
    { "BAT", MonsterKindType::BAT }, // コウモリ
    { "PLANESWALKER", MonsterKindType::PLANESWALKER }, // プレインズウォーカー
    { "BOAR", MonsterKindType::BOAR }, // 猪
    { "ARCHER", MonsterKindType::ARCHER }, // アーチャー
    { "GUNNER", MonsterKindType::GUNNER }, // ガンナー
    { "SQUIRREL", MonsterKindType::SQUIRREL }, // リス
    { "BARD", MonsterKindType::BARD }, // 吟遊詩人
    { "MAGICAL_GIRL", MonsterKindType::MAGICAL_GIRL }, // 魔法少女
    { "WEREWOLF", MonsterKindType::WEREWOLF }, // 人狼
    { "SMITH", MonsterKindType::SMITH }, // 鍛冶師
    { "WHEEL", MonsterKindType::WHEEL }, // 車輪
    { "GREAT_OLD_ONE", MonsterKindType::GREAT_OLD_ONE }, // 旧支配者
};

const std::unordered_map<std::string_view, MonsterEraType> r_info_era_flags = {
    { "PREHISTORIC", MonsterEraType::PREHISTORIC }, // 先史時代級
    { "ANCIENT", MonsterEraType::ANCIENT }, // 古代級
    { "MEDIEVAL", MonsterEraType::MEDIEVAL }, // 中世級
    { "EARLY_MODERN", MonsterEraType::EARLY_MODERN }, // 近代級
    { "MODERN", MonsterEraType::MODERN }, // 現代級
    { "INFORMATION_AGE", MonsterEraType::INFORMATION_AGE }, // 情報化時代級
    { "NANOTECH", MonsterEraType::NANOTECH }, // ナノテク級
};

const std::unordered_map<std::string_view, MonsterDropType> r_info_drop_flags = {
    { "ONLY_GOLD", MonsterDropType::ONLY_GOLD },
    { "ONLY_ITEM", MonsterDropType::ONLY_ITEM },
    { "DROP_GOOD", MonsterDropType::DROP_GOOD },
    { "DROP_GREAT", MonsterDropType::DROP_GREAT },
    { "DROP_CORPSE", MonsterDropType::DROP_CORPSE },
    { "DROP_SKELETON", MonsterDropType::DROP_SKELETON },
    { "DROP_JUNK", MonsterDropType::DROP_JUNK },
    { "DROP_60", MonsterDropType::DROP_60 },
    { "DROP_90", MonsterDropType::DROP_90 },
    { "DROP_1D2", MonsterDropType::DROP_1D2 },
    { "DROP_2D2", MonsterDropType::DROP_2D2 },
    { "DROP_3D2", MonsterDropType::DROP_3D2 },
    { "DROP_4D2", MonsterDropType::DROP_4D2 },
    { "DROP_COPPER", MonsterDropType::DROP_COPPER },
    { "DROP_SILVER", MonsterDropType::DROP_SILVER },
    { "DROP_GARNET", MonsterDropType::DROP_GARNET },
    { "DROP_GOLD", MonsterDropType::DROP_GOLD },
    { "DROP_OPAL", MonsterDropType::DROP_OPAL },
    { "DROP_SAPPHIRE", MonsterDropType::DROP_SAPPHIRE },
    { "DROP_RUBY", MonsterDropType::DROP_RUBY },
    { "DROP_DIAMOND", MonsterDropType::DROP_DIAMOND },
    { "DROP_EMERALD", MonsterDropType::DROP_EMERALD },
    { "DROP_MITHRIL", MonsterDropType::DROP_MITHRIL },
    { "DROP_ADAMANTITE", MonsterDropType::DROP_ADAMANTITE },
    { "DROP_OBSIDIAN", MonsterDropType::DROP_OBSIDIAN },
    { "DROP_NASTY", MonsterDropType::DROP_NASTY },
};

const std::unordered_map<std::string_view, MonsterWildernessType> r_info_wilderness_flags = {
    { "WILD_ONLY", MonsterWildernessType::WILD_ONLY },
    { "WILD_TOWN", MonsterWildernessType::WILD_TOWN },
    { "WILD_SHORE", MonsterWildernessType::WILD_SHORE },
    { "WILD_OCEAN", MonsterWildernessType::WILD_OCEAN },
    { "WILD_WASTE", MonsterWildernessType::WILD_WASTE },
    { "WILD_WOOD", MonsterWildernessType::WILD_WOOD },
    { "WILD_VOLCANO", MonsterWildernessType::WILD_VOLCANO },
    { "WILD_MOUNTAIN", MonsterWildernessType::WILD_MOUNTAIN },
    { "WILD_GRASS", MonsterWildernessType::WILD_GRASS },
    { "WILD_SWAMP", MonsterWildernessType::WILD_SWAMP },
    { "WILD_ALL", MonsterWildernessType::WILD_ALL },
};

const std::unordered_map<std::string_view, MonsterFeatureType> r_info_feature_flags = {
    { "PASS_WALL", MonsterFeatureType::PASS_WALL },
    { "KILL_WALL", MonsterFeatureType::KILL_WALL },
    { "AQUATIC", MonsterFeatureType::AQUATIC },
    { "CAN_SWIM", MonsterFeatureType::CAN_SWIM },
    { "CAN_FLY", MonsterFeatureType::CAN_FLY },
    { "RAILWAY_ONLY", MonsterFeatureType::RAILWAY_ONLY },
};

const std::unordered_map<std::string_view, MonsterPopulationType> r_info_population_flags = {
    { "NAZGUL", MonsterPopulationType::NAZGUL },
    { "ONLY_ONE", MonsterPopulationType::ONLY_ONE },
    { "BUNBUN_STRIKER", MonsterPopulationType::BUNBUN_STRIKER },
};

const std::unordered_map<std::string_view, MonsterSpeakType> r_info_speak_flags = {
    { "SPEAK_ALL", MonsterSpeakType::SPEAK_ALL },
    { "SPEAK_BATTLE", MonsterSpeakType::SPEAK_BATTLE },
    { "SPEAK_FEAR", MonsterSpeakType::SPEAK_FEAR },
    { "SPEAK_FRIEND", MonsterSpeakType::SPEAK_FRIEND },
    { "SPEAK_DEATH", MonsterSpeakType::SPEAK_DEATH },
    { "SPEAK_SPAWN", MonsterSpeakType::SPEAK_SPAWN },
};

const std::unordered_map<std::string_view, MonsterMessageType> r_info_message_flags = {
    { "SPEAK_ALL", MonsterMessageType::SPEAK_ALL },
    { "SPEAK_BATTLE", MonsterMessageType::SPEAK_BATTLE },
    { "SPEAK_FEAR", MonsterMessageType::SPEAK_FEAR },
    { "SPEAK_FRIEND", MonsterMessageType::SPEAK_FRIEND },
    { "SPEAK_DEATH", MonsterMessageType::SPEAK_DEATH },
    { "SPEAK_SPAWN", MonsterMessageType::SPEAK_SPAWN },
    { "WALK_CLOSERANGE", MonsterMessageType::WALK_CLOSERANGE },
    { "WALK_MIDDLERANGE", MonsterMessageType::WALK_MIDDLERANGE },
    { "WALK_LONGRANGE", MonsterMessageType::WALK_LONGRANGE },
    { "MESSAGE_STALKER", MonsterMessageType::MESSAGE_STALKER },
    { "MESSAGE_REFLECT", MonsterMessageType::MESSAGE_REFLECT },
    { "MESSAGE_TIMESTOP", MonsterMessageType::MESSAGE_TIMESTOP },
    { "MESSAGE_TIMESTART", MonsterMessageType::MESSAGE_TIMESTART },
    { "MESSAGE_BREATH_SOUND", MonsterMessageType::MESSAGE_BREATH_SOUND },
    { "MESSAGE_BREATH_SHARDS", MonsterMessageType::MESSAGE_BREATH_SHARDS },
    { "MESSAGE_BREATH_FORCE", MonsterMessageType::MESSAGE_BREATH_FORCE },
    { "MESSAGE_DETECT_UNIQUE", MonsterMessageType::MESSAGE_DETECT_UNIQUE },
};

const std::unordered_map<std::string_view, MonsterBrightnessType> r_info_brightness_flags = {
    { "HAS_LITE_1", MonsterBrightnessType::HAS_LITE_1 },
    { "SELF_LITE_1", MonsterBrightnessType::SELF_LITE_1 },
    { "HAS_LITE_2", MonsterBrightnessType::HAS_LITE_2 },
    { "SELF_LITE_2", MonsterBrightnessType::SELF_LITE_2 },
    { "HAS_DARK_1", MonsterBrightnessType::HAS_DARK_1 },
    { "SELF_DARK_1", MonsterBrightnessType::SELF_DARK_1 },
    { "HAS_DARK_2", MonsterBrightnessType::HAS_DARK_2 },
    { "SELF_DARK_2", MonsterBrightnessType::SELF_DARK_2 },
};

const std::unordered_map<std::string_view, MonsterMiscType> r_info_misc_flags = {
    { "FORCE_DEPTH", MonsterMiscType::FORCE_DEPTH },
    { "FORCE_MAXHP", MonsterMiscType::FORCE_MAXHP },
    { "FRIENDS", MonsterMiscType::HAS_FRIENDS },
    { "ESCORT", MonsterMiscType::ESCORT },
    { "ESCORTS", MonsterMiscType::MORE_ESCORT },
    { "RIDING", MonsterMiscType::RIDING },
    { "INVISIBLE", MonsterMiscType::INVISIBLE },
    { "COLD_BLOOD", MonsterMiscType::COLD_BLOOD },
    { "KAGE", MonsterMiscType::KAGE },
    { "CHAMELEON", MonsterMiscType::CHAMELEON },
    { "TANUKI", MonsterMiscType::TANUKI },
    { "NO_QUEST", MonsterMiscType::NO_QUEST },
    { "ELDRITCH_HORROR", MonsterMiscType::ELDRITCH_HORROR },
    { "MULTIPLY", MonsterMiscType::MULTIPLY },
    { "REGENERATE", MonsterMiscType::REGENERATE },
    { "POWERFUL", MonsterMiscType::POWERFUL },
    { "REFLECTING", MonsterMiscType::REFLECTING },
    { "QUESTOR", MonsterMiscType::QUESTOR },
    { "EMPTY_MIND", MonsterMiscType::EMPTY_MIND },
    { "WEIRD_MIND", MonsterMiscType::WEIRD_MIND },
    { "VOCIFEROUS", MonsterMiscType::VOCIFEROUS },
    { "STALKER", MonsterMiscType::STALKER },
    { "HOME_ONLY", MonsterMiscType::HOME_ONLY },
    { "SCATOLOGIST", MonsterMiscType::SCATOLOGIST },
    { "MASOCHIST", MonsterMiscType::MASOCHIST },
    { "SADIST", MonsterMiscType::SADIST },
    { "BREAK_DOWN", MonsterMiscType::BREAK_DOWN },
    { "NO_WAIFUZATION", MonsterMiscType::NO_WAIFUZATION },
    { "DIURNAL", MonsterMiscType::DIURNAL },
    { "NOCTURNAL", MonsterMiscType::NOCTURNAL },
};

const std::unordered_map<std::string_view, MonsterSex> r_info_sex = {
    { "NONE", MonsterSex::NONE },
    { "MALE", MonsterSex::MALE },
    { "FEMALE", MonsterSex::FEMALE },
};

const std::unordered_map<std::string_view, player_personality_type> r_info_personality = {
    { "ORDINARY", PERSONALITY_ORDINARY },
    { "MIGHTY", PERSONALITY_MIGHTY },
    { "SHREWD", PERSONALITY_SHREWD },
    { "PIOUS", PERSONALITY_PIOUS },
    { "NIMBLE", PERSONALITY_NIMBLE },
    { "FEARLESS", PERSONALITY_FEARLESS },
    { "COMBAT", PERSONALITY_COMBAT },
    { "LAZY", PERSONALITY_LAZY },
    { "SEXY", PERSONALITY_SEXY },
    { "LUCKY", PERSONALITY_LUCKY },
    { "PATIENT", PERSONALITY_PATIENT },
    { "MUNCHKIN", PERSONALITY_MUNCHKIN },
    { "CHARGEMAN", PERSONALITY_CHARGEMAN },
    { "TOUGH", PERSONALITY_TOUGH },
    { "SUSHI_EATER", PERSONALITY_SUSHI_EATER },
    { "MESUGAKI", PERSONALITY_MESUGAKI },
};

const std::unordered_map<std::string_view, CreatureMaterialType> r_info_materials = {
    { "FLESH", CreatureMaterialType::FLESH },
    { "WOODEN", CreatureMaterialType::WOODEN },
    { "PAPER", CreatureMaterialType::PAPER },
    { "STONE", CreatureMaterialType::STONE },
    { "IRON", CreatureMaterialType::IRON },
    { "COPPER", CreatureMaterialType::COPPER },
    { "SILVER", CreatureMaterialType::SILVER },
    { "GOLD", CreatureMaterialType::GOLD },
    { "MITHRIL", CreatureMaterialType::MITHRIL },
    { "ADAMANTITE", CreatureMaterialType::ADAMANTITE },
    { "DARKSTEEL", CreatureMaterialType::DARKSTEEL },
    { "WARPSTONE", CreatureMaterialType::WARPSTONE },
    { "FECES", CreatureMaterialType::FECES },
};

const std::unordered_map<std::string_view, BodyStructureType> r_info_body_structure = {
    { "HUMANOID", BodyStructureType::HUMANOID },
    { "BIPEDAL", BodyStructureType::BIPEDAL },
    { "QUADRUPED", BodyStructureType::QUADRUPED },
    { "SERPENTINE", BodyStructureType::SERPENTINE },
    { "AMORPHOUS", BodyStructureType::AMORPHOUS },
    { "INCORPOREAL", BodyStructureType::INCORPOREAL },
    { "DRACONIC", BodyStructureType::DRACONIC },
};

const std::unordered_map<std::string_view, ExtendedSlotType> r_info_extended_slot = {
    { "TAIL_RING", ExtendedSlotType::TAIL_RING },
    { "SECOND_NECK", ExtendedSlotType::SECOND_NECK },
    { "THIRD_HEAD", ExtendedSlotType::THIRD_HEAD },
    { "WING_LEFT", ExtendedSlotType::WING_LEFT },
    { "WING_RIGHT", ExtendedSlotType::WING_RIGHT },
};

const std::unordered_map<std::string_view, MonsterSpecialType> r_info_special_flags = {
    { "DIMINISH_MAX_DAMAGE", MonsterSpecialType::DIMINISH_MAX_DAMAGE },
};

/*!
 * @brief プレイヤー種族トークン (提案 C1: モンスターへの種族付与用)
 */
const std::unordered_map<std::string_view, PlayerRaceType> r_info_player_race = {
    { "HUMAN", PlayerRaceType::HUMAN },
    { "HALF_ELF", PlayerRaceType::HALF_ELF },
    { "ELF", PlayerRaceType::ELF },
    { "HOBBIT", PlayerRaceType::HOBBIT },
    { "GNOME", PlayerRaceType::GNOME },
    { "DWARF", PlayerRaceType::DWARF },
    { "HALF_ORC", PlayerRaceType::HALF_ORC },
    { "HALF_TROLL", PlayerRaceType::HALF_TROLL },
    { "AMBERITE", PlayerRaceType::AMBERITE },
    { "HIGH_ELF", PlayerRaceType::HIGH_ELF },
    { "BARBARIAN", PlayerRaceType::BARBARIAN },
    { "HALF_OGRE", PlayerRaceType::HALF_OGRE },
    { "HALF_GIANT", PlayerRaceType::HALF_GIANT },
    { "HALF_TITAN", PlayerRaceType::HALF_TITAN },
    { "CYCLOPS", PlayerRaceType::CYCLOPS },
    { "YEEK", PlayerRaceType::YEEK },
    { "KLACKON", PlayerRaceType::KLACKON },
    { "KOBOLD", PlayerRaceType::KOBOLD },
    { "NIBELUNG", PlayerRaceType::NIBELUNG },
    { "DARK_ELF", PlayerRaceType::DARK_ELF },
    { "DRACONIAN", PlayerRaceType::DRACONIAN },
    { "MIND_FLAYER", PlayerRaceType::MIND_FLAYER },
    { "IMP", PlayerRaceType::IMP },
    { "GOLEM", PlayerRaceType::GOLEM },
    { "SKELETON", PlayerRaceType::SKELETON },
    { "ZOMBIE", PlayerRaceType::ZOMBIE },
    { "VAMPIRE", PlayerRaceType::VAMPIRE },
    { "SPECTRE", PlayerRaceType::SPECTRE },
    { "SPRITE", PlayerRaceType::SPRITE },
    { "BEASTMAN", PlayerRaceType::BEASTMAN },
    { "ENT", PlayerRaceType::ENT },
    { "ARCHON", PlayerRaceType::ARCHON },
    { "BALROG", PlayerRaceType::BALROG },
    { "DUNADAN", PlayerRaceType::DUNADAN },
    { "S_FAIRY", PlayerRaceType::S_FAIRY },
    { "KUTAR", PlayerRaceType::KUTAR },
    { "ANDROID", PlayerRaceType::ANDROID },
    { "MERFOLK", PlayerRaceType::MERFOLK },
    { "CAT", PlayerRaceType::CAT },
    { "DOG", PlayerRaceType::DOG },
    { "HORSE", PlayerRaceType::HORSE },
    { "BIRD", PlayerRaceType::BIRD },
    { "RAT", PlayerRaceType::RAT },
    { "BEAR", PlayerRaceType::BEAR },
    { "SNAKE", PlayerRaceType::SNAKE },
    { "FISH", PlayerRaceType::FISH },
    { "INSECT", PlayerRaceType::INSECT },
    { "SPIDER", PlayerRaceType::SPIDER },
    { "FROG", PlayerRaceType::FROG },
    { "BAT", PlayerRaceType::BAT },
    { "TURTLE", PlayerRaceType::TURTLE },
    { "APE", PlayerRaceType::APE },
    { "AQUATIC_MAMMAL", PlayerRaceType::AQUATIC_MAMMAL },
    { "DINOSAUR", PlayerRaceType::DINOSAUR },
    { "BOVINE", PlayerRaceType::BOVINE },
    { "SHARK", PlayerRaceType::SHARK },
    { "HYDRA", PlayerRaceType::HYDRA },
    { "SLUG", PlayerRaceType::SLUG },
    { "OCTOPUS", PlayerRaceType::OCTOPUS },
    { "SQUID", PlayerRaceType::SQUID },
    { "HARPY", PlayerRaceType::HARPY },
    { "DEER", PlayerRaceType::DEER },
    { "ELEPHANT", PlayerRaceType::ELEPHANT },
    { "LIZARD", PlayerRaceType::LIZARD },
    { "HIPPO", PlayerRaceType::HIPPO },
    { "BOAR", PlayerRaceType::BOAR },
    { "RABBIT", PlayerRaceType::RABBIT },
    { "SCORPION", PlayerRaceType::SCORPION },
    { "TANUKI", PlayerRaceType::TANUKI },
    { "SQUIRREL", PlayerRaceType::SQUIRREL },
    { "WEREWOLF", PlayerRaceType::WEREWOLF },
    { "NAGA", PlayerRaceType::NAGA },
    { "CANCER", PlayerRaceType::CANCER },
    { "WORM", PlayerRaceType::WORM },
    { "KRAKEN", PlayerRaceType::KRAKEN },
    { "DEEPONE", PlayerRaceType::DEEPONE },
    { "LEECH", PlayerRaceType::LEECH },
    { "JELLYFISH", PlayerRaceType::JELLYFISH },
    { "MINOTAUR", PlayerRaceType::MINOTAUR },
    { "SPHINX", PlayerRaceType::SPHINX },
    { "BEHOLDER", PlayerRaceType::BEHOLDER },
    { "EYE", PlayerRaceType::EYE },
    { "VORTEX", PlayerRaceType::VORTEX },
    { "OOZE", PlayerRaceType::OOZE },
    { "GHOST", PlayerRaceType::GHOST },
    { "LICH", PlayerRaceType::LICH },
    { "PLANT", PlayerRaceType::PLANT },
    { "FUNGUS", PlayerRaceType::FUNGUS },
    { "ELEMENTAL", PlayerRaceType::ELEMENTAL },
    { "HORROR", PlayerRaceType::HORROR },
    { "NIGHTSHADE", PlayerRaceType::NIGHTSHADE },
    { "QUYLTHLUG", PlayerRaceType::QUYLTHLUG },
    { "IXITXACHITL", PlayerRaceType::IXITXACHITL },
    { "MIMIC", PlayerRaceType::MIMIC },
    { "MONKEY_SPACE", PlayerRaceType::MONKEY_SPACE },
    { "PUYO", PlayerRaceType::PUYO },
    { "YAZYU", PlayerRaceType::YAZYU },
    { "QUANTUM", PlayerRaceType::QUANTUM },
    { "ELDRAZI", PlayerRaceType::ELDRAZI },
    { "SKAVEN", PlayerRaceType::SKAVEN },
    { "ALIEN", PlayerRaceType::ALIEN },
    { "PHYREXIAN", PlayerRaceType::PHYREXIAN },
    { "GREAT_OLD_ONE", PlayerRaceType::GREAT_OLD_ONE },
    { "AVATAR", PlayerRaceType::AVATAR },
    { "PLANESWALKER", PlayerRaceType::PLANESWALKER },
    { "VIRUS", PlayerRaceType::VIRUS },
    { "CHOASIAN", PlayerRaceType::CHOASIAN },
    { "FACE", PlayerRaceType::FACE },
    { "HAND", PlayerRaceType::HAND },
    { "WALL", PlayerRaceType::WALL },
    { "SHIP", PlayerRaceType::SHIP },
    { "WHEEL", PlayerRaceType::WHEEL },
    { "EXPLOSIVE", PlayerRaceType::EXPLOSIVE },
    { "ALARM", PlayerRaceType::ALARM },
    { "PAPER", PlayerRaceType::PAPER },
    { "WOODEN", PlayerRaceType::WOODEN },
    { "IRON", PlayerRaceType::IRON },
    { "COPPER", PlayerRaceType::COPPER },
    { "STONE", PlayerRaceType::STONE },
    { "SILVER", PlayerRaceType::SILVER },
    { "GOLD", PlayerRaceType::GOLD },
    { "MITHRIL", PlayerRaceType::MITHRIL },
    { "ADAMANTITE", PlayerRaceType::ADAMANTITE },
    { "FECES", PlayerRaceType::FECES },
    { "FLESH", PlayerRaceType::FLESH },
    { "DARKSTEEL", PlayerRaceType::DARKSTEEL },
    { "WARPSTONE", PlayerRaceType::WARPSTONE },
    { "ORC", PlayerRaceType::ORC },
    { "TROLL", PlayerRaceType::TROLL },
    { "OGRE", PlayerRaceType::OGRE },
    { "GIANT", PlayerRaceType::GIANT },
    { "TITAN", PlayerRaceType::TITAN },
};

/*!
 * @brief プレイヤー職業トークン (提案 C1: モンスターへの職業付与用)
 */
const std::unordered_map<std::string_view, PlayerClassType> r_info_player_class = {
    { "WARRIOR", PlayerClassType::WARRIOR },
    { "MAGE", PlayerClassType::MAGE },
    { "PRIEST", PlayerClassType::PRIEST },
    { "ROGUE", PlayerClassType::ROGUE },
    { "RANGER", PlayerClassType::RANGER },
    { "PALADIN", PlayerClassType::PALADIN },
    { "WARRIOR_MAGE", PlayerClassType::WARRIOR_MAGE },
    { "CHAOS_WARRIOR", PlayerClassType::CHAOS_WARRIOR },
    { "MONK", PlayerClassType::MONK },
    { "MINDCRAFTER", PlayerClassType::MINDCRAFTER },
    { "HIGH_MAGE", PlayerClassType::HIGH_MAGE },
    { "TOURIST", PlayerClassType::TOURIST },
    { "IMITATOR", PlayerClassType::IMITATOR },
    { "BEASTMASTER", PlayerClassType::BEASTMASTER },
    { "SORCERER", PlayerClassType::SORCERER },
    { "ARCHER", PlayerClassType::ARCHER },
    { "MAGIC_EATER", PlayerClassType::MAGIC_EATER },
    { "BARD", PlayerClassType::BARD },
    { "RED_MAGE", PlayerClassType::RED_MAGE },
    { "SAMURAI", PlayerClassType::SAMURAI },
    { "FORCETRAINER", PlayerClassType::FORCETRAINER },
    { "BLUE_MAGE", PlayerClassType::BLUE_MAGE },
    { "CAVALRY", PlayerClassType::CAVALRY },
    { "BERSERKER", PlayerClassType::BERSERKER },
    { "SMITH", PlayerClassType::SMITH },
    { "MIRROR_MASTER", PlayerClassType::MIRROR_MASTER },
    { "NINJA", PlayerClassType::NINJA },
    { "SNIPER", PlayerClassType::SNIPER },
    { "ELEMENTALIST", PlayerClassType::ELEMENTALIST },
    { "SOLDIER", PlayerClassType::SOLDIER },
    { "PEASANT", PlayerClassType::PEASANT },
    { "NOBLE", PlayerClassType::NOBLE },
    { "CITIZEN", PlayerClassType::CITIZEN },
    { "RABBLE", PlayerClassType::RABBLE },
    { "YAKUZA", PlayerClassType::YAKUZA },
    { "SUMOU_WRESTLER", PlayerClassType::SUMOU_WRESTLER },
    { "GUNNER", PlayerClassType::GUNNER },
    { "BERSERK", PlayerClassType::BERSERK },
    { "TANK", PlayerClassType::TANK },
    { "MAGICAL_GIRL", PlayerClassType::MAGICAL_GIRL },
    { "GRANDMA", PlayerClassType::GRANDMA },
};

/*!
 * @brief 突然変異トークン (提案 C5: モンスターへの突然変異付与用)
 */
const std::unordered_map<std::string_view, PlayerMutationType> r_info_mutation = {
    { "SPIT_ACID", PlayerMutationType::SPIT_ACID },
    { "BR_FIRE", PlayerMutationType::BR_FIRE },
    { "HYPN_GAZE", PlayerMutationType::HYPN_GAZE },
    { "TELEKINES", PlayerMutationType::TELEKINES },
    { "VTELEPORT", PlayerMutationType::VTELEPORT },
    { "MIND_BLST", PlayerMutationType::MIND_BLST },
    { "RADIATION", PlayerMutationType::RADIATION },
    { "VAMPIRISM", PlayerMutationType::VAMPIRISM },
    { "SMELL_MET", PlayerMutationType::SMELL_MET },
    { "SMELL_MON", PlayerMutationType::SMELL_MON },
    { "BLINK", PlayerMutationType::BLINK },
    { "EAT_ROCK", PlayerMutationType::EAT_ROCK },
    { "SWAP_POS", PlayerMutationType::SWAP_POS },
    { "SHRIEK", PlayerMutationType::SHRIEK },
    { "ILLUMINE", PlayerMutationType::ILLUMINE },
    { "DET_CURSE", PlayerMutationType::DET_CURSE },
    { "BERSERK", PlayerMutationType::BERSERK },
    { "POLYMORPH", PlayerMutationType::POLYMORPH },
    { "MIDAS_TCH", PlayerMutationType::MIDAS_TCH },
    { "GROW_MOLD", PlayerMutationType::GROW_MOLD },
    { "RESIST", PlayerMutationType::RESIST },
    { "EARTHQUAKE", PlayerMutationType::EARTHQUAKE },
    { "EAT_MAGIC", PlayerMutationType::EAT_MAGIC },
    { "WEIGH_MAG", PlayerMutationType::WEIGH_MAG },
    { "STERILITY", PlayerMutationType::STERILITY },
    { "HIT_AND_AWAY", PlayerMutationType::HIT_AND_AWAY },
    { "DAZZLE", PlayerMutationType::DAZZLE },
    { "LASER_EYE", PlayerMutationType::LASER_EYE },
    { "RECALL", PlayerMutationType::RECALL },
    { "BANISH", PlayerMutationType::BANISH },
    { "COLD_TOUCH", PlayerMutationType::COLD_TOUCH },
    { "LAUNCHER", PlayerMutationType::LAUNCHER },
    { "BERS_RAGE", PlayerMutationType::BERS_RAGE },
    { "COWARDICE", PlayerMutationType::COWARDICE },
    { "RTELEPORT", PlayerMutationType::RTELEPORT },
    { "ALCOHOL", PlayerMutationType::ALCOHOL },
    { "HALLU", PlayerMutationType::HALLU },
    { "FLATULENT", PlayerMutationType::FLATULENT },
    { "SCOR_TAIL", PlayerMutationType::SCOR_TAIL },
    { "HORNS", PlayerMutationType::HORNS },
    { "BEAK", PlayerMutationType::BEAK },
    { "ATT_DEMON", PlayerMutationType::ATT_DEMON },
    { "PROD_MANA", PlayerMutationType::PROD_MANA },
    { "SPEED_FLUX", PlayerMutationType::SPEED_FLUX },
    { "BANISH_ALL", PlayerMutationType::BANISH_ALL },
    { "EAT_LIGHT", PlayerMutationType::EAT_LIGHT },
    { "TRUNK", PlayerMutationType::TRUNK },
    { "ATT_ANIMAL", PlayerMutationType::ATT_ANIMAL },
    { "TENTACLES", PlayerMutationType::TENTACLES },
    { "RAW_CHAOS", PlayerMutationType::RAW_CHAOS },
    { "NORMALITY", PlayerMutationType::NORMALITY },
    { "WRAITH", PlayerMutationType::WRAITH },
    { "POLY_WOUND", PlayerMutationType::POLY_WOUND },
    { "WASTING", PlayerMutationType::WASTING },
    { "ATT_DRAGON", PlayerMutationType::ATT_DRAGON },
    { "WEIRD_MIND", PlayerMutationType::WEIRD_MIND },
    { "NAUSEA", PlayerMutationType::NAUSEA },
    { "CHAOS_GIFT", PlayerMutationType::CHAOS_GIFT },
    { "WALK_SHAD", PlayerMutationType::WALK_SHAD },
    { "WARNING", PlayerMutationType::WARNING },
    { "INVULN", PlayerMutationType::INVULN },
    { "SP_TO_HP", PlayerMutationType::SP_TO_HP },
    { "HP_TO_SP", PlayerMutationType::HP_TO_SP },
    { "DISARM", PlayerMutationType::DISARM },
    { "HYPER_STR", PlayerMutationType::HYPER_STR },
    { "PUNY", PlayerMutationType::PUNY },
    { "HYPER_INT", PlayerMutationType::HYPER_INT },
    { "MORONIC", PlayerMutationType::MORONIC },
    { "RESILIENT", PlayerMutationType::RESILIENT },
    { "XTRA_FAT", PlayerMutationType::XTRA_FAT },
    { "ALBINO", PlayerMutationType::ALBINO },
    { "FLESH_ROT", PlayerMutationType::FLESH_ROT },
    { "SILLY_VOI", PlayerMutationType::SILLY_VOI },
    { "BLANK_FAC", PlayerMutationType::BLANK_FAC },
    { "ILL_NORM", PlayerMutationType::ILL_NORM },
    { "XTRA_EYES", PlayerMutationType::XTRA_EYES },
    { "MAGIC_RES", PlayerMutationType::MAGIC_RES },
    { "XTRA_NOIS", PlayerMutationType::XTRA_NOIS },
    { "INFRAVIS", PlayerMutationType::INFRAVIS },
    { "XTRA_LEGS", PlayerMutationType::XTRA_LEGS },
    { "SHORT_LEG", PlayerMutationType::SHORT_LEG },
    { "ELEC_TOUC", PlayerMutationType::ELEC_TOUC },
    { "FIRE_BODY", PlayerMutationType::FIRE_BODY },
    { "WART_SKIN", PlayerMutationType::WART_SKIN },
    { "SCALES", PlayerMutationType::SCALES },
    { "IRON_SKIN", PlayerMutationType::IRON_SKIN },
    { "WINGS", PlayerMutationType::WINGS },
    { "FEARLESS", PlayerMutationType::FEARLESS },
    { "REGEN", PlayerMutationType::REGEN },
    { "ESP", PlayerMutationType::ESP },
    { "LIMBER", PlayerMutationType::LIMBER },
    { "ARTHRITIS", PlayerMutationType::ARTHRITIS },
    { "BAD_LUCK", PlayerMutationType::BAD_LUCK },
    { "VULN_ELEM", PlayerMutationType::VULN_ELEM },
    { "MOTION", PlayerMutationType::MOTION },
    { "GOOD_LUCK", PlayerMutationType::GOOD_LUCK },
    { "DEFECATION", PlayerMutationType::DEFECATION },
    { "ZEERO_VIRUS", PlayerMutationType::ZEERO_VIRUS },
    { "HOMO_SEXUAL", PlayerMutationType::HOMO_SEXUAL },
    { "BI_SEXUAL", PlayerMutationType::BI_SEXUAL },
    { "WEAK_LOWER_BODY", PlayerMutationType::WEAK_LOWER_BODY },
    { "IKISUGI", PlayerMutationType::IKISUGI },
    { "ATT_NASTY", PlayerMutationType::ATT_NASTY },
    { "ATT_PERVERT", PlayerMutationType::ATT_PERVERT },
    { "DESTROYED_ASSHOLE", PlayerMutationType::DESTROYED_ASSHOLE },
    { "LOST_HEAD", PlayerMutationType::LOST_HEAD },
};

/*!
 * @brief 魔法領域トークン (提案 C6: モンスターへの realm 由来能力付与用)
 */
const std::unordered_map<std::string_view, RealmType> r_info_realm = {
    { "LIFE", RealmType::LIFE },
    { "SORCERY", RealmType::SORCERY },
    { "NATURE", RealmType::NATURE },
    { "CHAOS", RealmType::CHAOS },
    { "DEATH", RealmType::DEATH },
    { "TRUMP", RealmType::TRUMP },
    { "ARCANE", RealmType::ARCANE },
    { "CRAFT", RealmType::CRAFT },
    { "DAEMON", RealmType::DAEMON },
    { "CRUSADE", RealmType::CRUSADE },
    { "MUSIC", RealmType::MUSIC },
    { "HISSATSU", RealmType::HISSATSU },
    { "HEX", RealmType::HEX },
};
