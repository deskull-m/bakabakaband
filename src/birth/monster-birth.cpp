/*!
 * @file monster-birth.cpp
 * @brief モンスターをプレイヤーとしてゲーム開始する処理の実装
 */

#include "birth/monster-birth.h"
#include "artifact/random-art-generator.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-select-personality.h"
#include "birth/birth-stat.h"
#include "birth/game-play-initializer.h"
#include "birth/history-editor.h"
#include "birth/inventory-initializer.h"
#include "core/asking-player.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-sex-const.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "perception/object-perception.h"
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
#include "realm/realm-types.h"
#include "spell/spells-status.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "term/z-rand.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "view/display-messages.h"
#include "view/display-player-misc-info.h"
#include "view/display-util.h"
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
    // 性格は後続の select_player_personality() でプレイヤーが選択する。
    // ここではポインタ初期化のための既定値を入れておく。
    creature.ppersonality = PERSONALITY_ORDINARY;
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
 * @brief モンスター種族の kind_flags から対応するプレイヤー職業を求める
 * @details CreatureEntity::initialize_equivalent_player_classes() と同じ
 *          対応表・優先順位を用いる。複数該当する場合は最初に一致したものを
 *          採用し、該当フラグがなければ tl::nullopt を返す。
 * @param monrace モンスター種族定義
 * @return 対応する職業。なければ tl::nullopt
 */
tl::optional<PlayerClassType> determine_player_class_from_monrace(const MonraceDefinition &monrace)
{
    const auto &kind_flags = monrace.kind_flags;
    if (kind_flags.has(MonsterKindType::WARRIOR)) {
        return PlayerClassType::WARRIOR;
    }
    if (kind_flags.has(MonsterKindType::MAGE)) {
        return PlayerClassType::MAGE;
    }
    if (kind_flags.has(MonsterKindType::PRIEST)) {
        return PlayerClassType::PRIEST;
    }
    if (kind_flags.has(MonsterKindType::ROGUE)) {
        return PlayerClassType::ROGUE;
    }
    if (kind_flags.has(MonsterKindType::RANGER)) {
        return PlayerClassType::RANGER;
    }
    if (kind_flags.has(MonsterKindType::PALADIN)) {
        return PlayerClassType::PALADIN;
    }
    if (kind_flags.has(MonsterKindType::SAMURAI)) {
        return PlayerClassType::SAMURAI;
    }
    if (kind_flags.has(MonsterKindType::NINJA)) {
        return PlayerClassType::NINJA;
    }
    if (kind_flags.has(MonsterKindType::MINDCRAFTER)) {
        return PlayerClassType::MINDCRAFTER;
    }
    if (kind_flags.has(MonsterKindType::ARCHER)) {
        return PlayerClassType::ARCHER;
    }
    if (kind_flags.has(MonsterKindType::BARD)) {
        return PlayerClassType::BARD;
    }
    if (kind_flags.has(MonsterKindType::SMITH)) {
        return PlayerClassType::SMITH;
    }
    if (kind_flags.has(MonsterKindType::KARATEKA)) {
        return PlayerClassType::MONK;
    }

    return tl::nullopt;
}

/*!
 * @brief 職業に応じた魔法領域を自動的に決定する
 * @details 通常のキャラ作成 (get_player_realms) ではプレイヤーが選択するが、
 *          モンスター運用時は選択可能な第一領域があればランダムに 1 つ
 *          割り当てる。選択可能領域を持たない職業 (戦士・忍者等) は領域なし。
 *          第二領域は付与しない (第一領域のみでも妥当なプレイ状態となる)。
 * @param creature クリーチャーへの参照
 */
void assign_auto_realm(CreatureEntity &creature)
{
    PlayerRealm pr(creature);
    pr.reset();

    const auto choices = PlayerRealm::get_realm1_choices(creature.pclass);
    std::vector<RealmType> candidates;
    for (auto realm : EnumRange(RealmType::LIFE, RealmType::MAX)) {
        if (choices.has(realm)) {
            candidates.push_back(realm);
        }
    }

    if (!candidates.empty()) {
        pr.set(candidates[randint0(static_cast<int>(candidates.size()))]);
    }
}

/*!
 * @brief モンスター種族に対応する職業があればプレイヤーに反映する
 * @details kind_flags から職業を判定し、該当する場合は pclass / cp_ptr /
 *          mp_ptr 等を更新したうえで魔法領域を自動決定する。該当しない場合は
 *          setup_default_player_attributes() で設定された既定 (戦士) のまま。
 *          領域は HP/MP 計算 (get_extra) に影響するため、それより前に呼ぶこと。
 * @param creature クリーチャーへの参照
 * @param monrace_id 開始モンスターの種族ID
 */
void apply_monrace_class(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    const auto pclass = determine_player_class_from_monrace(monrace);
    if (!pclass) {
        return;
    }

    creature.pclass = *pclass;
    cp_ptr = &class_info.at(creature.pclass);
    creature.pclass_ref = &class_info.at(creature.pclass);
    mp_ptr = &class_magics_info[enum2i(creature.pclass)];

    assign_auto_realm(creature);
}

/*!
 * @brief モンスター種族の kind_flags から対応するプレイヤー種族を求める
 * @details CreatureEntity::initialize_equivalent_player_races() と同じ対応表を
 *          用いるが、複数該当する場合は「無難な (癖の少ない) 種族」を優先する
 *          順序で判定する。特殊な食性・制約を持つ天使/悪魔/不死系は後回しとし、
 *          該当フラグがなければ tl::nullopt を返す。
 * @param monrace モンスター種族定義
 * @return 対応する種族。なければ tl::nullopt
 */
tl::optional<PlayerRaceType> determine_player_race_from_monrace(const MonraceDefinition &monrace)
{
    const auto &kind_flags = monrace.kind_flags;
    // 無難な種族を優先する順序 (人間 → 一般的なファンタジー種族 → 大型 →
    // 特殊種族 → 食性等に制約のあるゴーレム/悪魔/天使/不死系)
    if (kind_flags.has(MonsterKindType::HUMAN)) {
        return PlayerRaceType::HUMAN;
    }
    if (kind_flags.has(MonsterKindType::HOBBIT)) {
        return PlayerRaceType::HOBBIT;
    }
    if (kind_flags.has(MonsterKindType::DWARF)) {
        return PlayerRaceType::DWARF;
    }
    if (kind_flags.has(MonsterKindType::ELF)) {
        return PlayerRaceType::HALF_ELF;
    }
    if (kind_flags.has(MonsterKindType::ORC)) {
        return PlayerRaceType::HALF_ORC;
    }
    if (kind_flags.has(MonsterKindType::AMBERITE)) {
        return PlayerRaceType::AMBERITE;
    }
    if (kind_flags.has(MonsterKindType::NIBELUNG)) {
        return PlayerRaceType::NIBELUNG;
    }
    if (kind_flags.has(MonsterKindType::KOBOLD)) {
        return PlayerRaceType::KOBOLD;
    }
    if (kind_flags.has(MonsterKindType::YEEK)) {
        return PlayerRaceType::YEEK;
    }
    if (kind_flags.has(MonsterKindType::TROLL)) {
        return PlayerRaceType::HALF_TROLL;
    }
    if (kind_flags.has(MonsterKindType::OGRE)) {
        return PlayerRaceType::HALF_OGRE;
    }
    if (kind_flags.has(MonsterKindType::GIANT)) {
        return PlayerRaceType::HALF_GIANT;
    }
    if (kind_flags.has(MonsterKindType::BEAST)) {
        return PlayerRaceType::BEASTMAN;
    }
    if (kind_flags.has(MonsterKindType::MERFOLK)) {
        return PlayerRaceType::MERFOLK;
    }
    if (kind_flags.has(MonsterKindType::DRAGON)) {
        return PlayerRaceType::DRACONIAN;
    }
    if (kind_flags.has(MonsterKindType::FAIRY)) {
        return PlayerRaceType::SPRITE;
    }
    if (kind_flags.has(MonsterKindType::TREEFOLK)) {
        return PlayerRaceType::ENT;
    }
    if (kind_flags.has(MonsterKindType::MINDFLAYER)) {
        return PlayerRaceType::MIND_FLAYER;
    }
    if (kind_flags.has(MonsterKindType::GOLEM)) {
        return PlayerRaceType::GOLEM;
    }
    if (kind_flags.has(MonsterKindType::ROBOT)) {
        return PlayerRaceType::ANDROID;
    }
    if (kind_flags.has(MonsterKindType::DEMON)) {
        return PlayerRaceType::IMP;
    }
    if (kind_flags.has(MonsterKindType::ANGEL)) {
        return PlayerRaceType::ARCHON;
    }
    if (kind_flags.has(MonsterKindType::VAMPIRE)) {
        return PlayerRaceType::VAMPIRE;
    }
    if (kind_flags.has(MonsterKindType::ZOMBIE)) {
        return PlayerRaceType::ZOMBIE;
    }
    if (kind_flags.has(MonsterKindType::SKELETON)) {
        return PlayerRaceType::SKELETON;
    }
    if (kind_flags.has(MonsterKindType::UNDEAD)) {
        return PlayerRaceType::SPECTRE;
    }
    if (kind_flags.has(MonsterKindType::CAT)) {
        return PlayerRaceType::CAT;
    }

    return tl::nullopt;
}

/*!
 * @brief モンスター種族に対応するプレイヤー種族があれば反映する
 * @details kind_flags から種族を判定し、該当する場合は prace / race を更新する。
 *          複数該当時は無難な種族を優先する。該当しない場合は
 *          setup_default_player_attributes() で設定された既定 (人間) のまま。
 *          種族は能力値・HP 等の算出 (get_stats / get_extra) に影響するため、
 *          それらより前に呼ぶこと。
 * @param creature クリーチャーへの参照
 * @param monrace_id 開始モンスターの種族ID
 */
void apply_monrace_race(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    const auto prace = determine_player_race_from_monrace(monrace);
    if (!prace) {
        return;
    }

    creature.prace = *prace;
    creature.race = &race_info[enum2i(creature.prace)];
}

/*!
 * @brief モンスター種族に性格が固定指定されていればプレイヤーに反映する
 * @details EMPTY_MIND (無心) のモンスターは常に性格「なし」(PERSONALITY_EMPTY) を
 *          適用する。それ以外は JSON の "personality" で指定された性格があれば
 *          常にそれを適用し、プレイヤーによる対話的な性格選択を省略する。
 *          いずれにも該当しない場合は何もせず false を返し、呼び出し側で通常の
 *          選択処理に委ねる。
 * @param creature クリーチャーへの参照
 * @param monrace_id 開始モンスターの種族ID
 * @return 固定指定を適用したら true、未指定なら false
 */
bool apply_monrace_personality(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (monrace.misc_flags.has(MonsterMiscType::EMPTY_MIND)) {
        creature.set_personality(PERSONALITY_EMPTY);
        return true;
    }

    if (monrace.personality == PERSONALITY_NONE) {
        return false;
    }

    creature.set_personality(monrace.personality);
    return true;
}

/*!
 * @brief モンスター種族定義の性別をプレイヤーの性別に反映する
 * @details one-monster-placer.cpp のモンスター生成時と同じ判定を用いる。
 *          データソースは kind_flags の MALE/FEMALE と MonraceDefinition::sex
 *          の 2 系統で、両方真→両性、片方→該当、なし→無性とする。
 *          性別は性格選択 (sex 制限) や各種ボーナスに影響するため、
 *          性格選択より前に確定させること。
 * @param creature クリーチャーへの参照
 * @param monrace_id 開始モンスターの種族ID
 */
void apply_monrace_sex(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    const auto has_male = monrace.kind_flags.has(MonsterKindType::MALE) || (monrace.sex == MonsterSex::MALE);
    const auto has_female = monrace.kind_flags.has(MonsterKindType::FEMALE) || (monrace.sex == MonsterSex::FEMALE);
    if (has_male && has_female) {
        creature.psex = SEX_BISEXUAL;
    } else if (has_male) {
        creature.psex = SEX_MALE;
    } else if (has_female) {
        creature.psex = SEX_FEMALE;
    } else {
        creature.psex = SEX_ASEXUAL;
    }

    sp_ptr = &sex_info[creature.psex];
}

/*!
 * @brief モンスター開始時の性格選択を行う
 * @details 通常のキャラクター作成 (birth-wizard.cpp の let_player_select_personality)
 *          と同様に、性格選択メニューと説明文付きの確認を繰り返す。
 *          'S' で前段階へ戻る選択をした場合は false を返す。
 * @param creature クリーチャーへの参照
 * @return 性格を確定したら true、戻る選択をしたら false
 */
bool select_player_personality(CreatureEntity &creature)
{
    creature.ppersonality = PERSONALITY_ORDINARY;
    while (true) {
        if (!get_player_personality(creature)) {
            return false;
        }

        clear_from(10);
        display_wrap_around(personality_explanations[creature.ppersonality], 74, 12, 3);

        if (input_check_strict(creature, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
            break;
        }

        display_player_name(creature, true);
    }

    return true;
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

/*!
 * @brief モンスター種族の固定ドロップ指定 (drop_kind) を初期所持品として付与する
 * @details JSON の "drop_kind" で指定された固定ドロップは、モンスターとして
 *          ゲームを開始した場合に確率を無視して 100% 所持した状態で始める。
 *          グレード別の魔法付与は死亡時ドロップ (on_dead_drop_kind_item) と
 *          揃え、生成基準階はモンスター種族レベルを用いる。
 *          付与後は wield_all() で装備可能なものを自動装備する。
 * @param creature クリーチャーへの参照
 * @param monrace_id 開始モンスターの種族ID
 */
void grant_fixed_drop_items(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (monrace.drop_kinds.empty()) {
        return;
    }

    // 生成基準階はモンスター種族レベル (最低 1)。上限は ItemMagicApplier 内部で
    // MAX_DEPTH 未満にクランプされるためここでは下限のみ担保する。
    const auto magic_level = static_cast<DEPTH>(std::max<int>(monrace.level, 1));
    auto granted_any = false;
    for (const auto &kind : monrace.drop_kinds) {
        // num/deno は出現確率の指定だが、プレイヤー運用時は無視して必ず付与する。
        const short kind_idx = std::get<2>(kind);
        const int grade = std::get<3>(kind);
        const int dn = std::get<4>(kind);
        const int ds = std::get<5>(kind);
        const int drop_nums = Dice::roll(dn, ds);

        for (int i = 0; i < drop_nums; i++) {
            ItemEntity item;
            item.generate(kind_idx);
            switch (grade) {
            case -2:
                ItemMagicApplier(creature, &item, magic_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED).execute();
                break;
            case -1:
                ItemMagicApplier(creature, &item, magic_level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED).execute();
                break;
            case 0:
                ItemMagicApplier(creature, &item, magic_level, AM_NO_FIXED_ART).execute();
                break;
            case 1:
                ItemMagicApplier(creature, &item, magic_level, AM_NO_FIXED_ART | AM_GOOD).execute();
                break;
            case 2:
                ItemMagicApplier(creature, &item, magic_level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT).execute();
                break;
            case 3:
                ItemMagicApplier(creature, &item, magic_level, AM_GOOD | AM_GREAT | AM_SPECIAL).execute();
                if (!item.is_fixed_artifact()) {
                    become_random_artifact(creature, &item, false);
                }
                break;
            default:
                break;
            }

            object_aware(creature, item);
            item.mark_as_known();
            const auto slot = creature.store_item(item);
            if (slot >= 0) {
                autopick_alter_item(creature, slot, false);
                granted_any = true;
            }
        }
    }

    if (granted_any) {
        wield_all(creature);
    }
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

    // 性別をモンスター種族定義 (MALE/FEMALE) から決定する
    // (性格の性別制限に影響するため性格選択より前に確定させる)
    apply_monrace_sex(creature, *monrace_id);

    // モンスター種族に対応するプレイヤー種族があれば反映する
    // (複数該当時は無難な種族を優先。能力値計算に影響するため先に確定させる)
    apply_monrace_race(creature, *monrace_id);

    // モンスター種族に対応する職業があれば反映し、魔法領域を自動決定する
    // (HP/MP 計算に影響するため get_extra() より前に確定させる)
    apply_monrace_class(creature, *monrace_id);

    // 性格: モンスター種族に固定指定があれば常にそれを使い、
    // なければ通常キャラ作成と同様にプレイヤーが選択する
    // (能力値・ボーナスに影響するため get_stats() より前に確定させる)
    if (!apply_monrace_personality(creature, *monrace_id)) {
        term_clear();
        if (!select_player_personality(creature)) {
            return false;
        }
    }

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

    // モンスター種族に固定ドロップ指定 (drop_kind) があれば 100% 所持して開始する
    grant_fixed_drop_items(creature, *monrace_id);

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
