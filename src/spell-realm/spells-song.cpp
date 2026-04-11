#include "spell-realm/spells-song.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "player-base/player-class.h"
#include "player-info/bard-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "realm/realm-song-numbers.h"
#include "spell/spell-info.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの歌に関する継続処理
 */
void check_music(CreatureEntity &creature)
{
    auto &player = creature;
    if (!CreatureClass(player).equals(PlayerClassType::BARD)) {
        return;
    }

    auto interupting_song_effect = get_interrupting_song_effect(creature);
    if ((get_singing_song_effect(creature) == 0) && (interupting_song_effect == 0)) {
        return;
    }

    if (player.anti_magic) {
        stop_singing(creature);
        return;
    }

    const auto spell_id = get_singing_song_id(creature);
    const auto &spell = PlayerRealm::get_spell_info(RealmType::MUSIC, spell_id);

    MANA_POINT need_mana = mod_need_mana(creature, spell.smana, spell_id, RealmType::MUSIC);
    uint32_t need_mana_frac = 0;

    s64b_rshift(&need_mana, &need_mana_frac, 1);
    if (s64b_cmp(player.csp, player.csp_frac, need_mana, need_mana_frac) < 0) {
        stop_singing(creature);
        return;
    }

    s64b_sub(&(player.csp), &(player.csp_frac), need_mana, need_mana_frac);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    if (interupting_song_effect != 0) {
        set_singing_song_effect(creature, interupting_song_effect);
        set_interrupting_song_effect(creature, MUSIC_NONE);
        msg_print(_("歌を再開した。", "You resume singing."));
        player.action = ACTION_SING;
        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::HP,
            StatusRecalculatingFlag::MONSTER_STATUSES,
        };
        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::MAP,
            MainWindowRedrawingFlag::TIMED_EFFECT,
            MainWindowRedrawingFlag::ACTION,
        };
        rfu.set_flags(flags_mwrf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        rfu.set_flags(flags_swrf);
    }

    PlayerSkill(player).gain_continuous_spell_skill_exp(RealmType::MUSIC, spell_id);
    exe_spell(creature, RealmType::MUSIC, spell_id, SpellProcessType::CONTNUATION);
}

/*!
 * @brief 隠遁の歌の継続時間をセットする / Set "tim_stealth", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_stealth(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.tim_stealth && !do_dec) {
            if (creature.tim_stealth > v) {
                return false;
            }
        } else if (!creature.is_time_limit_stealth()) {
            msg_print(_("足音が小さくなった！", "You begin to walk silently!"));
            notice = true;
        }
    } else {
        if (creature.tim_stealth && !music_singing(creature, MUSIC_STEALTH)) {
            msg_print(_("足音が大きくなった。", "You no longer walk silently."));
            notice = true;
        }
    }

    creature.tim_stealth = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 歌の停止を処理する / Stop singing if the player is a Bard
 */
void stop_singing(CreatureEntity &creature)
{
    auto &player = creature;
    if (!CreatureClass(player).equals(PlayerClassType::BARD)) {
        return;
    }

    if (get_interrupting_song_effect(creature) != 0) {
        set_interrupting_song_effect(creature, MUSIC_NONE);
        return;
    }

    if (get_singing_song_effect(creature) == 0) {
        return;
    }

    if (player.action == ACTION_SING) {
        set_action(creature, ACTION_NONE);
    }

    (void)exe_spell(creature, RealmType::MUSIC, get_singing_song_id(creature), SpellProcessType::STOP);
    set_singing_song_effect(creature, MUSIC_NONE);
    set_singing_song_id(creature, 0);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
}

bool music_singing(CreatureEntity &creature, int music_songs)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    return bird_data && (bird_data->singing_song == music_songs);
}

bool music_singing_any(CreatureEntity &creature)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    return bird_data && (bird_data->singing_song != MUSIC_NONE);
}

int32_t get_singing_song_effect(CreatureEntity &creature)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->singing_song;
}

void set_singing_song_effect(CreatureEntity &creature, const int32_t magic_num)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->singing_song = i2enum<realm_song_type>(magic_num);
}

int32_t get_interrupting_song_effect(CreatureEntity &creature)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->interrputing_song;
}

void set_interrupting_song_effect(CreatureEntity &creature, const int32_t magic_num)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->interrputing_song = i2enum<realm_song_type>(magic_num);
}

int32_t get_singing_count(CreatureEntity &creature)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->singing_duration;
}

void set_singing_count(CreatureEntity &creature, const int32_t magic_num)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->singing_duration = magic_num;
}

byte get_singing_song_id(CreatureEntity &creature)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return 0;
    }

    return bird_data->singing_song_spell_idx;
}

void set_singing_song_id(CreatureEntity &creature, const byte magic_num)
{
    auto bird_data = CreatureClass(creature).get_specific_data<bard_data_type>();
    if (!bird_data) {
        return;
    }

    bird_data->singing_song_spell_idx = magic_num;
}
