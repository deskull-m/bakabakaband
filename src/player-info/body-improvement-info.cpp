#include "player-info/body-improvement-info.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "spell-realm/spells-crusade.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_1(CreatureEntity &creature, self_info_type *self_ptr)
{
    const auto effects = creature.effects();
    if (is_blessed(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは高潔さを感じている。", "You feel rightous."));
    }

    if (is_hero(creature)) {
        self_ptr->info_list.emplace_back(_("あなたはヒーロー気分だ。", "You feel heroic."));
    }

    if (is_shero(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは戦闘狂だ。", "You are in a battle rage."));
    }

    if (effects->protection().is_protected()) {
        self_ptr->info_list.emplace_back(_("あなたは邪悪なる存在から守られている。", "You are protected from evil."));
    }

    if (creature.shield) {
        self_ptr->info_list.emplace_back(_("あなたは神秘のシールドで守られている。", "You are protected by a mystic shield."));
    }

    if (creature.is_invulnerable()) {
        self_ptr->info_list.emplace_back(_("あなたは現在傷つかない。", "You are temporarily invulnerable."));
    }

    if (creature.wraith_form) {
        self_ptr->info_list.emplace_back(_("あなたは一時的に幽体化している。", "You are temporarily incorporeal."));
    }
}

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_2(CreatureEntity &creature, self_info_type *self_ptr)
{
    auto &player = static_cast<PlayerType &>(creature);
    if (player.new_spells) {
        self_ptr->info_list.emplace_back(_("あなたは呪文や祈りを学ぶことができる。", "You can learn some spells/prayers."));
    }

    if (player.word_recall) {
        self_ptr->info_list.emplace_back(_("あなたはすぐに帰還するだろう。", "You will soon be recalled."));
    }

    if (player.alter_reality) {
        self_ptr->info_list.emplace_back(_("あなたはすぐにこの世界を離れるだろう。", "You will soon be altered."));
    }

    if (creature.see_infra) {
        self_ptr->info_list.emplace_back(_("あなたの瞳は赤外線に敏感である。", "Your eyes are sensitive to infrared light."));
    }

    if (creature.see_inv) {
        self_ptr->info_list.emplace_back(_("あなたは透明なモンスターを見ることができる。", "You can see invisible creatures."));
    }

    if (creature.levitation) {
        self_ptr->info_list.emplace_back(_("あなたは飛ぶことができる。", "You can fly."));
    }

    if (creature.free_act) {
        self_ptr->info_list.emplace_back(_("あなたは麻痺知らずの効果を持っている。", "You have free action."));
    }

    if (creature.regenerate) {
        self_ptr->info_list.emplace_back(_("あなたは素早く体力を回復する。", "You regenerate quickly."));
    }

    if (creature.slow_digest) {
        self_ptr->info_list.emplace_back(_("あなたは食欲が少ない。", "Your appetite is small."));
    }
}

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_3(CreatureEntity &creature, self_info_type *self_ptr)
{
    if (creature.hold_exp) {
        self_ptr->info_list.emplace_back(_("あなたは自己の経験値をしっかりと維持する。", "You have a firm hold on your experience."));
    }

    if (has_reflect(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは矢の呪文を反射する。", "You reflect bolt spells."));
    }

    if (has_resist_curse(creature)) {
        self_ptr->info_list.emplace_back(_("あなたはより強く呪いに抵抗できる。", "You can resist curses powerfully."));
    }

    if (has_sh_fire(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは炎のオーラに包まれている。", "You are surrounded with a fiery aura."));
    }

    if (get_player_flags(creature, TR_SELF_FIRE)) {
        self_ptr->info_list.emplace_back(_("あなたは身を焼く炎に包まれている。", "You are being damaged with fire."));
    }

    if (has_sh_elec(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは電気のオーラに包まれている。", "You are surrounded with an electricity aura."));
    }

    if (get_player_flags(creature, TR_SELF_ELEC)) {
        self_ptr->info_list.emplace_back(_("あなたは身を焦がす電撃に包まれている。", "You are being damaged with electricity."));
    }

    if (has_sh_cold(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは冷気のオーラに包まれている。", "You are surrounded with an aura of coldness."));
    }

    if (get_player_flags(creature, TR_SELF_COLD)) {
        self_ptr->info_list.emplace_back(_("あなたは身も凍る冷気に包まれている。", "You are being damaged with coldness."));
    }

    if (creature.tim_sh_holy) {
        self_ptr->info_list.emplace_back(_("あなたは聖なるオーラに包まれている。", "You are surrounded with a holy aura."));
    }

    if (creature.tim_sh_touki) {
        self_ptr->info_list.emplace_back(_("あなたは闘気のオーラに包まれている。", "You are surrounded with an energy aura."));
    }

    if (creature.anti_magic) {
        self_ptr->info_list.emplace_back(_("あなたは反魔法シールドに包まれている。", "You are surrounded by an anti-magic shell."));
    }

    if (creature.anti_tele) {
        self_ptr->info_list.emplace_back(_("あなたはテレポートできない。", "You cannot teleport."));
    }

    if (creature.lite) {
        self_ptr->info_list.emplace_back(_("あなたの身体は光っている。", "You are carrying a permanent light."));
    }

    if (creature.warning) {
        self_ptr->info_list.emplace_back(_("あなたは行動の前に危険を察知することができる。", "You will be warned before dangerous actions."));
    }

    if (creature.dec_mana) {
        self_ptr->info_list.emplace_back(_("あなたは少ない消費魔力で魔法を唱えることができる。", "You can cast spells with fewer mana points."));
    }

    if (creature.easy_spell) {
        self_ptr->info_list.emplace_back(_("あなたは低い失敗率で魔法を唱えることができる。", "Your magic fails less often."));
    }

    if (creature.mighty_throw) {
        self_ptr->info_list.emplace_back(_("あなたは強く物を投げる。", "You can throw objects powerfully."));
    }
}

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_4(CreatureEntity &creature, self_info_type *self_ptr)
{
    if (has_resist_fear(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは全く恐怖を感じない。", "You are completely fearless."));
    }

    if (has_resist_blind(creature)) {
        self_ptr->info_list.emplace_back(_("あなたの目は盲目への耐性を持っている。", "Your eyes are resistant to blindness."));
    }

    if (has_resist_time(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは時間逆転への耐性を持っている。", "You are resistant to time."));
    }
}

/*!< @todo 並び順の都合で連番を付ける。 */
void set_body_improvement_info_5(CreatureEntity &creature, self_info_type *self_ptr)
{
    if (creature.tim_exorcism > 0) {
        if (has_kill_demon_from_exorcism(creature)) {
            self_ptr->info_list.emplace_back(_("あなたはデーモンの天敵である。", "You are a great bane of demons."));
        } else if (has_slay_demon_from_exorcism(creature)) {
            self_ptr->info_list.emplace_back(_("あなたはデーモンに対して神聖なる力を発揮する。", "You strikes at demons with holy wrath."));
        }
        self_ptr->info_list.emplace_back(_("あなたはデーモンのエネルギーを吸い取る。", "You drain energy from demons."));
        if (has_kill_undead_from_exorcism(creature)) {
            self_ptr->info_list.emplace_back(_("あなたはアンデッドの天敵である。", "You are a great bane of undeads."));
        } else if (has_slay_undead_from_exorcism(creature)) {
            self_ptr->info_list.emplace_back(_("あなたはアンデッドに対して神聖なる力を発揮する。", "You strikes at undead with holy wrath."));
        }
        self_ptr->info_list.emplace_back(_("あなたはアンデッドのエネルギーを吸い取る。", "You drain energy from undead."));
    }
}
