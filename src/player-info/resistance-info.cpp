#include "player-info/resistance-info.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"

void set_element_resistance_info(CreatureEntity &creature, self_info_type *self_ptr)
{
    const auto race_tr_flags = CreatureRace(&creature).tr_flags();

    if (has_immune_acid(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは酸に対する完全なる免疫を持っている。", "You are completely immune to acid."));
    } else if (has_resist_acid(creature) && is_oppose_acid(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは酸への強力な耐性を持っている。", "You resist acid exceptionally well."));
    } else if (has_resist_acid(creature) || is_oppose_acid(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは酸への耐性を持っている。", "You are resistant to acid."));
    }

    if (race_tr_flags.has(TR_VUL_ACID) && !has_immune_acid(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは酸に弱い。", "You are susceptible to damage from acid."));
    }

    if (has_immune_elec(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは電撃に対する完全なる免疫を持っている。", "You are completely immune to lightning."));
    } else if (has_resist_elec(creature) && is_oppose_elec(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは電撃への強力な耐性を持っている。", "You resist lightning exceptionally well."));
    } else if (has_resist_elec(creature) || is_oppose_elec(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは電撃への耐性を持っている。", "You are resistant to lightning."));
    }

    if (race_tr_flags.has(TR_VUL_ELEC) && !has_immune_elec(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは電撃に弱い。", "You are susceptible to damage from lightning."));
    }

    if (has_immune_fire(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは火に対する完全なる免疫を持っている。", "You are completely immune to fire."));
    } else if (has_resist_fire(creature) && is_oppose_fire(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは火への強力な耐性を持っている。", "You resist fire exceptionally well."));
    } else if (has_resist_fire(creature) || is_oppose_fire(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは火への耐性を持っている。", "You are resistant to fire."));
    }

    if (race_tr_flags.has(TR_VUL_FIRE) && !has_immune_fire(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは火に弱い。", "You are susceptible to damage from fire."));
    }

    if (has_immune_cold(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは冷気に対する完全なる免疫を持っている。", "You are completely immune to cold."));
    } else if (has_resist_cold(creature) && is_oppose_cold(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは冷気への強力な耐性を持っている。", "You resist cold exceptionally well."));
    } else if (has_resist_cold(creature) || is_oppose_cold(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは冷気への耐性を持っている。", "You are resistant to cold."));
    }

    if (race_tr_flags.has(TR_VUL_COLD) && !has_immune_cold(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは冷気に弱い。", "You are susceptible to damage from cold."));
    }

    if (has_resist_pois(creature) && is_oppose_pois(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは毒への強力な耐性を持っている。", "You resist poison exceptionally well."));
    } else if (has_resist_pois(creature) || is_oppose_pois(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは毒への耐性を持っている。", "You are resistant to poison."));
    }
}

void set_high_resistance_info(CreatureEntity &creature, self_info_type *self_ptr)
{
    if (has_resist_lite(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは閃光への耐性を持っている。", "You are resistant to bright light."));
    }

    if (has_vuln_lite(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは閃光に弱い。", "You are susceptible to damage from bright light."));
    }

    if (has_immune_dark(creature) || creature.get_remaining_wraith_form()) {
        self_ptr->info_list.emplace_back(_("あなたは暗黒に対する完全なる免疫を持っている。", "You are completely immune to darkness."));
    } else if (has_resist_dark(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは暗黒への耐性を持っている。", "You are resistant to darkness."));
    }

    if (has_resist_conf(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは混乱への耐性を持っている。", "You are resistant to confusion."));
    }

    if (has_resist_sound(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは音波の衝撃への耐性を持っている。", "You are resistant to sonic attacks."));
    }

    if (has_resist_disen(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは劣化への耐性を持っている。", "You are resistant to disenchantment."));
    }

    if (has_resist_chaos(creature)) {
        self_ptr->info_list.emplace_back(_("あなたはカオスの力への耐性を持っている。", "You are resistant to chaos."));
    }

    if (has_resist_shard(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは破片の攻撃への耐性を持っている。", "You are resistant to blasts of shards."));
    }

    if (has_resist_shard(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは因果混乱の攻撃への耐性を持っている。", "You are resistant to nexus attacks."));
    }

    if (CreatureRace(&creature).equals(PlayerRaceType::SPECTRE)) {
        self_ptr->info_list.emplace_back(_("あなたは地獄の力を吸収できる。", "You can drain nether forces."));
    } else if (has_resist_neth(creature)) {
        self_ptr->info_list.emplace_back(_("あなたは地獄の力への耐性を持っている。", "You are resistant to nether forces."));
    }
}
