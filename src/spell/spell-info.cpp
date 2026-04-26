#include "spell/spell-info.h"
#include "io/input-key-requester.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-spell-status.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "realm/realm-types.h"
#include "spell/spells-execution.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 呪文の消費MPを返す /
 * Modify mana consumption rate using spell exp and dec_mana
 * @param creature クリーチャーへの参照
 * @param need_mana 基本消費MP
 * @param spell_id 呪文ID
 * @param realm 魔法領域
 * @return 消費MP
 */
MANA_POINT mod_need_mana(CreatureEntity &creature, MANA_POINT need_mana, SPELL_IDX spell_id, RealmType realm)
{
#define MANA_CONST 2400
#define MANA_DIV 4
#define DEC_MANA_DIV 3
    if (PlayerRealm::is_magic(realm) || PlayerRealm::is_technic(realm)) {
        need_mana = need_mana * (MANA_CONST + PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT) - PlayerSkill(creature).exp_of_spell(realm, spell_id)) + (MANA_CONST - 1);
        need_mana *= creature.has_dec_mana() ? DEC_MANA_DIV : MANA_DIV;
        need_mana /= MANA_CONST * MANA_DIV;
        if (need_mana < 1) {
            need_mana = 1;
        }
    } else {
        if (creature.has_dec_mana()) {
            need_mana = (need_mana + 1) * DEC_MANA_DIV / MANA_DIV;
        }
    }

#undef DEC_MANA_DIV
#undef MANA_DIV
#undef MANA_CONST

    return need_mana;
}

/*!
 * @brief 呪文の失敗率修正処理1(呪い、消費魔力減少、呪文簡易化) /
 * Modify spell fail rate
 * Using to_m_chance, dec_mana, easy_spell and heavy_spell
 * @param creature クリーチャーへの参照
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_1(CreatureEntity &creature, PERCENTAGE chance)
{
    chance += creature.to_m_chance;

    if (creature.has_hard_spell()) {
        chance += 20;
    }

    if (creature.has_dec_mana() && creature.has_easy_spell()) {
        chance -= 4;
    } else if (creature.has_easy_spell()) {
        chance -= 3;
    } else if (creature.has_dec_mana()) {
        chance -= 2;
    }

    return chance;
}

/*!
 * @brief 呪文の失敗率修正処理2(消費魔力減少、呪い、負値修正) /
 * Modify spell fail rate
 * Using to_m_chance, dec_mana, easy_spell and heavy_spell
 * @param creature クリーチャーへの参照
 * @param chance 修正前失敗率
 * @return 失敗率(%)
 * Modify spell fail rate (as "suffix" process)
 * Using dec_mana, easy_spell and heavy_spell
 * Note: variable "chance" cannot be negative.
 * @todo 統合を検討
 */
PERCENTAGE mod_spell_chance_2(CreatureEntity &creature, PERCENTAGE chance)
{
    if (creature.has_dec_mana()) {
        chance--;
    }
    if (creature.has_hard_spell()) {
        chance += 5;
    }
    return std::max(chance, 0);
}

/*!
 * @brief 呪文の失敗率計算メインルーチン /
 * Returns spell chance of failure for spell -RAK-
 * @param creature クリーチャーへの参照
 * @param spell_id 呪文ID
 * @param use_realm 魔法領域ID
 * @return 失敗率(%)
 */
PERCENTAGE spell_chance(CreatureEntity &creature, SPELL_IDX spell_id, RealmType use_realm)
{
    if (mp_ptr->spell_book == ItemKindType::NONE) {
        return 100;
    }
    if (use_realm == RealmType::HISSATSU) {
        return 0;
    }

    const auto &spell = PlayerRealm::get_spell_info(use_realm, spell_id);

    PERCENTAGE chance = spell.sfail;
    chance -= 3 * (creature.level - spell.slevel);
    chance -= 3 * (adj_mag_stat[creature.stat_index[mp_ptr->spell_stat]] - 1);
    if (creature.riding) {
        const auto &riding_monrace = creature.get_floor()->get_monster(creature.riding).get_monrace();
        chance += (std::max(riding_monrace.level - creature.skill_exp[PlayerSkillKindType::RIDING] / 100 - 10, 0));
    }

    MANA_POINT need_mana = mod_need_mana(creature, spell.smana, spell_id, use_realm);
    if (need_mana > creature.csp) {
        chance += 5 * (need_mana - creature.csp);
    }

    CreatureClass pc(creature);
    PlayerRealm pr(creature);
    if (!pr.realm1().equals(use_realm) && (pc.equals(PlayerClassType::MAGE) || pc.equals(PlayerClassType::PRIEST))) {
        chance += 5;
    }

    PERCENTAGE minfail = adj_mag_fail[creature.stat_index[mp_ptr->spell_stat]];
    if (mp_ptr->has_magic_fail_rate_cap) {
        if (minfail < 5) {
            minfail = 5;
        }
    }

    if ((pc.equals(PlayerClassType::PRIEST) || pc.equals(PlayerClassType::SORCERER))) {
        if (creature.is_icky_wield[0]) {
            chance += 25;
        }

        if (creature.is_icky_wield[1]) {
            chance += 25;
        }
    }

    chance = mod_spell_chance_1(creature, chance);
    PERCENTAGE penalty = (mp_ptr->spell_stat == A_WIS) ? 10 : 4;
    switch (use_realm) {
    case RealmType::NATURE:
        if ((creature.alignment > 50) || (creature.alignment < -50)) {
            chance += penalty;
        }
        break;
    case RealmType::LIFE:
    case RealmType::CRUSADE:
        if (creature.alignment < -20) {
            chance += penalty;
        }
        break;
    case RealmType::DEATH:
    case RealmType::DAEMON:
    case RealmType::HEX:
        if (creature.alignment > 20) {
            chance += penalty;
        }
        break;
    default:
        break;
    }

    if (chance < minfail) {
        chance = minfail;
    }

    chance += creature.effects()->stun().get_magic_chance_penalty();
    if (chance > 95) {
        chance = 95;
    }

    if (pr.realm1().equals(use_realm) || pr.realm2().equals(use_realm) || pc.is_every_magic()) {
        auto exp = PlayerSkill(creature).exp_of_spell(use_realm, spell_id);
        if (exp >= PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT)) {
            chance--;
        }
        if (exp >= PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER)) {
            chance--;
        }
    }

    return mod_spell_chance_2(creature, chance);
}

/*!
 * @brief 呪文情報の表示処理 /
 * Print a list of spells (for browsing or casting or viewing)
 * @param creature 術者への参照
 * @param target_spell_id 呪文ID
 * @param spell_ids 表示するスペルID配列の参照ポインタ
 * @param num 表示するスペルの数(spellsの要素数)
 * @param y 表示メッセージ左上Y座標
 * @param x 表示メッセージ左上X座標
 * @param use_realm 魔法領域ID
 */
void print_spells(CreatureEntity &creature, SPELL_IDX target_spell_id, const SPELL_IDX *spell_ids, int num, TERM_LEN y, TERM_LEN x, RealmType use_realm)
{
    if ((!PlayerRealm::is_magic(use_realm) && !PlayerRealm::is_technic(use_realm)) && AngbandWorld::get_instance().wizard) {
        msg_print(_("警告！ print_spell が領域なしに呼ばれた", "Warning! print_spells called with null realm"));
    }

    prt("", y, x);
    char buf[256];
    if (use_realm == RealmType::HISSATSU) {
        strcpy(buf, _("  Lv   MP", "  Lv   SP"));
    } else {
        strcpy(buf, _("熟練度 Lv   MP 失率 効果", "Profic Lv   SP Fail Effect"));
    }

    put_str(_("名前", "Name"), y, x + 5);
    put_str(buf, y, x + 29);

    int increment = 64;
    CreatureClass pc(creature);
    PlayerRealm pr(creature);
    if (pc.is_every_magic()) {
        increment = 0;
    } else if (pr.realm1().equals(use_realm)) {
        increment = 0;
    } else if (pr.realm2().equals(use_realm)) {
        increment = 32;
    }

    int i;
    char ryakuji[5];
    bool max = false;
    for (i = 0; i < num; i++) {
        const auto spell_id = spell_ids[i];
        const auto &spell = PlayerRealm::get_spell_info(use_realm, spell_id);

        MANA_POINT need_mana;
        if (use_realm == RealmType::HISSATSU) {
            need_mana = spell.smana;
        } else {
            auto exp = PlayerSkill(creature).exp_of_spell(use_realm, spell_id);
            need_mana = mod_need_mana(creature, spell.smana, spell_id, use_realm);
            PlayerSkillRank skill_rank;
            if ((increment == 64) || (spell.slevel >= 99)) {
                skill_rank = PlayerSkillRank::UNSKILLED;
            } else {
                skill_rank = PlayerSkill::spell_skill_rank(exp);
            }

            max = false;
            if (!increment && (skill_rank == PlayerSkillRank::MASTER)) {
                max = true;
            } else if ((increment == 32) && (skill_rank >= PlayerSkillRank::EXPERT)) {
                max = true;
            } else if (spell.slevel >= 99) {
                max = true;
            } else if (pc.equals(PlayerClassType::RED_MAGE) && (skill_rank >= PlayerSkillRank::SKILLED)) {
                max = true;
            }

            strncpy(ryakuji, PlayerSkill::skill_rank_str(skill_rank), 4);
            ryakuji[3] = ']';
            ryakuji[4] = '\0';
        }

        std::string out_val;
        if (use_menu && target_spell_id) {
            if (i == (target_spell_id - 1)) {
                out_val = _("  》 ", "  >  ");
            } else {
                out_val = "     ";
            }
        } else {
            out_val = format("  %c) ", I2A(i));
        }

        if (spell.slevel >= 99) {
            out_val.append(format("%-30s", _("(判読不能)", "(illegible)")));
            c_prt(TERM_L_DARK, out_val, y + i + 1, x);
            continue;
        }

        const auto info = exe_spell(creature, use_realm, spell_id, SpellProcessType::INFO);
        concptr comment = info->data();
        byte line_attr = TERM_WHITE;
        PlayerSpellStatus pss(creature);
        const auto realm_status = pr.realm1().equals(use_realm) ? pss.realm1() : pss.realm2();
        if (pc.is_every_magic()) {
            if (spell.slevel > creature.max_plv) {
                comment = _("未知", "unknown");
                line_attr = TERM_L_BLUE;
            } else if (spell.slevel > creature.level) {
                comment = _("忘却", "forgotten");
                line_attr = TERM_YELLOW;
            }
        } else if (!pr.realm1().equals(use_realm) && !pr.realm2().equals(use_realm)) {
            comment = _("未知", "unknown");
            line_attr = TERM_L_BLUE;
        } else if (realm_status.is_forgotten(spell_id)) {
            comment = _("忘却", "forgotten");
            line_attr = TERM_YELLOW;
        } else if (!realm_status.is_learned(spell_id)) {
            comment = _("未知", "unknown");
            line_attr = TERM_L_BLUE;
        } else if (!realm_status.is_worked(spell_id)) {
            comment = _("未経験", "untried");
            line_attr = TERM_L_GREEN;
        }

        const auto &spell_name = PlayerRealm::get_spell_name(use_realm, spell_id);
        if (use_realm == RealmType::HISSATSU) {
            out_val.append(format("%-25s %2d %4d", spell_name.data(), spell.slevel, need_mana));
        } else {
            out_val.append(format("%-25s%c%-4s %2d %4d %3d%% %s", spell_name.data(), (max ? '!' : ' '), ryakuji, spell.slevel,
                need_mana, spell_chance(creature, spell_id, use_realm), comment));
        }

        c_prt(line_attr, out_val, y + i + 1, x);
    }

    prt("", y + i + 1, x);
}
