#include "birth/quick-start.h"
#include "birth/birth-stat.h"
#include "birth/birth-util.h"
#include "birth/game-play-initializer.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "player/race-info-table.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"

/*
 * The last character rolled,
 * holded for quick start
 */
birther previous_char;

/*!
 * @brief クイックスタート処理の問い合わせと実行を行う。/Ask whether the player use Quick Start or not.
 */
bool ask_quick_start(CreatureEntity &creature)
{
    if (!previous_char.quick_ok) {
        return false;
    }

    term_clear();
    put_str(_("クイック・スタートを使うと以前と全く同じキャラクターで始められます。",
                "Do you want to use the quick start function (same character as your last one)."),
        11, 2);
    while (true) {
        char c;
        put_str(_("クイック・スタートを使いますか？[y/N]", "Use quick start? [y/N]"), 14, 10);
        c = inkey();
        if (c == 'Q') {
            quit("");
        } else if (c == 'S') {
            return false;
        } else if (c == '?') {
            show_help(creature, _("jbirth.txt#QuickStart", "birth.txt#QuickStart"));
        } else if ((c == 'y') || (c == 'Y')) {
            break;
        } else {
            return false;
        }
    }

    load_prev_data(creature, false);
    init_turn(creature);
    init_dungeon_quests(creature);

    sp_ptr = &sex_info[creature.psex];
    creature.race = &race_info[enum2i(creature.prace)];
    cp_ptr = &class_info.at(creature.pclass);
    creature.pclass_ref = &class_info.at(creature.pclass);
    auto short_pclass = enum2i(creature.pclass);
    mp_ptr = &class_magics_info[short_pclass];
    creature.personality = &personality_info[creature.ppersonality];

    get_extra(creature, false);
    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    update_creature(creature);
    creature.hp = creature.maxhp;
    creature.set_current_mp(creature.get_max_mp());
    process_player_name(creature);
    return true;
}
/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体から保存する / Save the current data for later
 * @param birther_ptr クイックスタート構造体の参照ポインタ
 * @return なし。
 */
void save_prev_data(CreatureEntity &creature, birther *birther_ptr)
{
    birther_ptr->psex = creature.psex;
    birther_ptr->prace = creature.prace;
    birther_ptr->pclass = creature.pclass;
    birther_ptr->ppersonality = creature.ppersonality;

    if (CreatureClass(creature).equals(PlayerClassType::ELEMENTALIST)) {
        birther_ptr->realm1 = static_cast<int16_t>(creature.get_element_realm());
        birther_ptr->realm2 = 0;
    } else {
        PlayerRealm pr(creature);
        birther_ptr->realm1 = static_cast<int16_t>(pr.realm1().to_enum());
        birther_ptr->realm2 = static_cast<int16_t>(pr.realm2().to_enum());
    }

    birther_ptr->age = creature.get_age();
    birther_ptr->ht = creature.get_ht();
    birther_ptr->wt = creature.get_wt();
    birther_ptr->prestige = creature.get_prestige();
    birther_ptr->au = creature.get_au();

    for (int i = 0; i < A_MAX; i++) {
        birther_ptr->stat_max[i] = creature.get_stat_max(i);
        birther_ptr->stat_max_max[i] = creature.get_stat_max_max(i);
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        birther_ptr->hp_table[i] = creature.hp_table[i];
    }

    birther_ptr->patron = creature.get_patron();
    birther_ptr->virtues = creature.virtues;

    for (int i = 0; i < 4; i++) {
        strcpy(birther_ptr->history[i], creature.history[i]);
    }
}

/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体へ読み込む / Load the previous data
 * @param swap TRUEならば現在のプレイヤー構造体上との内容をスワップする形で読み込む。
 * @return なし。
 */
void load_prev_data(CreatureEntity &creature, bool swap)
{
    birther temp;
    if (swap) {
        save_prev_data(creature, &temp);
    }

    creature.psex = previous_char.psex;
    creature.prace = previous_char.prace;
    creature.pclass = previous_char.pclass;
    creature.ppersonality = previous_char.ppersonality;

    PlayerRealm pr(creature);
    pr.reset();
    if (CreatureClass(creature).equals(PlayerClassType::ELEMENTALIST)) {
        creature.set_element_realm(i2enum<ElementRealmType>(previous_char.realm1));
    } else {
        const auto realm1 = i2enum<RealmType>(previous_char.realm1);
        const auto realm2 = i2enum<RealmType>(previous_char.realm2);
        if (realm1 != RealmType::NONE) {
            pr.set(realm1, realm2);
        }
    }

    creature.set_age(previous_char.age);
    creature.set_ht(previous_char.ht);
    creature.set_wt(previous_char.wt);
    creature.set_prestige(previous_char.prestige);
    creature.set_au(previous_char.au);

    for (int i = 0; i < A_MAX; i++) {
        creature.set_stat_max(i, previous_char.stat_max[i]);
        creature.set_stat_cur(i, previous_char.stat_max[i]);
        creature.set_stat_max_max(i, previous_char.stat_max_max[i]);
    }

    for (int i = 0; i < PY_MAX_LEVEL; i++) {
        creature.hp_table[i] = previous_char.hp_table[i];
    }

    creature.maxhp = creature.hp_table[0];
    creature.hp = creature.hp_table[0];
    creature.set_patron(previous_char.patron);
    creature.virtues = previous_char.virtues;

    CreatureClass(creature).init_specific_data();

    for (int i = 0; i < 4; i++) {
        strcpy(creature.history[i], previous_char.history[i]);
    }

    if (swap) {
        previous_char = temp;
    }
}
