/*!
 * @brief ウルティマ4を参考にした徳のシステムの実装 / Enable an Ultima IV style "avatar" game where you try to achieve perfection in various virtues.
 * @date 2013/12/23
 * @author
 * Topi Ylinen 1998
 * f1toyl@uta.fi
 * topi.ylinen@noodi.fi
 *
 * Copyright (c) 1989 James E. Wilson, Christopher J. Stuart
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "avatar/avatar.h"
#include "game-option/text-display-options.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/player-realm.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"

/*!
 * 徳の名称 / The names of the virtues
 */
const std::map<Virtue, std::string> virtue_names = {
    { Virtue::NONE, "" },
    { Virtue::COMPASSION, _("情", "Compassion") },
    { Virtue::HONOUR, _("誉", "Honour") },
    { Virtue::JUSTICE, _("正", "Justice") },
    { Virtue::SACRIFICE, _("犠", "Sacrifice") },
    { Virtue::KNOWLEDGE, _("識", "Knowledge") },
    { Virtue::FAITH, _("誠", "Faith") },
    { Virtue::ENLIGHTEN, _("啓", "Enlightenment") },
    { Virtue::ENCHANT, _("秘", "Mysticism") },
    { Virtue::CHANCE, _("運", "Chance") },
    { Virtue::NATURE, _("然", "Nature") },
    { Virtue::HARMONY, _("調", "Harmony") },
    { Virtue::VITALITY, _("活", "Vitality") },
    { Virtue::UNLIFE, _("死", "Unlife") },
    { Virtue::PATIENCE, _("忍", "Patience") },
    { Virtue::TEMPERANCE, _("節", "Temperance") },
    { Virtue::DILIGENCE, _("勤", "Diligence") },
    { Virtue::VALOUR, _("勇", "Valour") },
    { Virtue::INDIVIDUALISM, _("個", "Individualism") },
};

/*!
 * @brief 該当の徳がプレイヤーに指定されているか否かに応じつつ、大小を比較する。
 * @details 徳がない場合は値0として比較する。
 * @param virtue_names 比較したい徳のID
 * @param threshold 比較基準値
 * @return 比較の真偽値を返す
 */
bool compare_virtue(PlayerType *player_ptr, Virtue virtue, int threshold)
{
    const auto it = player_ptr->virtues.find(virtue);
    const auto virtue_value = (it != player_ptr->virtues.end()) ? it->second : 0;
    return virtue_value > threshold;
}

/*!
 * @brief プレイヤーの指定の徳が何番目のスロットに登録されているかを返す。 / Aux function
 * @param virtue_names 確認したい徳のID
 * @return スロットがあるならばスロットのID(0～7)+1、ない場合は0を返す。
 */
int virtue_number(PlayerType *player_ptr, Virtue virtue)
{
    return player_ptr->virtues.find(virtue) != player_ptr->virtues.end() ? 1 : 0;
}

/*!
 * @brief プレイヤーの職業や種族に依存しないランダムな徳を取得する / Aux function
 * @param which 確認したい徳のID
 */
static void get_random_virtue(PlayerType *player_ptr)
{
    ProbabilityTable<Virtue> pt;
    pt.entry_item(Virtue::SACRIFICE, 3);
    pt.entry_item(Virtue::COMPASSION, 3);
    pt.entry_item(Virtue::VALOUR, 6);
    pt.entry_item(Virtue::HONOUR, 5);
    pt.entry_item(Virtue::JUSTICE, 4);
    pt.entry_item(Virtue::TEMPERANCE, 2);
    pt.entry_item(Virtue::HARMONY, 2);
    pt.entry_item(Virtue::PATIENCE, 3);
    pt.entry_item(Virtue::DILIGENCE, 1);

    while (true) {
        const auto type = pt.pick_one_at_random();
        if (player_ptr->virtues.find(type) == player_ptr->virtues.end()) {
            player_ptr->virtues[type] = 0;
            return;
        }
    }
}

/*!
 * @brief プレイヤーの選んだ魔法領域に応じて対応する徳を返す。
 * @param realm 魔法領域のID
 * @return 対応する徳のID
 */
static enum Virtue get_realm_virtues(PlayerType *player_ptr, RealmType realm)
{
    switch (realm) {
    case RealmType::LIFE:
        if (virtue_number(player_ptr, Virtue::VITALITY)) {
            return Virtue::TEMPERANCE;
        } else {
            return Virtue::VITALITY;
        }
    case RealmType::SORCERY:
        if (virtue_number(player_ptr, Virtue::KNOWLEDGE)) {
            return Virtue::ENCHANT;
        } else {
            return Virtue::KNOWLEDGE;
        }
    case RealmType::NATURE:
        if (virtue_number(player_ptr, Virtue::NATURE)) {
            return Virtue::HARMONY;
        } else {
            return Virtue::NATURE;
        }
    case RealmType::CHAOS:
        if (virtue_number(player_ptr, Virtue::CHANCE)) {
            return Virtue::INDIVIDUALISM;
        } else {
            return Virtue::CHANCE;
        }
    case RealmType::DEATH:
        return Virtue::UNLIFE;
    case RealmType::TRUMP:
        return Virtue::KNOWLEDGE;
    case RealmType::ARCANE:
        return Virtue::NONE;
    case RealmType::CRAFT:
        if (virtue_number(player_ptr, Virtue::ENCHANT)) {
            return Virtue::INDIVIDUALISM;
        } else {
            return Virtue::ENCHANT;
        }
    case RealmType::DAEMON:
        if (virtue_number(player_ptr, Virtue::JUSTICE)) {
            return Virtue::FAITH;
        } else {
            return Virtue::JUSTICE;
        }
    case RealmType::CRUSADE:
        if (virtue_number(player_ptr, Virtue::JUSTICE)) {
            return Virtue::HONOUR;
        } else {
            return Virtue::JUSTICE;
        }
    case RealmType::HEX:
        if (virtue_number(player_ptr, Virtue::COMPASSION)) {
            return Virtue::JUSTICE;
        } else {
            return Virtue::COMPASSION;
        }
    default:
        return Virtue::NONE;
    };
}

/*!
 * @brief 作成中のプレイヤーキャラクターに徳8種類を与える。 / Select virtues & reset values for a new character
 * @details 職業に応じて1～4種が固定、種族に応じて1種類が与えられ、後は重複なくランダムに選択される。
 */
void initialize_virtues(PlayerType *player_ptr)
{
    player_ptr->virtues.clear();

    auto add_virtue = [&](Virtue v) {
        if (v != Virtue::NONE && player_ptr->virtues.find(v) == player_ptr->virtues.end()) {
            player_ptr->virtues[v] = 0;
        }
    };

    /* Get pre-defined types based on class */
    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::SAMURAI:
        add_virtue(Virtue::VALOUR);
        add_virtue(Virtue::HONOUR);
        break;
    case PlayerClassType::MAGE:
        add_virtue(Virtue::KNOWLEDGE);
        add_virtue(Virtue::ENCHANT);
        break;
    case PlayerClassType::PRIEST:
        add_virtue(Virtue::FAITH);
        add_virtue(Virtue::TEMPERANCE);
        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::SNIPER:
        add_virtue(Virtue::HONOUR);
        break;
    case PlayerClassType::RANGER:
    case PlayerClassType::ARCHER:
        add_virtue(Virtue::NATURE);
        add_virtue(Virtue::TEMPERANCE);
        break;
    case PlayerClassType::PALADIN:
        add_virtue(Virtue::JUSTICE);
        add_virtue(Virtue::VALOUR);
        add_virtue(Virtue::HONOUR);
        add_virtue(Virtue::FAITH);
        break;
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::RED_MAGE:
        add_virtue(Virtue::ENCHANT);
        add_virtue(Virtue::VALOUR);
        break;
    case PlayerClassType::CHAOS_WARRIOR:
        add_virtue(Virtue::CHANCE);
        add_virtue(Virtue::INDIVIDUALISM);
        break;
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        add_virtue(Virtue::FAITH);
        add_virtue(Virtue::HARMONY);
        add_virtue(Virtue::TEMPERANCE);
        add_virtue(Virtue::PATIENCE);
        break;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::MIRROR_MASTER:
        add_virtue(Virtue::HARMONY);
        add_virtue(Virtue::ENLIGHTEN);
        add_virtue(Virtue::PATIENCE);
        break;
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
        add_virtue(Virtue::ENLIGHTEN);
        add_virtue(Virtue::ENCHANT);
        add_virtue(Virtue::KNOWLEDGE);
        break;
    case PlayerClassType::TOURIST:
        add_virtue(Virtue::ENLIGHTEN);
        add_virtue(Virtue::CHANCE);
        break;
    case PlayerClassType::IMITATOR:
        add_virtue(Virtue::CHANCE);
        break;
    case PlayerClassType::BLUE_MAGE:
        add_virtue(Virtue::CHANCE);
        add_virtue(Virtue::KNOWLEDGE);
        break;
    case PlayerClassType::BEASTMASTER:
        add_virtue(Virtue::NATURE);
        add_virtue(Virtue::CHANCE);
        add_virtue(Virtue::VITALITY);
        break;
    case PlayerClassType::MAGIC_EATER:
        add_virtue(Virtue::ENCHANT);
        add_virtue(Virtue::KNOWLEDGE);
        break;
    case PlayerClassType::BARD:
        add_virtue(Virtue::HARMONY);
        add_virtue(Virtue::COMPASSION);
        break;
    case PlayerClassType::CAVALRY:
        add_virtue(Virtue::VALOUR);
        add_virtue(Virtue::HARMONY);
        break;
    case PlayerClassType::BERSERKER:
        add_virtue(Virtue::VALOUR);
        add_virtue(Virtue::INDIVIDUALISM);
        break;
    case PlayerClassType::SMITH:
        add_virtue(Virtue::HONOUR);
        add_virtue(Virtue::KNOWLEDGE);
        break;
    case PlayerClassType::NINJA:
        add_virtue(Virtue::PATIENCE);
        add_virtue(Virtue::KNOWLEDGE);
        add_virtue(Virtue::FAITH);
        add_virtue(Virtue::UNLIFE);
        break;
    case PlayerClassType::ELEMENTALIST:
        add_virtue(Virtue::NATURE);
        break;
    case PlayerClassType::MAX:
        break;
    };

    /* Get one virtue based on race */
    switch (player_ptr->prace) {
    case PlayerRaceType::HUMAN:
    case PlayerRaceType::HALF_ELF:
    case PlayerRaceType::DUNADAN:
        add_virtue(Virtue::INDIVIDUALISM);
        break;
    case PlayerRaceType::ELF:
    case PlayerRaceType::SPRITE:
    case PlayerRaceType::ENT:
    case PlayerRaceType::MERFOLK:
        add_virtue(Virtue::NATURE);
        break;
    case PlayerRaceType::HOBBIT:
    case PlayerRaceType::HALF_OGRE:
        add_virtue(Virtue::TEMPERANCE);
        break;
    case PlayerRaceType::DWARF:
    case PlayerRaceType::KLACKON:
    case PlayerRaceType::ANDROID:
        add_virtue(Virtue::DILIGENCE);
        break;
    case PlayerRaceType::GNOME:
    case PlayerRaceType::CYCLOPS:
        add_virtue(Virtue::KNOWLEDGE);
        break;
    case PlayerRaceType::HALF_ORC:
    case PlayerRaceType::AMBERITE:
    case PlayerRaceType::KOBOLD:
        add_virtue(Virtue::HONOUR);
        break;
    case PlayerRaceType::HALF_TROLL:
    case PlayerRaceType::BARBARIAN:
        add_virtue(Virtue::VALOUR);
        break;
    case PlayerRaceType::HIGH_ELF:
    case PlayerRaceType::KUTAR:
        add_virtue(Virtue::VITALITY);
        break;
    case PlayerRaceType::HALF_GIANT:
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ARCHON:
    case PlayerRaceType::BALROG:
        add_virtue(Virtue::JUSTICE);
        break;
    case PlayerRaceType::HALF_TITAN:
        add_virtue(Virtue::HARMONY);
        break;
    case PlayerRaceType::YEEK:
        add_virtue(Virtue::SACRIFICE);
        break;
    case PlayerRaceType::MIND_FLAYER:
        add_virtue(Virtue::ENLIGHTEN);
        break;
    case PlayerRaceType::DARK_ELF:
    case PlayerRaceType::DRACONIAN:
    case PlayerRaceType::S_FAIRY:
        add_virtue(Virtue::ENCHANT);
        break;
    case PlayerRaceType::NIBELUNG:
        add_virtue(Virtue::PATIENCE);
        break;
    case PlayerRaceType::IMP:
        add_virtue(Virtue::FAITH);
        break;
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SPECTRE:
        add_virtue(Virtue::UNLIFE);
        break;
    case PlayerRaceType::BEASTMAN:
        add_virtue(Virtue::CHANCE);
        break;
    case PlayerRaceType::MAX:
        break;
    }

    /* Get virtues for realms */
    PlayerRealm pr(player_ptr);
    if (pr.realm1().is_available()) {
        auto tmp_vir = get_realm_virtues(player_ptr, pr.realm1().to_enum());
        add_virtue(tmp_vir);
    }

    if (pr.realm2().is_available()) {
        auto tmp_vir = get_realm_virtues(player_ptr, pr.realm2().to_enum());
        add_virtue(tmp_vir);
    }

    /* Fill up to 8 virtues with random ones */
    while (player_ptr->virtues.size() < 8) {
        get_random_virtue(player_ptr);
    }
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に加減を行う。
 * @details 範囲は-125～125、基本的に絶対値が大きいほど絶対値が上がり辛くなる。
 * @param virtue_names 徳のID
 * @param amount 加減量
 */
void chg_virtue(PlayerType *player_ptr, Virtue virtue_id, int amount)
{
    auto it = player_ptr->virtues.find(virtue_id);
    if (it == player_ptr->virtues.end()) {
        return;
    }

    if (amount > 0) {
        if ((amount + it->second > 50) && one_in_(2)) {
            it->second = std::max<short>(it->second, 50);
            return;
        }

        if ((amount + it->second > 80) && one_in_(2)) {
            it->second = std::max<short>(it->second, 80);
            return;
        }

        if ((amount + it->second > 100) && one_in_(2)) {
            it->second = std::max<short>(it->second, 100);
            return;
        }

        if (amount + it->second > 125) {
            it->second = 125;
        } else {
            it->second = it->second + amount;
        }
    } else {
        if ((amount + it->second < -50) && one_in_(2)) {
            it->second = std::min<short>(it->second, -50);
            return;
        }

        if ((amount + it->second < -80) && one_in_(2)) {
            it->second = std::min<short>(it->second, -80);
            return;
        }

        if ((amount + it->second < -100) && one_in_(2)) {
            it->second = std::min<short>(it->second, -100);
            return;
        }

        if (amount + it->second < -125) {
            it->second = -125;
        } else {
            it->second = it->second + amount;
        }
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に固定値をセットする
 * @param virtue_names 徳のID
 * @param amount セットしたい値
 */
void set_virtue(PlayerType *player_ptr, Virtue virtue_id, int amount)
{
    auto it = player_ptr->virtues.find(virtue_id);
    if (it != player_ptr->virtues.end()) {
        it->second = static_cast<int16_t>(amount);
    }
}

/*!
 * @brief 徳のダンプ表示を行う
 * @param out_file ファイルポインタ
 */
void dump_virtues(PlayerType *player_ptr, FILE *out_file)
{
    if (!out_file) {
        return;
    }

    for (const auto &[virtue_type, value] : player_ptr->virtues) {
        const auto &vir_name = virtue_names.at(virtue_type);
        const auto vir_val_str = format(" (%d)", value);
        const auto vir_val = show_actual_value ? vir_val_str.data() : "";
        if ((virtue_type == Virtue::NONE) || (virtue_type >= Virtue::MAX)) {
            fprintf(out_file, _("おっと。%sの情報なし。", "Oops. No info about %s."), vir_name.data());
        }

        else if (value < -100) {
            fprintf(out_file, _("[%s]の対極%s", "You are the polar opposite of %s.%s"), vir_name.data(), vir_val);
        } else if (value < -80) {
            fprintf(out_file, _("[%s]の大敵%s", "You are an arch-enemy of %s.%s"), vir_name.data(), vir_val);
        } else if (value < -60) {
            fprintf(out_file, _("[%s]の強敵%s", "You are a bitter enemy of %s.%s"), vir_name.data(), vir_val);
        } else if (value < -40) {
            fprintf(out_file, _("[%s]の敵%s", "You are an enemy of %s.%s"), vir_name.data(), vir_val);
        } else if (value < -20) {
            fprintf(out_file, _("[%s]の罪者%s", "You have sinned against %s.%s"), vir_name.data(), vir_val);
        } else if (value < 0) {
            fprintf(out_file, _("[%s]の迷道者%s", "You have strayed from the path of %s.%s"), vir_name.data(), vir_val);
        } else if (value == 0) {
            fprintf(out_file, _("[%s]の中立者%s", "You are neutral to %s.%s"), vir_name.data(), vir_val);
        } else if (value < 20) {
            fprintf(out_file, _("[%s]の小徳者%s", "You are somewhat virtuous in %s.%s"), vir_name.data(), vir_val);
        } else if (value < 40) {
            fprintf(out_file, _("[%s]の中徳者%s", "You are virtuous in %s.%s"), vir_name.data(), vir_val);
        } else if (value < 60) {
            fprintf(out_file, _("[%s]の高徳者%s", "You are very virtuous in %s.%s"), vir_name.data(), vir_val);
        } else if (value < 80) {
            fprintf(out_file, _("[%s]の覇者%s", "You are a champion of %s.%s"), vir_name.data(), vir_val);
        } else if (value < 100) {
            fprintf(out_file, _("[%s]の偉大な覇者%s", "You are a great champion of %s.%s"), vir_name.data(), vir_val);
        } else {
            fprintf(out_file, _("[%s]の具現者%s", "You are the living embodiment of %s.%s"), vir_name.data(), vir_val);
        }

        fprintf(out_file, "\n");
    }
}
