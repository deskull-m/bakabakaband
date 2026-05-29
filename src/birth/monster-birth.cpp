/*!
 * @file monster-birth.cpp
 * @brief モンスターをプレイヤーとしてゲーム開始する処理の実装
 */

#include "birth/monster-birth.h"
#include "avatar/avatar.h"
#include "birth/birth-stat.h"
#include "birth/game-play-initializer.h"
#include "birth/history-editor.h"
#include "core/asking-player.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "player-ability/player-ability-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-sex.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "player/race-info-table.h"
#include "spell/spells-status.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "wizard/wizard-spells.h"
#include "world/world.h"

namespace {
/*!
 * @brief モンスター開始用に最低限のプレイヤー属性を初期化する
 * @details race/class/sex/personality 等を既定値で埋め、後続の get_extra() /
 *          update_creature() で player のステータスを算出できる状態にする。
 *          ここで設定するのは「土台」であり、その上に MonraceDefinition の
 *          パラメータを反映していく方針。
 */
void setup_default_player_attributes(CreatureEntity &creature)
{
    creature.psex = SEX_MALE;
    creature.prace = PlayerRaceType::HUMAN;
    creature.pclass = PlayerClassType::WARRIOR;
    creature.ppersonality = PERSONALITY_LUCKY;
    creature.set_patron(0);
    creature.element_realm = ElementRealmType::NONE;

    sp_ptr = &sex_info[creature.psex];
    creature.race = &race_info[enum2i(creature.prace)];
    cp_ptr = &class_info.at(creature.pclass);
    creature.pclass_ref = &class_info.at(creature.pclass);
    mp_ptr = &class_magics_info[enum2i(creature.pclass)];
    creature.personality = &personality_info[creature.ppersonality];

    PlayerRealm pr(creature);
    pr.reset();
}

/*!
 * @brief MonraceDefinition のパラメータをプレイヤーステータスに反映する
 * @details HP / AC / 速度等、モンスターから派生できるパラメータを設定する。
 *          stat_modifiers がある場合はそれも適用する。
 */
void apply_monrace_to_player(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);

    // 名前: モンスター種族名を採用 (プレイヤー名未設定時のみ)
    if (creature.name.empty()) {
        creature.name = monrace.name.string();
    }

    // r_idx / ap_r_idx を当該モンスターに揃える
    creature.polymorph_to(monrace_id);

    // 能力値補正 (stat_modifiers) を適用 (one-monster-placer.cpp の処理と揃える)
    // 補正値は内部 10 単位 (表示 1.0 = 10) で格納されており、tl::nullopt の能力値は補正しない。
    constexpr short stat_min = STAT_MIN_VALUE;
    constexpr short stat_max = STAT_MAX_VALUE;
    for (auto i = 0; i < A_MAX; ++i) {
        const auto &mod = monrace.stat_modifiers[i];
        if (!mod.has_value()) {
            continue;
        }
        auto adjusted = static_cast<int>(creature.get_stat_max(i)) + *mod;
        adjusted = std::clamp(adjusted, static_cast<int>(stat_min), static_cast<int>(stat_max));
        creature.set_stat_max(i, static_cast<short>(adjusted));
        creature.set_stat_cur(i, static_cast<short>(adjusted));
        if (creature.get_stat_max_max(i) < creature.get_stat_max(i)) {
            creature.set_stat_max_max(i, creature.get_stat_max(i));
        }
        creature.set_stat_use(i, creature.get_stat_max(i));
    }

    // HP ダイスをモンスター種族のものに揃える (player_hp[] は get_extra で再計算)
    creature.hit_dice = monrace.hit_dice;

    // 速度: モンスターの基本速度を採用 (110 = 通常)
    creature.set_speed(monrace.speed);

    // 体構造由来の拡張装備スロット (尾の指輪・翼装飾 等) を確保する。
    // 装備可能スロットの判定 (can_equip_to) は r_idx の monrace 側
    // body_structure を参照するように調整済み。
    creature.init_extended_inventory();
}

/*!
 * @brief 開始時のプレイヤーレベルと経験値を設定する
 * @details NPC 生成時の慣例に倣い、モンスター種族の natural depth
 *          (monrace.level) を「生成階層」とみなして 2 で割った値を
 *          初期プレイヤーレベルとする。経験値はそのレベル到達に必要な値に
 *          揃える。get_extra() で expfact が設定された後に呼ぶこと。
 */
void apply_initial_level(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    const auto initial_plv = static_cast<PLAYER_LEVEL>(std::clamp(monrace.level / 2, 1, PY_MAX_LEVEL));

    if (initial_plv >= 2) {
        const auto required_exp = static_cast<EXP>(player_exp[initial_plv - 2] * creature.expfact / 100L);
        creature.set_exp(required_exp);
        creature.set_max_exp(required_exp);
        creature.set_max_max_exp(required_exp);
    } else {
        creature.set_exp(0);
        creature.set_max_exp(0);
        creature.set_max_max_exp(0);
    }

    creature.set_level(initial_plv);
    creature.set_max_plv(initial_plv);
}
}

bool player_birth_as_monster(CreatureEntity &creature)
{
    term_clear();
    put_str(_("モンスターを選んでください。", "Select a monster to play as."), 2, 2);

    const auto monrace_id = wiz_select_summon_monrace_id();
    if (!monrace_id) {
        return false;
    }

    // 土台となるプレイヤー属性を埋める (race/class/sex/personality 等)
    setup_default_player_attributes(creature);

    // プレイヤー作成と同じローラーで基礎能力値を振る
    get_stats(creature);

    // モンスター固有のパラメータを反映 (HP / AC / 速度 / 能力値補正)
    apply_monrace_to_player(creature, *monrace_id);

    // HP / MP / 経験値テーブル等の通常初期化
    get_extra(creature, true);
    get_max_stats(creature);
    initialize_virtues(creature);

    // 初期レベルを「生成階層 / 2」相当に揃える (get_extra で expfact が
    // セットされた後に呼ぶ必要があるためここで実施)
    apply_initial_level(creature, *monrace_id);

    // プレイヤー名入力 (モンスター種族名がデフォルト)
    get_name(creature);
    process_player_name(creature, AngbandWorld::get_instance().creating_savefile);

    edit_history(creature);

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    update_creature(creature);
    creature.hp = creature.maxhp;
    creature.set_csp(creature.get_msp());

    init_turn(creature);
    init_dungeon_quests(creature);

    msg_format(_("%s としてゲームを開始します。", "Starting game as %s."),
        MonraceList::get_instance().get_monrace(*monrace_id).name.string().data());

    return true;
}
