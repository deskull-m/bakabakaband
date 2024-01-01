﻿#include "flavor/flavor-util.h"
#include "flavor/flag-inscriptions-table.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "sv-definition/sv-food-types.h"
#include "system/artifact-type-definition.h"
#include "system/item-entity.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include <sstream>

static bool has_lite_flag(const TrFlags &flags)
{
    return flags.has(TR_LITE_1) || flags.has(TR_LITE_2) || flags.has(TR_LITE_3);
}

static bool has_dark_flag(const TrFlags &flags)
{
    return flags.has(TR_LITE_M1) || flags.has(TR_LITE_M2) || flags.has(TR_LITE_M3);
}

/*!
 * @brief オブジェクトフラグを追加する
 * @param short_flavor フラグの短縮表現 (反魔法/アンチテレポの"["、耐性の"r"、耐混乱の"乱" 等)
 * @param ptr 特性短縮表記を格納する文字列ポインタ
 *
 * @todo バッファサイズが不明なのでとりあえず16バイト決め打ちで angband_strcpy を呼び出している。
 * get_ability_abbrev のインターフェースを std::string を返すように変更するときに合わせて修正すること。
 */
static void add_inscription(char **short_flavor, concptr str)
{
    *short_flavor += angband_strcpy(*short_flavor, str, 16);
}

/*!
 * @brief get_inscriptionのサブセットとしてアイテムの特性フラグを表す文字列を返す
 * @param fi_vec 参照する特性表示記号テーブル
 * @param flags 対応するアイテムの特性フラグ
 * @param is_kanji trueならば漢字記述/falseならば英語記述
 * @return アイテムの特性フラグを表す文字列
 */
static std::string inscribe_flags_aux(const std::vector<flag_insc_table> &fi_vec, const TrFlags &flags, [[maybe_unused]] bool is_kanji)
{
    std::stringstream ss;

    for (const auto &fi : fi_vec) {
        if (flags.has(fi.flag) && (!fi.except_flag.has_value() || flags.has_not(fi.except_flag.value()))) {
            const auto flag_str = _(is_kanji ? fi.japanese : fi.english, fi.english);
            ss << flag_str;
        }
    }

    return ss.str();
}

/*!
 * @brief オブジェクトの特性表示記号テーブル1つに従いオブジェクトの特性フラグ配列に1つでも該当の特性があるかを返す / Special variation of has_flag for
 * auto-inscription
 * @param fi_vec 参照する特性表示記号テーブル
 * @param flags 対応するオブジェクトのフラグ文字列
 * @return 1つでも該当の特性があったらTRUEを返す。
 */
static bool has_flag_of(const std::vector<flag_insc_table> &fi_vec, const TrFlags &flags)
{
    for (const auto &fi : fi_vec) {
        if (flags.has(fi.flag) && (!fi.except_flag.has_value() || flags.has_not(fi.except_flag.value()))) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief アイテムの特性短縮表記をまとめて提示する。
 * @param item 特性短縮表記を得たいアイテムの参照
 * @param is_kanji trueならば漢字表記 / falseなら英語表記
 * @param all falseならばベースアイテム上で明らかなフラグは省略する
 * @return アイテムの特性短縮表記
 */
std::string get_ability_abbreviation(const ItemEntity &item, bool is_kanji, bool all)
{
    auto flags = object_flags(&item);
    if (!all) {
        const auto &baseitem = baseitems_info[item.bi_id];
        flags.reset(baseitem.flags);

        if (item.is_fixed_artifact()) {
            const auto &a_ref = artifacts_info.at(item.fixed_artifact_idx);
            flags.reset(a_ref.flags);
        }

        if (item.is_ego()) {
            auto *e_ptr = &egos_info[item.ego_idx];
            flags.reset(e_ptr->flags);
        }
    }

    if (has_dark_flag(flags)) {
        if (flags.has(TR_LITE_1)) {
            flags.reset(TR_LITE_1);
        }

        if (flags.has(TR_LITE_2)) {
            flags.reset(TR_LITE_2);
        }

        if (flags.has(TR_LITE_3)) {
            flags.reset(TR_LITE_3);
        }
    } else if (has_lite_flag(flags)) {
        flags.set(TR_LITE_1);
        if (flags.has(TR_LITE_2)) {
            flags.reset(TR_LITE_2);
        }

        if (flags.has(TR_LITE_3)) {
            flags.reset(TR_LITE_3);
        }
    }

    std::stringstream ss;
    auto prev_tellp = ss.tellp();

    if (has_flag_of(flag_insc_plus, flags) && is_kanji) {
        ss << '+';
    }

    ss << inscribe_flags_aux(flag_insc_plus, flags, is_kanji);

    if (has_flag_of(flag_insc_immune, flags)) {
        if (!is_kanji && (ss.tellp() != prev_tellp)) {
            ss << ';';
            prev_tellp = ss.tellp();
        }

        ss << '*';
    }

    ss << inscribe_flags_aux(flag_insc_immune, flags, is_kanji);

    if (has_flag_of(flag_insc_vuln, flags)) {
        if (!is_kanji && (ss.tellp() != prev_tellp)) {
            ss << ';';
            prev_tellp = ss.tellp();
        }

        ss << 'v';
    }

    ss << inscribe_flags_aux(flag_insc_vuln, flags, is_kanji);

    if (has_flag_of(flag_insc_resistance, flags)) {
        if (is_kanji) {
            ss << 'r';
        } else if (ss.tellp() != prev_tellp) {
            ss << ';';
            prev_tellp = ss.tellp();
        }
    }

    ss << inscribe_flags_aux(flag_insc_resistance, flags, is_kanji);

    if (has_flag_of(flag_insc_misc, flags) && (ss.tellp() != prev_tellp)) {
        ss << ';';
        prev_tellp = ss.tellp();
    }

    ss << inscribe_flags_aux(flag_insc_misc, flags, is_kanji);

    if (has_flag_of(flag_insc_aura, flags)) {
        ss << '[';
    }

    ss << inscribe_flags_aux(flag_insc_aura, flags, is_kanji);

    if (has_flag_of(flag_insc_brand, flags)) {
        ss << '|';
    }

    ss << inscribe_flags_aux(flag_insc_brand, flags, is_kanji);

    if (has_flag_of(flag_insc_kill, flags)) {
        ss << "/X";
    }

    ss << inscribe_flags_aux(flag_insc_kill, flags, is_kanji);

    if (has_flag_of(flag_insc_slay, flags)) {
        ss << '/';
    }

    ss << inscribe_flags_aux(flag_insc_slay, flags, is_kanji);

    if (is_kanji) {
        if (has_flag_of(flag_insc_esp1, flags) || has_flag_of(flag_insc_esp2, flags)) {
            ss << '~';
        }

        ss << inscribe_flags_aux(flag_insc_esp1, flags, is_kanji)
           << inscribe_flags_aux(flag_insc_esp2, flags, is_kanji);
    } else {
        if (has_flag_of(flag_insc_esp1, flags)) {
            ss << '~';
        }

        ss << inscribe_flags_aux(flag_insc_esp1, flags, is_kanji);

        if (has_flag_of(flag_insc_esp2, flags)) {
            ss << '~';
        }

        ss << inscribe_flags_aux(flag_insc_esp2, flags, is_kanji);
    }

    if (has_flag_of(flag_insc_sust, flags)) {
        ss << '(';
    }

    ss << inscribe_flags_aux(flag_insc_sust, flags, is_kanji);

    return ss.str();
}

/*!
 * @brief オブジェクト名の特性短縮表記＋刻み内容を提示する。 / Get object inscription with auto inscription of object flags.
 * @param buff 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 */
void get_inscription(char *buff, const ItemEntity *o_ptr)
{
    concptr insc = quark_str(o_ptr->inscription);
    char *ptr = buff;
    if (!o_ptr->is_fully_known()) {
        while (*insc) {
            if (*insc == '#') {
                break;
            }
            if (buff > ptr + MAX_INSCRIPTION - 1) {
                break;
            }
#ifdef JP
            if (iskanji(*insc)) {
                *buff++ = *insc++;
            }
#endif
            *buff++ = *insc++;
        }

        *buff = '\0';
        return;
    }

    *buff = '\0';
    for (; *insc; insc++) {
        if (*insc == '#') {
            break;
        } else if ('%' == *insc) {
            bool kanji = false;
            bool all;
            concptr start = ptr;
            if (ptr >= buff + MAX_NLEN) {
                continue;
            }

#ifdef JP
            if ('%' == insc[1]) {
                insc++;
                kanji = false;
            } else {
                kanji = true;
            }
#endif

            if ('a' == insc[1] && 'l' == insc[2] && 'l' == insc[3]) {
                all = true;
                insc += 3;
            } else {
                all = false;
            }

            const auto ability_abbrev = get_ability_abbreviation(*o_ptr, kanji, all);
            ptr += angband_strcpy(ptr, ability_abbrev.data(), ability_abbrev.size());
            if (ptr == start) {
                add_inscription(&ptr, " ");
            }
        } else {
            *ptr++ = *insc;
        }
    }

    *ptr = '\0';
}

#ifdef JP
/*!
 * @brief アイテムにふさわしい助数詞をつけて数を記述する
 * @param item 数を記述したいアイテムの参照
 * @return 記述した文字列
 */
std::string describe_count_with_counter_suffix(const ItemEntity &item)
{
    std::stringstream ss;
    ss << item.number;

    switch (item.bi_key.tval()) {
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::POLEARM:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::DIGGING:
        ss << "本";
        break;
    case ItemKindType::SCROLL:
        ss << "巻";
        break;
    case ItemKindType::POTION:
        ss << "服";
        break;
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
        ss << "冊";
        break;
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::CLOAK:
        ss << "着";
        break;
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::BOW:
        ss << "振";
        break;
    case ItemKindType::BOOTS:
        ss << "足";
        break;

    case ItemKindType::CARD:
        ss << "枚";
        break;

    case ItemKindType::FOOD:
        if (item.bi_key.sval().value() == SV_FOOD_JERKY) {
            ss << "切れ";
            break;
        }
        [[fallthrough]];
    default:
        if (item.number < 10) {
            ss << "つ";
        } else {
            ss << "個";
        }
        break;
    }

    return ss.str();
}
#endif
