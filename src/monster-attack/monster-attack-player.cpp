/*!
 * @brief モンスターからプレイヤーへの直接攻撃処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "monster-attack/monster-attack-player.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-pet.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "combat/aura-counterattack.h"
#include "combat/combat-options-type.h"
#include "combat/hallucination-attacks-table.h"
#include "core/disturbance.h"
#include "dungeon/dungeon-flag-types.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "mind/mind-samurai.h"
#include "monster-attack/monster-attack-describer.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-switcher.h"
#include "monster-attack/monster-attack-table.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "pet/pet-fall-off.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "system/angband.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 打撃を行うモンスターのID
 */
MonsterAttackPlayer::MonsterAttackPlayer(PlayerType *player_ptr, short m_idx)
    : m_idx(m_idx)
    , m_ptr(&player_ptr->current_floor_ptr->m_list[m_idx])
    , method(RaceBlowMethodType::NONE)
    , effect(RaceBlowEffectType::NONE)
    , do_silly_attack(one_in_(2) && player_ptr->effects()->hallucination().is_hallucinated())
    , player_ptr(player_ptr)
{
}

/*!
 * @brief モンスターからプレイヤーへの打撃処理 / Attack the player via physical attacks.
 */
void MonsterAttackPlayer::make_attack_normal()
{
    if (!this->check_no_blow()) {
        return;
    }

    const auto &monrace = this->m_ptr->get_monrace();
    this->rlev = ((monrace.level >= 1) ? monrace.level : 1);
    angband_strcpy(this->m_name, monster_desc(this->player_ptr, *this->m_ptr, 0), sizeof(this->m_name));
    angband_strcpy(this->ddesc, monster_desc(this->player_ptr, *this->m_ptr, MD_WRONGDOER_NAME), sizeof(this->ddesc));
    if (PlayerClass(this->player_ptr).samurai_stance_is(SamuraiStanceType::IAI)) {
        msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, drew and cut in one motion before %s moved."), this->m_name);
        if (do_cmd_attack(this->player_ptr, this->m_ptr->fy, this->m_ptr->fx, HISSATSU_IAI)) {
            return;
        }
    }

    auto can_activate_kawarimi = randint0(55) < (this->player_ptr->lev * 3 / 5 + 20);
    if (can_activate_kawarimi && kawarimi(this->player_ptr, true)) {
        return;
    }

    this->blinked = false;
    if (this->process_monster_blows()) {
        return;
    }

    this->postprocess_monster_blows();
}

/*!
 * @brief 能力値の実値を求める
 * @param raw PlayerTypeに格納されている生値
 * @return 実値
 * @details AD&Dの記法に則り、19以上の値を取らなくしているので、格納方法が面倒
 */
int MonsterAttackPlayer::stat_value(const int raw)
{
    if (raw <= 18) {
        return raw;
    }

    return (raw - 18) / 10 + 18;
}

bool MonsterAttackPlayer::check_no_blow()
{
    const auto &monrace = this->m_ptr->get_monrace();
    if (monrace.behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (this->player_ptr->current_floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        return false;
    }

    return this->m_ptr->is_hostile();
}

/*!
 * @brief モンスターからプレイヤーへの打撃処理本体
 * @return 打撃に反応してプレイヤーがその場から離脱したかどうか
 */
bool MonsterAttackPlayer::process_monster_blows()
{
    const auto &monrace = this->m_ptr->get_monrace();
    for (auto ap_cnt = 0; ap_cnt < MAX_NUM_BLOWS; ap_cnt++) {
        this->obvious = false;
        this->damage = 0;
        this->act = nullptr;
        this->effect = monrace.blows[ap_cnt].effect;
        this->method = monrace.blows[ap_cnt].method;
        this->damage_dice = monrace.blows[ap_cnt].damage_dice;

        if (!this->check_monster_continuous_attack()) {
            break;
        }

        // effect が RaceBlowEffectType::NONE (無効値)になることはあり得ないはずだが、万一そう
        // なっていたら単に攻撃を打ち切る。
        // MonsterRaceDefinitions の "B:" トークンに effect 以降を書き忘れた場合が該当する。
        if (this->effect == RaceBlowEffectType::NONE) {
            plog("unexpected: MonsterAttackPlayer::effect == RaceBlowEffectType::NONE");
            break;
        }

        // フレーバーの打撃は必中扱い。それ以外は通常の命中判定を行う。
        this->ac = this->player_ptr->ac + this->player_ptr->to_a;
        bool hit;
        if (this->effect == RaceBlowEffectType::FLAVOR) {
            hit = true;
        } else {
            const int power = mbe_info[enum2i(this->effect)].power;
            hit = check_hit_from_monster_to_player(this->player_ptr, power, this->rlev, this->m_ptr->get_remaining_stun());
        }

        if (hit) {
            // 命中した。命中処理と思い出処理を行う。
            // 打撃そのものは対邪悪結界で撃退した可能性がある。
            const bool protect = !this->process_monster_attack_hit();
            this->increase_blow_type_seen(ap_cnt);

            // 撃退成功時はそのまま次の打撃へ移行。
            if (protect) {
                continue;
            }

            // 撃退失敗時は落馬処理、変わり身のテレポート処理を行う。
            check_fall_off_horse(this->player_ptr, this);

            // 変わり身のテレポートが成功したら攻撃を打ち切り、プレイヤーが離脱した旨を返す。
            if (kawarimi(this->player_ptr, false)) {
                return true;
            }
        } else {
            // 命中しなかった。回避時の処理、思い出処理を行う。
            this->process_monster_attack_evasion();
            this->increase_blow_type_seen(ap_cnt);
        }
    }

    // 通常はプレイヤーはその場にとどまる。
    return false;
}

/*!
 * @brief プレイヤー死亡等でモンスターからプレイヤーへの直接攻撃処理を途中で打ち切るかどうかを判定する
 * @return 攻撃続行ならばTRUE、打ち切りになったらFALSE
 */
bool MonsterAttackPlayer::check_monster_continuous_attack()
{
    if (!this->m_ptr->is_valid() || (this->method == RaceBlowMethodType::NONE)) {
        return false;
    }

    const auto &monrace = this->m_ptr->get_monrace();
    if (this->m_ptr->is_pet() && monrace.kind_flags.has(MonsterKindType::UNIQUE) && (this->method == RaceBlowMethodType::EXPLODE)) {
        this->method = RaceBlowMethodType::HIT;
        this->damage_dice.num /= 10;
    }

    const auto is_neighbor = Grid::calc_distance(this->player_ptr->get_position(), this->m_ptr->get_position()) <= 1;
    return this->player_ptr->playing && !this->player_ptr->is_dead && is_neighbor && !this->player_ptr->leaving;
}

/*!
 * @brief モンスターから直接攻撃を1回受けた時の処理
 * @return 対邪悪結界により攻撃が当たらなかったらFALSE、それ以外はTRUE
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @details 最大4 回/モンスター/ターン、このルーチンを通る
 */
bool MonsterAttackPlayer::process_monster_attack_hit()
{
    disturb(this->player_ptr, true, true);
    if (this->effect_protecion_from_evil()) {
        return false;
    }

    this->do_cut = 0;
    this->do_stun = 0;
    describe_monster_attack_method(this);
    this->describe_silly_attacks();
    this->obvious = true;
    this->damage = this->damage_dice.roll();
    if (this->explode) {
        this->damage = 0;
    }

    switch_monster_blow_to_player(this->player_ptr, this);
    this->select_cut_stun();
    this->calc_player_cut();
    this->process_player_stun();
    this->monster_explode();
    process_aura_counterattack(this->player_ptr, this);
    return true;
}

/*!
 * @brief 対邪悪結界が効いている状態で邪悪なモンスターから直接攻撃を受けた時の処理
 * @return briefに書いた条件＋確率が満たされたらTRUE、それ以外はFALSE
 */
bool MonsterAttackPlayer::effect_protecion_from_evil()
{
    auto &monrace = this->m_ptr->get_monrace();
    auto is_protected = this->player_ptr->effects()->protection().is_protected();
    is_protected &= monrace.kind_flags.has(MonsterKindType::EVIL);
    is_protected &= (randint0(100) + this->player_ptr->lev - this->rlev) > 50;
    if (!is_protected) {
        return false;
    }

    if (is_original_ap_and_seen(this->player_ptr, *this->m_ptr)) {
        monrace.r_kind_flags.set(MonsterKindType::EVIL);
    }

#ifdef JP
    this->abbreviate ? msg_format("撃退した。") : msg_format("%s^は撃退された。", this->m_name);
    this->abbreviate = 1; /* 2回目以降は省略 */
#else
    msg_format("%s^ is repelled.", this->m_name);
#endif

    return true;
}

void MonsterAttackPlayer::describe_silly_attacks()
{
    if (this->act == nullptr) {
        return;
    }

    if (this->do_silly_attack) {
#ifdef JP
        this->abbreviate = -1;
#endif
        this->act = rand_choice(silly_attacks);
    }

#ifdef JP
    if (this->abbreviate == 0) {
        msg_format("%s^に%s", this->m_name, this->act);
    } else if (this->abbreviate == 1) {
        msg_format("%s", this->act);
    } else {
        /* if (this->abbreviate == -1) */
        msg_format("%s^%s", this->m_name, this->act);
    }

    this->abbreviate = 1; /*2回目以降は省略 */
#else
    msg_format("%s^ %s%s", this->m_name, this->act, this->do_silly_attack ? " you." : "");
#endif
}

/*!
 * @brief 切り傷と朦朧が同時に発生した時、片方を無効にする
 */
void MonsterAttackPlayer::select_cut_stun()
{
    if ((this->do_cut == 0) || (this->do_stun == 0)) {
        return;
    }

    if (one_in_(2)) {
        this->do_cut = 0;
    } else {
        this->do_stun = 0;
    }
}

void MonsterAttackPlayer::calc_player_cut()
{
    if (this->do_cut == 0) {
        return;
    }

    auto cut_plus = PlayerCut::get_accumulation(this->damage_dice.maxroll(), this->damage);
    if (cut_plus > 0) {
        (void)BadStatusSetter(this->player_ptr).mod_cut(cut_plus);
    }
}

/*!
 * @brief 朦朧を蓄積させる
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスター打撃への参照ポインタ
 * @details
 * 痛恨の一撃ならば朦朧蓄積ランクを1上げる.
 * 2%の確率で朦朧蓄積ランクを1上げる.
 * 肉体のパラメータが合計80を超える水準に強化されていたら朦朧蓄積ランクを1下げる.
 */
void MonsterAttackPlayer::process_player_stun()
{
    if (this->do_stun == 0) {
        return;
    }

    auto total = this->damage_dice.maxroll();
    auto accumulation_rank = PlayerStun::get_accumulation_rank(total, this->damage);
    if (accumulation_rank == 0) {
        return;
    }

    if ((total < this->damage) && (accumulation_rank <= 6)) {
        accumulation_rank++;
    }

    if (one_in_(50)) {
        accumulation_rank++;
    }

    auto str = this->stat_value(this->player_ptr->stat_cur[A_STR]);
    auto dex = this->stat_value(this->player_ptr->stat_cur[A_DEX]);
    auto con = this->stat_value(this->player_ptr->stat_cur[A_CON]);
    auto is_powerful_body = str + dex + con > 80;
    if (is_powerful_body) {
        accumulation_rank--;
    }

    auto stun_plus = PlayerStun::get_accumulation(accumulation_rank);
    if (stun_plus > 0) {
        (void)BadStatusSetter(this->player_ptr).mod_stun(stun_plus);
    }
}

void MonsterAttackPlayer::monster_explode()
{
    if (!this->explode) {
        return;
    }

    sound(SoundKind::EXPLODE);
    MonsterDamageProcessor mdp(this->player_ptr, this->m_idx, this->m_ptr->hp + 1, &this->fear, AttributeType::NONE);
    if (mdp.mon_take_hit("")) {
        this->blinked = false;
        this->alive = false;
    }
}

/*!
 * @brief 一部の打撃種別の場合のみ、避けた旨のメッセージ表示と盾技能スキル向上を行う
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void MonsterAttackPlayer::process_monster_attack_evasion()
{
    switch (this->method) {
    case RaceBlowMethodType::HIT:
    case RaceBlowMethodType::TOUCH:
    case RaceBlowMethodType::PUNCH:
    case RaceBlowMethodType::KICK:
    case RaceBlowMethodType::CLAW:
    case RaceBlowMethodType::BITE:
    case RaceBlowMethodType::STING:
    case RaceBlowMethodType::SLASH:
    case RaceBlowMethodType::BUTT:
    case RaceBlowMethodType::CRUSH:
    case RaceBlowMethodType::ENGULF:
    case RaceBlowMethodType::CHARGE:
        this->describe_attack_evasion();
        this->gain_armor_exp();
        this->damage = 0;
        return;
    default:
        return;
    }
}

void MonsterAttackPlayer::describe_attack_evasion()
{
    if (!this->m_ptr->ml) {
        return;
    }

    disturb(this->player_ptr, true, true);
#ifdef JP
    auto is_suiken = any_bits(this->player_ptr->special_attack, ATTACK_SUIKEN);
    if (this->abbreviate) {
        msg_format("%sかわした。", is_suiken ? "奇妙な動きで" : "");
    } else {
        msg_format("%s%s^の攻撃をかわした。", is_suiken ? "奇妙な動きで" : "", this->m_name);
    }

    this->abbreviate = 1; /* 2回目以降は省略 */
#else
    msg_format("%s^ misses you.", this->m_name);
#endif
}

void MonsterAttackPlayer::gain_armor_exp()
{
    const auto o_ptr_mh = this->player_ptr->inventory[INVEN_MAIN_HAND].get();
    const auto o_ptr_sh = this->player_ptr->inventory[INVEN_SUB_HAND].get();
    if (!o_ptr_mh->is_protector() && !o_ptr_sh->is_protector()) {
        return;
    }

    auto cur = this->player_ptr->skill_exp[PlayerSkillKindType::SHIELD];
    auto max = class_skills_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::SHIELD];
    if (cur >= max) {
        return;
    }

    const auto &monrace = this->m_ptr->get_monrace();
    auto target_level = monrace.level;
    short increment = 0;
    if ((cur / 100) < target_level) {
        auto addition = (cur / 100 + 15) < target_level ? (target_level - (cur / 100 + 15)) : 0;
        increment += 1 + addition;
    }

    this->player_ptr->skill_exp[PlayerSkillKindType::SHIELD] = std::min<short>(max, cur + increment);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
}

/*!
 * @brief モンスターの打撃情報を蓄積させる
 * @param ap_cnt モンスターの打撃 N回目
 * @details どの敵が何をしてきたか正しく認識できていなければ情報を蓄積しない.
 * 非自明な類の打撃については、そのダメージが 0 ならば基本的に知識が増えない.
 * 但し、既に一定以上の知識があれば常に知識が増える(何をされたのか察知できる).
 */
void MonsterAttackPlayer::increase_blow_type_seen(const int ap_cnt)
{
    if (!is_original_ap_and_seen(this->player_ptr, *this->m_ptr) || this->do_silly_attack) {
        return;
    }

    auto &monrace = this->m_ptr->get_monrace();
    if (!this->obvious && (this->damage == 0) && (monrace.r_blows[ap_cnt] <= 10)) {
        return;
    }

    if (monrace.r_blows[ap_cnt] < MAX_UCHAR) {
        monrace.r_blows[ap_cnt]++;
    }
}

void MonsterAttackPlayer::postprocess_monster_blows()
{
    SpellHex spell_hex(this->player_ptr);
    spell_hex.store_vengeful_damage(this->get_damage);
    spell_hex.eyes_on_eyes(this->m_idx, this->get_damage);
    musou_counterattack(this->player_ptr, this);
    this->process_thief_teleport(spell_hex);
    auto &monrace = this->m_ptr->get_monrace();
    if (this->player_ptr->is_dead && (monrace.r_deaths < MAX_SHORT) && !this->player_ptr->current_floor_ptr->inside_arena) {
        monrace.r_deaths++;
    }

    if (this->m_ptr->ml && this->fear && this->alive && !this->player_ptr->is_dead) {
        sound(SoundKind::FLEE);
        msg_format(_("%s^は恐怖で逃げ出した！", "%s^ flees in terror!"), this->m_name);
    }

    PlayerClass(this->player_ptr).break_samurai_stance({ SamuraiStanceType::IAI });
}

void MonsterAttackPlayer::process_thief_teleport(const SpellHex &spell_hex)
{
    if (!this->blinked || !this->alive || this->player_ptr->is_dead) {
        return;
    }

    if (spell_hex.check_hex_barrier(this->m_idx, HEX_ANTI_TELE)) {
        msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
    } else {
        msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
        teleport_away(this->player_ptr, this->m_idx, MAX_PLAYER_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
    }
}
