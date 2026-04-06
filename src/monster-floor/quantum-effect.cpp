#include "monster-floor/quantum-effect.h"
#include "effect/attribute-types.h"
#include "floor/line-of-sight.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/mspell-result.h"
#include "spell-kind/spells-teleport.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ユニークでない量子生物を消滅させる
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 */
static void vanish_nonunique(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m)
{
    const auto &monster = creature.current_floor_ptr->get_monster(m_idx);
    if (see_m) {
        const auto m_name = monster_desc(creature, monster, 0);
        msg_format(_("%sは消え去った！", "%s^ disappears!"), m_name.data());
    }

    monster_death(creature, m_idx, false, AttributeType::QUANTUM_VANISH);
    delete_monster_idx(creature, m_idx);
    if (monster.is_pet() && !(monster.get_monster_profile().ml)) {
        msg_print(_("少しの間悲しい気分になった。", "You feel sad for a moment."));
    }
}

/*!
 * @brief 量子生物ユニークの量子的効果 (ショート・テレポートまたは距離10のテレポート・アウェイ)を実行する
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @details
 * プレイヤーが量子生物を観測しているか、量子生物がプレイヤーを観測している場合、互いの相対的な位置を確定させる
 * 波動関数の収縮はテレポートではないので反テレポート無効
 * @notes
 * パターンは収縮どころか拡散しているが、この際気にしてはいけない
 * @todo ユニークとプレイヤーとの間でしか効果が発生しない。ユニークとその他のモンスター間では何もしなくてよい？
 */
static void produce_quantum_effect(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m)
{
    const auto &floor = *creature.current_floor_ptr;
    const auto &monster = floor.get_monster(m_idx);
    const auto coherent = los(floor, monster.get_position(), creature.get_position());
    if (!see_m && !coherent) {
        return;
    }

    if (see_m) {
        const auto m_name = monster_desc(creature, monster, MD_NONE);
        msg_format(_("%sは量子的効果を起こした！", "%s^ produced a decoherence!"), m_name.data());
    } else {
        msg_print(_("量子的効果が起こった！", "A decoherence was produced!"));
    }

    bool target = one_in_(2);
    if (target) {
        (void)monspell_to_monster(creature, MonsterAbilityType::BLINK, monster.y, monster.x, m_idx, m_idx, true);
    } else {
        teleport_player_away(m_idx, creature, 10, true);
    }
}

/*!
 * @brief 量子生物の量子的効果を実行する
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @return モンスターが量子的効果により消滅したらTRUE
 */
bool process_quantum_effect(CreatureEntity &creature, MONSTER_IDX m_idx, bool see_m)
{
    const auto &monster = creature.current_floor_ptr->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::QUANTUM) && monrace.kind_flags.has_not(MonsterKindType::MONKEY_SPACE)) {
        return false;
    }
    if (!randint0(2)) {
        return false;
    }
    if (randint0((m_idx % 100) + 10)) {
        return false;
    }

    bool can_disappear = monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
    can_disappear &= monrace.misc_flags.has_not(MonsterMiscType::QUESTOR);
    if (can_disappear) {
        vanish_nonunique(creature, m_idx, see_m);
        return true;
    }

    produce_quantum_effect(creature, m_idx, see_m);
    return false;
}
