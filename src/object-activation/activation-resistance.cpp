#include "object-activation/activation-resistance.h"
#include "effect/attribute-types.h"
#include "hpmp/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/temporary-resistance.h"
#include "sv-definition/sv-ring-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_resistance_elements(CreatureEntity &creature)
{
    msg_print(_("様々な色に輝いている...", "It glows many colours..."));
    (void)set_oppose_acid(creature, randint1(40) + 40, false);
    (void)set_oppose_elec(creature, randint1(40) + 40, false);
    (void)set_oppose_fire(creature, randint1(40) + 40, false);
    (void)set_oppose_cold(creature, randint1(40) + 40, false);
    (void)set_oppose_pois(creature, randint1(40) + 40, false);
    return true;
}

/*!
 * @brief 酸属性のボールを放ち、酸の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_acid_ball_and_resistance(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name.data());

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::ACID, dir, 100, 2);
    (void)set_oppose_acid(creature, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 電撃属性のボールを放ち、電撃の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_elec_ball_and_resistance(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name.data());

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::ELEC, dir, 100, 2);
    (void)set_oppose_elec(creature, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 火炎属性のボールを放ち、火炎の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_fire_ball_and_resistance(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name.data());

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::FIRE, dir, 100, 2);
    (void)set_oppose_fire(creature, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 冷気属性のボールを放ち、冷気の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_cold_ball_and_resistance(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name.data());

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::COLD, dir, 100, 2);
    (void)set_oppose_cold(creature, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 毒属性のボールを放ち、毒の一時耐性を得る
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 発動をキャンセルした場合FALSE、それ以外はTRUEを返す
 */
bool activate_pois_ball_and_resistance(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが緑に輝いた...", "The %s grows green."), name.data());

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_ball(creature, AttributeType::POIS, dir, 100, 2);
    (void)set_oppose_pois(creature, randint1(20) + 20, false);

    return true;
}

/*!
 * @brief 酸の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_acid(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが黒く輝いた...", "The %s grows black."), name.data());
    (void)set_oppose_acid(creature, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 電撃の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_elec(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが青く輝いた...", "The %s grows blue."), name.data());
    (void)set_oppose_elec(creature, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 火炎の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_fire(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが赤く輝いた...", "The %s grows red."), name.data());
    (void)set_oppose_fire(creature, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 冷気の一時耐性を得る。
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_cold(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが白く輝いた...", "The %s grows white."), name.data());
    (void)set_oppose_cold(creature, randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 毒の一時耐性を得る
 * @param creature クリーチャーへの参照
 * @param name アイテム名
 * @return 常にTRUE
 */
bool activate_resistance_pois(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sが緑に輝いた...", "The %s grows green."), name.data());
    (void)set_oppose_pois(creature, randint1(20) + 20, false);
    return true;
}

bool activate_ultimate_resistance(CreatureEntity &creature)
{
    TIME_EFFECT v = randint1(25) + 25;
    (void)BadStatusSetter(creature).set_fear(0);
    (void)set_hero(creature, v, false);
    (void)hp_player(creature, 10);
    (void)set_blessed(creature, v, false);
    (void)set_oppose_acid(creature, v, false);
    (void)set_oppose_elec(creature, v, false);
    (void)set_oppose_fire(creature, v, false);
    (void)set_oppose_cold(creature, v, false);
    (void)set_oppose_pois(creature, v, false);
    (void)set_ultimate_res(creature, v, false);
    return true;
}
