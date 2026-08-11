/*!
 * @brief モンスターの思い出を表示する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "view/display-lore.h"
#include "alliance/alliance.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "locale/english.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "lore/monster-lore.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-era-flags.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-record.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include "world/world.h"

/*!
 * @brief モンスター情報のヘッダを記述する
 * @param monrace_id モンスターの種族ID
 */
void roff_top(MonraceId monrace_id)
{
    term_erase(0, 0);
    term_gotoxy(0, 0);

    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
#ifdef JP
#else
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        term_addstr(-1, TERM_WHITE, "The ");
    }
#endif

    if (AngbandWorld::get_instance().wizard || cheat_know) {
        term_addstr(-1, TERM_WHITE, "[");
        term_addstr(-1, TERM_L_BLUE, format("%d", enum2i(monrace_id)));
        term_addstr(-1, TERM_WHITE, "] ");
    }

    term_addstr(-1, TERM_WHITE, monrace.name);

    term_addstr(-1, TERM_WHITE, " ('");
    term_add_bigch(monrace.symbol_definition);
    term_addstr(-1, TERM_WHITE, "')");

    term_addstr(-1, TERM_WHITE, "/('");
    term_add_bigch(monrace.symbol_config);
    term_addstr(-1, TERM_WHITE, "'):");
}

/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param creature クリーチャーへの参照
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 */
void screen_roff(CreatureEntity &creature, MonraceId r_idx, monster_lore_mode mode)
{
    msg_erase();
    term_erase(0, 1);
    hook_c_roff = c_roff;
    process_monster_lore(creature, r_idx, mode);
    roff_top(r_idx);
}

/*!
 * @brief モンスター情報の現在のウィンドウに表示する /
 * Hack -- describe the given monster race in the current "term" window
 * @param creature クリーチャーへの参照
 */
void display_roff(CreatureEntity &creature)
{
    for (int y = 0; y < game_term->hgt; y++) {
        term_erase(0, y);
    }

    term_gotoxy(0, 1);
    hook_c_roff = c_roff;
    const auto &tracker = LoreTracker::get_instance();
    if (!tracker.is_tracking()) {
        return;
    }

    const auto monrace_id = tracker.get_trackee();
    process_monster_lore(creature, monrace_id, MONSTER_LORE_NORMAL);
    roff_top(monrace_id);
}

/*!
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @todo ここのroff_funcの引数にFILE* を追加しないとspoiler_file をローカル関数化することができないと判明した、保留.
 */
void output_monster_spoiler(MonraceId r_idx, hook_c_roff_pf roff_func)
{
    hook_c_roff = roff_func;
    PlayerType dummy;

    dummy.set_level(1);
    dummy.set_max_plv(1);
    process_monster_lore(dummy, r_idx, MONSTER_LORE_DEBUG);
}

static void display_killed(lore_type *lore_ptr)
{
#ifdef JP
    hooked_roff(format("このモンスターはあなたの先祖を %d 人葬っている", lore_ptr->monrace->r_deaths));
#else
    const auto present_perfect_tense = lore_ptr->monrace->r_deaths == 1 ? "has" : "have";
    hooked_roff(format("%d of your ancestors %s been killed by this creature, ", lore_ptr->monrace->r_deaths, present_perfect_tense));
#endif
    if (lore_ptr->monrace->r_pkills) {
        hooked_roff(format(_("が、あなたはこのモンスターを少なくとも %d 体は倒している。", "and you have exterminated at least %d of the creatures.  "),
            lore_ptr->monrace->r_pkills));
    } else if (lore_ptr->monrace->r_tkills) {
        hooked_roff(format(
            _("が、あなたの先祖はこのモンスターを少なくとも %d 体は倒している。", "and your ancestors have exterminated at least %d of the creatures.  "),
            lore_ptr->monrace->r_tkills));
    } else {
        hooked_roff(format(_("が、まだ%sを倒したことはない。", "and %s is not ever known to have been defeated.  "), Who::who(lore_ptr->msex).data()));
    }
}

static void display_no_killed(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->r_pkills) {
        hooked_roff(format(
            _("あなたはこのモンスターを少なくとも %d 体は殺している。", "You have killed at least %d of these creatures.  "), lore_ptr->monrace->r_pkills));
    } else if (lore_ptr->monrace->r_tkills) {
        hooked_roff(format(_("あなたの先祖はこのモンスターを少なくとも %d 体は殺している。", "Your ancestors have killed at least %d of these creatures.  "),
            lore_ptr->monrace->r_tkills));
    } else {
        hooked_roff(_("このモンスターを倒したことはない。", "No battles to the death are recalled.  "));
    }
}

/*!
 * @brief 生存数制限のあるモンスターの最大生存数を表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @details
 * 一度も倒したことのないモンスターの情報は不明。
 */
static void display_number_of_nazguls(lore_type *lore_ptr)
{
    if (lore_ptr->mode != MONSTER_LORE_DEBUG && lore_ptr->monrace->r_tkills == 0) {
        return;
    }
    if (!lore_ptr->monrace->population_flags.has(MonsterPopulationType::NAZGUL)) {
        return;
    }

    const auto remain = lore_ptr->monrace->max_num;
    const auto killed = lore_ptr->monrace->r_akills;
    if (remain == 0) {
        const auto whom = Who::whom(lore_ptr->msex, (killed > 1));
#ifdef JP
        hooked_roff(format("%sはかつて %d 体存在した。", whom.data(), killed));
#else
        hooked_roff(format("You already killed all %d of %s.  ", killed, whom.data()));
#endif
    } else {
        const auto whom = Who::whom(lore_ptr->msex, (remain + killed > 1));
#ifdef JP
        hooked_roff(format("%sはまだ %d 体生きている。", whom.data(), remain));
#else
        std::string be((remain > 1) ? "are" : "is");
        hooked_roff(format("%d of %s %s still alive.  ", remain, whom.data(), be.data()));
#endif
    }
}

void display_kill_numbers(lore_type *lore_ptr)
{
    if ((lore_ptr->mode & 0x02) != 0) {
        return;
    }

    const auto kill_unique_description = lore_ptr->build_kill_unique_description();
    if (kill_unique_description) {
        for (const auto &[text, color] : *kill_unique_description) {
            hook_c_roff(color, text);
        }
        return;
    }

    if (lore_ptr->monrace->r_deaths == 0) {
        display_no_killed(lore_ptr);
    } else {
        display_killed(lore_ptr);
    }

    display_number_of_nazguls(lore_ptr);

    hooked_roff("\n");
}

void display_where_to_appear_summary(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->level == 0) {
        hooked_roff(_("出現:町 ", "live:town "));
        lore_ptr->old = true;
    } else if (lore_ptr->monrace->r_tkills || lore_ptr->know_everything) {
        if (depth_in_feet) {
            hooked_roff(format(
                _("出現:%d フィート ", "depth:%d ft "), lore_ptr->monrace->level * 50));
        } else {
            hooked_roff(format(_("出現:%d階 ", "depth:%d F "), lore_ptr->monrace->level));
        }
    }
}

/*!
 * @brief どこに出没するかを表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @return たぬきならFALSE、それ以外はTRUE
 */
bool display_where_to_appear(lore_type *lore_ptr)
{
    lore_ptr->old = false;
    if (lore_ptr->monrace->level == 0) {
        hooked_roff(format(_("%s^は町に住み", "%s^ lives in the town"), Who::who(lore_ptr->msex).data()));
        lore_ptr->old = true;
    } else if (lore_ptr->monrace->r_tkills || lore_ptr->know_everything) {
        if (depth_in_feet) {
            hooked_roff(format(
                _("%s^は通常地下 %d フィートで出現し", "%s^ is normally found at depths of %d feet"), Who::who(lore_ptr->msex).data(), lore_ptr->monrace->level * 50));
        } else {
            hooked_roff(format(_("%s^は通常地下 %d 階で出現し", "%s^ is normally found on dungeon level %d"), Who::who(lore_ptr->msex).data(), lore_ptr->monrace->level));
        }

        lore_ptr->old = true;
    }

    if (lore_ptr->monrace_id == MonraceId::CHAMELEON) {
        hooked_roff(_("、他のモンスターに化ける。", "and can take the shape of other monster."));
        return false;
    }

    if (lore_ptr->old) {
        hooked_roff(_("、", ", and "));
    } else {
        hooked_roff(format(_("%s^は", "%s^ "), Who::who(lore_ptr->msex).data()));
        lore_ptr->old = true;
    }

    return true;
}

void display_monster_speed_summary(lore_type *lore_ptr)
{
    const int speed = lore_ptr->speed - STANDARD_SPEED;
    const auto speed_color = lore_ptr->get_speed_color();

    hook_c_roff(speed_color, format(_("速度:%+d ", "speed:%+d "), speed));
}

void display_monster_move(lore_type *lore_ptr)
{
    for (const auto &[text, color] : lore_ptr->build_speed_description()) {
        hook_c_roff(color, text);
    }
}

void display_monster_never_move(lore_type *lore_ptr)
{
    if (lore_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE)) {
        return;
    }

    if (lore_ptr->old) {
        hooked_roff(_("、しかし", ", but "));
    } else {
        hooked_roff(format(_("%s^は", "%s^ "), Who::who(lore_ptr->msex).data()));
        lore_ptr->old = true;
    }

    hooked_roff(_("侵入者を追跡しない", "does not deign to chase intruders"));
}

void display_monster_exp_summary(lore_type *lore_ptr)
{
    if ((lore_ptr->monrace->r_tkills == 0) && !lore_ptr->know_everything) {
        hooked_roff(_("経験:??? ", "Exp:??? "));
        return;
    }
    hooked_roff(format(_("経験:%d ", "Exp:%d "), lore_ptr->monrace->mexp));
}

void display_monster_kills_summary(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        if (lore_ptr->monrace->r_pkills == 0) {
            hook_c_roff(TERM_L_GREEN, _("生存 ", "alive "));
            return;
        }

        hook_c_roff(TERM_RED, _("死亡 ", "dead "));
        return;
    }

    hooked_roff(format(_("殺:%d ", "kill:%d "), lore_ptr->monrace->r_pkills));

    if (!lore_ptr->monrace->population_flags.has(MonsterPopulationType::NAZGUL)) {
        return;
    }
    hooked_roff(format(_("残:%d ", "remain:%d "), lore_ptr->monrace->max_num));
}

void display_monster_kind_tags(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        hooked_roff(_("[ユニーク]", "[UNIQ]"));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR)) {
        hook_c_roff(TERM_VIOLET, _("[狂気]", "[sanity-blasting]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        hook_c_roff(TERM_L_GREEN, _("[自然界]", "[natural]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        hook_c_roff(TERM_L_DARK, _("[邪悪]", "[evil]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        hook_c_roff(TERM_YELLOW, _("[善良]", "[good]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        hook_c_roff(TERM_VIOLET, _("[アンデッド]", "[undead]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::AMBERITE)) {
        hook_c_roff(TERM_VIOLET, _("[アンバー]", "[Amberite]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
        hook_c_roff(TERM_ORANGE, _("[ドラゴン]", "[dragon]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        hook_c_roff(TERM_VIOLET, _("[デーモン]", "[demon]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GIANT)) {
        hook_c_roff(TERM_L_UMBER, _("[巨人]", "[giant]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TROLL)) {
        hook_c_roff(TERM_BLUE, _("[トロル]", "[troll]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ORC)) {
        hook_c_roff(TERM_UMBER, _("[オーク]", "[orc]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
        hook_c_roff(TERM_L_WHITE, _("[人間]", "[human]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::QUANTUM)) {
        hook_c_roff(TERM_VIOLET, _("[量子生物]", "[quantum]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANGEL)) {
        hook_c_roff(TERM_YELLOW, _("[天使]", "[angel]"));
    }

    // [モンスター体構造] 体構造タグ (HUMANOID は記載省略)
    switch (lore_ptr->monrace->body_structure) {
    case BodyStructureType::HUMANOID:
        break; // デフォルト、記載省略
    case BodyStructureType::BIPEDAL:
        hook_c_roff(TERM_L_BLUE, _("[二足型]", "[bipedal]"));
        break;
    case BodyStructureType::QUADRUPED:
        hook_c_roff(TERM_L_BLUE, _("[四足型]", "[quadruped]"));
        break;
    case BodyStructureType::SERPENTINE:
        hook_c_roff(TERM_L_BLUE, _("[蛇型]", "[serpentine]"));
        break;
    case BodyStructureType::AMORPHOUS:
        hook_c_roff(TERM_L_BLUE, _("[不定形]", "[amorphous]"));
        break;
    case BodyStructureType::INCORPOREAL:
        hook_c_roff(TERM_L_DARK, _("[非実体]", "[incorporeal]"));
        break;
    case BodyStructureType::DRACONIC:
        hook_c_roff(TERM_ORANGE, _("[竜体]", "[draconic]"));
        break;
    case BodyStructureType::MAX:
        break;
    }

    hooked_roff("\n");
}

void display_monster_kind(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has(MonsterKindType::PAPER)) {
        hook_c_roff(TERM_WHITE, _("紙で出来た", " made of paper"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WOODEN)) {
        hook_c_roff(TERM_UMBER, _("木で出来た", " made of wood"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::IRON)) {
        hook_c_roff(TERM_SLATE, _("鉄で出来た", " made of iron"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::COPPER)) {
        hook_c_roff(TERM_ORANGE, _("銅で出来た", " made of copper"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::STONE)) {
        hook_c_roff(TERM_L_WHITE, _("石で出来た", " made of stone"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SILVER)) {
        hook_c_roff(TERM_WHITE, _("銀で出来た", " made of silver"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GOLD)) {
        hook_c_roff(TERM_YELLOW, _("金で出来た", " made of gold"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MITHRIL)) {
        hook_c_roff(TERM_L_BLUE, _("ミスリルで出来た", " made of mithril"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ADAMANTITE)) {
        hook_c_roff(TERM_L_DARK, _("アダマンタイトで出来た", " made of adamantite"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FECES)) {
        hook_c_roff(TERM_UMBER, _("糞で出来た", " made of feces"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FLESH)) {
        hook_c_roff(TERM_L_RED, _("肉で出来た", " made of flesh"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DARKSTEEL)) {
        hook_c_roff(TERM_L_DARK, _("ダークスティールで出来た", " made of darksteel"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WARPSTONE)) {
        hook_c_roff(TERM_VIOLET, _("ワープストーンで出来た", " made of warpstone"));
    }

    bool has_specific_kind = false;

    if (lore_ptr->kind_flags.has(MonsterKindType::ELDRAZI)) {
        hook_c_roff(TERM_WHITE, _("エルドラージ", " eldrazi"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ELF)) {
        hook_c_roff(TERM_GREEN, _("エルフ", " elf"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DWARF)) {
        hook_c_roff(TERM_ORANGE, _("ドワーフ", " dwarf"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HOBBIT)) {
        hook_c_roff(TERM_WHITE, _("ホビット", " hobbit"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::QUYLTHLUG)) {
        hook_c_roff(TERM_RED, _("クイルスルグ", " quylthlug"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SPIDER)) {
        hook_c_roff(TERM_SLATE, _("蜘蛛", " spider"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
        hook_c_roff(TERM_ORANGE, _("ドラゴン", " dragon"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        hook_c_roff(TERM_VIOLET, _("デーモン", " demon"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GIANT)) {
        hook_c_roff(TERM_L_UMBER, _("巨人", " giant"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TROLL)) {
        hook_c_roff(TERM_BLUE, _("トロル", " troll"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ORC)) {
        hook_c_roff(TERM_UMBER, _("オーク", " orc"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
        hook_c_roff(TERM_L_WHITE, _("人間", " human"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::QUANTUM)) {
        hook_c_roff(TERM_VIOLET, _("量子生物", " quantum creature"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANGEL)) {
        hook_c_roff(TERM_YELLOW, _("天使", " angel"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TANK)) {
        hook_c_roff(TERM_SLATE, _("戦車", " tank"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ELEMENTAL)) {
        hook_c_roff(TERM_ORANGE, _("エレメンタル", " elemental"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GOLEM)) {
        hook_c_roff(TERM_ORANGE, _("ゴーレム", " golem"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PUYO)) {
        hook_c_roff(TERM_WHITE, _("ぷよ", " puyo"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::INSECT)) {
        hook_c_roff(TERM_UMBER, _("昆虫", " insect"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ROBOT)) {
        hook_c_roff(TERM_SLATE, _("ロボット", " robot"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::YAZYU)) {
        hook_c_roff(TERM_SLATE, _("野獣先輩", " Beast Senior"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DOG)) {
        hook_c_roff(TERM_SLATE, _("犬", " dog"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::CAT)) {
        hook_c_roff(TERM_SLATE, _("猫", " cat"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::RABBIT)) {
        hook_c_roff(TERM_SLATE, _("兎", " rabbit"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PEASANT)) {
        hook_c_roff(TERM_UMBER, _("農民", " peasant"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::RABBLE)) {
        hook_c_roff(TERM_L_DARK, _("賤民", " rabble"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::NOBLE)) {
        hook_c_roff(TERM_VIOLET, _("貴族", " noble"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BEAST)) {
        hook_c_roff(TERM_ORANGE, _("ビースト", " beast"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::LEECH)) {
        hook_c_roff(TERM_L_RED, _("ヒル", " leech"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::JELLYFISH)) {
        hook_c_roff(TERM_L_BLUE, _("クラゲ", " jellyfish"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::CITIZEN)) {
        hook_c_roff(TERM_L_GREEN, _("市民", " citizen"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TREEFOLK)) {
        hook_c_roff(TERM_UMBER, _("ツリーフォーク", " treefolk"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::VIRUS)) {
        hook_c_roff(TERM_L_GREEN, _("ウイルス", " virus"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SPHINX)) {
        hook_c_roff(TERM_YELLOW, _("スフィンクス", " sphinx"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SCORPION)) {
        hook_c_roff(TERM_L_UMBER, _("蜥", " scorpion"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MINDCRAFTER)) {
        hook_c_roff(TERM_VIOLET, _("超能力者", " mindcrafter"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TANUKI)) {
        hook_c_roff(TERM_L_UMBER, _("狸", " tanuki"));
        has_specific_kind = true;
    }
    if (lore_ptr->kind_flags.has(MonsterKindType::CHAMELEON)) {
        hook_c_roff(TERM_L_GREEN, _("カメレオン", " chameleon"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::APE)) {
        hook_c_roff(TERM_SLATE, _("類人猿", " ape"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HORSE)) {
        hook_c_roff(TERM_UMBER, _("馬", " horse"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DEER)) {
        hook_c_roff(TERM_L_UMBER, _("鹿", " deer"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ELEPHANT)) {
        hook_c_roff(TERM_SLATE, _("象", " elephant"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::LIZARD)) {
        hook_c_roff(TERM_GREEN, _("トカゲ", " lizard"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::AVATAR)) {
        hook_c_roff(TERM_VIOLET, _("アヴァター", " avatar"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::NIGHTSHADE)) {
        hook_c_roff(TERM_L_DARK, _("ナイトシェード", " nightshade"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HIPPO)) {
        hook_c_roff(TERM_SLATE, _("カバ", " hippo"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BAT)) {
        hook_c_roff(TERM_L_DARK, _("コウモリ", " bat"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PLANESWALKER)) {
        hook_c_roff(TERM_VIOLET, _("プレインズウォーカー", " planeswalker"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BOAR)) {
        hook_c_roff(TERM_UMBER, _("猪", " boar"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ARCHER)) {
        hook_c_roff(TERM_BLUE, _("アーチャー", " archer"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GUNNER)) {
        hook_c_roff(TERM_L_DARK, _("ガンナー", " gunner"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SMITH)) {
        hook_c_roff(TERM_ORANGE, _("鍍冶師", " smith"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WHEEL)) {
        hook_c_roff(TERM_SLATE, _("車輪", " wheel"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GREAT_OLD_ONE)) {
        hook_c_roff(TERM_VIOLET, _("旧支配者", " Great Old One"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FROG)) {
        hook_c_roff(TERM_GREEN, _("カエル", " frog"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BEHOLDER)) {
        hook_c_roff(TERM_VIOLET, _("ビホルダー", " beholder"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::YEEK)) {
        hook_c_roff(TERM_YELLOW, _("イーク", " yeek"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::AQUATIC_MAMMAL)) {
        hook_c_roff(TERM_L_BLUE, _("水棲哺乳類", " aquatic mammal"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FISH)) {
        hook_c_roff(TERM_BLUE, _("魚類", " fish"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BIRD)) {
        hook_c_roff(TERM_L_UMBER, _("鳥", " bird"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WALL)) {
        hook_c_roff(TERM_L_DARK, _("壁", " wall"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PLANT)) {
        hook_c_roff(TERM_L_GREEN, _("植物", " plant"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FUNGUS)) {
        hook_c_roff(TERM_YELLOW, _("菌類", " fungus"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TURTLE)) {
        hook_c_roff(TERM_L_BLUE, _("亀", " turtle"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SNAKE)) {
        hook_c_roff(TERM_L_GREEN, _("蛇", " snake"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FAIRY)) {
        hook_c_roff(TERM_VIOLET, _("妖精", " fairy"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::VAMPIRE)) {
        hook_c_roff(TERM_L_DARK, _("吸血鬼", " vampire"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BEAR)) {
        hook_c_roff(TERM_UMBER, _("熊", " bear"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::VORTEX)) {
        hook_c_roff(TERM_L_BLUE, _("ボルテックス", " vortex"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::OOZE)) {
        hook_c_roff(TERM_SLATE, _("ウーズ", " ooze"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DINOSAUR)) {
        hook_c_roff(TERM_UMBER, _("恐竜", " dinosaur"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::LICH)) {
        hook_c_roff(TERM_L_DARK, _("リッチ", " lich"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GHOST)) {
        hook_c_roff(TERM_WHITE, _("幽霊", " ghost"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BERSERK)) {
        hook_c_roff(TERM_RED, _("狂戦士", " berserk"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::EXPLOSIVE)) {
        hook_c_roff(TERM_ORANGE, _("爆発物", " explosive"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::RAT)) {
        hook_c_roff(TERM_L_UMBER, _("ネズミ", " rat"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MINOTAUR)) {
        hook_c_roff(TERM_UMBER, _("ミノタウロス", " minotaur"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SKAVEN)) {
        hook_c_roff(TERM_L_RED, _("スケイヴン", " skaven"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::KOBOLD)) {
        hook_c_roff(TERM_ORANGE, _("コボルド", " kobold"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::OGRE)) {
        hook_c_roff(TERM_RED, _("オーガ", " ogre"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::BOVINE)) {
        hook_c_roff(TERM_L_UMBER, _("牛", " bovine"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MERFOLK)) {
        hook_c_roff(TERM_L_BLUE, _("マーフォーク", " merfolk"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SHARK)) {
        hook_c_roff(TERM_L_BLUE, _("サメ", " shark"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MESUGAKI)) {
        hook_c_roff(TERM_YELLOW, _("メスガキ", " mesugaki"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SAIYAN)) {
        hook_c_roff(TERM_L_RED, _("サイヤ人", " saiyan"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HYDRA)) {
        hook_c_roff(TERM_L_GREEN, _("ヒドラ", " hydra"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SHIP)) {
        hook_c_roff(TERM_UMBER, _("船舶", " ship"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SLUG)) {
        hook_c_roff(TERM_L_UMBER, _("ナメクジ", " slug"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::EYE)) {
        hook_c_roff(TERM_L_BLUE, _("目", " eye"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ALIEN)) {
        hook_c_roff(TERM_L_GREEN, _("異星人", " alien"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GRANDMA)) {
        hook_c_roff(TERM_L_UMBER, _("ババア", " grandma"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FUNGAS)) {
        hook_c_roff(TERM_L_GREEN, _("菌類", " fungus"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MIMIC)) {
        hook_c_roff(TERM_YELLOW, _("ミミック", " mimic"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::IXITXACHITL)) {
        hook_c_roff(TERM_L_BLUE, _("イクシツザチトル", " ixitxachitl"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::NAGA)) {
        hook_c_roff(TERM_ORANGE, _("ナーガ", " naga"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PERVERT)) {
        hook_c_roff(TERM_VIOLET, _("変質者", " pervert"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DEEPONE)) {
        hook_c_roff(TERM_L_BLUE, _("深きもの", " deep one"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PHYREXIAN)) {
        hook_c_roff(TERM_L_DARK, _("ファイレクシア人", " phyrexian"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HORROR)) {
        hook_c_roff(TERM_RED, _("ホラー", " horror"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WORM)) {
        hook_c_roff(TERM_YELLOW, _("ワーム", " worm"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::OCTOPUS)) {
        hook_c_roff(TERM_BLUE, _("タコ", " octopus"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SQUID)) {
        hook_c_roff(TERM_L_BLUE, _("イカ", " squid"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::FACE)) {
        hook_c_roff(TERM_L_RED, _("顔面", " face"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HAND)) {
        hook_c_roff(TERM_L_WHITE, _("手", " hand"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MINDFLAYER)) {
        hook_c_roff(TERM_VIOLET, _("マインドフレア", " mindflayer"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::NIBELUNG)) {
        hook_c_roff(TERM_YELLOW, _("ニーベルング", " nibelung"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GNOME)) {
        hook_c_roff(TERM_L_GREEN, _("ノーム", " gnome"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::KRAKEN)) {
        hook_c_roff(TERM_L_DARK, _("クラーケン", " kraken"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HARPY)) {
        hook_c_roff(TERM_L_UMBER, _("ハーピー", " harpy"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ALARM)) {
        hook_c_roff(TERM_L_RED, _("警報機", " alarm"));
        has_specific_kind = true;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WEREWOLF)) {
        hook_c_roff(TERM_L_DARK, _("人狼", " werewolf"));
        has_specific_kind = true;
    }

    // フォールバック処理：特定の種族が見つからない場合は「モンスター」と表示
    if (!has_specific_kind) {
        hooked_roff(_("モンスター", " creature"));
    }
}

void display_monster_alignment(lore_type *lore_ptr)
{
    if (lore_ptr->era_flags.has(MonsterEraType::PREHISTORIC)) {
        hook_c_roff(TERM_L_UMBER, _("先史時代級の", " prehistoric-era"));
    }

    if (lore_ptr->era_flags.has(MonsterEraType::ANCIENT)) {
        hook_c_roff(TERM_YELLOW, _("古代級の", " ancient-era"));
    }

    if (lore_ptr->era_flags.has(MonsterEraType::MEDIEVAL)) {
        hook_c_roff(TERM_L_BLUE, _("中世級の", " medieval-era"));
    }

    if (lore_ptr->era_flags.has(MonsterEraType::EARLY_MODERN)) {
        hook_c_roff(TERM_L_GREEN, _("近代級の", " early-modern-era"));
    }

    if (lore_ptr->era_flags.has(MonsterEraType::MODERN)) {
        hook_c_roff(TERM_L_WHITE, _("現代級の", " modern-era"));
    }

    if (lore_ptr->era_flags.has(MonsterEraType::INFORMATION_AGE)) {
        hook_c_roff(TERM_L_BLUE, _("情報化時代級の", " information-age"));
    }

    if (lore_ptr->era_flags.has(MonsterEraType::NANOTECH)) {
        hook_c_roff(TERM_VIOLET, _("ナノテク級の", " nanotech-era"));
    }

    /* TODO 再定義
    if (lore_ptr->msex == monster_sex::MSEX_MALE && lore_ptr->msex == monster_sex::MSEX_FEMALE) {
        hook_c_roff(TERM_VIOLET, _("両性具有であり", " hermaphroditic"));
    }
    */
    if (lore_ptr->kind_flags.has(MonsterKindType::MONKEY_SPACE)) {
        hook_c_roff(TERM_L_UMBER, _("猿空間に属する", " belonging to monkey space"));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR)) {
        hook_c_roff(TERM_VIOLET, _("狂気を誘う", " sanity-blasting"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::NASTY)) {
        hook_c_roff(TERM_L_DARK, _("クッソ汚い", " nasty"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::JOKE)) {
        hook_c_roff(TERM_L_DARK, _("ふざけた", " jokeful"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        hook_c_roff(TERM_L_GREEN, _("自然界の", " natural"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        hook_c_roff(TERM_L_DARK, _("邪悪なる", " evil"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        hook_c_roff(TERM_YELLOW, _("善良な", " good"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::WARRIOR)) {
        hook_c_roff(TERM_ORANGE, _("戦士の", " warrior"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SOLDIER)) {
        hook_c_roff(TERM_L_BLUE, _("兵士の", " soldier"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ROGUE)) {
        hook_c_roff(TERM_L_DARK, _("盗賊の", " rogue"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PRIEST)) {
        hook_c_roff(TERM_WHITE, _("プリーストの", " priest"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::MAGE)) {

        hook_c_roff(TERM_RED, _("メイジの", " mage"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::PALADIN)) {
        hook_c_roff(TERM_YELLOW, _("パラディンの", " paladin"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::RANGER)) {
        hook_c_roff(TERM_GREEN, _("レンジャーの", " ranger"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SAMURAI)) {
        hook_c_roff(TERM_RED, _("サムライの", " ranger"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::NINJA)) {
        hook_c_roff(TERM_L_DARK, _("ニンジャの", " ninja"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::KARATEKA)) {
        hook_c_roff(TERM_ORANGE, _("カラテカの", " karateka"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::YAKUZA)) {
        hook_c_roff(TERM_L_DARK, _("ヤクザな", " yakuza"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::SUMOU_WRESTLER)) {
        hook_c_roff(TERM_YELLOW, _("スモトリの", " sumou wrestler"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        hook_c_roff(TERM_VIOLET, _("アンデッドの", " undead"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::AMBERITE)) {
        hook_c_roff(TERM_VIOLET, _("アンバーの王族の", " Amberite"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::CHOASIAN)) {
        hook_c_roff(TERM_L_RED, _("混沌の王族の", " Chaosian"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HENTAI)) {
        hook_c_roff(TERM_L_RED, _("それなんてエロゲな", " comming from HENTAI world"));
    }
}

/*!
 * @brief モンスターの経験値の思い出を表示する
 * @param creature クリーチャーへの参照
 * @param lore_ptr モンスターの思い出の情報へのポインター
 */
void display_monster_exp(CreatureEntity &creature, lore_type *lore_ptr)
{
#ifdef JP
    hooked_roff("を倒すことは");
#endif

    if (lore_ptr->monrace->plus_collapse) {
#ifdef JP
        hooked_roff(format("時空崩壊度に %s%d.%06d%% の変動を与え、", lore_ptr->monrace->plus_collapse > 0 ? "+" : "-", std::abs(lore_ptr->monrace->plus_collapse / 1000000), std::abs(lore_ptr->monrace->plus_collapse % 1000000)));
#else
        hooked_roff(format(" gives %s%d.%06d%% collapse degree to world and ", lore_ptr->monrace->plus_collapse > 0 ? "+" : "-", std::abs(lore_ptr->monrace->plus_collapse / 1000000), std::abs(lore_ptr->monrace->plus_collapse % 1000000)));

#endif
    }

    int64_t base_exp = lore_ptr->monrace->mexp * lore_ptr->monrace->level * 3 / 2;
    int64_t player_factor = (int64_t)creature.get_max_plv() + 2;

    int64_t exp_integer = base_exp / player_factor;
    int64_t exp_decimal = ((base_exp % player_factor * 1000 / player_factor) + 5) / 10;

#ifdef JP
    hooked_roff(format(" %d レベルのキャラクタにとって 約%lld.%02lld ポイントの経験となる。", creature.get_level(), exp_integer, exp_decimal));
#else
    hooked_roff(format(" is worth about %lld.%02lld point%s", exp_integer, exp_decimal, ((exp_integer == 1) && (exp_decimal == 0)) ? "" : "s"));

    concptr ordinal;
    switch (creature.get_level() % 10) {
    case 1:
        ordinal = "st";
        break;
    case 2:
        ordinal = "nd";
        break;
    case 3:
        ordinal = "rd";
        break;
    default:
        ordinal = "th";
        break;
    }

    concptr vowel;
    switch (creature.get_level()) {
    case 8:
    case 11:
    case 18:
        vowel = "n";
        break;
    default:
        vowel = "";
        break;
    }

    hooked_roff(format(" for a%s %d%s level character.  ", vowel, creature.get_level(), ordinal));
#endif
}

void set_monster_aura_summary(lore_type *lore_ptr)
{
    auto has_fire_aura = lore_ptr->aura_flags.has(MonsterAuraType::FIRE);
    auto has_cold_aura = lore_ptr->aura_flags.has(MonsterAuraType::COLD);
    auto has_elec_aura = lore_ptr->aura_flags.has(MonsterAuraType::ELEC);
    auto has_dirt_aura = lore_ptr->aura_flags.has(MonsterAuraType::DIRT);

    if (has_fire_aura || has_elec_aura || has_cold_aura || has_dirt_aura) {
        lore_ptr->lore_msgs.emplace_back(_("オーラ:", "aura:"), TERM_WHITE);
    }
    if (has_fire_aura) {
        lore_ptr->lore_msgs.emplace_back(_("炎", "fire"), TERM_RED);
    }
    if (has_cold_aura) {
        lore_ptr->lore_msgs.emplace_back(_("氷", "cold"), TERM_BLUE);
    }
    if (has_elec_aura) {
        lore_ptr->lore_msgs.emplace_back(_("電", "elec"), TERM_L_BLUE);
    }
    if (has_dirt_aura) {
        lore_ptr->lore_msgs.emplace_back(_("糞", "filth"), TERM_L_DARK);
    }
    if (has_fire_aura || has_elec_aura || has_cold_aura || has_dirt_aura) {
        lore_ptr->lore_msgs.emplace_back(" | ", TERM_WHITE);
    }
}

void display_monster_aura(lore_type *lore_ptr)
{
    auto has_fire_aura = lore_ptr->aura_flags.has(MonsterAuraType::FIRE);
    auto has_elec_aura = lore_ptr->aura_flags.has(MonsterAuraType::ELEC);
    auto has_cold_aura = lore_ptr->aura_flags.has(MonsterAuraType::COLD);
    if (has_fire_aura && has_elec_aura && has_cold_aura) {
        hook_c_roff(
            TERM_VIOLET, format(_("%s^は炎と氷とスパークに包まれている。", "%s^ is surrounded by flames, ice and electricity.  "), Who::who(lore_ptr->msex).data()));
    } else if (has_fire_aura && has_elec_aura) {
        hook_c_roff(TERM_L_RED, format(_("%s^は炎とスパークに包まれている。", "%s^ is surrounded by flames and electricity.  "), Who::who(lore_ptr->msex).data()));
    } else if (has_fire_aura && has_cold_aura) {
        hook_c_roff(TERM_BLUE, format(_("%s^は炎と氷に包まれている。", "%s^ is surrounded by flames and ice.  "), Who::who(lore_ptr->msex).data()));
    } else if (has_cold_aura && has_elec_aura) {
        hook_c_roff(TERM_L_GREEN, format(_("%s^は氷とスパークに包まれている。", "%s^ is surrounded by ice and electricity.  "), Who::who(lore_ptr->msex).data()));
    } else if (has_fire_aura) {
        hook_c_roff(TERM_RED, format(_("%s^は炎に包まれている。", "%s^ is surrounded by flames.  "), Who::who(lore_ptr->msex).data()));
    } else if (has_cold_aura) {
        hook_c_roff(TERM_BLUE, format(_("%s^は氷に包まれている。", "%s^ is surrounded by ice.  "), Who::who(lore_ptr->msex).data()));
    } else if (has_elec_aura) {
        hook_c_roff(TERM_L_BLUE, format(_("%s^はスパークに包まれている。", "%s^ is surrounded by electricity.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->aura_flags.has(MonsterAuraType::DIRT)) {
        hook_c_roff(TERM_L_DARK, format(_("%s^は汚物にまみれている。", "%s^ is covered with filth.  "), Who::who(lore_ptr->msex).data()));
    }
}

void display_lore_this(CreatureEntity &creature, lore_type *lore_ptr)
{
    if ((lore_ptr->monrace->r_tkills == 0) && !lore_ptr->know_everything) {
        return;
    }

#ifdef JP
    hooked_roff("この");
#else
    if (lore_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        hooked_roff("Killing this");
    } else {
        hooked_roff("A kill of this");
    }
#endif

    if (lore_ptr->monrace->alliance_idx != AllianceType::NONE) {
#ifdef JP
        hooked_roff(alliance_list.at(lore_ptr->monrace->alliance_idx)->name.c_str());
        hooked_roff("に所属している");
#else
        hooked_roff("belonging to ");
        hooked_roff(alliance_list.at(lore_ptr->monrace->alliance_idx)->name.c_str());
#endif
    }

    display_monster_alignment(lore_ptr);
    display_monster_kind(lore_ptr);
    display_monster_exp(creature, lore_ptr);
}

static void display_monster_escort_contents(lore_type *lore_ptr)
{
    if (!lore_ptr->has_reinforce()) {
        return;
    }

    hooked_roff(_("護衛の構成は", "These escorts"));
    if (lore_ptr->misc_flags.has(MonsterMiscType::ESCORT) || lore_ptr->misc_flags.has(MonsterMiscType::MORE_ESCORT)) {
        hooked_roff(_("少なくとも", " at the least"));
    }

    const auto &reinforces = lore_ptr->monrace->get_reinforces();
#ifdef JP
#else
    hooked_roff(" contain");
    const auto max_idx = reinforces.size() - 1;
    auto idx = 0U;
#endif

    for (const auto &reinforce : reinforces) {
#ifdef JP
#else
        const std::string prefix = (idx == 0) ? " " : (idx == max_idx) ? " and "
                                                                       : ", ";
        ++idx;
#endif
        if (!reinforce.is_valid()) {
            continue;
        }

        const auto &monrace = reinforce.get_monrace();
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            hooked_roff(format("%s%s", _("、", prefix.data()), monrace.name.data()));
            continue;
        }

#ifdef JP
        hooked_roff(format("、 %s 体の%s", reinforce.get_dice_as_string().data(), monrace.name.data()));
#else
        const auto is_plural = reinforce.roll_max_dice() > 1;
        const auto &name = is_plural ? pluralize(monrace.name) : monrace.name.string();
        hooked_roff(format("%s%s %s", prefix.data(), reinforce.get_dice_as_string().data(), name.data()));
#endif
    }

    hooked_roff(_("で成り立っている。", ".  "));
}

void display_monster_collective(lore_type *lore_ptr)
{
    if (lore_ptr->misc_flags.has(MonsterMiscType::ESCORT) || lore_ptr->misc_flags.has(MonsterMiscType::MORE_ESCORT) || lore_ptr->has_reinforce()) {
        hooked_roff(format(_("%s^は通常護衛を伴って現れる。", "%s^ usually appears with escorts.  "), Who::who(lore_ptr->msex).data()));
        display_monster_escort_contents(lore_ptr);
    } else if (lore_ptr->misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
        hooked_roff(format(_("%s^は通常集団で現れる。", "%s^ usually appears in groups.  "), Who::who(lore_ptr->msex).data()));
    }
}

/*!
 * @brief モンスターの発射に関する情報を表示するルーチン /
 * Display monster launching information
 * @param creature クリーチャーへの参照
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @details
 * This function should only be called when display/dump a recall of
 * a monster.
 * @todo max_blows はゲームの中核的なパラメータの1つなのでどこかのヘッダに定数宣言しておきたい
 */
void display_monster_launching(CreatureEntity &creature, lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::ROCKET)) {
        add_lore_of_damage_skill(creature, lore_ptr, MonsterAbilityType::ROCKET, _("ロケット%sを発射する", "shoot a rocket%s"), TERM_UMBER);
        lore_ptr->rocket = true;
    }

    if (lore_ptr->ability_flags.has_not(MonsterAbilityType::SHOOT)) {
        return;
    }

    std::string msg;
    if (lore_ptr->is_details_known() || lore_ptr->know_everything) {
        msg = format(_("威力 %s の射撃をする", "fire an arrow (Power:%s)"), lore_ptr->monrace->shoot_damage_dice.to_string().data());
    } else {
        msg = _("射撃をする", "fire an arrow");
    }

    lore_ptr->lore_msgs.emplace_back(msg, TERM_UMBER);
    lore_ptr->shoot = true;
}

void display_monster_sometimes(lore_type *lore_ptr)
{
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex).data()));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != std::ssize(lore_ptr->lore_msgs) - 1) {
            const auto verb = conjugate_jverb(msg, JVerbConjugationType::OR);
            hook_c_roff(color, verb);
            hook_c_roff(color, "り");
            hooked_roff("、");
        } else {
            hook_c_roff(color, msg);
        }
#else
        if (n == 0) {
            hooked_roff(" may ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }

        hook_c_roff(color, msg);
#endif
        n++;
    }

    hooked_roff(_("ことがある。", ".  "));
}

void display_monster_dead_spawns(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->dead_spawns.empty()) {
        return;
    }

    if (!lore_ptr->know_everything && lore_ptr->monrace->r_tkills == 0) {
        return;
    }

    for (const auto &[numerator, denominator, spawn_monrace_id, dice_side, dice_num] : lore_ptr->monrace->dead_spawns) {
        const auto &spawn_monrace = MonraceList::get_instance().get_monrace(spawn_monrace_id);

#ifdef JP
        hooked_roff(format("%s^は撃破されると確率%d/%dで%dd%d体の%sを産み落とす。",
            Who::who(lore_ptr->msex).data(),
            numerator, denominator,
            dice_num, dice_side,
            spawn_monrace.name.data()));
#else
        hooked_roff(format("%s^ spawns %dd%d %s with probability %d/%d when defeated.  ",
            Who::who(lore_ptr->msex).data(),
            dice_num, dice_side,
            pluralize(spawn_monrace.name).data(),
            numerator, denominator));
#endif
    }
}

/*!
 * @brief モンスターの思い出にdrop_kind情報を表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 */
void display_drop_kind_items(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->drop_kinds.empty()) {
        return;
    }

    if (!lore_ptr->know_everything && lore_ptr->monrace->r_tkills == 0) {
        return;
    }

#ifdef JP
    hooked_roff(format("%s^は倒すと、", Who::who(lore_ptr->msex).data()));
#else
    hooked_roff(format("When defeated, %s^ may drop ", Who::who(lore_ptr->msex).data()));
#endif

    bool first = true;
    for (const auto &[numerator, denominator, item_id, grade, dice_num, dice_side] : lore_ptr->monrace->drop_kinds) {
        if (!first) {
#ifdef JP
            hooked_roff("、");
#else
            hooked_roff(", ");
#endif
        }
        first = false;

        const auto &baseitem = BaseitemList::get_instance().get_baseitem(item_id);
        const auto &item_name = baseitem.name;

        // グレード修飾語を取得
        std::string grade_modifier = "";
#ifdef JP
        if (grade == 1) {
            grade_modifier = "上質な";
        } else if (grade == 2) {
            grade_modifier = "高級品の";
        }
        hooked_roff(format("確率%d/%dで%s%sを%dd%d個", numerator, denominator, grade_modifier.data(), item_name.data(), dice_num, dice_side));
#else
        if (grade == 1) {
            grade_modifier = "excellent ";
        } else if (grade == 2) {
            grade_modifier = "premium ";
        }
        hooked_roff(format("with probability %d/%d %dd%d %s%s", numerator, denominator, dice_num, dice_side, grade_modifier.data(), item_name.data()));
#endif
    }

#ifdef JP
    hooked_roff("落とす。");
#else
    hooked_roff(".  ");
#endif
}

void display_monster_guardian(lore_type *lore_ptr)
{
    bool is_kingpin = lore_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    is_kingpin &= lore_ptr->record->has_been_seen();
    is_kingpin &= lore_ptr->monrace->max_num > 0;
    is_kingpin &= (lore_ptr->monrace_id == MonraceId::OBERON) || (lore_ptr->monrace_id == MonraceId::SERPENT);
    if (is_kingpin) {
        hook_c_roff(TERM_VIOLET, _("あなたはこのモンスターを殺したいという強い欲望を感じている...", "You feel an intense desire to kill this monster...  "));
    } else if (lore_ptr->misc_flags.has(MonsterMiscType::GUARDIAN)) {
        hook_c_roff(TERM_L_RED, _("このモンスターはダンジョンの主である。", "This monster is the master of a dungeon."));
    }

    hooked_roff("\n");
}
