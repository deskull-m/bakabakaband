/*!
 * @brief モンスターの移動に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-move.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "monster-attack/monster-attack-processor.h"
#include "monster-floor/monster-movement-direction-list.h"
#include "monster-floor/monster-object.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
#include "player/player-status-flags.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static bool check_hp_for_terrain_destruction(const TerrainType &terrain, const MonsterEntity &monster)
{
    auto can_destroy = terrain.flags.has_not(TerrainCharacteristics::GLASS);
    can_destroy |= monster.get_monrace().behavior_flags.has(MonsterBehaviorType::STUPID);
    can_destroy |= monster.hp >= std::max(monster.maxhp / 3, 200);
    return can_destroy;
}

/*!
 * @brief モンスターによる壁の透過・破壊を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monster モンスターへの参照
 * @param pos モンスターの移動先座標
 * @param can_cross モンスターが地形を踏破できるならばTRUE
 * @return 透過も破壊もしなかった場合はFALSE、それ以外はTRUE
 */
static bool process_wall(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const Pos2D &pos, bool can_cross)
{
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (player_ptr->is_located_at(pos)) {
        turn_flags_ptr->do_move = true;
        return true;
    }

    if (grid.has_monster()) {
        turn_flags_ptr->do_move = true;
        return true;
    }

    using Mft = MonsterFeatureType;
    using Tc = TerrainCharacteristics;
    const auto &monrace = monster.get_monrace();
    auto can_kill_wall = monrace.feature_flags.has(Mft::KILL_WALL);
    can_kill_wall &= can_cross ? terrain.flags.has_not(Tc::LOS) : !turn_flags_ptr->is_riding_mon;
    can_kill_wall &= terrain.flags.has(Tc::HURT_DISI);
    can_kill_wall &= terrain.flags.has_not(Tc::PERMANENT);
    can_kill_wall &= check_hp_for_terrain_destruction(terrain, monster);
    if (can_kill_wall) {
        turn_flags_ptr->do_move = true;
        if (!can_cross) {
            turn_flags_ptr->must_alter_to_move = true;
        }

        turn_flags_ptr->did_kill_wall = true;
        return true;
    }

    if (!can_cross) {
        return false;
    }

    turn_flags_ptr->do_move = true;
    if ((monrace.feature_flags.has(Mft::PASS_WALL)) && (!turn_flags_ptr->is_riding_mon || has_pass_wall(player_ptr)) && terrain.flags.has(Tc::CAN_PASS)) {
        turn_flags_ptr->did_pass_wall = true;
    }

    return true;
}

/*!
 * @brief モンスターが普通のドアを開ける処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param monster モンスターへの参照
 * @param pos モンスターの移動先座標
 * @return ドアを打ち破るならここでの処理は実行せずtrue、開けるだけなら開けてfalseを返す
 * @todo 関数名と処理内容が不一致、後で直す
 */
static bool bash_normal_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const Pos2D &pos)
{
    const auto &monrace = monster.get_monrace();
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    turn_flags_ptr->do_move = false;
    using Tc = TerrainCharacteristics;
    auto can_bash = monrace.behavior_flags.has_not(MonsterBehaviorType::OPEN_DOOR);
    can_bash |= terrain.flags.has_not(Tc::OPEN);
    can_bash |= monster.is_pet() && ((player_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0);
    if (can_bash) {
        return true;
    }

    if (terrain.power == 0) {
        turn_flags_ptr->did_open_door = true;
        turn_flags_ptr->do_turn = true;
        return false;
    }

    if (randint0(monster.hp / 10) > terrain.power) {
        cave_alter_feat(player_ptr, pos.y, pos.x, Tc::DISARM);
        turn_flags_ptr->do_turn = true;
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがガラスのドアを開ける処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param monster モンスターへの参照
 * @param terrain 地形への参照
 * @param may_bash ドアを打ち破るならtrue、開けるだけならfalse
 * @todo 関数名と処理内容が不一致、後で直す
 */
static void bash_glass_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const TerrainType &terrain, bool may_bash)
{
    const auto &monrace = monster.get_monrace();
    auto can_bash = may_bash;
    can_bash &= monrace.behavior_flags.has(MonsterBehaviorType::BASH_DOOR);
    can_bash &= terrain.flags.has(TerrainCharacteristics::BASH);
    can_bash &= !monster.is_pet() || any_bits(player_ptr->pet_extra_flags, PF_OPEN_DOORS);
    if (!can_bash) {
        return;
    }

    if (!check_hp_for_terrain_destruction(terrain, monster) || (randint0(monster.hp / 10) <= terrain.power)) {
        return;
    }

    if (terrain.flags.has(TerrainCharacteristics::GLASS)) {
        msg_print(_("ガラスが砕ける音がした！", "You hear glass breaking!"));
    } else {
        msg_print(_("ドアを叩き開ける音がした！", "You hear a door burst open!"));
    }

    if (disturb_minor) {
        disturb(player_ptr, false, false);
    }

    turn_flags_ptr->did_bash_door = true;
    turn_flags_ptr->do_move = true;
    turn_flags_ptr->must_alter_to_move = true;
}

/*!
 * @brief モンスターによるドアの開放・破壊を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param monster モンスターへの参照
 * @param pos モンスターの移動先座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const Pos2D &pos)
{
    auto &monrace = monster.get_monrace();
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.has_closed_door_at(pos)) {
        return true;
    }

    const auto &grid = floor.get_grid(pos);
    auto &terrain = grid.get_terrain();
    auto may_bash = bash_normal_door(player_ptr, turn_flags_ptr, monster, pos);
    bash_glass_door(player_ptr, turn_flags_ptr, monster, terrain, may_bash);
    if (!turn_flags_ptr->did_open_door && !turn_flags_ptr->did_bash_door) {
        return true;
    }

    const auto &dungeon = floor.get_dungeon_definition();
    const auto is_open = dungeon.convert_terrain_id(grid.feat, TerrainCharacteristics::OPEN) == grid.feat;
    if (turn_flags_ptr->did_bash_door && (one_in_(2) || is_open || terrain.flags.has(TerrainCharacteristics::GLASS))) {
        cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::BASH);
        if (!monster.is_valid()) {
            auto &rfu = RedrawingFlagsUpdater::get_instance();
            rfu.set_flag(StatusRecalculatingFlag::FLOW);
            static constexpr auto flags = {
                SubWindowRedrawingFlag::OVERHEAD,
                SubWindowRedrawingFlag::DUNGEON,
            };
            rfu.set_flags(flags);
            if (is_original_ap_and_seen(player_ptr, monster)) {
                monrace.r_behavior_flags.set(MonsterBehaviorType::BASH_DOOR);
            }

            return false;
        }
    } else {
        cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::OPEN);
    }

    turn_flags_ptr->do_view = true;
    return true;
}

/*!
 * @brief 守りのルーンによるモンスターの移動制限を処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param pos モンスターの移動先座標
 * @return ルーンに侵入できるか否か
 */
static bool process_protection_rune(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const Pos2D &pos)
{
    auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &monrace = monster.get_monrace();
    auto can_enter = turn_flags_ptr->do_move;
    can_enter &= grid.is_rune_protection();
    can_enter &= (monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) || !player_ptr->is_located_at(pos);
    if (!can_enter) {
        return false;
    }

    turn_flags_ptr->do_move = false;
    if (monster.is_pet() || (randint1(BREAK_RUNE_PROTECTION) >= monrace.level)) {
        return true;
    }

    if (grid.is_mark()) {
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
    }

    grid.info &= ~(CAVE_MARK);
    grid.info &= ~(CAVE_OBJECT);
    grid.mimic = 0;
    turn_flags_ptr->do_move = true;
    note_spot(player_ptr, pos);
    return true;
}

/*!
 * @brief 爆発のルーンを処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param pos モンスターの移動先座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_explosive_rune(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const Pos2D &pos)
{
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &monrace = monster.get_monrace();
    auto should_explode = turn_flags_ptr->do_move;
    should_explode &= (monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) || !player_ptr->is_located_at(pos);
    if (!should_explode) {
        return true;
    }

    if (monster.is_pet() && grid.is_rune_explosion()) {
        turn_flags_ptr->do_move = false;
        return true;
    }

    activate_explosive_rune(player_ptr, pos, monrace);

    if (!monster.is_valid()) {
        return false;
    }

    turn_flags_ptr->do_move = true;
    return true;
}

/*!
 * @brief モンスターが壁を掘った後続処理を実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param monster モンスターへの参照
 * @param pos モンスターの移動先座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_post_dig_wall(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, const Pos2D &pos)
{
    auto &monrace = monster.get_monrace();
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (!turn_flags_ptr->did_kill_wall || !turn_flags_ptr->do_move) {
        return true;
    }

    constexpr auto chance_sound = 20;
    if (one_in_(chance_sound)) {
        if (terrain.flags.has(TerrainCharacteristics::GLASS)) {
            msg_print(_("何かの砕ける音が聞こえる。", "There is a crashing sound."));
        } else {
            msg_print(_("ギシギシいう音が聞こえる。", "There is a grinding sound."));
        }
    }

    cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_DISI);

    if (!monster.is_valid()) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::FLOW);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        rfu.set_flags(flags);
        if (is_original_ap_and_seen(player_ptr, monster)) {
            monrace.r_feature_flags.set(MonsterFeatureType::KILL_WALL);
        }

        return false;
    }

    turn_flags_ptr->do_view = true;
    turn_flags_ptr->do_turn = true;
    return true;
}

/*!
 * @brief 爆発のルーンを作動させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 爆発のルーンの位置
 * @param monrace モンスター種族への参照
 */
void activate_explosive_rune(PlayerType *player_ptr, const Pos2D &pos, const MonraceDefinition &monrace)
{
    auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto level = player_ptr->lev;
    if (!grid.is_rune_explosion()) {
        return;
    }

    if (randint1(BREAK_RUNE_EXPLOSION) > monrace.level) {
        if (grid.info & CAVE_MARK) {
            msg_print(_("ルーンが爆発した！", "The rune explodes!"));
            BIT_FLAGS project_flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI;
            project(player_ptr, 0, 2, pos.y, pos.x, 2 * (level + Dice::roll(7, 7)), AttributeType::MANA, project_flags);
        }
    } else {
        msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
    }

    reset_bits(grid.info, CAVE_MARK);
    reset_bits(grid.info, CAVE_OBJECT);
    grid.mimic = 0;

    note_spot(player_ptr, pos);
    lite_spot(player_ptr, pos);
}

/*!
 * @brief モンスターの移動に関するメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param mmdl モンスターの移動方向リスト
 * @param pos モンスターの移動前座標
 * @param count 移動回数 (のはず todo)
 * @return 移動が阻害される何か (ドア等)があったらFALSE
 * @todo 少し長いが、これといってブロックとしてまとまった部分もないので暫定でこのままとする
 */
bool process_monster_movement(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterMovementDirectionList &mmdl, const Pos2D &pos, int *count)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto m_idx = mmdl.get_m_idx();
    for (const auto &dir : mmdl.get_movement_directions()) {
        const auto &dir_move = dir.has_direction() ? dir : rand_choice(Direction::directions_8());
        const auto pos_neighbor = pos + dir_move.vec();
        if (!floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
            continue;
        }

        auto &grid = floor.get_grid(pos_neighbor);
        auto &monster = floor.m_list[m_idx];
        auto &monrace = monster.get_monrace();
        auto can_cross = monster_can_cross_terrain(player_ptr, grid.feat, monrace, turn_flags_ptr->is_riding_mon ? CEM_RIDING : 0);
        if (!process_wall(player_ptr, turn_flags_ptr, monster, pos_neighbor, can_cross)) {
            if (!process_door(player_ptr, turn_flags_ptr, monster, pos_neighbor)) {
                return false;
            }
        }

        if (!process_protection_rune(player_ptr, turn_flags_ptr, monster, pos_neighbor)) {
            if (!process_explosive_rune(player_ptr, turn_flags_ptr, monster, pos_neighbor)) {
                return false;
            }
        }

        exe_monster_attack_to_player(player_ptr, turn_flags_ptr, m_idx, pos_neighbor);
        if (process_monster_attack_to_monster(player_ptr, turn_flags_ptr, m_idx, grid, can_cross)) {
            return false;
        }

        if (turn_flags_ptr->is_riding_mon) {
            const auto &monster_riding = floor.m_list[player_ptr->riding];
            if (!player_ptr->riding_ryoute && !monster_riding.is_fearful()) {
                turn_flags_ptr->do_move = false;
            }
        }

        if (!process_post_dig_wall(player_ptr, turn_flags_ptr, monster, pos_neighbor)) {
            return false;
        }

        if (turn_flags_ptr->must_alter_to_move && monrace.feature_flags.has(MonsterFeatureType::AQUATIC)) {
            if (!monster_can_cross_terrain(player_ptr, grid.feat, monrace, turn_flags_ptr->is_riding_mon ? CEM_RIDING : 0)) {
                turn_flags_ptr->do_move = false;
            }
        }

        if (turn_flags_ptr->do_move && !can_cross && !turn_flags_ptr->did_kill_wall && !turn_flags_ptr->did_bash_door) {
            turn_flags_ptr->do_move = false;
        }

        if (turn_flags_ptr->do_move && monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE)) {
            if (is_original_ap_and_seen(player_ptr, monster)) {
                monrace.r_behavior_flags.set(MonsterBehaviorType::NEVER_MOVE);
            }

            turn_flags_ptr->do_move = false;
        }

        if (!turn_flags_ptr->do_move) {
            if (turn_flags_ptr->do_turn) {
                break;
            }

            continue;
        }

        turn_flags_ptr->do_turn = true;
        const auto &terrain = grid.get_terrain();
        auto can_recover_energy = terrain.flags.has(TerrainCharacteristics::TREE);
        can_recover_energy &= monrace.feature_flags.has_not(MonsterFeatureType::CAN_FLY);
        can_recover_energy &= monrace.wilderness_flags.has_not(MonsterWildernessType::WILD_WOOD);
        if (can_recover_energy) {
            monster.energy_need += ENERGY_NEED();
        }

        if (!update_riding_monster(player_ptr, turn_flags_ptr, m_idx, pos.y, pos.x, pos_neighbor.y, pos_neighbor.x)) {
            break;
        }

        const auto &apparent_monrace = monster.get_appearance_monrace();
        const auto p_pos = player_ptr->get_position(); //!< @details 関数が長すぎてプレイヤーの座標が不変であることを保証できない.
        const auto m_pos = monster.get_position();
        const auto is_projectable = projectable(floor, p_pos, m_pos);
        const auto can_see = disturb_near && monster.mflag.has(MonsterTemporaryFlagType::VIEW) && is_projectable;
        const auto is_high_level = disturb_high && (apparent_monrace.r_tkills > 0) && (apparent_monrace.level >= player_ptr->lev);
        const auto is_unknown_level = disturb_unknown && (apparent_monrace.r_tkills == 0);
        if (monster.ml && (disturb_move || can_see || is_high_level || is_unknown_level)) {
            if (monster.is_hostile()) {
                disturb(player_ptr, false, true);
            }
        }

        auto is_takable_or_killable = !grid.o_idx_list.empty();
        is_takable_or_killable &= monrace.behavior_flags.has_any_of({ MonsterBehaviorType::TAKE_ITEM, MonsterBehaviorType::KILL_ITEM });
        auto is_pickup_items = (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) != 0;
        is_pickup_items &= monrace.behavior_flags.has(MonsterBehaviorType::TAKE_ITEM);
        is_takable_or_killable &= !monster.is_pet() || is_pickup_items;
        if (!is_takable_or_killable) {
            if (turn_flags_ptr->do_turn) {
                break;
            }

            continue;
        }

        update_object_by_monster_movement(player_ptr, turn_flags_ptr, m_idx, pos_neighbor.y, pos_neighbor.x);
        if (turn_flags_ptr->do_turn) {
            break;
        }

        (*count)++;
    }

    return true;
}

static bool can_speak(const MonraceDefinition &ap_r_ref, MonsterSpeakType mon_speak_type)
{
    const auto can_speak_all = ap_r_ref.speak_flags.has(MonsterSpeakType::SPEAK_ALL);
    const auto can_speak_specific = ap_r_ref.speak_flags.has(mon_speak_type);
    return can_speak_all || can_speak_specific;
}

static tl::optional<MonsterMessageType> get_speak_type(const MonsterEntity &monster)
{
    const auto &ap_monrace = monster.get_appearance_monrace();
    if (monster.is_fearful() && can_speak(ap_monrace, MonsterSpeakType::SPEAK_FEAR)) {
        return MonsterMessageType::SPEAK_FEAR;
    }

    if (monster.is_pet() && can_speak(ap_monrace, MonsterSpeakType::SPEAK_BATTLE)) {
        return MonsterMessageType::SPEAK_BATTLE;
    }

    if (monster.is_friendly() && can_speak(ap_monrace, MonsterSpeakType::SPEAK_FRIEND)) {
        return MonsterMessageType::SPEAK_FRIEND;
    }

    if (can_speak(ap_monrace, MonsterSpeakType::SPEAK_BATTLE)) {
        return MonsterMessageType::SPEAK_BATTLE;
    }

    return tl::nullopt;
}

/*!
 * @brief 姿の見えないモンスターのメッセージを表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param message 対応するメッセージ
 */
void show_sound_message(PlayerType *player_ptr, std::string_view message)
{
    if (disturb_minor) {
        disturb(player_ptr, false, false);
    }
    msg_print(message);
}

/*!
 * @brief モンスターの足音を立てる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 */
void process_sound(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    if (monster.ml || player_ptr->skill_srh < randint1(100)) {
        return;
    }
    const auto m_name = std::string(_("それ", "It"));

    if (monster.cdis <= MAX_PLAYER_SIGHT / 2) {
        const auto message = monrace.get_message(m_name, MonsterMessageType::WALK_CLOSERANGE);
        if (message) {
            show_sound_message(player_ptr, *message);
        }
        return;
    }
    if (monster.cdis <= MAX_PLAYER_SIGHT) {
        const auto message = monrace.get_message(m_name, MonsterMessageType::WALK_MIDDLERANGE);
        if (message) {
            show_sound_message(player_ptr, *message);
        }
        return;
    }
    if (monster.cdis <= MAX_PLAYER_SIGHT * 2) {
        const auto message = monrace.get_message(m_name, MonsterMessageType::WALK_LONGRANGE);
        if (message) {
            show_sound_message(player_ptr, *message);
        }
        return;
    }
}

/*!
 * @brief モンスターを喋らせる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy モンスターが元々いたY座標
 * @param ox モンスターが元々いたX座標
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 */
void process_speak(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware)
{
    const Pos2D pos(oy, ox);
    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    constexpr auto chance_speak = 8;
    auto vociferous = monrace.r_misc_flags.has(MonsterMiscType::VOCIFEROUS) && (monster.cdis <= MAX_PLAYER_SIGHT * 2) && one_in_(chance_speak / 3 + 1);
    const auto p_pos = player_ptr->get_position();
    const auto can_speak = monster.get_appearance_monrace().speak_flags.any();
    if ((!vociferous) && (!can_speak || !aware || !floor.has_los_at({ oy, ox }) || !projectable(floor, pos, p_pos))) {
        return;
    }

    const auto m_name = monster.ml ? monster_desc(player_ptr, monster, 0) : std::string(_("それ", "It"));
    const auto message_type = get_speak_type(monster);
    if (!message_type) {
        return;
    }

    const auto monster_message = monrace.get_message(m_name, *message_type);
    if (monster_message) {
        msg_print(*monster_message);
    }
}
