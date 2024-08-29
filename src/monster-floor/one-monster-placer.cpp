/*!
 * @brief モンスターをフロアに1体配置する処理
 * @date 2020/06/13
 * @author Hourier
 */

#include "core/disturbance.h"
#include "core/speed-table.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-save-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "object/warning.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"
#include <time.h>

static bool is_friendly_idx(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    if (m_idx == 0) {
        return false;
    }

    const auto &m_ref = player_ptr->current_floor_ptr->m_list[m_idx];
    return m_ref.is_friendly();
}

/*!
 * @brief たぬきの変身対象となるモンスターかどうか判定する / Hook for Tanuki
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_tanuki(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    const auto &monrace = monraces_info[r_idx];
    bool unselectable = monrace.kind_flags.has(MonsterKindType::UNIQUE);
    unselectable |= monrace.misc_flags.has(MonsterMiscType::MULTIPLY);
    unselectable |= monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY);
    unselectable |= monrace.feature_flags.has(MonsterFeatureType::AQUATIC);
    unselectable |= monrace.misc_flags.has(MonsterMiscType::CHAMELEON);
    unselectable |= monrace.is_explodable();
    if (unselectable) {
        return false;
    }

    auto hook_pf = get_monster_hook(player_ptr);
    return hook_pf(player_ptr, r_idx);
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief モンスターの表層IDを設定する / Set initial racial appearance of a monster
 * @param r_idx モンスター種族ID
 * @return モンスター種族の表層ID
 */
static MonsterRaceId initial_r_appearance(PlayerType *player_ptr, MonsterRaceId r_idx, BIT_FLAGS generate_mode)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (is_chargeman(player_ptr) && any_bits(generate_mode, PM_JURAL) && none_bits(generate_mode, PM_MULTIPLY | PM_KAGE)) {
        return MonsterRaceId::ALIEN_JURAL;
    }

    if (monraces_info[r_idx].misc_flags.has_not(MonsterMiscType::TANUKI)) {
        return r_idx;
    }

    get_mon_num_prep(player_ptr, monster_hook_tanuki, nullptr);
    int attempts = 1000;
    DEPTH min = std::min(floor_ptr->base_level - 5, 50);
    while (--attempts) {
        auto ap_r_idx = get_mon_num(player_ptr, 0, floor_ptr->base_level + 10, PM_NONE);
        if (monraces_info[ap_r_idx].level >= min) {
            return ap_r_idx;
        }
    }

    return r_idx;
}

/*!
 * @brief ユニークが生成可能か評価する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @return ユニークの生成が不可能な条件ならFALSE、それ以外はTRUE
 */
static bool check_unique_placeable(const FloorType &floor, MonsterRaceId r_idx, BIT_FLAGS mode)
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return true;
    }

    if (any_bits(mode, PM_CLONE)) {
        return true;
    }

    auto *r_ptr = &monraces_info[r_idx];
    auto is_unique = r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->population_flags.has(MonsterPopulationType::NAZGUL);
    is_unique &= r_ptr->cur_num >= r_ptr->mob_num;
    if (is_unique) {
        return false;
    }

    if (r_ptr->population_flags.has(MonsterPopulationType::ONLY_ONE) && (r_ptr->cur_num >= 1)) {
        return false;
    }

    if (!MonraceList::get_instance().is_selectable(r_idx)) {
        return false;
    }

    const auto is_deep = r_ptr->misc_flags.has(MonsterMiscType::FORCE_DEPTH) && (floor.dun_level < r_ptr->level);
    const auto is_questor = !ironman_nightmare || r_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    return !is_deep || !is_questor;
}

/*!
 * @brief クエスト内に生成可能か評価する
 * @param floor フロアへの参照
 * @param r_idx 生成モンスター種族
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_quest_placeable(const FloorType &floor, MonsterRaceId r_idx)
{
    if (!inside_quest(floor.get_quest_id())) {
        return true;
    }

    const auto &quests = QuestList::get_instance();
    const auto quest_id = floor.get_quest_id();
    const auto &quest = quests.get_quest(quest_id);
    if ((quest.type != QuestKindType::KILL_LEVEL) && (quest.type != QuestKindType::RANDOM)) {
        return true;
    }
    if (r_idx != quest.r_idx) {
        return true;
    }
    int number_mon = 0;
    for (int i2 = 0; i2 < floor.width; ++i2) {
        for (int j2 = 0; j2 < floor.height; j2++) {
            auto quest_monster = floor.grid_array[j2][i2].has_monster();
            quest_monster &= (floor.m_list[floor.grid_array[j2][i2].m_idx].r_idx == quest.r_idx);
            if (quest_monster) {
                number_mon++;
            }
        }
    }

    if (number_mon + quest.cur_num >= quest.max_num) {
        return false;
    }
    return true;
}

/*!
 * @brief 守りのルーン上にモンスターの配置を試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_procection_rune(PlayerType *player_ptr, MonsterRaceId r_idx, POSITION y, POSITION x)
{
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    if (!g_ptr->is_rune_protection()) {
        return true;
    }

    auto *r_ptr = &monraces_info[r_idx];
    if (randint1(BREAK_RUNE_PROTECTION) >= (r_ptr->level + 20)) {
        return false;
    }

    if (any_bits(g_ptr->info, CAVE_MARK)) {
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
    }

    reset_bits(g_ptr->info, CAVE_MARK);
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;
    note_spot(player_ptr, y, x);
    return true;
}

static void warn_unique_generation(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    if (!player_ptr->warning || !w_ptr->character_dungeon) {
        return;
    }

    auto *r_ptr = &monraces_info[r_idx];
    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return;
    }

    std::string color;
    if (r_ptr->level > player_ptr->lev + 30) {
        color = _("黒く", "black");
    } else if (r_ptr->level > player_ptr->lev + 15) {
        color = _("紫色に", "purple");
    } else if (r_ptr->level > player_ptr->lev + 5) {
        color = _("ルビー色に", "deep red");
    } else if (r_ptr->level > player_ptr->lev - 5) {
        color = _("赤く", "red");
    } else if (r_ptr->level > player_ptr->lev - 15) {
        color = _("ピンク色に", "pink");
    } else {
        color = _("白く", "white");
    }

    auto *o_ptr = choose_warning_item(player_ptr);
    if (o_ptr != nullptr) {
        const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        msg_format(_("%sは%s光った。", "%s glows %s."), item_name.data(), color.data());
    } else {
        msg_format(_("%s光る物が頭に浮かんだ。", "A %s image forms in your mind."), color.data());
    }
}

/*!
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx 召喚を行ったモンスターID
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚主のモンスターID
 * @return 成功したらtrue
 */
bool place_monster_one(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION y, POSITION x, MonsterRaceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto *g_ptr = &floor.grid_array[y][x];
    auto *r_ptr = &monraces_info[r_idx];
    concptr name = r_ptr->name.data();

    if (player_ptr->wild_mode || !in_bounds(&floor, y, x) || !MonraceList::is_valid(r_idx)) {
        return false;
    }

    if (none_bits(mode, PM_IGNORE_TERRAIN) && (pattern_tile(&floor, y, x) || !monster_can_enter(player_ptr, y, x, r_ptr, 0))) {
        return false;
    }

    if (!check_unique_placeable(floor, r_idx, mode) || !check_quest_placeable(floor, r_idx) || !check_procection_rune(player_ptr, r_idx, y, x)) {
        return false;
    }

    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%s(Lv%d)を生成しました。", "%s(Lv%d) was generated."), name, r_ptr->level);
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->population_flags.has(MonsterPopulationType::NAZGUL) || (r_ptr->level < 10)) {
        reset_bits(mode, PM_KAGE);
    }

    g_ptr->m_idx = m_pop(&floor);
    hack_m_idx_ii = g_ptr->m_idx;
    if (!g_ptr->has_monster()) {
        return false;
    }

    MonsterEntity *m_ptr;
    m_ptr = &floor.m_list[g_ptr->m_idx];
    m_ptr->r_idx = r_idx;
    m_ptr->alliance_idx = r_ptr->alliance_idx;
    m_ptr->ap_r_idx = initial_r_appearance(player_ptr, r_idx, mode);

    m_ptr->mflag.clear();
    m_ptr->mflag2.clear();
    if (any_bits(mode, PM_MULTIPLY) && is_monster(src_idx) && !floor.m_list[src_idx].is_original_ap()) {
        m_ptr->ap_r_idx = floor.m_list[src_idx].ap_r_idx;
        if (floor.m_list[src_idx].mflag2.has(MonsterConstantFlagType::KAGE)) {
            m_ptr->mflag2.set(MonsterConstantFlagType::KAGE);
        }
    }

    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    if (t->tm_mon == 11 && t->tm_mday >= 24 && t->tm_mday <= 25 && one_in_(6)) {
        if (none_bits(mode, PM_MULTIPLY | PM_KAGE) && m_ptr->r_idx != MonsterRaceId::SANTA) {
            m_ptr->mflag2.set(MonsterConstantFlagType::SANTA);
        }
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE) && one_in_(15)) {
        m_ptr->mflag2.set(MonsterConstantFlagType::LARGE);
    }

    if (is_monster(src_idx) && r_ptr->kind_flags.has_none_of(alignment_mask)) {
        m_ptr->sub_align = floor.m_list[src_idx].sub_align;
    } else {
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            set_bits(m_ptr->sub_align, SUB_ALIGN_EVIL);
        }
        if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            set_bits(m_ptr->sub_align, SUB_ALIGN_GOOD);
        }
    }

    m_ptr->fy = y;
    m_ptr->fx = x;
    m_ptr->current_floor_ptr = &floor;

    for (int cmi = 0; cmi < MAX_MTIMED; cmi++) {
        m_ptr->mtimed[cmi] = 0;
    }

    m_ptr->cdis = 0;
    reset_target(m_ptr);
    m_ptr->nickname.clear();
    m_ptr->exp = 0;

    if (is_monster(src_idx) && floor.m_list[src_idx].is_pet()) {
        set_bits(mode, PM_FORCE_PET);
        m_ptr->parent_m_idx = src_idx;
    } else {
        m_ptr->parent_m_idx = 0;
    }

    if (r_ptr->misc_flags.has(MonsterMiscType::CHAMELEON)) {
        choose_new_monster(player_ptr, g_ptr->m_idx, true, MonraceList::empty_id(), summoner_m_idx);
        r_ptr = &m_ptr->get_monrace();
        m_ptr->mflag2.set(MonsterConstantFlagType::CHAMELEON);
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) && (!is_monster(src_idx))) {
            m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        }
    } else if (any_bits(mode, PM_KAGE) && none_bits(mode, PM_FORCE_PET)) {
        m_ptr->ap_r_idx = MonsterRaceId::KAGE;
        m_ptr->mflag2.set(MonsterConstantFlagType::KAGE);
    }

    if (any_bits(mode, PM_CLONE)) {
        m_ptr->mflag2.set(MonsterConstantFlagType::CLONED);
    }

    if (any_bits(mode, PM_NO_PET)) {
        m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
    }

    m_ptr->ml = false;
    if (any_bits(mode, PM_FORCE_PET)) {
        set_pet(player_ptr, m_ptr);
    } else if ((is_player(src_idx) && r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY)) || is_friendly_idx(player_ptr, src_idx) || any_bits(mode, PM_FORCE_FRIENDLY)) {
        if (!monster_has_hostile_align(player_ptr, nullptr, 0, -1, r_ptr) && !player_ptr->current_floor_ptr->inside_arena) {
            set_friendly(m_ptr);
        }
    }

    m_ptr->mtimed[MTIMED_CSLEEP] = 0;
    if (any_bits(mode, PM_ALLOW_SLEEP) && r_ptr->sleep && !ironman_nightmare) {
        int val = r_ptr->sleep;
        (void)set_monster_csleep(player_ptr, g_ptr->m_idx, (val * 2) + randint1(val * 10));
    }

    if (r_ptr->misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        m_ptr->max_maxhp = r_ptr->hit_dice.maxroll();
    } else {
        m_ptr->max_maxhp = r_ptr->hit_dice.roll();
    }

    if (m_ptr->mflag2.has(MonsterConstantFlagType::LARGE)) {
        m_ptr->max_maxhp *= (randint1(5) + 10) / 8;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, m_ptr->max_maxhp);
    }
    if (ironman_nightmare) {
        auto hp = m_ptr->max_maxhp * 2;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    m_ptr->maxhp = m_ptr->max_maxhp;
    if (r_ptr->cur_hp_per != 0) {
        m_ptr->hp = m_ptr->maxhp * r_ptr->cur_hp_per / 100;
    } else {
        m_ptr->hp = m_ptr->maxhp;
    }

    m_ptr->dealt_damage = 0;
    if (r_ptr->suicide_dice_num && r_ptr->suicide_dice_side) {
        m_ptr->death_count = Dice::roll(r_ptr->suicide_dice_num, r_ptr->suicide_dice_side);
    }

    m_ptr->set_individual_speed(floor.inside_arena);

    if (any_bits(mode, PM_HASTE)) {
        (void)set_monster_fast(player_ptr, g_ptr->m_idx, 100);
    }

    if (!ironman_nightmare) {
        m_ptr->energy_need = ENERGY_NEED() - (int16_t)randint0(100);
    } else {
        m_ptr->energy_need = ENERGY_NEED() - (int16_t)randint0(100) * 2;
    }

    if (!ironman_nightmare) {
        m_ptr->mflag.set(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }

    if (g_ptr->m_idx < hack_m_idx) {
        m_ptr->mflag.set(MonsterTemporaryFlagType::BORN);
    }

    auto is_awake_lightning_monster = r_ptr->brightness_flags.has_any_of(self_ld_mask);
    is_awake_lightning_monster |= r_ptr->brightness_flags.has_any_of(has_ld_mask) && !m_ptr->is_asleep();
    if (is_awake_lightning_monster) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    update_monster(player_ptr, g_ptr->m_idx, true);

    m_ptr->get_real_monrace().cur_num++;

    if (any_bits(mode, PM_AMBUSH)) {
        auto m_name = monster_desc(player_ptr, m_ptr, 0);
        msg_format(_("突如%sがあなたに襲い掛かってきた！", "Suddenly %s has ambushed you!"), m_name.data());
        disturb(player_ptr, false, true);
        MonsterAttackPlayer(player_ptr, g_ptr->m_idx).make_attack_normal();
    }

    /*
     * Memorize location of the unique monster in saved floors.
     * A unique monster move from old saved floor.
     */
    if (w_ptr->character_dungeon && (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->population_flags.has(MonsterPopulationType::NAZGUL))) {
        m_ptr->get_real_monrace().floor_id = player_ptr->floor_id;
    }

    if (r_ptr->misc_flags.has(MonsterMiscType::MULTIPLY)) {
        floor.num_repro++;
    }

    warn_unique_generation(player_ptr, r_idx);
    if (!g_ptr->is_rune_explosion()) {
        return true;
    }

    if (randint1(BREAK_RUNE_EXPLOSION) > r_ptr->level) {
        if (any_bits(g_ptr->info, CAVE_MARK)) {
            msg_print(_("ルーンが爆発した！", "The rune explodes!"));
            project(player_ptr, 0, 2, y, x, 2 * (player_ptr->lev + Dice::roll(7, 7)), AttributeType::MANA,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));
        }
    } else {
        msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
    }

    reset_bits(g_ptr->info, CAVE_MARK);
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);

    return true;
}
