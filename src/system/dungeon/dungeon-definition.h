#pragma once

#include <map>
#include <string>
#include <vector>

#include "alliance/alliance.h"
#include "dungeon/dungeon-flag-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-population-flags.h"
#include "monster-race/race-resistance-mask.h"
#include "monster-race/race-speak-flags.h"
#include "monster-race/race-special-flags.h"
#include "monster-race/race-visual-flags.h"
#include "monster-race/race-wilderness-flags.h"
#include "room/room-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include "util/probability-table.h"
#include <string>
#include <tl/optional.hpp>
#include <utility>
#include <vector>

/*! @todo 後でenum classとして再定義する */
#define DUNGEON_ANGBAND 1
#define DUNGEON_GALGALS 2
#define DUNGEON_ORC 3
#define DUNGEON_MAZE 4
#define DUNGEON_DRAGON 5
#define DUNGEON_GRAVE 6
#define DUNGEON_WOOD 7
#define DUNGEON_VOLCANO 8
#define DUNGEON_HELL 9
#define DUNGEON_HEAVEN 10
#define DUNGEON_OCEAN 11
#define DUNGEON_CASTLE 12
#define DUNGEON_CTH 13
#define DUNGEON_MOUNTAIN 14
#define DUNGEON_GOLD 15
#define DUNGEON_NO_MAGIC 16
#define DUNGEON_NO_MELEE 17
#define DUNGEON_CHAMELEON 18
#define DUNGEON_DARKNESS 19
#define DUNGEON_GLASS 20
#define DUNGEON_VOID_TERRITORY 23
#define DUNGEON_MAX 31

enum class FixedArtifactId : short;
enum class MonraceId : int16_t;
enum class MonsterSex;
constexpr auto TERRAIN_PROBABILITY_NUM = 3;

enum class DungeonMode {
    AND = 1,
    NAND = 2,
    OR = 3,
    NOR = 4,
};

/* A structure for the != dungeon types */
enum class DoorKind;
enum class FixedArtifactId : short;
enum class MonraceId : short;
enum class MonsterSex;
enum class TerrainCharacteristics;
enum class TerrainTag;
class MonraceDefinition;
class DungeonDefinition {
public:
    std::string name; /* Name */
    std::string text; /* Description */

    ProbabilityTable<short> prob_table_floor{}; /* Floor probability */
    ProbabilityTable<short> prob_table_wall{}; /* Cave wall probability */
    short outer_wall{}; /* 外壁の地形ID */
    short inner_wall{}; /* 内壁の地形ID */
    FEAT_IDX stream1{}; /* stream tile */
    FEAT_IDX stream2{}; /* stream tile */

    DEPTH mindepth{}; /* Minimal depth */
    DEPTH maxdepth{}; /* Maximal depth */
    PLAYER_LEVEL min_plev{}; /* Minimal plev needed to enter -- it's an anti-cheating mesure */
    BIT_FLAGS16 pit{};
    BIT_FLAGS16 nest{};
    DungeonMode mode{}; /* Mode of combinaison of the monster flags */

    int min_m_alloc_level{}; /* Minimal number of monsters per level */
    int max_m_alloc_chance{}; /* There is a 1/max_m_alloc_chance chance per round of creating a new monster */

    EnumClassFlagGroup<DungeonFeatureType> flags{}; /* Dungeon Flags */

    EnumClassFlagGroup<MonsterFeedType> mon_meat_feed_flags;
    EnumClassFlagGroup<MonsterAbilityType> mon_ability_flags;
    EnumClassFlagGroup<MonsterBehaviorType> mon_behavior_flags;
    EnumClassFlagGroup<MonsterVisualType> mon_visual_flags;
    EnumClassFlagGroup<MonsterKindType> mon_kind_flags;
    EnumClassFlagGroup<MonsterResistanceType> mon_resistance_flags;
    EnumClassFlagGroup<MonsterDropType> mon_drop_flags;
    EnumClassFlagGroup<MonsterWildernessType> mon_wilderness_flags;
    EnumClassFlagGroup<MonsterFeatureType> mon_feature_flags;
    EnumClassFlagGroup<MonsterPopulationType> mon_population_flags;
    EnumClassFlagGroup<MonsterSpeakType> mon_speak_flags;
    EnumClassFlagGroup<MonsterBrightnessType> mon_brightness_flags;
    EnumClassFlagGroup<MonsterSpecialType> mon_special_flags;
    EnumClassFlagGroup<MonsterMiscType> mon_misc_flags;
    MonsterSex mon_sex{};

    std::vector<char> r_chars; /* Monster symbols allowed */
    short final_object{}; /* The object you'll find at the bottom */
    FixedArtifactId final_artifact{}; /* The artifact you'll find at the bottom */
    MonraceId final_guardian{}; /* The artifact's guardian. If an artifact is specified, then it's NEEDED */

    PROB special_div{}; /* % of monsters affected by the flags/races allowed, to add some variety */
    int tunnel_percent{};
    int obj_great{};
    int obj_good{};

    int monster_rate = 100; //!< トラップ生成比率
    int trap_rate = 100; //!< トラップ生成比率

    std::vector<std::tuple<int, int, int>> fixed_room_list;

    std::map<RoomType, int> room_rate; /* ダンジョン独自の部屋生成率 */
    AllianceType alliance_idx = AllianceType::NONE; /*!< 支配アライアンス */
    bool has_river_flag() const;
    bool has_guardian() const;
    const MonraceDefinition &get_guardian() const;
    short convert_terrain_id(short terrain_id, TerrainCharacteristics action) const;
    short convert_terrain_id(short terrain_id) const;
    bool is_open(short terrain_id) const;
    bool is_conquered() const;
    std::string build_entrance_message() const;
    std::string describe_depth() const;
    int calc_cavern_terrains() const;
    tl::optional<std::pair<TerrainTag, TerrainTag>> decide_river_terrains(int threshold) const;
    DoorKind select_door_kind() const;
    short select_floor_terrain_id() const;
    short select_wall_terrain_id() const;
    const Pos2D &get_position() const;

    void initialize_position(const Pos2D &pos_tokens);

    //!< @details ここから下は、地形など全ての定義ファイルを読み込んだ後に呼び出される初期化処理.
    void set_guardian_flag();

private:
    Pos2D pos = { 0, 0 };

    MonraceDefinition &get_guardian();
};
