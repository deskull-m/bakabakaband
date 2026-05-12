#include "spell-kind/spells-pet.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ペット爆破処理 /
 */
void discharge_minion(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    // [提案 14b]
    const bool has_named_pet = creature.has_visible_creature([](const CreatureEntity &mon) {
        return mon.is_pet() && mon.is_named();
    });

    if (has_named_pet || creature.riding) {
        if (!input_check(_("本当に全ペットを爆破しますか？", "You will blast all pets. Are you sure? "))) {
            return;
        }
    }

    const auto pets = creature.collect_creatures([](const CreatureEntity &mon) {
        return mon.is_pet();
    });
    for (auto i : pets) {
        const auto &monster = floor.get_monster(i);
        const auto &monrace = monster.get_monrace();
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            const auto m_name = monster_desc(creature, monster, 0x00);
            msg_format(_("%sは爆破されるのを嫌がり、勝手に自分の世界へと帰った。", "%s^ resists being blasted and runs away."), m_name.data());
            delete_monster_idx(creature, i);
            continue;
        }

        auto dam = monster.maxhp / 2;
        if (dam > 100) {
            dam = (dam - 100) / 2 + 100;
        }
        if (dam > 400) {
            dam = (dam - 400) / 2 + 400;
        }
        if (dam > 800) {
            dam = 800;
        }
        project(creature, i, 2 + (monrace.level / 20), monster.y, monster.x, dam, AttributeType::PLASMA, PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);

        if (record_named_pet && monster.is_named()) {
            const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
            exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_BLAST, m_name);
        }

        delete_monster_idx(creature, i);
    }
}
