/*!
 * @brief ボルトでもボールでもブレスでもなく、ダメージを与える特殊なスペルの実行 /
 * Performing special spells that take damage, not bolts, balls or breaths
 * @date 2020/05/16
 * @author Hourier
 * @details 肥大化しやすいファイル名なので、関数の追加時は共通部分を別ファイルに抜き出せるか検討すること /
 * This is a filename that tends to be bloated.
 * So when adding a function, please consider whether you can extract the common part to another file.
 */

#include "mspell/mspell-attack/mspell-particularity.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/creature-entity.h"

MSpellAttackOther::MSpellAttackOther(CreatureEntity &creature, MONSTER_IDX m_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire)
    : AbstractMSpellAttack(creature, m_idx, ability, data, target_type, fire)
{
}

MSpellAttackOther::MSpellAttackOther(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire)
    : AbstractMSpellAttack(creature, m_idx, t_idx, ability, data, target_type, fire)
{
}

/*!
 * @brief RF4_ROCKETの処理。ロケット。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_ROCKET(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto data = MSpellData({ _("%s^が何かを射った。", "%s^ shoots something."),
                                     _("%s^がロケットを発射した。", "%s^ fires a rocket."),
                                     _("%s^が%sにロケットを発射した。", "%s^ fires a rocket at %s.") },
        AttributeType::ROCKET, DRS_SHARD);
    return MSpellAttackOther(creature, m_idx, t_idx, MonsterAbilityType::ROCKET, data, target_type,
        [creature_ptr = &creature, m_idx, target_type](auto y, auto x, int dam, auto attribute) { return rocket(*creature_ptr, y, x, m_idx, attribute, dam, 2, target_type); })
        .shoot(y, x);
}

static bool message_hand_doom(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_simple msg(_("%s^が<破滅の手>を放った！", "%s^ invokes the Hand of Doom!"),
        _("%s^が%sに<破滅の手>を放った！", "%s^ invokes the Hand of Doom upon %s!"));

    simple_monspell_message(creature, m_idx, t_idx, msg, target_type);

    return true;
}

static auto project_hand_doom(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION y, POSITION x, int target_type)
{
    ProjectResult proj_res;
    auto attribute = AttributeType::HAND_DOOM;
    if (target_type == MONSTER_TO_PLAYER) {
        const auto dam = monspell_damage(creature, MonsterAbilityType::HAND_DOOM, m_idx, DAM_ROLL);
        proj_res = pointed(creature, y, x, m_idx, attribute, dam, MONSTER_TO_PLAYER);
    } else if (target_type == MONSTER_TO_MONSTER) {
        const auto dam = 20; /* Dummy power */
        proj_res = pointed(creature, y, x, m_idx, attribute, dam, MONSTER_TO_MONSTER);
    }
    return proj_res;
}

/*!
 * @brief RF6_HAND_DOOMの処理。破滅の手。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF6_HAND_DOOM(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    return MSpellAttackOther(creature, m_idx, t_idx, MonsterAbilityType::HAND_DOOM, { message_hand_doom, AttributeType::MAX }, target_type,
        [creature_ptr = &creature, m_idx, target_type](auto y, auto x, int, AttributeType) { return project_hand_doom(*creature_ptr, m_idx, y, x, target_type); })
        .shoot(y, x);
}

/*!
 * @brief RF6_PSY_SPEARの処理。光の剣。 /
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF6_PSY_SPEAR(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto data = MSpellData({ _("%s^が何かをつぶやいた。", "%s^ mumbles."), _("%s^が光の剣を放った。", "%s^ throws a Psycho-Spear."),
                                     _("%s^が%sに向かって光の剣を放った。", "%s^ throws a Psycho-spear at %s.") },
        AttributeType::PSY_SPEAR);

    return MSpellAttackOther(creature, m_idx, t_idx, MonsterAbilityType::PSY_SPEAR, data, target_type,
        [creature_ptr = &creature, m_idx, target_type](auto y, auto x, int dam, auto attribute) { return beam(*creature_ptr, m_idx, y, x, attribute, dam, target_type); })
        .shoot(y, x);
}
