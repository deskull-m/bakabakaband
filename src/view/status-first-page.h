#pragma once

constexpr auto ENTRY_BARE_HAND = 0;
constexpr auto ENTRY_TWO_HANDS = 1;
constexpr auto ENTRY_RIGHT_HAND1 = 2;
constexpr auto ENTRY_LEFT_HAND1 = 3;
constexpr auto ENTRY_LEFT_HAND2 = 4;
constexpr auto ENTRY_RIGHT_HAND2 = 5;
constexpr auto ENTRY_POSTURE = 6;
constexpr auto ENTRY_SHOOT_HIT_DAM = 7;
constexpr auto ENTRY_SHOOT_POWER = 8;
constexpr auto ENTRY_SPEED = 9;
constexpr auto ENTRY_BASE_AC = 10;
constexpr auto ENTRY_LEVEL = 11;
constexpr auto ENTRY_CUR_EXP = 12;
constexpr auto ENTRY_MAX_EXP = 13;
constexpr auto ENTRY_EXP_TO_ADV = 14;
constexpr auto ENTRY_GOLD = 15;
constexpr auto ENTRY_DAY = 16;
constexpr auto ENTRY_HP = 17;
constexpr auto ENTRY_SP = 18;
constexpr auto ENTRY_PLAY_TIME = 19;
constexpr auto ENTRY_SKILL_FIGHT = 20;
constexpr auto ENTRY_SKILL_SHOOT = 21;
constexpr auto ENTRY_SKILL_SAVING = 22;
constexpr auto ENTRY_SKILL_STEALTH = 23;
constexpr auto ENTRY_SKILL_PERCEP = 24;
constexpr auto ENTRY_SKILL_SEARCH = 25;
constexpr auto ENTRY_SKILL_DISARM = 26;
constexpr auto ENTRY_SKILL_DEVICE = 27;
constexpr auto ENTRY_SKILL_DIG = 45;

constexpr auto ENTRY_BLOWS = 28;
constexpr auto ENTRY_SHOTS = 29;
constexpr auto ENTRY_AVG_DMG = 30;
constexpr auto ENTRY_INFRA = 31;

constexpr auto ENTRY_NAME = 32;
constexpr auto ENTRY_SEX = 33;
constexpr auto ENTRY_RACE = 34;
constexpr auto ENTRY_CLASS = 35;
constexpr auto ENTRY_REALM = 36;
constexpr auto ENTRY_PATRON = 37;
constexpr auto ENTRY_AGE = 38;
constexpr auto ENTRY_HEIGHT = 39;
constexpr auto ENTRY_WEIGHT = 40;
constexpr auto ENTRY_SOCIAL = 41;
constexpr auto ENTRY_ALIGN = 42;

constexpr auto ENTRY_EXP_ANDR = 43;
constexpr auto ENTRY_EXP_TO_ADV_ANDR = 44;

constexpr auto ENTRY_DEATH_COUNT = 46;
constexpr auto ENTRY_WORLD_COLLAPSE = 47;
constexpr auto ENTRY_ACTUAL_RACE = 48;
constexpr auto ENTRY_APPARENT_RACE = 49;
constexpr auto ENTRY_HP_REGEN = 50;
constexpr auto ENTRY_MP_REGEN = 51;
constexpr auto ENTRY_ALLIANCE = 52;
constexpr auto ENTRY_BODY_STRUCTURE = 53;

class CreatureEntity;
class ItemEntity;
void calc_player_shot_params(CreatureEntity &creature, ItemEntity *o_ptr, int *shots, int *shot_frac);
void calc_player_two_hands(CreatureEntity &creature, int *damage, int *to_h);
void display_player_various(CreatureEntity &creature);
