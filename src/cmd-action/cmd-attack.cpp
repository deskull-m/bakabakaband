/*!
 * @brief 攻撃コマンド処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "cmd-action/cmd-attack.h"
#include "action/action-limited.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-object.h"
#include "game-option/cheat-types.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "object/item-use-flags.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-junk-types.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief プレイヤーの変異要素による打撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 攻撃目標となったモンスターの参照ID
 * @param attack 変異要素による攻撃要素の種類
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 */
static void natural_attack(PlayerType *player_ptr, MONSTER_IDX m_idx, PlayerMutationType attack, bool *fear, bool *mdeath)
{
    WEIGHT n_weight = 0;
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    Dice dice{};
    concptr atk_desc;
    switch (attack) {
    case PlayerMutationType::SCOR_TAIL:
        dice = Dice(3, 7);
        n_weight = 5;
        atk_desc = _("尻尾", "tail");
        break;
    case PlayerMutationType::HORNS:
        dice = Dice(2, 6);
        n_weight = 15;
        atk_desc = _("角", "horns");
        break;
    case PlayerMutationType::BEAK:
        dice = Dice(2, 4);
        n_weight = 5;
        atk_desc = _("クチバシ", "beak");
        break;
    case PlayerMutationType::TRUNK:
        dice = Dice(1, 4);
        n_weight = 35;
        atk_desc = _("象の鼻", "trunk");
        break;
    case PlayerMutationType::TENTACLES:
        dice = Dice(2, 5);
        n_weight = 5;
        atk_desc = _("触手", "tentacles");
        break;
    default:
        THROW_EXCEPTION(std::range_error, _("未定義の部位", "undefined body part"));
    }

    const auto m_name = monster_desc(player_ptr, monster, 0);
    int bonus = player_ptr->to_h_m + (player_ptr->lev * 6 / 5);
    int chance = (player_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

    player_ptr->plus_incident_tree("ATTACK_EXE_COUNT", 1);
    bool is_hit = (monrace.kind_flags.has_not(MonsterKindType::QUANTUM)) || !randint0(2);
    is_hit &= test_hit_norm(player_ptr, chance, monster.get_ac(), monster.ml);
    if (!is_hit) {
        sound(SoundKind::MISS);
        msg_format(_("ミス！ %sにかわされた。", "You miss %s."), m_name.data());
        return;
    }

    sound(SoundKind::HIT);
    msg_format(_("%sを%sで攻撃した。", "You hit %s with your %s."), m_name.data(), atk_desc);

    auto k = critical_norm(player_ptr, n_weight, bonus, dice.roll(), (int16_t)bonus, HISSATSU_NONE);
    k += player_ptr->to_d_m;
    if (k < 0) {
        k = 0;
    }

    k = mon_damage_mod(player_ptr, monster, k, false);
    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), k, monster.hp - k,
        monster.maxhp, monster.max_maxhp);
    if (k > 0) {
        anger_monster(player_ptr, monster);
    }

    switch (attack) {
    case PlayerMutationType::SCOR_TAIL:
        project(player_ptr, 0, 0, monster.y, monster.x, k, AttributeType::POIS, PROJECT_KILL);
        *mdeath = !monster.is_valid();
        break;
    case PlayerMutationType::HORNS:
    case PlayerMutationType::BEAK:
    case PlayerMutationType::TRUNK:
    case PlayerMutationType::TENTACLES:
    default: {
        MonsterDamageProcessor mdp(player_ptr, m_idx, k, fear, AttributeType::ATTACK);
        *mdeath = mdp.mon_take_hit("");
        break;
    }
    }

    touch_zap_player(monster, player_ptr);
}

/*!
 * @brief 頭突き攻撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 攻撃目標となったモンスターの参照ID
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 */
static void headbutt_attack(PlayerType *player_ptr, MONSTER_IDX m_idx, bool *fear, bool *mdeath)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    // 頭突きの基本パラメータ
    Dice dice(3, 6); // 3d6のダメージ（角攻撃より少し強力）
    WEIGHT n_weight = 20; // 重量（角攻撃より重い）
    concptr atk_desc = _("頭突き", "headbutt");

    const auto m_name = monster_desc(player_ptr, monster, 0);
    int bonus = player_ptr->to_h_m + (player_ptr->lev * 6 / 5);

    // 狂戦士状態の場合は命中とダメージにボーナス
    if (is_shero(player_ptr)) {
        bonus += 10;
        dice = Dice(4, 8); // より強力なダメージ
    }

    int chance = (player_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

    player_ptr->plus_incident_tree("ATTACK_EXE_COUNT", 1);
    player_ptr->plus_incident_tree("HEADBUTT", 1);
    bool is_hit = (monrace.kind_flags.has_not(MonsterKindType::QUANTUM)) || !randint0(2);
    is_hit &= test_hit_norm(player_ptr, chance, monster.get_ac(), monster.ml);

    if (!is_hit) {
        sound(SoundKind::MISS);
        msg_format(_("ミス！ %sに頭突きをかわされた。", "You miss %s with your headbutt."), m_name.data());

        // 頭突きを外した場合のペナルティ（少しふらつく）
        if (one_in_(4)) {
            msg_print(_("勢い余ってふらついた。", "You stagger from the missed headbutt."));
            BadStatusSetter bss(player_ptr);
            bss.set_stun(randint0(3) + 5);
        }
        return;
    }

    sound(SoundKind::HIT);
    msg_format(_("%sに%sを決めた！", "You deliver a %s to %s!"), m_name.data(), atk_desc);

    // クリティカル判定とダメージ計算
    auto k = critical_norm(player_ptr, n_weight, bonus, dice.roll(), (int16_t)bonus, HISSATSU_NONE);
    k += player_ptr->to_d_m;

    // 狂戦士状態の場合は追加ダメージ
    if (is_shero(player_ptr)) {
        k += randint1(10);
        msg_print(_("狂戦士の怒りが頭突きの威力を高めた！", "Your berserker rage enhances the headbutt!"));
    }

    if (k < 0) {
        k = 0;
    }

    k = mon_damage_mod(player_ptr, monster, k, false);
    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), k, monster.hp - k,
        monster.maxhp, monster.max_maxhp);

    if (k > 0) {
        anger_monster(player_ptr, monster);
    }

    // 頭突きによるダメージ処理
    MonsterDamageProcessor mdp(player_ptr, m_idx, k, fear, AttributeType::ATTACK);
    *mdeath = mdp.mon_take_hit(_("は頭突きで倒れた。", " falls from your headbutt."));

    // 頭突き後の反動処理
    if (!*mdeath && one_in_(6)) {
        msg_print(_("頭突きの反動で少しダメージを受けた。", "You take some damage from the headbutt recoil."));
        take_hit(player_ptr, DAMAGE_NOESCAPE, randint1(3), _("頭突きの反動", "headbutt recoil"));
    }

    touch_zap_player(monster, player_ptr);
}

/*!
 * @brief 体当たり攻撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 攻撃目標となったモンスターの参照ID
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 */
static void bodyslam_attack(PlayerType *player_ptr, MONSTER_IDX m_idx, bool *fear, bool *mdeath)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    // 体当たりの基本パラメータ（プレイヤーの体重や筋力に依存）
    Dice dice(2, 8); // 基本2d8のダメージ
    WEIGHT n_weight = 30; // 重い攻撃
    concptr atk_desc = _("体当たり", "body slam");

    // プレイヤーの体重による影響（推定）
    int body_weight_bonus = (player_ptr->lev + player_ptr->stat_index[A_STR]) / 3;

    const auto m_name = monster_desc(player_ptr, monster, 0);
    int bonus = player_ptr->to_h_m + (player_ptr->lev * 6 / 5) + body_weight_bonus;

    // 狂戦士状態や英雄状態での強化
    if (is_shero(player_ptr)) {
        bonus += 15;
        dice = Dice(3, 10); // より強力なダメージ
        atk_desc = _("猛烈な体当たり", "devastating body slam");
    } else if (is_hero(player_ptr)) {
        bonus += 8;
        dice = Dice(2, 10);
        atk_desc = _("勇猛な体当たり", "heroic body slam");
    }

    int chance = (player_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

    player_ptr->plus_incident_tree("ATTACK_EXE_COUNT", 1);
    bool is_hit = (monrace.kind_flags.has_not(MonsterKindType::QUANTUM)) || !randint0(2);
    is_hit &= test_hit_norm(player_ptr, chance, monster.get_ac(), monster.ml);

    if (!is_hit) {
        sound(SoundKind::MISS);
        msg_format(_("ミス！ %sに体当たりをかわされた。", "You miss %s with your body slam."), m_name.data());

        // 体当たりを外した場合のペナルティ（転倒リスク）
        if (one_in_(5)) {
            msg_print(_("勢い余って転倒しそうになった。", "You nearly fall from the missed body slam."));
            BadStatusSetter bss(player_ptr);
            bss.set_stun(randint0(4) + 3);
        }
        return;
    }

    sound(SoundKind::HIT);
    msg_format(_("%sに%sを叩き込んだ！", "You deliver a %s to %s!"), m_name.data(), atk_desc);

    // クリティカル判定とダメージ計算
    auto k = critical_norm(player_ptr, n_weight, bonus, dice.roll(), (int16_t)bonus, HISSATSU_NONE);
    k += player_ptr->to_d_m + body_weight_bonus;

    // 状態による追加ダメージ
    if (is_shero(player_ptr)) {
        k += randint1(15);
        msg_print(_("狂戦士の怒りが体当たりの威力を倍増させた！", "Your berserker rage doubles the body slam power!"));
    } else if (is_hero(player_ptr)) {
        k += randint1(8);
        msg_print(_("英雄の勇気が体当たりを強化した！", "Your heroic courage enhances the body slam!"));
    }

    if (k < 0) {
        k = 0;
    }

    k = mon_damage_mod(player_ptr, monster, k, false);
    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), k, monster.hp - k,
        monster.maxhp, monster.max_maxhp);

    if (k > 0) {
        anger_monster(player_ptr, monster);
    }

    // 体当たりによる特殊効果（ノックバック可能性）
    if (k > 20 && one_in_(4) && !monrace.resistance_flags.has(MonsterResistanceType::NO_STUN)) {
        msg_format(_("%sは体当たりでよろめいた！", "%s staggers from your body slam!"), m_name.data());
        (void)set_monster_stunned(player_ptr, m_idx, monster.get_remaining_stun() + randint1(5) + 5);
    }

    // 体当たりによるダメージ処理
    MonsterDamageProcessor mdp(player_ptr, m_idx, k, fear, AttributeType::ATTACK);
    *mdeath = mdp.mon_take_hit(_("は体当たりで倒れた。", " falls from your body slam."));

    // 体当たり後の自己ダメージ（反動）
    if (!*mdeath && one_in_(8)) {
        int self_damage = randint1(4);
        msg_print(_("体当たりの反動で体が痛んだ。", "You feel the recoil from your body slam."));
        take_hit(player_ptr, DAMAGE_NOESCAPE, self_damage, _("体当たりの反動", "body slam recoil"));
    }

    touch_zap_player(monster, player_ptr);
}

/*!
 * @brief プレイヤーの打撃処理メインルーチン
 * @param y 攻撃目標のY座標
 * @param x 攻撃目標のX座標
 * @param mode 発動中の剣術ID
 * @return 実際に攻撃処理が行われた場合TRUEを返す。
 * @details
 * If no "weapon" is available, then "punch" the monster one time.
 */
bool do_cmd_attack(PlayerType *player_ptr, POSITION y, POSITION x, combat_options mode)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.grid_array[y][x];
    const auto &monster = floor.m_list[grid.m_idx];
    const auto &monrace = monster.get_monrace();

    const auto mutation_attack_methods = {
        PlayerMutationType::HORNS,
        PlayerMutationType::BEAK,
        PlayerMutationType::SCOR_TAIL,
        PlayerMutationType::TRUNK,
        PlayerMutationType::TENTACLES,
    };

    disturb(player_ptr, false, true);

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (!can_attack_with_main_hand(player_ptr) && !can_attack_with_sub_hand(player_ptr) && player_ptr->muta.has_none_of(mutation_attack_methods)) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_(format("%s攻撃できない。", (empty_hands(player_ptr, false) == EMPTY_HAND_NONE) ? "両手がふさがって" : ""), "You cannot attack."));
        return false;
    }

    const auto m_name = monster_desc(player_ptr, monster, 0);
    const auto effects = player_ptr->effects();
    const auto is_hallucinated = effects->hallucination().is_hallucinated();
    if (monster.ml) {
        if (!is_hallucinated) {
            LoreTracker::get_instance().set_trackee(monster.ap_r_idx);
        }

        health_track(player_ptr, grid.m_idx);
    }

    const auto is_confused = effects->confusion().is_confused();
    const auto is_stunned = effects->stun().is_stunned();
    if (monster.is_female() && !(is_stunned || is_confused || is_hallucinated || !monster.ml)) {
        if (player_ptr->is_wielding(FixedArtifactId::ZANTETSU)) {
            sound(SoundKind::ATTACK_FAILED);
            msg_print(_("拙者、おなごは斬れぬ！", "I can not attack women!"));
            return false;
        }
    }

    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
        return false;
    }

    if (!monster.is_hostile() && !(is_stunned || is_confused || is_hallucinated || is_shero(player_ptr) || !monster.ml)) {
        if (player_ptr->is_wielding(FixedArtifactId::STORMBRINGER)) {
            msg_format(_("黒い刃は強欲に%sを攻撃した！", "Your black blade greedily attacks %s!"), m_name.data());
            chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::INDIVIDUALISM, 1);
            chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::HONOUR, -1);
            chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::JUSTICE, -1);
            chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::COMPASSION, -1);
        } else if (!PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
            if (input_check(_("本当に攻撃しますか？", "Really hit it? "))) {
                chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::INDIVIDUALISM, 1);
                chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::HONOUR, -1);
                chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::JUSTICE, -1);
                chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::COMPASSION, -1);
            } else {
                msg_format(_("%sを攻撃するのを止めた。", "You stop to avoid hitting %s."), m_name.data());
                return false;
            }
        }
    }

    if (effects->fear().is_fearful()) {
        if (monster.ml) {
            sound(SoundKind::ATTACK_FAILED);
            msg_format(_("恐くて%sを攻撃できない！", "You are too fearful to attack %s!"), m_name.data());
        } else {
            sound(SoundKind::ATTACK_FAILED);
            msg_format(_("そっちには何か恐いものがいる！", "There is something scary in your way!"));
        }

        (void)set_monster_csleep(player_ptr, grid.m_idx, 0);
        return false;
    }

    if (monster.is_asleep()) {
        if (monrace.kind_flags.has_not(MonsterKindType::EVIL) || one_in_(5)) {
            chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::COMPASSION, -1);
        }
        if (monrace.kind_flags.has_not(MonsterKindType::EVIL) || one_in_(5)) {
            chg_virtue(static_cast<CreatureEntity &>(*player_ptr), Virtue::HONOUR, -1);
        }
    }

    if (can_attack_with_main_hand(player_ptr) && can_attack_with_sub_hand(player_ptr)) {
        if (((player_ptr->skill_exp[PlayerSkillKindType::TWO_WEAPON] - 1000) / 200) < monrace.level) {
            PlayerSkill(player_ptr).gain_two_weapon_skill_exp();
        }
    }

    if (player_ptr->riding) {
        PlayerSkill(player_ptr).gain_riding_skill_exp_on_melee_attack(monrace);
    }

    player_ptr->plus_incident_tree("ATTACK_ACT_COUNT", 1);

    player_ptr->riding_t_m_idx = grid.m_idx;
    bool fear = false;
    bool mdeath = false;
    if (can_attack_with_main_hand(player_ptr)) {
        exe_player_attack_to_monster(player_ptr, y, x, &fear, &mdeath, 0, mode);
    }
    if (can_attack_with_sub_hand(player_ptr) && !mdeath) {
        exe_player_attack_to_monster(player_ptr, y, x, &fear, &mdeath, 1, mode);
    }

    if (!mdeath) {
        for (auto m : mutation_attack_methods) {
            if (player_ptr->muta.has(m) && !mdeath) {
                natural_attack(player_ptr, grid.m_idx, m, &fear, &mdeath);
            }
        }
    }

    if (fear && monster.ml && !mdeath) {
        // 1/20の確率で恐怖せず狂乱状態になる
        if (one_in_(20)) {
            auto &current_monster = floor.m_list[grid.m_idx];
            current_monster.mflag2.set(MonsterConstantFlagType::FRENZY);
            (void)set_monster_monfear(player_ptr, grid.m_idx, 0);
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖を怒りに変えて狂乱状態になった！", "%s^ turns fear into rage and goes berserk!"), m_name.data());
            fear = false;
        } else {
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), m_name.data());
        }
    }

    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::IAI) && ((mode != HISSATSU_IAI) || mdeath)) {
        set_action(player_ptr, ACTION_NONE);
    }

    return mdeath;
}

/*!
 * @brief 頭突きコマンド処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 実際に攻撃処理が行われた場合TRUEを返す。
 */
bool do_cmd_headbutt(PlayerType *player_ptr)
{
    const auto dir = get_direction(player_ptr);
    if (!dir) {
        return false;
    }

    const auto pos = player_ptr->get_neighbor(dir);

    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);

    if (!grid.has_monster()) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_("そこには敵がいない。", "There is no monster there."));
        return false;
    }

    const auto &monster = floor.m_list[grid.m_idx];
    const auto m_name = monster_desc(player_ptr, monster, 0);

    // エネルギー消費
    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    // 混乱状態では方向がずれる可能性
    const auto effects = player_ptr->effects();
    const auto is_confused = effects->confusion().is_confused();
    if (is_confused && one_in_(3)) {
        msg_print(_("混乱して方向を間違えた！", "You are confused and miss the direction!"));
        return false;
    }

    // 恐怖状態では攻撃できない
    if (effects->fear().is_fearful()) {
        sound(SoundKind::ATTACK_FAILED);
        msg_format(_("恐くて%sに頭突きできない！", "You are too fearful to headbutt %s!"), m_name.data());
        return false;
    }

    // ダンジョンが近接攻撃禁止の場合
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_("なぜか頭突きすることができない。", "Something prevents you from headbutting."));
        return false;
    }

    // 敵対的でないモンスターへの確認
    const auto is_stunned = effects->stun().is_stunned();
    const auto is_hallucinated = effects->hallucination().is_hallucinated();
    if (!monster.is_hostile() && !(is_stunned || is_confused || is_hallucinated || is_shero(player_ptr) || !monster.ml)) {
        if (!PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
            if (!input_check(_("本当に頭突きしますか？", "Really headbutt it? "))) {
                msg_format(_("%sへの頭突きを止めた。", "You stop to avoid headbutting %s."), m_name.data());
                return false;
            }
        }
    }

    // モンスターを起こす
    (void)set_monster_csleep(player_ptr, grid.m_idx, 0);

    // 頭突き攻撃実行
    bool fear = false;
    bool mdeath = false;

    msg_format(_("%sに向かって勢いよく頭突きした！", "You charge forward with a powerful headbutt at %s!"), m_name.data());
    headbutt_attack(player_ptr, grid.m_idx, &fear, &mdeath);

    if (fear && monster.ml && !mdeath) {
        // 1/20の確率で恐怖せず狂乱状態になる
        if (one_in_(20)) {
            auto &current_monster = floor.m_list[grid.m_idx];
            current_monster.mflag2.set(MonsterConstantFlagType::FRENZY);
            (void)set_monster_monfear(player_ptr, grid.m_idx, 0);
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖を怒りに変えて狂乱状態になった！", "%s^ turns fear into rage and goes berserk!"), m_name.data());
        } else {
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), m_name.data());
        }
    }

    return true;
}

/*!
 * @brief 体当たりコマンドの実行
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_body_slam(PlayerType *player_ptr)
{
    const auto dir = get_direction(player_ptr);
    if (!dir) {
        return;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);

    if (!grid.has_monster()) {
        msg_print(_("そこには敵がいない。", "There is no enemy there."));
        return;
    }

    const auto m_idx = grid.m_idx;
    const auto &monster = floor.m_list[m_idx];

    auto m_name = monster_desc(player_ptr, monster, 0);

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    // 混乱状態では方向がずれる可能性
    const auto effects = player_ptr->effects();
    const auto is_confused = effects->confusion().is_confused();
    if (is_confused && one_in_(3)) {
        msg_print(_("混乱して方向を間違えた！", "You are confused and miss the direction!"));
        return;
    }

    // 恐怖状態では攻撃できない
    if (effects->fear().is_fearful()) {
        sound(SoundKind::ATTACK_FAILED);
        msg_format(_("恐くて%sに体当たりできない！", "You are too fearful to body slam %s!"), m_name.data());
        return;
    }

    // ダンジョンが近接攻撃禁止の場合
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_("なぜか体当たりすることができない。", "Something prevents you from body slamming."));
        return;
    }

    bool fear = false;
    bool mdeath = false;

    msg_format(_("%sに向かって全力で体当たりを仕掛けた！", "You charge at %s with a full body slam!"), m_name.data());
    bodyslam_attack(player_ptr, m_idx, &fear, &mdeath);

    if (fear && monster.ml && !mdeath) {
        // 1/20の確率で恐怖せず狂乱状態になる
        if (one_in_(20)) {
            auto &current_monster = floor.m_list[m_idx];
            current_monster.mflag2.set(MonsterConstantFlagType::FRENZY);
            (void)set_monster_monfear(player_ptr, m_idx, 0);
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖を怒りに変えて狂乱状態になった！", "%s^ turns fear into rage and goes berserk!"), m_name.data());
        } else {
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), m_name.data());
        }
    }
}

/*!
 * @brief 浣腸攻撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 攻撃目標となったモンスターの参照ID
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 */
static void enema_attack(PlayerType *player_ptr, MONSTER_IDX m_idx, bool *fear, bool *mdeath)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    // 浣腸の基本パラメータ（プレイヤーの体重や筋力に依存）
    Dice dice(2, 4); // 基本2d4のダメージ
    WEIGHT n_weight = 10; // 重い攻撃
    concptr atk_desc = _("浣腸", "enema");

    const auto m_name = monster_desc(player_ptr, monster, 0);
    int bonus = player_ptr->to_h_m + (player_ptr->lev * 6 / 5) + (player_ptr->lev + player_ptr->stat_index[A_DEX]) / 3;
    ;

    int chance = (player_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));

    player_ptr->plus_incident_tree("ATTACK_EXE_COUNT", 1);
    bool is_hit = (monrace.kind_flags.has_not(MonsterKindType::QUANTUM)) || !randint0(2);
    is_hit &= test_hit_norm(player_ptr, chance, monster.get_ac(), monster.ml);

    if (!is_hit) {
        sound(SoundKind::MISS);
        msg_format(_("ミス！ %sに浣腸をかわされた。", "You miss %s with your enema."), m_name.data());
    }

    sound(SoundKind::HIT);
    msg_format(_("%sに%sを叩き込んだ！", "You deliver a %s to %s!"), m_name.data(), atk_desc);

    // クリティカル判定とダメージ計算
    auto k = critical_norm(player_ptr, n_weight, bonus, dice.roll(), (int16_t)bonus, HISSATSU_NONE);
    k += player_ptr->to_d_m;

    if (k < 0) {
        k = 0;
    }

    if (monster.maxhp / 100 > k && !monrace.r_kind_flags.has(MonsterKindType::NONLIVING)) {
        auto &baseitems = BaseitemList::get_instance();
        ItemEntity item;
        msg_print(_("ブッチッパ！", "BRUUUUP! Oops."));
        msg_erase();

        // 脱糞させたインシデントを記録
        player_ptr->plus_incident_tree("ATTACK_EXE_COUNT/DEFECATION", 1);

        item.generate(baseitems.lookup_baseitem_id({ ItemKindType::JUNK, SV_JUNK_FECES }));
        (void)drop_near(player_ptr, item, player_ptr->get_position());
    }

    k = mon_damage_mod(player_ptr, monster, k, false);
    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), k, monster.hp - k,
        monster.maxhp, monster.max_maxhp);

    if (k > 0) {
        anger_monster(player_ptr, monster);
    }

    // 浣腸によるダメージ処理
    MonsterDamageProcessor mdp(player_ptr, m_idx, k, fear, AttributeType::ATTACK);
    *mdeath = mdp.mon_take_hit(_("は浣腸で逝った。", " falls from your enema."));

    touch_zap_player(monster, player_ptr);
}

/*!
 * @brief 浣腸コマンドの実行
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_enema(PlayerType *player_ptr)
{
    const auto dir = get_direction(player_ptr);
    if (!dir) {
        return;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);

    if (!grid.has_monster()) {
        msg_print(_("そこには敵がいない。", "There is no enemy there."));
        return;
    }

    const auto m_idx = grid.m_idx;
    const auto &monster = floor.m_list[m_idx];

    auto m_name = monster_desc(player_ptr, monster, 0);

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    // 混乱状態では方向がずれる可能性
    const auto effects = player_ptr->effects();
    const auto is_confused = effects->confusion().is_confused();
    if (is_confused && one_in_(3)) {
        msg_print(_("混乱して方向を間違えた！", "You are confused and miss the direction!"));
        return;
    }

    // 恐怖状態では攻撃できない
    if (effects->fear().is_fearful()) {
        sound(SoundKind::ATTACK_FAILED);
        msg_format(_("恐くて%sに浣腸できない！", "You are too fearful to enema %s!"), m_name.data());
        return;
    }

    // ダンジョンが近接攻撃禁止の場合
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_("なぜか浣腸することができない。", "Something prevents you from enemaing."));
        return;
    }

    bool fear = false;
    bool mdeath = false;

    msg_format(_("%sに向かって全力で浣腸を仕掛けた！", "You charge at %s with a full enema!"), m_name.data());
    enema_attack(player_ptr, m_idx, &fear, &mdeath);

    if (fear && monster.ml && !mdeath) {
        // 1/20の確率で恐怖せず狂乱状態になる
        if (one_in_(20)) {
            auto &current_monster = floor.m_list[m_idx];
            current_monster.mflag2.set(MonsterConstantFlagType::FRENZY);
            (void)set_monster_monfear(player_ptr, m_idx, 0);
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖を怒りに変えて狂乱状態になった！", "%s^ turns fear into rage and goes berserk!"), m_name.data());
        } else {
            sound(SoundKind::FLEE);
            msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), m_name.data());
        }
    }
}
