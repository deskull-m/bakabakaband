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
    { "HYDRA", MonsterKindType::HYDRA }, // ヒドラ
    { "SHIP", MonsterKindType::SHIP }, // 船舶
    { "SLUG", MonsterKindType::SLUG }, // ナメクジ
    { "EYE", MonsterKindType::EYE }, // 目
    { "ALIEN", MonsterKindType::ALIEN }, // 異星人
    { "GRANDMA", MonsterKindType::GRANDMA }, // ババア
    { "PAPER", MonsterKindType::PAPER }, // 紙で出来た
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
    { "SQUIRREL", MonsterKindType::SQUIRREL }, // リス
    { "BARD", MonsterKindType::BARD }, // 呀遊詩人
    { "MAGICAL_GIRL", MonsterKindType::MAGICAL_GIRL }, // 魔法少女
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

const std::unordered_map<std::string_view, MonsterSpecialType> r_info_special_flags = {
    { "DIMINISH_MAX_DAMAGE", MonsterSpecialType::DIMINISH_MAX_DAMAGE },
};
