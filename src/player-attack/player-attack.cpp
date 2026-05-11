/*!
 * @brief プレイヤーからモンスターへの打撃処理
 * @date 2020/05/22
 * @author Hourier
 */

#include "player-attack/player-attack.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-attack.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "combat/martial-arts-style.h"
#include "combat/martial-arts-table.h"
#include "combat/slaying.h"
#include "floor/geometry.h"
#include "game-option/cheat-types.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "mind/mind-samurai.h"
#include "mind/monk-attack.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/vorpal-weapon.h"
#include "object-hook/hook-weapon.h"
#include "object/tval-types.h"
#include "player-attack/attack-chaos-effect.h"
#include "player-attack/blood-sucking-processor.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/earthquake.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

/*! 吸血処理の最大回復HP */
constexpr auto MAX_VAMPIRIC_DRAIN = 50;

/*!
 * @brief プレイヤーの攻撃情報を初期化する(コンストラクタ以外の分)
 */
player_attack_type::player_attack_type(FloorType &floor, POSITION y, POSITION x, int16_t hand, combat_options mode, bool *fear, bool *mdeath)
    : hand(hand)
    , mode(mode)
    , fear(fear)
    , mdeath(mdeath)
    , drain_left(MAX_VAMPIRIC_DRAIN)
    , g_ptr(&floor.grid_array[y][x])
    , chaos_effect(CE_NONE)
    , magical_effect(MagicalBrandEffectType::NONE)
{
    this->m_idx = this->g_ptr->m_idx;
    this->m_ptr = &floor.get_monster(this->g_ptr->m_idx);
    this->r_idx = this->m_ptr->get_r_idx();
    this->r_ptr = &this->m_ptr->get_monrace();
    this->ma_ptr = &ma_blows[0];
}

/*!
 * @brief 一部職業で攻撃に倍率がかかったりすることの処理
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void attack_classify(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    switch (creature.pclass) {
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA:
        process_surprise_attack(creature, pa_ptr);
        return;
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
    case PlayerClassType::BERSERKER:
        if ((empty_hands(creature, true) & EMPTY_HAND_MAIN) && !creature.get_riding()) {
            pa_ptr->monk_attack = true;
        }
        return;
    default:
        return;
    }
}

/*!
 * @brief マーシャルアーツの技能値を増加させる
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void get_bare_knuckle_exp(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    if ((monrace.level + 10) <= creature.level) {
        return;
    }

    PlayerSkill(creature).gain_martial_arts_skill_exp();
}

/*!
 * @brief 装備している武器の技能値を増加させる
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void get_weapon_exp(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();

    PlayerSkill(creature).gain_melee_weapon_exp(o_ptr);
}

/*!
 * @brief 直接攻撃に伴う技能値の上昇処理
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void get_attack_exp(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    if (!o_ptr->is_valid()) {
        get_bare_knuckle_exp(creature, pa_ptr);
        return;
    }

    if (!o_ptr->is_melee_weapon() || ((monrace.level + 10) <= creature.level)) {
        return;
    }

    get_weapon_exp(creature, pa_ptr);
}

/*!
 * @brief 攻撃回数を決定する
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @details 毒針は確定で1回
 */
static void calc_num_blow(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    if ((pa_ptr->mode == HISSATSU_KYUSHO) || (pa_ptr->mode == HISSATSU_MINEUCHI) || (pa_ptr->mode == HISSATSU_3DAN) || (pa_ptr->mode == HISSATSU_IAI)) {
        pa_ptr->num_blow = 1;
    } else if (pa_ptr->mode == HISSATSU_COLD) {
        pa_ptr->num_blow = creature.num_blow[pa_ptr->hand] + 2;
    } else {
        pa_ptr->num_blow = creature.num_blow[pa_ptr->hand];
    }

    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        pa_ptr->num_blow = 1;
    }
}

/*!
 * @brief 混沌属性の武器におけるカオス効果を決定する
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接放撃構造体への参照ポインタ
 * @return カオス効果
 * @details
 * 吸蠀20%、地陇0.12%、混乱26.892%、テレポート・アウェイ1.494%、変身1.494% /
 * Vampiric 20%, Quake 0.12%, Confusion 26.892%, Teleport away 1.494% and Polymorph 1.494%
 */
static chaotic_effect select_chaotic_effect(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    if (pa_ptr->flags.has_not(TR_CHAOTIC) || one_in_(2)) {
        return CE_NONE;
    }

    if (one_in_(10)) {
        chg_virtue(creature, Virtue::CHANCE, 1);
    }

    if (randint1(5) < 3) {
        return CE_VAMPIRIC;
    }

    if (one_in_(250)) {
        return CE_QUAKE;
    }

    if (!one_in_(10)) {
        return CE_CONFUSION;
    }

    return one_in_(2) ? CE_TELE_AWAY : CE_POLYMORPH;
}

/*!
 * @brief 魔術属性による追加ダイス数を返す
 * @param creature クリーチャーへの参照
 * @param pa_ptr プレイヤー攻撃情報への参照ポインタ
 * @return 魔術属性効果
 */
static MagicalBrandEffectType select_magical_brand_effect(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    if (pa_ptr->flags.has_not(TR_BRAND_MAGIC)) {
        return MagicalBrandEffectType::NONE;
    }

    if (one_in_(10)) {
        chg_virtue(creature, Virtue::CHANCE, 1);
    }

    if (one_in_(5)) {
        return MagicalBrandEffectType::STUN;
    }

    if (one_in_(5)) {
        return MagicalBrandEffectType::SCARE;
    }

    if (one_in_(10)) {
        return MagicalBrandEffectType::DISPELL;
    }

    if (one_in_(16)) {
        return MagicalBrandEffectType::PROBE;
    }

    return MagicalBrandEffectType::EXTRA;
}

/*!
 * @brief 魔法属性による追加ダイス数を返す
 * @param pa_ptr プレイヤー攻撃情報への参照ポインタ
 * @return ダイス数
 */
static int magical_brand_extra_dice(player_attack_type *pa_ptr)
{
    switch (pa_ptr->magical_effect) {
    case MagicalBrandEffectType::NONE:
        return 0;
    case MagicalBrandEffectType::EXTRA:
        return 1;
    default:
        return 2;
    }
}

/*!
 * @brief 装備品が地震を起こすか判定
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 地震を起こすならtrue、起こさないならfalse
 * @details
 * 打撃に使用する武器または武器以外の装備品が地震を起こすなら、
 * ダメージ量が50より多いか1/7で地震を起こす
 */
static bool does_equip_cause_earthquake(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    if (!creature.has_earthquake_flag()) {
        return false;
    }

    auto do_quake = false;

    auto hand = (pa_ptr->hand == 0) ? FLAG_CAUSE_INVEN_MAIN_HAND : FLAG_CAUSE_INVEN_SUB_HAND;
    if (any_bits(creature.earthquake, hand)) {
        do_quake = true;
    } else {
        auto flags = creature.earthquake;
        reset_bits(flags, FLAG_CAUSE_INVEN_MAIN_HAND | FLAG_CAUSE_INVEN_SUB_HAND);
        do_quake = flags != 0;
    }

    if (do_quake) {
        return pa_ptr->attack_damage > 50 || one_in_(7);
    }

    return false;
}

/*!
 * @brief 手にしている装備品がフラグを持つか判定
 * @param attacker_flags 装備状況で集計されたフラグ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 持つならtrue、持たないならfalse
 */
static bool does_weapon_has_flag(BIT_FLAGS &attacker_flags, player_attack_type *pa_ptr)
{
    if (!attacker_flags) {
        return false;
    }

    auto hand = (pa_ptr->hand == 0) ? FLAG_CAUSE_INVEN_MAIN_HAND : FLAG_CAUSE_INVEN_SUB_HAND;
    if (any_bits(attacker_flags, hand)) {
        return true;
    }

    auto flags = attacker_flags;
    reset_bits(flags, FLAG_CAUSE_INVEN_MAIN_HAND | FLAG_CAUSE_INVEN_SUB_HAND);
    return flags != 0;
}

/*!
 * @brief 武器による直接攻撃メインルーチン
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param vorpal_cut メッタ斬りにできるかどうか
 * @param vorpal_chance ヴォーパル倍率上昇の機会値
 * @return 攻撃の結果、地震を起こすことになったらTRUE、それ以外はFALSE
 */
static void process_weapon_attack(CreatureEntity &creature, player_attack_type *pa_ptr, bool *do_quake, const bool vorpal_cut, const int vorpal_chance)
{
    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    const auto num = o_ptr->damage_dice.num + creature.damage_dice_bonus[pa_ptr->hand].num + magical_brand_extra_dice(pa_ptr);
    const auto sides = o_ptr->damage_dice.sides + creature.damage_dice_bonus[pa_ptr->hand].sides;
    pa_ptr->attack_damage = calc_attack_damage_with_slay(creature, o_ptr, Dice::roll(num, sides), *pa_ptr->m_ptr, pa_ptr->mode, false);
    calc_surprise_attack_damage(creature, pa_ptr);

    if (does_equip_cause_earthquake(creature, pa_ptr) || (pa_ptr->chaos_effect == CE_QUAKE) || (pa_ptr->mode == HISSATSU_QUAKE)) {
        *do_quake = true;
    }

    auto do_impact = does_weapon_has_flag(creature.impact, pa_ptr);
    if ((o_ptr->bi_key != BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) && !(pa_ptr->mode == HISSATSU_KYUSHO)) {
        pa_ptr->attack_damage = critical_norm(creature, o_ptr->weight, o_ptr->to_h, pa_ptr->attack_damage, creature.to_h[pa_ptr->hand], pa_ptr->mode, do_impact);
    }

    pa_ptr->drain_result = pa_ptr->attack_damage;
    process_vorpal_attack(creature, pa_ptr, vorpal_cut, vorpal_chance);
    pa_ptr->attack_damage += o_ptr->to_d;
    pa_ptr->drain_result += o_ptr->to_d;
}

/*!
 * @brief 武器または素手による攻撃ダメージを計算する
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param do_quake 攻撃の結果、地震を起こすことになったらTRUE、それ以外はFALSE
 * @param vorpal_cut メッタ斬りにできるかどうか
 * @param vorpal_change ヴォーパル倍率上昇の機会値
 * @details 取り敢えず素手と仮定し1とする.
 */
static void calc_attack_damage(CreatureEntity &creature, player_attack_type *pa_ptr, bool *do_quake, const bool vorpal_cut, const int vorpal_chance)
{
    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    pa_ptr->attack_damage = 1;
    if (pa_ptr->monk_attack) {
        process_monk_attack(creature, pa_ptr);
        return;
    }

    if (o_ptr->is_valid()) {
        process_weapon_attack(creature, pa_ptr, do_quake, vorpal_cut, vorpal_chance);
    }
}

/*!
 * @brief 武器のダメージボーナスや剣術家の技によってダメージにボーナスを与える
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void apply_damage_bonus(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    pa_ptr->attack_damage += creature.to_d[pa_ptr->hand];
    pa_ptr->drain_result += creature.to_d[pa_ptr->hand];

    if ((pa_ptr->mode == HISSATSU_SUTEMI) || (pa_ptr->mode == HISSATSU_3DAN)) {
        pa_ptr->attack_damage *= 2;
    }

    if ((pa_ptr->mode == HISSATSU_SEKIRYUKA) && !pa_ptr->m_ptr->has_living_flag()) {
        pa_ptr->attack_damage = 0;
    }

    auto is_cut = creature.is_cut();
    if ((pa_ptr->mode == HISSATSU_SEKIRYUKA) && !is_cut) {
        pa_ptr->attack_damage /= 2;
    }
}

/*!
 * @brief 特殊な条件でダメージが減ったり0になったりする処理
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param is_zantetsu_nullified 斬鉄剣で切れないならばTRUE
 * @param is_ej_nullified 蜘蛛相手ならばTRUE
 * @details ダメージが0未満なら0に補正する
 * @todo かなりのレアケースだが、右手に混沌属性の武器を持ち、左手にエクスカリバー・ジュニアを持ち、
 * 右手の最終打撃で蜘蛛に変身したとしても、左手の攻撃でダメージが減らない気がする
 * モンスターへの参照ポインタは変身時に変わるのにis_ej_nullifiedはその前に代入されて参照されるだけであるため
 */
static void apply_damage_negative_effect(player_attack_type *pa_ptr, bool is_zantetsu_nullified, bool is_ej_nullified)
{
    if (pa_ptr->attack_damage < 0) {
        pa_ptr->attack_damage = 0;
    }

    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    if ((pa_ptr->mode == HISSATSU_ZANMA) && !(!pa_ptr->m_ptr->has_living_flag() && monrace.kind_flags.has(MonsterKindType::EVIL))) {
        pa_ptr->attack_damage = 0;
    }

    if (is_zantetsu_nullified) {
        sound(SoundKind::ATTACK_FAILED);
        msg_print(_("こんな軟らかいものは切れん！", "You cannot cut such an elastic thing!"));
        pa_ptr->attack_damage = 0;
    }

    if (is_ej_nullified) {
        msg_print(_("蜘蛛は苦手だ！", "Spiders are difficult for you to deal with!"));
        pa_ptr->attack_damage /= 2;
    }
}

/*!
 * @brief モンスターのHPを減らした後、恐怖させるか死なす (フロアから消滅させる)
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 死んだらTRUE、生きていたらFALSE
 */
static bool check_fear_death(CreatureEntity &creature, player_attack_type *pa_ptr, const int num, const bool is_lowlevel)
{
    MonsterDamageProcessor mdp(creature, pa_ptr->m_idx, pa_ptr->attack_damage, pa_ptr->fear, pa_ptr->attribute_flags);
    if (!mdp.mon_take_hit("")) {
        return false;
    }

    *(pa_ptr->mdeath) = true;
    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER) && creature.energy_use) {
        PlayerEnergy energy(creature);
        if (can_attack_with_main_hand(creature) && can_attack_with_sub_hand(creature)) {
            ENERGY energy_use;
            if (pa_ptr->hand) {
                energy_use = creature.energy_use * 3 / 5 + creature.energy_use * num * 2 / (creature.num_blow[pa_ptr->hand] * 5);
            } else {
                energy_use = creature.energy_use * num * 3 / (creature.num_blow[pa_ptr->hand] * 5);
            }

            energy.set_player_turn_energy(energy_use);
        } else {
            auto energy_use = (ENERGY)(creature.energy_use * num / creature.num_blow[pa_ptr->hand]);
            energy.set_player_turn_energy(energy_use);
        }
    }

    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    if ((o_ptr->is_specific_artifact(FixedArtifactId::ZANTETSU)) && is_lowlevel) {
        msg_print(_("またつまらぬものを斬ってしまった．．．", "Sigh... Another trifling thing I've cut...."));
    }

    return true;
}

/*!
 * @brief 直接攻撃が当たった時の処理
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param do_quake 攻撃後に地震を起こすかどうか
 * @param is_zantetsu_nullified 斬鉄剣で切れないならばTRUE
 * @param is_ej_nullified 蜘蛛相手ならばTRUE
 */
static void apply_actual_attack(
    CreatureEntity &creature, player_attack_type *pa_ptr, bool *do_quake, const bool is_zantetsu_nullified, const bool is_ej_nullified)
{
    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    int vorpal_chance = (o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD)) ? 2 : 4;

    sound(SoundKind::HIT);
    print_surprise_attack(pa_ptr);

    pa_ptr->flags = o_ptr->get_flags();
    pa_ptr->chaos_effect = select_chaotic_effect(creature, pa_ptr);
    pa_ptr->magical_effect = select_magical_brand_effect(creature, pa_ptr);
    decide_blood_sucking(creature, pa_ptr);
    decide_exorcism(creature, pa_ptr);

    bool vorpal_cut = (pa_ptr->flags.has(TR_VORPAL) || SpellHex(creature).is_spelling_specific(HEX_RUNESWORD)) && (randint1(vorpal_chance * 3 / 2) == 1) && !is_zantetsu_nullified;
    calc_attack_damage(creature, pa_ptr, do_quake, vorpal_cut, vorpal_chance);
    apply_damage_bonus(creature, pa_ptr);
    apply_damage_negative_effect(pa_ptr, is_zantetsu_nullified, is_ej_nullified);
    mineuchi(creature, pa_ptr);

    const auto is_death_scythe = o_ptr->bi_key == BaseitemKey(ItemKindType::POLEARM, SV_DEATH_SCYTHE);
    const auto is_berserker = CreatureClass(creature).equals(PlayerClassType::BERSERKER);
    pa_ptr->attack_damage = mon_damage_mod(creature, *pa_ptr->m_ptr, pa_ptr->attack_damage, is_death_scythe || (is_berserker && one_in_(2)));
    critical_attack(creature, pa_ptr);
    msg_format_wizard(creature, CHEAT_MONSTER, _("%dのダメージを与えた。(残hp %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"),
        pa_ptr->attack_damage, pa_ptr->m_ptr->hp - pa_ptr->attack_damage, pa_ptr->m_ptr->maxhp, pa_ptr->m_ptr->max_maxhp);
}

/*!
 * @brief 地震を起こす
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param do_quake 攻撃後に地震を起こすかどうか
 * @param y モンスターのY座標
 * @param x モンスターのX座標
 */
static void cause_earthquake(CreatureEntity &creature, player_attack_type *pa_ptr, const bool do_quake, const POSITION y, const POSITION x)
{
    if (!do_quake) {
        return;
    }

    earthquake(creature, creature.get_position(), 10);
    if (!creature.get_floor()->grid_array[y][x].has_monster()) {
        *(pa_ptr->mdeath) = true;
    }
}

/*!
 * @brief プレイヤーの打撃処理サブルーチン /
 * Player attacks a (poor, defenseless) creature        -RAK-
 * @param y 攻撃目標のY座標
 * @param x 攻撃目標のX座標
 * @param fear 攻撃を受けたモンスターが恐慌状態に陥ったかを返す参照ポインタ
 * @param mdeath 攻撃を受けたモンスターが死亡したかを返す参照ポインタ
 * @param hand 攻撃を行うための武器を持つ手
 * @param mode 発動中の剣術ID
 * @details
 * If no "weapon" is available, then "punch" the monster one time.
 */
void exe_player_attack_to_monster(CreatureEntity &creature, POSITION y, POSITION x, bool *fear, bool *mdeath, int16_t hand, combat_options mode)
{
    bool do_quake = false;
    bool drain_msg = true;

    player_attack_type tmp_attack(*creature.get_floor(), y, x, hand, mode, fear, mdeath);
    auto pa_ptr = &tmp_attack;

    const auto is_human = pa_ptr->r_ptr->symbol_char_is_any_of("p");
    const auto is_lowlevel = (pa_ptr->r_ptr->level < (creature.level - 15));

    attack_classify(creature, pa_ptr);
    get_attack_exp(creature, pa_ptr);

    /* Disturb the monster */
    (void)set_monster_csleep(*creature.get_floor(), pa_ptr->m_idx, 0);
    angband_strcpy(pa_ptr->m_name, monster_desc(creature, *pa_ptr->m_ptr, 0), sizeof(pa_ptr->m_name));

    int chance = calc_attack_quality(creature, pa_ptr);
    auto *o_ptr = creature.inventory[enum2i(INVEN_MAIN_HAND) + pa_ptr->hand].get();
    const auto is_zantetsu_nullified = o_ptr->is_specific_artifact(FixedArtifactId::ZANTETSU) && pa_ptr->r_ptr->symbol_char_is_any_of("j");
    const auto is_ej_nullified = o_ptr->is_specific_artifact(FixedArtifactId::EXCALIBUR_J) && pa_ptr->r_ptr->symbol_char_is_any_of("S");
    calc_num_blow(creature, pa_ptr);

    /* Attack once for each legal blow */
    int num = 0;
    while ((num++ < pa_ptr->num_blow) && !creature.is_dead()) {

        creature.plus_incident_tree("ATTACK_EXE_COUNT", 1);

        if (!process_attack_hit(creature, pa_ptr, chance)) {
            continue;
        }

        pa_ptr->attribute_flags = melee_attribute(creature, o_ptr, pa_ptr->mode);
        apply_actual_attack(creature, pa_ptr, &do_quake, is_zantetsu_nullified, is_ej_nullified);
        calc_drain(pa_ptr);
        if (check_fear_death(creature, pa_ptr, num, is_lowlevel)) {
            break;
        }

        /* Anger the monster */
        if (pa_ptr->attack_damage > 0) {
            anger_monster(creature, *pa_ptr->m_ptr);
        }

        touch_zap_player(*pa_ptr->m_ptr, creature);
        process_drain(creature, pa_ptr, is_human, &drain_msg);
        pa_ptr->can_drain = false;
        pa_ptr->drain_result = 0;
        change_monster_stat(creature, pa_ptr, y, x, &num);
        pa_ptr->backstab = false;
        pa_ptr->surprise_attack = false;
    }

    if (pa_ptr->weak && !(*mdeath)) {
        msg_format(_("%sは弱くなったようだ。", "%s^ seems weakened."), pa_ptr->m_name);
    }

    if ((pa_ptr->drain_left != MAX_VAMPIRIC_DRAIN) && one_in_(4)) {
        chg_virtue(creature, Virtue::UNLIFE, 1);
    }

    cause_earthquake(creature, pa_ptr, do_quake, y, x);
}

/*!
 * @brief 皆殺し(全方向攻撃)処理
 * @param creature クリーチャーへの参照
 */
void massacre(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    for (const auto &d : Direction::directions_8()) {
        const auto pos = creature.get_neighbor(d);
        const auto &grid = floor.get_grid(pos);
        const auto &monster = floor.get_monster(grid.m_idx);
        if (grid.has_monster() && (monster.is_visible_on_map() || floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION))) {
            do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NONE);
        }
    }
}
