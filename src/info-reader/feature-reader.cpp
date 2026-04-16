#include "info-reader/feature-reader.h"
#include "floor/wild.h"
#include "system/terrain/terrain-list.h"

/*!
 * @brief 地形の汎用定義をタグを通じて取得する /
 * Initialize feature variables
 */
void init_feat_variables()
{
    TerrainList::get_instance().emplace_tags();
    init_wilderness_terrains();
}
