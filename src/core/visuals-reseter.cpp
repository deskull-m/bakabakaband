#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io/read-pref-file.h"
#include "system/baseitem/baseitem-service.h"
#include "system/creature-entity.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @param creature クリーチャーへの参照
 */
void reset_visuals(CreatureEntity &creature)
{
    for (auto &terrain : TerrainList::get_instance()) {
        for (int j = 0; j < F_LIT_MAX; j++) {
            terrain.symbol_configs[j] = terrain.symbol_definitions[j];
        }
    }

    BaseitemService::reset_all_visuals();
    MonraceList::get_instance().reset_all_visuals();
    const auto pref_file = use_graphics ? "graf.prf" : "font.prf";
    process_pref_file(creature, pref_file);
    std::stringstream ss;
    ss << (use_graphics ? "graf-" : "font-") << creature.base_name << ".prf";
    process_pref_file(creature, ss.str());
}
