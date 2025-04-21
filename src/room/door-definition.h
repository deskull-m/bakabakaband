#pragma once

#include "util/enum-range.h"
#include <map>

#define DUN_ROOMS_MAX 100 /*!< 部屋生成処理の基本比率(ダンジョンのサイズに比例する) / Max number rate of rooms */

enum class DoorKind {
    DOOR = 0,
    GLASS_DOOR = 1,
    CURTAIN = 2,
};

enum class LockJam {
    LOCKED,
    JAMMED,
};

/* A structure type for doors */
enum class TerrainTag;
class Door {
public:
    Door(TerrainTag open, TerrainTag broken, TerrainTag closd, const EnumRangeInclusive<TerrainTag> &locked, const EnumRangeInclusive<TerrainTag> &jammed);

    TerrainTag open{};
    TerrainTag broken{};
    TerrainTag closed{};
    EnumRangeInclusive<TerrainTag> locked;
    EnumRangeInclusive<TerrainTag> jammed;
};

class Doors {
public:
    ~Doors() = default;
    Doors(const Doors &) = delete;
    Doors(Doors &&) = delete;
    Doors &operator=(const Doors &) = delete;
    Doors &operator=(Doors &&) = delete;
    static const Doors &get_instance();

    const Door &get_door(DoorKind dk) const;
    TerrainTag select_locked_tag(DoorKind dk) const;
    TerrainTag select_jammed_tag(DoorKind dk) const;

private:
    Doors();

    static Doors instance;
    std::map<DoorKind, Door> doors;
};
