#include "mspell/mspell-summon.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/birth-options.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "mspell/specified-summon.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/* summoning number */
constexpr int S_NUM_6 = 6;
constexpr int S_NUM_4 = 4;

/*!
 * @brief モンスターが召喚呪文を使った際にプレイヤーの連続行動を止める処理 /
 * @param creature クリーチャーへの参照
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @param known モンスターが近くにいる場合TRUE
 * @param see_either モンスターを視認可能な場合TRUE
 */
static void summon_disturb(CreatureEntity &creature, int target_type, bool known, bool see_either)
{
    bool mon_to_mon = target_type == MONSTER_TO_MONSTER;
    bool mon_to_player = target_type == MONSTER_TO_PLAYER;
    if (mon_to_player || (mon_to_mon && known && see_either)) {
        disturb(creature, true, true);
    }
}

/*!
 * @brief 特定条件のモンスター召喚のみPM_ALLOW_UNIQUEを許可する /
 * @param floor フロアへの参照
 * @param m_idx モンスターID
 * @return 召喚可能であればPM_ALLOW_UNIQUEを返す。
 */
static BIT_FLAGS monster_u_mode(const FloorType &floor, MONSTER_IDX m_idx)
{
    BIT_FLAGS u_mode = 0L;
    const auto &monster = floor.get_monster(m_idx);
    bool pet = monster.is_pet();
    if (!pet) {
        u_mode |= PM_ALLOW_UNIQUE;
    }
    return u_mode;
}

/*!
 * @brief 救援召喚の通常処理。同シンボルのモンスターを召喚する。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
static MONSTER_NUMBER summon_Kin(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    for (int k = 0; k < 4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_KIN, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    return count;
}

/*!
 * @brief 救援召喚の同アライアンス処理。同じアライアンスのモンスターを召喚する。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪法を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
static MONSTER_NUMBER summon_Alliance(CreatureEntity &creature, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    for (int k = 0; k < 4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx) ? 1 : 0;
    }

    return count;
}

static void decide_summon_kin_caster(
    CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type, concptr m_name, concptr m_poss, const bool known)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool mon_to_mon = target_type == MONSTER_TO_MONSTER;
    bool mon_to_player = target_type == MONSTER_TO_PLAYER;

    if (monster.r_idx == MonraceId::SERPENT || monster.r_idx == MonraceId::ZOMBI_SERPENT) {
        mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
            _("%s^がダンジョンの主を召喚した。", "%s^ magically summons guardians of dungeons."),
            _("%s^がダンジョンの主を召喚した。", "%s^ magically summons guardians of dungeons."));

        monspell_message(creature, m_idx, t_idx, msg, target_type);
        return;
    }

    summon_disturb(creature, target_type, known, see_either);

    if (creature.is_blind()) {
        if (mon_to_player) {
            msg_format(_("%s^が何かをつぶやいた。", "%s^ mumbles."), m_name);
        }
    } else if (mon_to_player || (mon_to_mon && known && see_either)) {
#ifdef JP
        (void)m_poss;
#endif
        _(msg_format("%sが魔法で%sを召喚した。", m_name, monster.get_monrace().get_pronoun_of_summoned_kin().data()),
            msg_format("%s^ magically summons %s %s.", m_name, m_poss, monster.get_monrace().get_pronoun_of_summoned_kin().data()));
    }

    if (mon_to_mon && known && !see_either) {
        floor.monster_noise = true;
    }
}

/*!
 * @brief RF6_S_KINの処理。救援召喚。使用するモンスターの種類により、実処理に分岐させる。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_KIN(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    DEPTH rlev = monster_level_idx(floor, m_idx);
    const auto m_name = monster_name(creature, m_idx);
    const auto m_poss = monster_desc(creature, monster, MD_PRON_VISIBLE | MD_POSSESSIVE);

    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    summon_disturb(creature, target_type, known, see_either);

    decide_summon_kin_caster(creature, m_idx, t_idx, target_type, m_name.data(), m_poss.data(), known);
    int count = 0;
    auto alliance_id = MonraceList::get_instance().get_monrace(monster.r_idx).alliance_idx;

    if (alliance_id == AllianceType::NONE) {

        switch (monster.r_idx) {
        case MonraceId::MENELDOR:
        case MonraceId::GWAIHIR:
        case MonraceId::THORONDOR:
            count += summon_EAGLE(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::BULLGATES:
            count += summon_EDGE(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::SERPENT:
        case MonraceId::ZOMBI_SERPENT:
            count += summon_guardian(creature, y, x, rlev, m_idx, t_idx, target_type);
            break;
        case MonraceId::TIAMAT:
            count += summon_HIGHEST_DRAGON(creature, y, x, m_idx);
            break;
        case MonraceId::CALDARM:
            count += summon_LOCKE_CLONE(creature, y, x, m_idx);
            break;
        case MonraceId::LOUSY:
            count += summon_LOUSE(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::VAIF:
            count += summon_MOAI(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::DESLAYER_SENIOR:
            count += summon_DEMON_SLAYER(creature, y, x, m_idx);
            break;
        case MonraceId::ALDUIN:
            count += summon_HIGHEST_DRAGON(creature, y, x, m_idx);
            break;
        case MonraceId::MIRAAK:
            count += summon_APOCRYPHA(creature, y, x, m_idx);
            break;
        case MonraceId::IMHOTEP:
            count += summon_PYRAMID(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::JOBZ:
            count += summon_EYE_PHORN(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::QUEEN_VESPOID:
            count += summon_VESPOID(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::YENDOR_WIZARD_1:
            count += summon_YENDER_WIZARD(creature, y, x, m_idx);
            break;
        case MonraceId::LEE_QIEZI:
            msg_print(_("しかし、誰も来てくれなかった…。", "However, no one answered the call..."));
            break;
        case MonraceId::THUNDERS:
            count += summon_THUNDERS(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::OOTSUKI:
            count += summon_PLASMA(creature, y, x, rlev, m_idx);
            break;
        case MonraceId::LAFFEY_II:
            count += summon_LAFFEY_II(creature, Pos2D(y, x), m_idx);
            break;
        case MonraceId::HUNGRY_OLD_MAN:
            count += summon_POLYGON(creature, y, x, m_idx);
            break;
        default:
            count += summon_Kin(creature, y, x, rlev, m_idx);
            break;
        }
    } else {
        count += summon_Alliance(creature, y, x, rlev, m_idx);
    }

    if (creature.is_blind() && count && (target_type == MONSTER_TO_PLAYER)) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (known && !see_monster(creature, t_idx) && count && (target_type == MONSTER_TO_MONSTER)) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_CYBERの処理。サイバー・デーモン召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_CYBER(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);
    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^がサイバーデーモンを召喚した！", "%s^ magically summons Cyberdemons!"),
        _("%s^がサイバーデーモンを召喚した！", "%s^ magically summons Cyberdemons!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    if (monster.is_friendly() && mon_to_mon) {
        count += summon_specific(creature, y, x, rlev, SUMMON_CYBER, (PM_ALLOW_GROUP), m_idx) ? 1 : 0;
    } else {
        count += summon_cyber(creature, y, x, m_idx);
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("重厚な足音が近くで聞こえる。", "You hear heavy steps nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_MONSTERの処理。モンスター一体召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_MONSTER(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."), _("%s^が魔法で仲間を召喚した！", "%s^ magically summons help!"),
        _("%s^が魔法で仲間を召喚した！", "%s^ magically summons help!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (int k = 0; k < 1; k++) {
        if (mon_to_player) {
            count += summon_specific(creature, y, x, rlev, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_ALLIANCE_LIMIT), m_idx) ? 1 : 0;
        }

        if (mon_to_mon) {
            count += summon_specific(creature, y, x, rlev, SUMMON_NONE, PM_ALLIANCE_LIMIT | (monster_u_mode(floor, m_idx)), m_idx) ? 1 : 0;
        }
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_MONSTERSの処理。モンスター複数召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_MONSTERS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でモンスターを召喚した！", "%s^ magically summons monsters!"), _("%s^が魔法でモンスターを召喚した！", "%s^ magically summons monsters!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_6; k++) {
        if (mon_to_player) {
            count += summon_specific(creature, y, x, rlev, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_ALLIANCE_LIMIT), m_idx) ? 1 : 0;
        }

        if (mon_to_mon) {
            count += summon_specific(creature, y, x, rlev, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | monster_u_mode(floor, m_idx)), m_idx) ? 1 : 0;
        }
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_ANTの処理。アリ召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_ANT(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."), _("%s^が魔法でアリを召喚した。", "%s^ magically summons ants."),
        _("%s^が魔法でアリを召喚した。", "%s^ magically summons ants."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_6; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_ANT, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_SPIDERの処理。クモ召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_SPIDER(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."), _("%s^が魔法でクモを召喚した。", "%s^ magically summons spiders."),
        _("%s^が魔法でクモを召喚した。", "%s^ magically summons spiders."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_6; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_SPIDER, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HOUNDの処理。ハウンド召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HOUND(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でハウンドを召喚した。", "%s^ magically summons hounds."), _("%s^が魔法でハウンドを召喚した。", "%s^ magically summons hounds."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_HOUND, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HYDRAの処理。ヒドラ召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HYDRA(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でヒドラを召喚した。", "%s^ magically summons hydras."), _("%s^が魔法でヒドラを召喚した。", "%s^ magically summons hydras."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_HYDRA, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_FAIRYの処理。フェアリー召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_FAIRY(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でフェアリーを召喚した。", "%s^ magically summons fairies."), _("%s^が魔法でフェアリーを召喚した。", "%s^ magically summons fairies."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_FAIRY, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_APEの処理。類人猿召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_APE(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で類人猿を召喚した。", "%s^ magically summons apes."), _("%s^が魔法で類人猿を召喚した。", "%s^ magically summons apes."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_APE, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_BIRDの処理。鳥召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_BIRD(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で鳥を召喚した。", "%s^ magically summons birds."), _("%s^が魔法で鳥を召喚した。", "%s^ magically summons birds."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_BIRD, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_ANGELの処理。天使一体召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_ANGEL(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で天使を召喚した！", "%s^ magically summons an angel!"), _("%s^が魔法で天使を召喚した！", "%s^ magically summons an angel!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    const auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    int num = 1;
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        num += monrace.level / 40;
    }

    int count = 0;
    for (int k = 0; k < num; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_ANGEL, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    const auto is_blind = creature.is_blind();
    if (count < 2) {
        if (is_blind && count) {
            msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
        }
    } else {
        if (is_blind) {
            msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
        }
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_DEMONの処理。デーモン一体召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_DEMON(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^は魔法で混沌の宮廷から悪魔を召喚した！", "%s^ magically summons a demon from the Courts of Chaos!"),
        _("%s^は魔法で混沌の宮廷から悪魔を召喚した！", "%s^ magically summons a demon from the Courts of Chaos!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (int k = 0; k < 1; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_DEMON, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_UNDEADの処理。アンデッド一体召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_UNDEAD(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でアンデッドの強敵を召喚した！", "%s^ magically summons an undead adversary!"),
        _("%sが魔法でアンデッドを召喚した。", "%s^ magically summons undead."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (int k = 0; k < 1; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_UNDEAD, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_DRAGONの処理。ドラゴン一体召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_DRAGON(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でドラゴンを召喚した！", "%s^ magically summons a dragon!"), _("%s^が魔法でドラゴンを召喚した！", "%s^ magically summons a dragon!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    if (mon_to_player) {
        count += summon_specific(creature, y, x, rlev, SUMMON_DRAGON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_ALLIANCE_LIMIT), m_idx) ? 1 : 0;
    }

    if (mon_to_mon) {
        count += summon_specific(creature, y, x, rlev, SUMMON_DRAGON, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | monster_u_mode(floor, m_idx)), m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HI_UNDEADの処理。強力なアンデッド召喚。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HI_UNDEAD(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    if (monster.can_ring_boss_call_nazgul() && mon_to_player) {
        count += summon_NAZGUL(creature, y, x, m_idx);
    } else {
        mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
            _("%s^が魔法で強力なアンデッドを召喚した！", "%s^ magically summons greater undead!"),
            _("%sが魔法でアンデッドを召喚した。", "%s^ magically summons undead."));

        monspell_message(creature, m_idx, t_idx, msg, target_type);

        for (auto k = 0; k < S_NUM_6; k++) {
            if (mon_to_player) {
                count += summon_specific(creature, y, x, rlev, SUMMON_HI_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_ALLIANCE_LIMIT), m_idx) ? 1 : 0;
            }

            if (mon_to_mon) {
                count += summon_specific(creature, y, x, rlev, SUMMON_HI_UNDEAD, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | monster_u_mode(floor, m_idx)), m_idx) ? 1 : 0;
            }
        }
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("間近で何か多くのものが這い回る音が聞こえる。", "You hear many creepy things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HI_DRAGONの処理。古代ドラゴン召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HI_DRAGON(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で古代ドラゴンを召喚した！", "%s^ magically summons ancient dragons!"),
        _("%s^が魔法で古代ドラゴンを召喚した！", "%s^ magically summons ancient dragons!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        if (mon_to_player) {
            count += summon_specific(creature, y, x, rlev, SUMMON_HI_DRAGON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_ALLIANCE_LIMIT), m_idx) ? 1 : 0;
        }

        if (mon_to_mon) {
            count += summon_specific(creature, y, x, rlev, SUMMON_HI_DRAGON, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | monster_u_mode(floor, m_idx)), m_idx) ? 1 : 0;
        }
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("多くの力強いものが間近に現れた音が聞こえる。", "You hear many powerful things appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_AMBERITESの処理。アンバーの王族召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_AMBERITES(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^がアンバーの王族を召喚した！", "%s^ magically summons Lords of Amber!"),
        _("%s^がアンバーの王族を召喚した！", "%s^ magically summons Lords of Amber!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_AMBERITES, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | PM_ALLOW_UNIQUE), m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("何者かが次元を超えて現れた気配がした。", "You feel shadow shifting by immortal beings."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_CHOASIANSの処理。混沌の王族召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_CHOASIANS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が混沌の王族を召喚した！", "%s^ magically summons Lords of Chaos!"),
        _("%s^が混沌の王族を召喚した！", "%s^ magically summons Lords of Chaos!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_CHOASIANS, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | PM_ALLOW_UNIQUE), m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("混沌のざわめきが響いた。", "You feel chaotic entities materializing nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_UNIQUEの処理。ユニーク・モンスター召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_UNIQUE(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で特別な強敵を召喚した！", "%s^ magically summons special opponents!"),
        _("%s^が魔法で特別な強敵を召喚した！", "%s^ magically summons special opponents!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    const auto &monster = floor.get_monster(m_idx);
    bool uniques_are_summoned = false;
    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_UNIQUE, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | PM_ALLOW_UNIQUE), m_idx) ? 1 : 0;
    }

    if (count) {
        uniques_are_summoned = true;
    }

    summon_type non_unique_type = SUMMON_HI_UNDEAD;
    if ((monster.get_monster_profile().sub_align & (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL)) == (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL)) {
        non_unique_type = SUMMON_NONE;
    } else if (monster.get_monster_profile().sub_align & SUB_ALIGN_GOOD) {
        non_unique_type = SUMMON_ANGEL;
    }

    for (auto k = count; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, non_unique_type, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | PM_ALLOW_UNIQUE), m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_format(_("多くの%sが間近に現れた音が聞こえる。", "You hear many %s appear nearby."),
            uniques_are_summoned ? _("力強いもの", "powerful things") : _("もの", "things"));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_DEAD_UNIQUEの処理。撃破済みユニーク・モンスターをクローンとして召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_DEAD_UNIQUE(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    auto rlev = monster_level_idx(floor, m_idx);
    auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    auto see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    auto known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%s^が魔法で特別な強敵を蘇らせた！", "%^s magically animates special opponents!"),
        _("%s^が魔法で特別な強敵を蘇らせた！", "%^s magically animates special opponents!"));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    auto count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_DEAD_UNIQUE, (PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT | PM_ALLOW_UNIQUE | PM_CLONE), m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_format(_("多くの力強いものが間近に蘇った音が聞こえる。", "You hear many powerful things animate nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}
/*!
 * @brief RF6_S_NASTYの処理。汚いモンスターの召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_NASTY(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^がクッソ汚いモンスターを召喚した！", "%s^ summons nasty monsters!"),
        _("%s^がクッソ汚いモンスターを召喚した！", "%s^ summons nasty monsters!"));

    const bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const bool mon_to_player = (target_type == MONSTER_TO_PLAYER);

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    auto rlev = monster_level_idx(floor, m_idx);
    auto count = 0;
    for (auto k = 0; k < std::min(1, rlev / 20); k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_NASTY, (PM_ALLOW_GROUP)) ? 1 : 0;
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon && mon_to_player) {
        msg_format(_("クッソ汚いものが間近にひしめく音が聞こえる。", "You hear many nasty things crowding nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_GOLEMの処理。ゴーレム召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_GOLEM(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でゴーレムを召喚した！", "%s^ magically summons golems!"),
        _("%s^が魔法でゴーレムを召喚した！", "%s^ magically summons golems!"));

    const bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const bool mon_to_player = (target_type == MONSTER_TO_PLAYER);

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    auto rlev = monster_level_idx(floor, m_idx);
    auto count = 0;
    for (auto k = 0; k < std::min(1, rlev / 25); k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_GOLEM, (PM_ALLOW_GROUP)) ? 1 : 0;
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon && mon_to_player) {
        msg_format(_("ゴーレムが間近にひしめく音が聞こえる。", "You hear many golems crowding nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_CATSの処理。猫召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_CATS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で猫を召喚した！", "%s^ magically summons cats!"),
        _("%s^が魔法で猫を召喚した！", "%s^ magically summons cats!"));

    const bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const bool mon_to_player = (target_type == MONSTER_TO_PLAYER);

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    auto rlev = monster_level_idx(floor, m_idx);
    auto count = 0;
    for (auto k = 0; k < std::max(1, rlev / 25); k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_CATS, (PM_ALLOW_GROUP)) ? 1 : 0;
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon && mon_to_player) {
        msg_format(_("猫が間近にたくさんいる音が聞こえる。", "You hear many cats crowding nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_PERVERTSの処理。変質者召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_PERVERTS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で変質者を召喚した！", "%s^ magically summons perverts!"),
        _("%s^が魔法で変質者を召喚した！", "%s^ magically summons perverts!"));

    const bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const bool mon_to_player = (target_type == MONSTER_TO_PLAYER);

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    auto rlev = monster_level_idx(floor, m_idx);
    auto count = 0;
    for (auto k = 0; k < std::max(1, rlev / 25); k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_PERVERTS, (PM_ALLOW_GROUP)) ? 1 : 0;
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon && mon_to_player) {
        msg_format(_("変質者が間近にたくさんいる気配を感じる。", "You sense many perverts crowding nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_PUYOの処理。ぷよ召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_PUYO(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でぷよを召喚した！", "%s^ magically summons puyo!"),
        _("%s^が魔法でぷよを召喚した！", "%s^ magically summons puyo!"));

    const bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const bool mon_to_player = (target_type == MONSTER_TO_PLAYER);

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    auto rlev = monster_level_idx(floor, m_idx);
    auto count = 0;
    for (auto k = 0; k < std::max(1, rlev / 25); k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_PUYO, (PM_ALLOW_GROUP)) ? 1 : 0;
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon && mon_to_player) {
        msg_format(_("ぷよが間近にたくさんいる音が聞こえる。", "You hear many puyo crowding nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}
/*!
 * @brief RF6_S_HOMOの処理。ホモ召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_HOMO(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法でホモを召喚した！", "%s^ magically summons Gays!"),
        _("%s^が魔法でホモを召喚した！", "%s^ magically summons Gays!"));

    const bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const bool mon_to_player = (target_type == MONSTER_TO_PLAYER);

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    auto rlev = monster_level_idx(floor, m_idx);
    auto count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        if (mon_to_player) {
            count += summon_specific(creature, y, x, rlev, SUMMON_HOMO, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_ALLIANCE_LIMIT), m_idx) ? 1 : 0;
        }

        if (mon_to_mon) {
            count += summon_specific(creature, y, x, rlev, SUMMON_HOMO, PM_ALLIANCE_LIMIT | (monster_u_mode(floor, m_idx)), m_idx) ? 1 : 0;
        }
    }

    if (creature.is_blind() && count && mon_to_player) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}
/*!
 * @brief RF6_S_WALLの処理。壁一体召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_WALL(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で壁を召喚した！", "%s^ magically summons walls!"),
        _("%sが魔法で壁を召喚した。", "%s^ magically summons walls."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (int k = 0; k < 1; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_WALL, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_INSECTの処理。昆虫召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_INSECT(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    bool known = monster_near_player(creature, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が魔法で昆虫を召喚した！", "%s^ magically summons insects!"),
        _("%sが魔法で昆虫を召喚した。", "%s^ magically summons insects."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (int k = 0; k < 1; k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_INSECT, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_ELDRAZIの処理。エルドラージ召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
MonsterSpellResult spell_RF6_S_ELDRAZI(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto &floor = *creature.get_floor();
    auto rlev = monster_level_idx(floor, m_idx);
    const auto known = monster_near_player(creature, m_idx, t_idx);
    const auto see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    const auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);

    mspell_cast_msg_blind msg(_("%sが何かをつぶやいた。", "%s^ mumbles."),
        _("%sが魔法でエルドラージを召喚した！", "%s^ magically summons Eldrazi!"),
        _("%sが魔法でエルドラージを召喚した。", "%s^ magically summons Eldrazi."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);
    summon_disturb(creature, target_type, known, see_either);

    int count = 0;
    for (int k = 0; k < std::max(1, rlev / 30); k++) {
        count += summon_specific(creature, y, x, rlev, SUMMON_ELDRAZI, PM_ALLOW_GROUP | PM_ALLIANCE_LIMIT, m_idx) ? 1 : 0;
    }

    if (creature.is_blind() && count) {
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }

    if (monster_near_player(creature, m_idx, t_idx) && !see_monster(creature, t_idx) && count && mon_to_mon) {
        floor.monster_noise = true;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}
