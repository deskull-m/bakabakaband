#pragma once

enum class TerrainConversionType : int {
    NONE = -1, //!< 変換地形種別未定義
    FLOOR = 0,
    WALL = 1,
    INNER = 2,
    OUTER = 3,
    SOLID = 4,
    STREAM = 5,
};
