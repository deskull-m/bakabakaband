#include "market/building-craft-weapon.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-accuracy.h"
#include "combat/shoot.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "market/building-util.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 武器の各条件毎のダメージ期待値を表示する。
 * @param r 表示行
 * @param c 表示列
 * @param mindice ダイス部分最小値
 * @param maxdice ダイス部分最大値
 * @param blows 攻撃回数
 * @param dam_bonus ダメージ修正値
 * @param attr 条件内容
 * @param color 条件内容の表示色
 * @details
 * Display the damage figure of an object\n
 * (used by compare_weapon_aux)\n
 * \n
 * Only accurate for the current weapon, because it includes\n
 * the current +dam of the player.\n
 */
static void show_weapon_dmg(int r, int c, int mindice, int maxdice, int blows, int dam_bonus, concptr attr, byte color)
{
    c_put_str(color, attr, r, c);
    GAME_TEXT tmp_str[80];
    int mindam = blows * (mindice + dam_bonus);
    int maxdam = blows * (maxdice + dam_bonus);
    sprintf(tmp_str, _("１ターン: %d-%d ダメージ", "Attack: %d-%d damage"), mindam, maxdam);
    put_str(tmp_str, r, c + 8);
}

/*!
 * @brief 武器一つ毎のダメージ情報を表示する。
 * @param o_ptr オブジェクトの構造体の参照ポインタ。
 * @param col 表示する行の上端
 * @param r 表示する列の左端
 * @details
 * Show the damage figures for the various monster types\n
 * \n
 * Only accurate for the current weapon, because it includes\n
 * the current number of blows for the player.\n
 */
static void compare_weapon_aux(PlayerType *player_ptr, ItemEntity *o_ptr, int col, int r)
{
    int blow = player_ptr->num_blow[0];
    bool force = false;
    bool dokubari = false;

    int eff_dd = o_ptr->damage_dice.num + player_ptr->damage_dice_bonus[0].num;
    int eff_ds = o_ptr->damage_dice.sides + player_ptr->damage_dice_bonus[0].sides;

    int mindice = eff_dd;
    int maxdice = eff_ds * eff_dd;
    int mindam = 0;
    int maxdam = 0;
    int vorpal_mult = 1;
    int vorpal_div = 1;
    int dmg_bonus = o_ptr->to_d + player_ptr->to_d[0];

    const auto flags = o_ptr->get_flags();
    if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        dokubari = true;
    }

    bool impact = flags.has(TR_IMPACT) || (player_ptr->impact != 0);
    mindam = calc_expect_crit(player_ptr, o_ptr->weight, o_ptr->to_h, mindice, player_ptr->to_h[0], dokubari, impact);
    maxdam = calc_expect_crit(player_ptr, o_ptr->weight, o_ptr->to_h, maxdice, player_ptr->to_h[0], dokubari, impact);
    show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("会心:", "Critical:"), TERM_L_RED);
    if ((flags.has(TR_VORPAL) || SpellHex(player_ptr).is_spelling_specific(HEX_RUNESWORD))) {
        // @todo status-first-page::strengthen_basedam() と多重実装.
        if (o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
            vorpal_mult = 5;
            vorpal_div = 3;
        } else {
            vorpal_mult = 11;
            vorpal_div = 9;
        }

        mindam = calc_expect_dice(player_ptr, mindice, 1, 1, false, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 1, 1, false, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("切れ味:", "Vorpal:"), TERM_L_RED);
    }

    if (!PlayerClass(player_ptr).equals(PlayerClassType::SAMURAI) && flags.has(TR_FORCE_WEAPON) && (player_ptr->csp > (o_ptr->damage_dice.maxroll() / 5))) {
        force = true;

        mindam = calc_expect_dice(player_ptr, mindice, 1, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 1, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("理力:", "Force  :"), TERM_L_BLUE);
    }

    if (flags.has(TR_KILL_ANIMAL)) {
        mindam = calc_expect_dice(player_ptr, mindice, 4, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 4, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("動物:", "Animals:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_ANIMAL)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("動物:", "Animals:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_EVIL)) {
        mindam = calc_expect_dice(player_ptr, mindice, 7, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 7, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("邪悪:", "Evil:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_EVIL)) {
        mindam = calc_expect_dice(player_ptr, mindice, 2, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 2, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("邪悪:", "Evil:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_GOOD)) {
        mindam = calc_expect_dice(player_ptr, mindice, 7, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 7, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("善良:", "Good:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_GOOD)) {
        mindam = calc_expect_dice(player_ptr, mindice, 2, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 2, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("善良:", "Good:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_HUMAN)) {
        mindam = calc_expect_dice(player_ptr, mindice, 4, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 4, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("人間:", "Human:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_HUMAN)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("人間:", "Human:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_UNDEAD)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("不死:", "Undead:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_UNDEAD)) {
        mindam = calc_expect_dice(player_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("不死:", "Undead:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_DEMON)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("悪魔:", "Demons:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_DEMON)) {
        mindam = calc_expect_dice(player_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("悪魔:", "Demons:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_ORC)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("オーク:", "Orcs:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_ORC)) {
        mindam = calc_expect_dice(player_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("オーク:", "Orcs:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_TROLL)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("トロル:", "Trolls:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_TROLL)) {
        mindam = calc_expect_dice(player_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("トロル:", "Trolls:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_GIANT)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("巨人:", "Giants:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_GIANT)) {
        mindam = calc_expect_dice(player_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("巨人:", "Giants:"), TERM_YELLOW);
    }

    if (flags.has(TR_KILL_DRAGON)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("竜:", "Dragons:"), TERM_YELLOW);
    } else if (flags.has(TR_SLAY_DRAGON)) {
        mindam = calc_expect_dice(player_ptr, mindice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 3, 1, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("竜:", "Dragons:"), TERM_YELLOW);
    }

    if (flags.has(TR_BRAND_ACID)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("酸属性:", "Acid:"), TERM_RED);
    }

    if (flags.has(TR_BRAND_ELEC)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("電属性:", "Elec:"), TERM_RED);
    }

    if (flags.has(TR_BRAND_FIRE)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("炎属性:", "Fire:"), TERM_RED);
    }

    if (flags.has(TR_BRAND_COLD)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("冷属性:", "Cold:"), TERM_RED);
    }

    if (flags.has(TR_BRAND_POIS)) {
        mindam = calc_expect_dice(player_ptr, mindice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        maxdam = calc_expect_dice(player_ptr, maxdice, 5, 2, force, o_ptr->weight, o_ptr->to_h, player_ptr->to_h[0], dokubari, impact, vorpal_mult, vorpal_div);
        show_weapon_dmg(r++, col, mindam, maxdam, blow, dmg_bonus, _("毒属性:", "Poison:"), TERM_RED);
    }
}

/*!
 * @brief 武器匠における武器一つ毎の完全情報を表示する。
 * @param PlayerType プレイヤーへの参照ポインタ
 * @param item アイテムへの参照
 * @param row 表示する列の左端
 * @param col 表示する行の上端
 * @details
 * Displays all info about a weapon
 *
 * Only accurate for the current weapon, because it includes
 * various info about the player's +to_dam and number of blows.
 */
static void list_weapon(PlayerType *player_ptr, const ItemEntity &item, TERM_LEN row, TERM_LEN col)
{
    const auto eff_dd = item.damage_dice.num + player_ptr->damage_dice_bonus[0].num;
    const auto eff_ds = item.damage_dice.sides + player_ptr->damage_dice_bonus[0].sides;
    const auto hit_reliability = player_ptr->skill_thn + (player_ptr->to_h[0] + item.to_h) * BTH_PLUS_ADJ;
    const auto item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
    c_put_str(TERM_YELLOW, item_name, row, col);
    put_str(format(_("攻撃回数: %d", "Number of Blows: %d"), player_ptr->num_blow[0]), row + 1, col);

    put_str(_("命中率:  0  50 100 150 200 (敵のAC)", "To Hit:  0  50 100 150 200 (AC)"), row + 2, col);
    put_str(format("        %2d  %2d  %2d  %2d  %2d (%%)",
                (int)hit_chance(player_ptr, hit_reliability, 0),
                (int)hit_chance(player_ptr, hit_reliability, 50),
                (int)hit_chance(player_ptr, hit_reliability, 100),
                (int)hit_chance(player_ptr, hit_reliability, 150),
                (int)hit_chance(player_ptr, hit_reliability, 200)),
        row + 3, col);
    c_put_str(TERM_YELLOW, _("可能なダメージ:", "Possible Damage:"), row + 5, col);

    put_str(format(_("攻撃一回につき %d-%d", "One Strike: %d-%d damage"),
                (int)(eff_dd + item.to_d + player_ptr->to_d[0]),
                (int)(eff_ds * eff_dd + item.to_d + player_ptr->to_d[0])),
        row + 6, col + 1);

    put_str(format(_("１ターンにつき %d-%d", "One Attack: %d-%d damage"),
                (int)(player_ptr->num_blow[0] * (eff_dd + item.to_d + player_ptr->to_d[0])),
                (int)(player_ptr->num_blow[0] * (eff_ds * eff_dd + item.to_d + player_ptr->to_d[0]))),
        row + 7, col + 1);
}

/*!
 * @brief 武器匠鑑定1回分（オブジェクト2種）の処理。/ Compare weapons
 * @details
 * Copies the weapons to compare into the weapon-slot and\n
 * compares the values for both weapons.\n
 * 武器1つだけで比較をしないなら費用は半額になる。
 * @param bcost 基本鑑定費用
 * @return 最終的にかかった費用
 */
PRICE compare_weapons(PlayerType *player_ptr, PRICE bcost)
{
    ItemEntity *o_ptr[2]{};
    TERM_LEN row = 2;
    TERM_LEN wid = 38, mgn = 2;
    auto &world = AngbandWorld::get_instance();
    bool old_character_xtra = world.character_xtra;
    char ch;
    PRICE total = 0;
    PRICE cost = 0; /* First time no price */

    screen_save();
    clear_bldg(0, 22);
    auto *i_ptr = player_ptr->inventory[INVEN_MAIN_HAND].get();
    auto orig_weapon = i_ptr->clone();

    constexpr auto first_q = _("第一の武器は？", "What is your first weapon? ");
    constexpr auto first_s = _("比べるものがありません。", "You have nothing to compare.");

    short i_idx_first;
    constexpr auto options = USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT;
    o_ptr[0] = choose_object(player_ptr, &i_idx_first, first_q, first_s, options, FuncItemTester(&ItemEntity::is_orthodox_melee_weapons));
    if (!o_ptr[0]) {
        screen_load();
        return 0;
    }

    int n = 1;
    total = bcost;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    while (true) {
        clear_bldg(0, 22);
        world.character_xtra = true;
        for (int i = 0; i < n; i++) {
            int col = (wid * i + mgn);
            if (o_ptr[i] != i_ptr) {
                *i_ptr = o_ptr[i]->clone();
            }

            rfu.set_flag(StatusRecalculatingFlag::BONUS);
            handle_stuff(player_ptr);

            list_weapon(player_ptr, *o_ptr[i], row, col);
            compare_weapon_aux(player_ptr, o_ptr[i], col, row + 8);
            *i_ptr = orig_weapon.clone();
        }

        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        handle_stuff(player_ptr);

        world.character_xtra = old_character_xtra;
#ifdef JP
        put_str(format("[ 比較対象: 's'で変更 ($%d) ]", cost), 1, (wid + mgn));
        put_str("(一番高いダメージが適用されます。複数の倍打効果は足し算されません。)", row + 4, 0);
        prt("現在の技量から判断すると、あなたの武器は以下のような威力を発揮します:", 0, 0);
#else
        put_str(format("[ 's' Select secondary weapon($%d) ]", cost), 1, (wid + mgn));
        put_str("(Only highest damage applies per monster. Special damage not cumulative.)", row + 4, 0);
        prt("Based on your current abilities, here is what your weapons will do", 0, 0);
#endif

        flush();
        ch = inkey();
        if (ch != 's') {
            break;
        }

        if (total + cost > player_ptr->au) {
            msg_print(_("お金が足りません！", "You don't have enough money!"));
            msg_erase();
            continue;
        }

        constexpr auto q = _("第二の武器は？", "What is your second weapon? ");
        constexpr auto s = _("比べるものがありません。", "You have nothing to compare.");
        short i_idx_second;
        auto *i2_ptr = choose_object(player_ptr, &i_idx_second, q, s, (USE_EQUIP | USE_INVEN | IGNORE_BOTHHAND_SLOT), FuncItemTester(&ItemEntity::is_orthodox_melee_weapons));
        if (!i2_ptr) {
            continue;
        }

        if (i2_ptr == o_ptr[0] || (n == 2 && i2_ptr == o_ptr[1])) {
            msg_print(_("表示中の武器は選べません！", "Select a different weapon than those displayed."));
            msg_erase();
            continue;
        }

        o_ptr[1] = i2_ptr;
        total += cost;
        cost = bcost / 2;
        n = 2;
    }

    screen_load();
    return total;
}
