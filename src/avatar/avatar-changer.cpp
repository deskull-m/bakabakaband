/*
 * @brief 敵への攻撃によって徳を変化させる処理
 * @date 2021/08/05
 * @author Hourier
 */

#include "avatar/avatar-changer.h"
#include "avatar/avatar.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-ability-mask.h"
#include "monster/monster-info.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief AvaterChangerコンストラクタ
 */
AvatarChanger::AvatarChanger(PlayerType *player_ptr, const MonsterEntity &monster)
    : player_ptr(player_ptr)
    , m_ptr(&monster)
{
}

/*!
 * @brief 徳変化処理のメインルーチン
 */
void AvatarChanger::change_virtue()
{
    this->change_virtue_non_beginner();
    this->change_virtue_unique();
    const auto &r_ref = this->m_ptr->get_real_monrace();
    if (this->m_ptr->r_idx == MonraceId::BEGGAR || this->m_ptr->r_idx == MonraceId::LEPER) {
        chg_virtue(this->player_ptr, Virtue::COMPASSION, -1);
    }

    this->change_virtue_good_evil();
    if (r_ref.kind_flags.has(MonsterKindType::UNDEAD) && r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
        chg_virtue(this->player_ptr, Virtue::VITALITY, 2);
    }

    this->change_virtue_revenge();
    if (r_ref.misc_flags.has(MonsterMiscType::MULTIPLY) && (r_ref.r_akills > 1000) && one_in_(10)) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, -1);
    }

    this->change_virtue_wild_thief();
    this->change_virtue_good_animal();
}

/*!
 * @brief 非BEGINNERダンジョン時に伴う徳の変化処理
 */
void AvatarChanger::change_virtue_non_beginner()
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monrace = this->m_ptr->get_monrace();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::BEGINNER)) {
        return;
    }

    if (!floor.is_underground() && !this->player_ptr->ambush_flag && !floor.inside_arena) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, -1);
    } else if (monrace.level > floor.dun_level) {
        if (randint1(10) <= (monrace.level - floor.dun_level)) {
            chg_virtue(this->player_ptr, Virtue::VALOUR, 1);
        }
    }

    if (monrace.level > 60) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, 1);
    }

    if (monrace.level >= 2 * (this->player_ptr->lev + 1)) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, 2);
    }
}

/*!
 * @brief ユニークを攻撃対象にした場合限定の徳変化処理
 */
void AvatarChanger::change_virtue_unique()
{
    const auto &monrace = this->m_ptr->get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return;
    }

    if (monrace.kind_flags.has_any_of(alignment_mask)) {
        chg_virtue(this->player_ptr, Virtue::HARMONY, 2);
    }

    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        chg_virtue(this->player_ptr, Virtue::UNLIFE, 2);
        chg_virtue(this->player_ptr, Virtue::VITALITY, -2);
    }

    if (one_in_(3)) {
        chg_virtue(this->player_ptr, Virtue::INDIVIDUALISM, -1);
    }
}

/*!
 * @brief 攻撃を与えたモンスターが天使か悪魔だった場合、徳を変化させる
 * @details 天使かつ悪魔だった場合、天使であることが優先される
 */
void AvatarChanger::change_virtue_good_evil()
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monrace = this->m_ptr->get_monrace();
    if (monrace.kind_flags.has(MonsterKindType::GOOD) && ((monrace.level) / 10 + (3 * floor.dun_level) >= randint1(100))) {
        chg_virtue(this->player_ptr, Virtue::UNLIFE, 1);
    }

    if (monrace.kind_flags.has(MonsterKindType::ANGEL)) {
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(this->player_ptr, Virtue::FAITH, -2);
        } else if ((monrace.level) / 10 + (3 * floor.dun_level) >= randint1(100)) {
            auto change_value = monrace.kind_flags.has(MonsterKindType::GOOD) ? -1 : 1;
            chg_virtue(this->player_ptr, Virtue::FAITH, change_value);
        }

        return;
    }

    if (monrace.kind_flags.has(MonsterKindType::DEMON)) {
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(this->player_ptr, Virtue::FAITH, 2);
        } else if ((monrace.level) / 10 + (3 * floor.dun_level) >= randint1(100)) {
            chg_virtue(this->player_ptr, Virtue::FAITH, 1);
        }
    }
}

/*!
 * @brief 過去に＠を殺したことがあるユニークにリゾンべを果たせたら徳を上げる
 */
void AvatarChanger::change_virtue_revenge()
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monrace = this->m_ptr->get_monrace();
    if (monrace.r_deaths == 0) {
        return;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        chg_virtue(this->player_ptr, Virtue::HONOUR, 10);
        return;
    }

    if ((monrace.level) / 10 + (2 * floor.dun_level) >= randint1(100)) {
        chg_virtue(this->player_ptr, Virtue::HONOUR, 1);
    }
}

/*!
 * @brief 盗み逃げをするモンスター及び地上のモンスターを攻撃した際に徳を変化させる
 */
void AvatarChanger::change_virtue_wild_thief()
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monrace = this->m_ptr->get_monrace();
    auto innocent = true;
    auto thief = false;
    for (const auto &blow : monrace.blows) {
        if (blow.damage_dice.num != 0) {
            innocent = false;
        }

        if ((blow.effect == RaceBlowEffectType::EAT_ITEM) || (blow.effect == RaceBlowEffectType::EAT_GOLD)) {
            thief = true;
        }
    }

    if (monrace.level > 0) {
        innocent = false;
    }

    if (thief) {
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(this->player_ptr, Virtue::JUSTICE, 3);
            return;
        }

        if (1 + ((monrace.level) / 10 + (2 * floor.dun_level)) >= randint1(100)) {
            chg_virtue(this->player_ptr, Virtue::JUSTICE, 1);
        }

        return;
    }

    if (innocent) {
        chg_virtue(this->player_ptr, Virtue::JUSTICE, -1);
    }
}

/*!
 * @brief 邪悪でなく、魔法も持たない動物を攻撃した時に徳を下げる
 */
void AvatarChanger::change_virtue_good_animal()
{
    const auto &monrace = this->m_ptr->get_monrace();
    auto magic_ability_flags = monrace.ability_flags;
    magic_ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
    if (monrace.kind_flags.has_not(MonsterKindType::ANIMAL) || monrace.kind_flags.has(MonsterKindType::EVIL) || magic_ability_flags.any()) {
        return;
    }

    if (one_in_(4)) {
        chg_virtue(this->player_ptr, Virtue::NATURE, -1);
    }
}
