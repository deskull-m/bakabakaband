#pragma once

#include "util/enum-converter.h"

/*!
 * @details 既にplayer_raceが存在するので_typeと付けた
 */
enum class PlayerRaceType : int {
    HUMAN = 0,
    HALF_ELF = 1,
    ELF = 2,
    HOBBIT = 3,
    GNOME = 4,
    DWARF = 5,
    HALF_ORC = 6,
    HALF_TROLL = 7,
    AMBERITE = 8,
    HIGH_ELF = 9,
    BARBARIAN = 10,
    HALF_OGRE = 11,
    HALF_GIANT = 12,
    HALF_TITAN = 13,
    CYCLOPS = 14,
    YEEK = 15,
    KLACKON = 16,
    KOBOLD = 17,
    NIBELUNG = 18,
    DARK_ELF = 19,
    DRACONIAN = 20,
    MIND_FLAYER = 21,
    IMP = 22,
    GOLEM = 23,
    SKELETON = 24,
    ZOMBIE = 25,
    VAMPIRE = 26,
    SPECTRE = 27,
    SPRITE = 28,
    BEASTMAN = 29,
    ENT = 30,
    ARCHON = 31,
    BALROG = 32,
    DUNADAN = 33,
    S_FAIRY = 34,
    KUTAR = 35,
    ANDROID = 36,
    MERFOLK = 37,
    CAT = 38, //!< 猫。NPC専用 (プレイヤー作成時は選択不能)
    DOG = 39, //!< 犬。NPC専用
    HORSE = 40, //!< 馬。NPC専用
    BIRD = 41, //!< 鳥。NPC専用
    RAT = 42, //!< 鼠。NPC専用
    BEAR = 43, //!< 熊。NPC専用
    SNAKE = 44, //!< 蛇。NPC専用
    FISH = 45, //!< 魚。NPC専用
    INSECT = 46, //!< 昆虫。NPC専用
    SPIDER = 47, //!< 蜘蛛。NPC専用
    FROG = 48, //!< 蛙。NPC専用
    BAT = 49, //!< 蝙蝠。NPC専用
    TURTLE = 50, //!< 亀。NPC専用
    APE = 51, //!< 類人猿。NPC専用
    AQUATIC_MAMMAL = 52, //!< 水棲哺乳類。NPC専用
    DINOSAUR = 53, //!< 恐竜。NPC専用
    BOVINE = 54, //!< 牛。NPC専用
    SHARK = 55, //!< サメ。NPC専用
    HYDRA = 56, //!< ヒドラ。NPC専用
    SLUG = 57, //!< ナメクジ。NPC専用
    OCTOPUS = 58, //!< タコ。NPC専用
    SQUID = 59, //!< イカ。NPC専用
    HARPY = 60, //!< ハーピー。NPC専用
    DEER = 61, //!< 鹿。NPC専用
    ELEPHANT = 62, //!< 象。NPC専用
    LIZARD = 63, //!< トカゲ。NPC専用
    HIPPO = 64, //!< カバ。NPC専用
    BOAR = 65, //!< 猪。NPC専用
    RABBIT = 66, //!< 兎。NPC専用
    SCORPION = 67, //!< 蠍。NPC専用
    TANUKI = 68, //!< 狸。NPC専用
    SQUIRREL = 69, //!< 栗鼠。NPC専用
    WEREWOLF = 70, //!< 人狼。NPC専用
    NAGA = 71, //!< ナーガ。NPC専用
    CANCER = 72, //!< 蟹。NPC専用
    WORM = 73, //!< ワーム。NPC専用
    KRAKEN = 74, //!< クラーケン。NPC専用
    MAX,
    NONE = -1,
};

constexpr auto MAX_RACES = enum2i(PlayerRaceType::MAX);
