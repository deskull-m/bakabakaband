/*!
 * @brief 魔法によるプレイヤーへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-player.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-player-switcher.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/player-status-flags.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "spell/spells-util.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include <string>

/*!
 * @brief EffectPlayerTypeクラスのコンストラクタ
 * @param floor モンスターのいるフロア
 * @param src_idx 魔法を唱えたモンスター (0ならプレイヤー自身)
 * @param dam 基本威力
 * @param attribute 効果属性
 * @param flag 効果フラグ
 */
EffectPlayerType::EffectPlayerType(const FloorType &floor, short src_idx, int dam, AttributeType attribute, BIT_FLAGS flag)
    : rlev(0)
    , m_ptr(nullptr)
    , src_idx(src_idx)
    , killer("")
    , m_name("")
    , get_damage(0)
    , dam(dam)
    , attribute(attribute)
    , flag(flag)
{
    this->src_ptr = this->is_monster() ? &floor.get_monster(src_idx) : nullptr;
}

bool EffectPlayerType::is_player() const
{
    return this->src_idx == 0;
}

bool EffectPlayerType::is_monster() const
{
    return this->src_idx > 0;
}

/*!
 * @brief ボルト魔法を反射する
 * @param creature クリーチャーへの参照
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @return 当たったらFALSE、反射したらTRUE
 */
static bool process_bolt_reflection(CreatureEntity &creature, EffectPlayerType *ep_ptr, project_func project)
{
    auto can_reflect = (creature.has_reflect()) != 0;
    can_reflect &= any_bits(ep_ptr->flag, PROJECT_REFLECTABLE);
    can_reflect &= !one_in_(10);
    if (!can_reflect) {
        return false;
    }

    auto max_attempts = 10;
    sound(SoundKind::REFLECT);

    std::string mes;
    if (creature.is_blind()) {
        mes = _("何かが跳ね返った！", "Something bounces!");
    } else if (CreatureClass(creature).samurai_stance_is(SamuraiStanceType::FUUJIN)) {
        mes = _("風の如く武器を振るって弾き返した！", "The attack bounces!");
    } else {
        mes = _("攻撃が跳ね返った！", "The attack bounces!");
    }

    msg_print(mes);
    const auto p_pos = creature.get_position();
    Pos2D pos(0, 0);
    if (ep_ptr->is_monster()) {
        const auto &floor = *creature.get_floor();
        const auto &monster = floor.get_monster(ep_ptr->src_idx);
        do {
            const Pos2DVec vec(randint1(3) - 1, randint1(3) - 1);
            pos = monster.get_position() + vec;
            max_attempts--;
        } while (max_attempts && floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE) && !projectable(floor, p_pos, pos));

        if (max_attempts < 1) {
            pos = monster.get_position();
        }
    } else {
        const Pos2DVec vec(randint1(3) - 1, randint1(3) - 1);
        pos = p_pos + vec;
    }

    (*project)(creature, 0, 0, pos.y, pos.x, ep_ptr->dam, ep_ptr->attribute, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE), tl::nullopt);
    disturb(creature, true, true);
    return true;
}

/*!
 * @brief 反射・忍者の変わり身などでそもそも当たらない状況を判定する
 * @param creature クリーチャーへの参照
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @param y 目標Y座標
 * @param x 目標X座標
 * @return 当たらなかったらFALSE、反射したらTRUE、当たったらCONTINUE
 */
static ProcessResult check_continue_player_effect(CreatureEntity &creature, EffectPlayerType *ep_ptr, const Pos2D &pos, project_func project)
{
    if (!creature.is_located_at(pos)) {
        return ProcessResult::PROCESS_FALSE;
    }

    auto is_effective = ep_ptr->dam > 0;
    is_effective &= randint0(55) < (creature.level * 3 / 5 + 20);
    is_effective &= ep_ptr->is_monster();
    is_effective &= !ep_ptr->src_ptr || !ep_ptr->src_ptr->is_riding();
    if (is_effective && kawarimi(creature, true)) {
        return ProcessResult::PROCESS_FALSE;
    }

    if (ep_ptr->is_player() || (ep_ptr->src_ptr && ep_ptr->src_ptr->is_riding())) {
        return ProcessResult::PROCESS_FALSE;
    }

    if (process_bolt_reflection(creature, ep_ptr, project)) {
        return ProcessResult::PROCESS_TRUE;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 魔法を発したモンスター名を記述する
 * @param creature クリーチャーへの参照
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @param src_name モンスター名
 */
static void describe_effect_source(CreatureEntity &creature, EffectPlayerType *ep_ptr, concptr src_name)
{
    if (ep_ptr->is_monster()) {
        ep_ptr->m_ptr = &creature.get_floor()->get_monster(ep_ptr->src_idx);
        ep_ptr->rlev = ep_ptr->m_ptr->get_monrace().level >= 1 ? ep_ptr->m_ptr->get_monrace().level : 1;
        ep_ptr->m_name = monster_desc(creature, *ep_ptr->m_ptr, 0);
        ep_ptr->killer = src_name;
        return;
    }

    switch (ep_ptr->src_idx) {
    case PROJECT_WHO_UNCTRL_POWER:
        ep_ptr->killer = _("制御できない力の氾流", "uncontrollable power storm");
        break;
    case PROJECT_WHO_GLASS_SHARDS:
        ep_ptr->killer = _("ガラスの破片", "shards of glass");
        break;
    default:
        ep_ptr->killer = _("罠", "a trap");
        break;
    }

    ep_ptr->m_name = ep_ptr->killer;
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるプレイヤーへの効果処理 / Helper function for "project()" below.
 * @param src_idx 魔法を発動したモンスター(0ならばプレイヤー、負値ならば自然発生) / Index of "source" monster (zero for "player")
 * @param src_name 効果を起こしたモンスターの名前
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param attribute 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_player(MONSTER_IDX src_idx, CreatureEntity &creature, concptr src_name, int r, POSITION y, POSITION x, int dam, AttributeType attribute,
    BIT_FLAGS flag, FallOffHorseEffect &fall_off_horse_effect, project_func project)
{
    EffectPlayerType tmp_effect(*creature.get_floor(), src_idx, dam, attribute, flag);
    auto *ep_ptr = &tmp_effect;
    auto check_result = check_continue_player_effect(creature, ep_ptr, { y, x }, project);
    if (check_result != ProcessResult::PROCESS_CONTINUE) {
        return check_result == ProcessResult::PROCESS_TRUE;
    }

    if (ep_ptr->dam > 1600) {
        ep_ptr->dam = 1600;
    }

    ep_ptr->dam = (ep_ptr->dam + r) / (r + 1);
    describe_effect_source(creature, ep_ptr, src_name);
    switch_effects_player(creature, ep_ptr);

    SpellHex(creature).store_vengeful_damage(ep_ptr->get_damage);
    if ((creature.get_timed_effect(CreatureTimedEffect::TIM_EYEEYE) || SpellHex(creature).is_spelling_specific(HEX_EYE_FOR_EYE)) && (ep_ptr->get_damage > 0) && !creature.is_dead() && ep_ptr->is_monster()) {
        const auto m_name_self = monster_desc(creature, *ep_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
        msg_print(_(format("攻撃が%s自身を傷つけた！", ep_ptr->m_name.data()), format("The attack of %s has wounded %s!", ep_ptr->m_name.data(), m_name_self.data())));
        (*project)(creature, 0, 0, ep_ptr->m_ptr->y, ep_ptr->m_ptr->x, ep_ptr->get_damage, AttributeType::MISSILE, PROJECT_KILL, tl::nullopt);
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_EYEEYE)) {
            set_tim_eyeeye(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_EYEEYE) - 5, true);
        }
    }

    if (creature.get_riding()) {
        fall_off_horse_effect.set_fall_off(ep_ptr->dam);
    }

    disturb(creature, true, true);
    if (ep_ptr->dam && ep_ptr->is_monster() && (!ep_ptr->src_ptr || !ep_ptr->src_ptr->is_riding())) {
        (void)kawarimi(creature, false);
    }

    return true;
}
