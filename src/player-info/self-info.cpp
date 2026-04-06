/*!
 * @brief 自己分析処理/ Self knowledge
 * @date 2018/09/07
 * @author deskull
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "player-info/self-info.h"
#include "system/creature-entity.h"
#include "avatar/avatar.h"
#include "io/input-key-acceptor.h"
#include "object-enchant/trc-types.h"
#include "player-info/base-status-info.h"
#include "player-info/body-improvement-info.h"
#include "player-info/class-ability-info.h"
#include "player-info/mutation-info.h"
#include "player-info/race-ability-info.h"
#include "player-info/race-info.h"
#include "player-info/resistance-info.h"
#include "player-info/self-info-util.h"
#include "player-info/weapon-effect-info.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "timed-effect/timed-effects.h"
#include "view/display-self-info.h"

static void set_bad_status_info(const TimedEffects &effects, self_info_type *self_ptr)
{
    if (effects.blindness().is_blind()) {
        self_ptr->info_list.emplace_back(_("あなたは目が見えない。", "You cannot see."));
    }

    if (effects.confusion().is_confused()) {
        self_ptr->info_list.emplace_back(_("あなたは混乱している。", "You are confused."));
    }

    if (effects.fear().is_fearful()) {
        self_ptr->info_list.emplace_back(_("あなたは恐怖に侵されている。", "You are terrified."));
    }

    if (effects.cut().is_cut()) {
        self_ptr->info_list.emplace_back(_("あなたは出血している。", "You are bleeding."));
    }

    if (effects.stun().is_stunned()) {
        self_ptr->info_list.emplace_back(_("あなたはもうろうとしている。", "You are stunned."));
    }

    if (effects.poison().is_poisoned()) {
        self_ptr->info_list.emplace_back(_("あなたは毒に侵されている。", "You are poisoned."));
    }

    if (effects.hallucination().is_hallucinated()) {
        self_ptr->info_list.emplace_back(_("あなたは幻覚を見ている。", "You are hallucinating."));
    }
}

static void set_curse_info(CreatureEntity &subject, self_info_type *self_ptr)
{
    if (subject.cursed.has(CurseTraitType::TY_CURSE)) {
        self_ptr->info_list.emplace_back(_("あなたは邪悪な怨念に包まれている。", "You carry an ancient foul curse."));
    }

    if (has_aggravate(subject)) {
        self_ptr->info_list.emplace_back(_("あなたはモンスターを怒らせている。", "You aggravate monsters."));
    }

    if (subject.cursed.has(CurseTraitType::DRAIN_EXP)) {
        self_ptr->info_list.emplace_back(_("あなたは経験値を吸われている。", "You occasionally lose experience for no reason."));
    }

    if (subject.cursed.has(CurseTraitType::SLOW_REGEN)) {
        self_ptr->info_list.emplace_back(_("あなたの回復力は非常に遅い。", "You regenerate slowly."));
    }

    if (subject.cursed.has(CurseTraitType::ADD_L_CURSE)) {
        self_ptr->info_list.emplace_back(_("あなたの弱い呪いは増える。", "Your weak curses multiply."));
    } /* 暫定的 -- henkma */

    if (subject.cursed.has(CurseTraitType::ADD_H_CURSE)) {
        self_ptr->info_list.emplace_back(_("あなたの強い呪いは増える。", "Your heavy curses multiply."));
    } /* 暫定的 -- henkma */

    if (subject.cursed.has(CurseTraitType::CALL_ANIMAL)) {
        self_ptr->info_list.emplace_back(_("あなたは動物に狙われている。", "You attract animals."));
    }

    if (subject.cursed.has(CurseTraitType::CALL_DEMON)) {
        self_ptr->info_list.emplace_back(_("あなたは悪魔に狙われている。", "You attract demons."));
    }

    if (subject.cursed.has(CurseTraitType::CALL_DRAGON)) {
        self_ptr->info_list.emplace_back(_("あなたはドラゴンに狙われている。", "You attract dragons."));
    }

    if (subject.cursed.has(CurseTraitType::COWARDICE)) {
        self_ptr->info_list.emplace_back(_("あなたは時々臆病になる。", "You are subject to cowardice."));
    }

    if (subject.cursed.has(CurseTraitType::BERS_RAGE)) {
        self_ptr->info_list.emplace_back(_("あなたは狂戦士化の発作を起こす。", "You are subject to berserker fits."));
    }

    if (subject.cursed.has(CurseTraitType::TELEPORT)) {
        self_ptr->info_list.emplace_back(_("あなたの位置はひじょうに不安定だ。", "Your position is very uncertain."));
    }

    if (subject.cursed.has(CurseTraitType::LOW_MELEE)) {
        self_ptr->info_list.emplace_back(_("あなたの武器は攻撃を外しやすい。", "Your weapon causes you to miss blows."));
    }

    if (subject.cursed.has(CurseTraitType::LOW_AC)) {
        self_ptr->info_list.emplace_back(_("あなたは攻撃を受けやすい。", "You are subject to be hit."));
    }

    if (subject.cursed.has(CurseTraitType::HARD_SPELL)) {
        self_ptr->info_list.emplace_back(_("あなたは魔法を失敗しやすい。", "Your spells fail more frequently."));
    }

    if (subject.cursed.has(CurseTraitType::FAST_DIGEST)) {
        self_ptr->info_list.emplace_back(_("あなたはすぐお腹がへる。", "You have a good appetite."));
    }

    if (subject.cursed.has(CurseTraitType::DRAIN_HP)) {
        self_ptr->info_list.emplace_back(_("あなたは体力を吸われている。", "You occasionally lose hit points for no reason."));
    }

    if (subject.cursed.has(CurseTraitType::DRAIN_MANA)) {
        self_ptr->info_list.emplace_back(_("あなたは魔力を吸われている。", "You occasionally lose spell points for no reason."));
    }
}

static void set_special_attack_info(CreatureEntity &subject, self_info_type *self_ptr)
{
    if (subject.special_attack & ATTACK_CONFUSE) {
        self_ptr->info_list.emplace_back(_("あなたの手は赤く輝いている。", "Your hands are glowing dull red."));
    }

    if (subject.special_attack & ATTACK_FIRE) {
        self_ptr->info_list.emplace_back(_("あなたの手は火炎に覆われている。", "You can strike the enemy with flame."));
    }

    if (subject.special_attack & ATTACK_COLD) {
        self_ptr->info_list.emplace_back(_("あなたの手は冷気に覆われている。", "You can strike the enemy with cold."));
    }

    if (subject.special_attack & ATTACK_ACID) {
        self_ptr->info_list.emplace_back(_("あなたの手は酸に覆われている。", "You can strike the enemy with acid."));
    }

    if (subject.special_attack & ATTACK_ELEC) {
        self_ptr->info_list.emplace_back(_("あなたの手は電撃に覆われている。", "You can strike the enemy with electoric shock."));
    }

    if (subject.special_attack & ATTACK_POIS) {
        self_ptr->info_list.emplace_back(_("あなたの手は毒に覆われている。", "You can strike the enemy with poison."));
    }
}

static void set_esp_info(CreatureEntity &subject, self_info_type *self_ptr)
{
    if (subject.telepathy) {
        self_ptr->info_list.emplace_back(_("あなたはテレパシー能力を持っている。", "You have ESP."));
    }

    if (subject.esp_animal) {
        self_ptr->info_list.emplace_back(_("あなたは自然界の生物の存在を感じる能力を持っている。", "You sense natural creatures."));
    }

    if (subject.esp_undead) {
        self_ptr->info_list.emplace_back(_("あなたはアンデッドの存在を感じる能力を持っている。", "You sense undead."));
    }

    if (subject.esp_demon) {
        self_ptr->info_list.emplace_back(_("あなたは悪魔の存在を感じる能力を持っている。", "You sense demons."));
    }

    if (subject.esp_orc) {
        self_ptr->info_list.emplace_back(_("あなたはオークの存在を感じる能力を持っている。", "You sense orcs."));
    }

    if (subject.esp_troll) {
        self_ptr->info_list.emplace_back(_("あなたはトロルの存在を感じる能力を持っている。", "You sense trolls."));
    }

    if (subject.esp_giant) {
        self_ptr->info_list.emplace_back(_("あなたは巨人の存在を感じる能力を持っている。", "You sense giants."));
    }

    if (subject.esp_dragon) {
        self_ptr->info_list.emplace_back(_("あなたはドラゴンの存在を感じる能力を持っている。", "You sense dragons."));
    }

    if (subject.esp_human) {
        self_ptr->info_list.emplace_back(_("あなたは人間の存在を感じる能力を持っている。", "You sense humans."));
    }

    if (subject.esp_evil) {
        self_ptr->info_list.emplace_back(_("あなたは邪悪な生き物の存在を感じる能力を持っている。", "You sense evil creatures."));
    }

    if (subject.esp_good) {
        self_ptr->info_list.emplace_back(_("あなたは善良な生き物の存在を感じる能力を持っている。", "You sense good creatures."));
    }

    if (subject.esp_nonliving) {
        self_ptr->info_list.emplace_back(_("あなたは活動する無生物体の存在を感じる能力を持っている。", "You sense non-living creatures."));
    }

    if (subject.esp_unique) {
        self_ptr->info_list.emplace_back(_("あなたは特別な強敵の存在を感じる能力を持っている。", "You sense unique monsters."));
    }
}

/*!
 * @brief 自己分析処理(Nethackからのアイデア) / self-knowledge... idea from nethack.
 * @details
 * <pre>
 * Useful for determining powers and
 * resistences of items.  It saves the screen, clears it, then starts listing
 * attributes, a screenful at a time.  (There are a LOT of attributes to
 * list.  It will probably take 2 or 3 screens for a powerful character whose
 * using several artifacts...) -CFT
 *
 * It is now a lot more efficient. -BEN-
 *
 * See also "identify_fully()".
 *
 * Use the "show_file()" method, perhaps.
 * </pre>
 */
void self_knowledge(CreatureEntity &subject)
{
    self_info_type tmp_si;
    self_info_type *self_ptr = &tmp_si;
    display_life_rating(subject, self_ptr);
    chg_virtue(subject, Virtue::KNOWLEDGE, 1);
    chg_virtue(subject, Virtue::ENLIGHTEN, 1);
    display_max_base_status(subject, self_ptr);
    display_virtue(subject, self_ptr);
    self_ptr->info_list.emplace_back("");
    if (subject.mimic_form != MimicKindType::NONE) {
        display_mimic_race_ability(subject, self_ptr);
    } else {
        set_race_ability_info(subject, self_ptr);
    }

    set_class_ability_info(subject, self_ptr);
    set_mutation_info(subject, self_ptr);
    set_bad_status_info(*subject.effects(), self_ptr);
    set_curse_info(subject, self_ptr);
    set_body_improvement_info_1(subject, self_ptr);
    set_special_attack_info(subject, self_ptr);
    switch (subject.action) {
    case ACTION_SEARCH:
        self_ptr->info_list.emplace_back(_("あなたはひじょうに注意深く周囲を見渡している。", "You are looking around very carefully."));
        break;
    }

    set_body_improvement_info_2(subject, self_ptr);
    set_esp_info(subject, self_ptr);
    set_body_improvement_info_3(subject, self_ptr);
    set_element_resistance_info(subject, self_ptr);
    set_high_resistance_info(subject, self_ptr);
    set_body_improvement_info_4(subject, self_ptr);
    set_status_sustain_info(subject, self_ptr);
    set_equipment_influence(subject, self_ptr);
    set_weapon_effect_info(subject, self_ptr);
    set_body_improvement_info_5(subject, self_ptr);
    display_self_info(self_ptr);
}

/*!
 * @brief 魔法効果時間のターン数に基づいて表現IDを返す。
 * @param dur 効果ターン数
 * @return 効果時間の表現ID
 */
static int report_magics_aux(int dur)
{
    if (dur <= 5) {
        return 0;
    } else if (dur <= 10) {
        return 1;
    } else if (dur <= 20) {
        return 2;
    } else if (dur <= 50) {
        return 3;
    } else if (dur <= 100) {
        return 4;
    } else if (dur <= 200) {
        return 5;
    } else {
        return 6;
    }
}

static const std::vector<std::string> report_magic_durations = { _("ごく短い間", "for a short time"), _("少しの間", "for a little while"), _("しばらくの間", "for a while"),
    _("多少長い間", "for a long while"), _("長い間", "for a long time"), _("非常に長い間", "for a very long time"),
    _("信じ難いほど長い間", "for an incredibly long time"), _("モンスターを攻撃するまで", "until you hit a monster") };

/*!
 * @brief 現在の一時的効果一覧を返す / Report all currently active magical effects.
 */
void report_magics(CreatureEntity &subject)
{
    auto &player = static_cast<PlayerType &>(subject);
    std::vector<std::pair<int, std::string>> info;
    const auto effects = player.effects();
    const auto &blindness = effects->blindness();
    if (blindness.is_blind()) {
        info.emplace_back(report_magics_aux(blindness.current()),
            _("あなたは目が見えない", "You cannot see"));
    }

    const auto &confusion = effects->confusion();
    if (confusion.is_confused()) {
        info.emplace_back(report_magics_aux(confusion.current()),
            _("あなたは混乱している", "You are confused"));
    }

    const auto &fear = effects->fear();
    if (fear.is_fearful()) {
        info.emplace_back(report_magics_aux(fear.current()),
            _("あなたは恐怖に侵されている", "You are terrified"));
    }

    const auto &player_poison = effects->poison();
    if (player_poison.is_poisoned()) {
        info.emplace_back(report_magics_aux(player_poison.current()),
            _("あなたは毒に侵されている", "You are poisoned"));
    }

    const auto &hallucination = effects->hallucination();
    if (hallucination.is_hallucinated()) {
        info.emplace_back(report_magics_aux(hallucination.current()),
            _("あなたは幻覚を見ている", "You are hallucinating"));
    }

    if (player.blessed) {
        info.emplace_back(report_magics_aux(player.blessed),
            _("あなたは高潔さを感じている", "You feel rightous"));
    }

    if (player.hero) {
        info.emplace_back(report_magics_aux(player.hero),
            _("あなたはヒーロー気分だ", "You feel heroic"));
    }

    if (player.is_shero()) {
        info.emplace_back(report_magics_aux(player.berserk),
            _("あなたは戦闘狂だ", "You are in a battle rage"));
    }

    const auto &protection = effects->protection();
    if (protection.is_protected()) {
        info.emplace_back(report_magics_aux(protection.current()),
            _("あなたは邪悪なる存在から守られている", "You are protected from evil"));
    }

    if (player.shield) {
        info.emplace_back(report_magics_aux(player.shield),
            _("あなたは神秘のシールドで守られている", "You are protected by a mystic shield"));
    }

    if (player.invuln) {
        info.emplace_back(report_magics_aux(player.invuln),
            _("あなたは無敵だ", "You are invulnerable"));
    }

    if (player.wraith_form) {
        info.emplace_back(report_magics_aux(player.wraith_form),
            _("あなたは幽体化している", "You are incorporeal"));
    }

    if (player.special_attack & ATTACK_CONFUSE) {
        info.emplace_back(7, _("あなたの手は赤く輝いている", "Your hands are glowing dull red."));
    }

    if (player.word_recall) {
        info.emplace_back(report_magics_aux(player.word_recall),
            _("この後帰還の詔が発動する", "You are waiting to be recalled"));
    }

    if (player.alter_reality) {
        info.emplace_back(report_magics_aux(player.alter_reality),
            _("この後現実変容が発動する", "You waiting to be altered"));
    }

    if (player.oppose_acid) {
        info.emplace_back(report_magics_aux(player.oppose_acid),
            _("あなたは酸への耐性を持っている", "You are resistant to acid"));
    }

    if (player.oppose_elec) {
        info.emplace_back(report_magics_aux(player.oppose_elec),
            _("あなたは電撃への耐性を持っている", "You are resistant to lightning"));
    }

    if (player.oppose_fire) {
        info.emplace_back(report_magics_aux(player.oppose_fire),
            _("あなたは火への耐性を持っている", "You are resistant to fire"));
    }

    if (player.oppose_cold) {
        info.emplace_back(report_magics_aux(player.oppose_cold),
            _("あなたは冷気への耐性を持っている", "You are resistant to cold"));
    }

    if (player.oppose_pois) {
        info.emplace_back(report_magics_aux(player.oppose_pois),
            _("あなたは毒への耐性を持っている", "You are resistant to poison"));
    }

    screen_save();

    /* Erase the screen */
    for (int k = 1; k < MAIN_TERM_MIN_ROWS; k++) {
        prt("", k, 13);
    }

    prt(_("    現在かかっている魔法     :", "     Your Current Magic:"), 1, 15);
    const int size = std::ssize(info);
    auto k = 2;
    for (int j = 0; j < size; j++) {
        constexpr auto fmt = _("%-28s : 期間 - %s ", "%s %s.");
        const auto mes = format(fmt, info[j].second.data(), report_magic_durations[info[j].first].data());
        prt(mes, k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k == 22) && (j + 1 < size)) {
            prt(_("-- 続く --", "-- more --"), k, 15);
            inkey();
            for (; k > 2; k--) {
                prt("", k, 15);
            }
        }
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}
