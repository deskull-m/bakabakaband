/*!
 * @brief 地形の知識の一覧に載せる地形を集める処理のテスト
 */

#include "knowledge/knowledge-features.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/finalizer.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <string>
#include <utility>
#include <vector>

namespace {

/*!
 * @brief テスト用の地形の定義
 */
struct TerrainDefine {
    std::string name; //!< 地形名
    FEAT_IDX mimic; //!< 外形地形ID
};

/*!
 * @brief 地形の一覧を指定した地形に差し替え、スコープを抜けるときに元へ戻す
 * @param defines 地形ごとの名前と外形地形ID
 */
auto replace_terrains(const std::vector<TerrainDefine> &defines)
{
    auto &terrains = TerrainList::get_instance();
    std::vector<TerrainType> backup(terrains.begin(), terrains.end());
    terrains.resize(defines.size());
    for (FEAT_IDX id = 0; const auto &[name, mimic] : defines) {
        auto &terrain = terrains.get_terrain(id);
        terrain = TerrainType{};
        terrain.idx = id;
        terrain.name = name;
        terrain.mimic = mimic;
        ++id;
    }

    return util::make_finalizer([&terrains, backup = std::move(backup)] {
        terrains.resize(backup.size());
        std::copy(backup.begin(), backup.end(), terrains.begin());
    });
}

}

TEST_CASE("collect_features lists every terrain")
{
    const auto restore = replace_terrains({ { "floor", 0 }, { "wall", 1 }, { "door", 2 } });

    CHECK(collect_features() == std::vector<FEAT_IDX>{ 0, 1, 2 });
}

TEST_CASE("collect_features skips unnamed and mimicking terrains")
{
    const auto restore = replace_terrains({ { "floor", 0 }, { "", 1 }, { "secret door", 0 }, { "wall", 3 } });

    CHECK(collect_features() == std::vector<FEAT_IDX>{ 0, 3 });
}

TEST_CASE("collect_features returns an empty list if no terrain is listed")
{
    const auto restore = replace_terrains({ { "", 0 }, { "secret door", 0 } });

    CHECK(collect_features().empty());
}
