#pragma once

#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "system/angband.h"

enum flag_cause : uint32_t {
    FLAG_CAUSE_NONE = 0x0U,
    FLAG_CAUSE_INVEN_MAIN_HAND = 0x01U << 0, /*!< アイテムスロット…利手 */
    FLAG_CAUSE_INVEN_SUB_HAND = 0x01U << 1, /*!< アイテムスロット…逆手 */
    FLAG_CAUSE_INVEN_BOW = 0x01U << 2, /*!< アイテムスロット…射撃 */
    FLAG_CAUSE_INVEN_MAIN_RING = 0x01U << 3, /*!< アイテムスロット…利手指 */
    FLAG_CAUSE_INVEN_SUB_RING = 0x01U << 4, /*!< アイテムスロット…逆手指 */
    FLAG_CAUSE_INVEN_NECK = 0x01U << 5, /*!< アイテムスロット…首 */
    FLAG_CAUSE_INVEN_LITE = 0x01U << 6, /*!< アイテムスロット…光源 */
    FLAG_CAUSE_INVEN_BODY = 0x01U << 7, /*!< アイテムスロット…体 */
    FLAG_CAUSE_INVEN_OUTER = 0x01U << 8, /*!< アイテムスロット…体の上 */
    FLAG_CAUSE_INVEN_HEAD = 0x01U << 9, /*!< アイテムスロット…頭部 */
    FLAG_CAUSE_INVEN_ARMS = 0x01U << 10, /*!< アイテムスロット…腕部 */
    FLAG_CAUSE_INVEN_FEET = 0x01U << 11, /*!< アイテムスロット…脚部 */
    FLAG_CAUSE_INVEN_ASSHOLE = 0x01U << 12, /*!< アイテムスロット…尻の穴 */
    FLAG_CAUSE_RACE = 0x01U << 13, /*!< 種族上の体得 */
    FLAG_CAUSE_CLASS = 0x01U << 14, /*!< 職業上の体得 */
    FLAG_CAUSE_PERSONALITY = 0x01U << 15, /*!< 性格上の体得 */
    FLAG_CAUSE_MAGIC_TIME_EFFECT = 0x01U << 16, /*!< 魔法による時限効果 */
    FLAG_CAUSE_MUTATION = 0x01U << 17, /*!< 変異による効果 */
    FLAG_CAUSE_STANCE = 0x01U << 18, /*!< 構えによる効果 */
    FLAG_CAUSE_RIDING = 0x01U << 19, /*!< 乗馬による効果 */
    FLAG_CAUSE_INVEN_PACK = 0x01U << 20, /*!< その他インベントリによる効果 重量超過等 */
    FLAG_CAUSE_ACTION = 0x01U << 21, /*!< ACTIONによる効果 探索モード等 */
    FLAG_CAUSE_INVEN_EXTENDED = 0x01U << 22, /*!< モンスター拡張装備スロット (Phase 2) */
    FLAG_CAUSE_MAX = 0x01U << 22
};

enum melee_type {
    MELEE_TYPE_BAREHAND_TWO = 0,
    MELEE_TYPE_BAREHAND_MAIN = 1,
    MELEE_TYPE_BAREHAND_SUB = 2,
    MELEE_TYPE_WEAPON_MAIN = 3,
    MELEE_TYPE_WEAPON_SUB = 4,
    MELEE_TYPE_WEAPON_TWOHAND = 5,
    MELEE_TYPE_WEAPON_DOUBLE = 6,
    MELEE_TYPE_SHIELD_DOUBLE = 7
};

enum aggravate_state {
    AGGRAVATE_NONE = 0x00000000L,
    AGGRAVATE_S_FAIRY = 0x00000001L,
    AGGRAVATE_NORMAL = 0x00000002L,
    NASTY_AGGRAVATE = 0x00000004L,
};

class CreatureEntity;
class PlayerType;
BIT_FLAGS convert_inventory_slot_type_to_flag_cause(inventory_slot_type inventory_slot);
BIT_FLAGS check_equipment_flags(CreatureEntity &creature, tr_type tr_flag);
BIT_FLAGS get_player_flags(CreatureEntity &creature, tr_type tr_flag);
bool has_pass_wall(CreatureEntity &creature);
bool has_kill_wall(CreatureEntity &creature);
BIT_FLAGS has_xtra_might(CreatureEntity &creature);
BIT_FLAGS has_esp_evil(CreatureEntity &creature);
BIT_FLAGS has_esp_animal(CreatureEntity &creature);
BIT_FLAGS has_esp_nasty(CreatureEntity &creature);
BIT_FLAGS has_esp_homo(CreatureEntity &creature);
BIT_FLAGS has_esp_undead(CreatureEntity &creature);
BIT_FLAGS has_esp_demon(CreatureEntity &creature);
BIT_FLAGS has_esp_orc(CreatureEntity &creature);
BIT_FLAGS has_esp_troll(CreatureEntity &creature);
BIT_FLAGS has_esp_giant(CreatureEntity &creature);
BIT_FLAGS has_esp_dragon(CreatureEntity &creature);
BIT_FLAGS has_esp_human(CreatureEntity &creature);
BIT_FLAGS has_esp_good(CreatureEntity &creature);
BIT_FLAGS has_esp_nonliving(CreatureEntity &creature);
BIT_FLAGS has_esp_unique(CreatureEntity &creature);
BIT_FLAGS has_esp_telepathy(CreatureEntity &creature);
BIT_FLAGS has_bless_blade(CreatureEntity &creature);
BIT_FLAGS has_easy2_weapon(CreatureEntity &creature);
BIT_FLAGS has_down_saving(CreatureEntity &creature);
BIT_FLAGS has_no_ac(CreatureEntity &creature);
BIT_FLAGS has_invuln_arrow(CreatureEntity &creature);
void check_no_flowed(CreatureEntity &creature);
BIT_FLAGS has_mighty_throw(CreatureEntity &creature);
BIT_FLAGS has_dec_mana(CreatureEntity &creature);
BIT_FLAGS has_reflect(CreatureEntity &creature);
BIT_FLAGS has_see_nocto(CreatureEntity &creature);
BIT_FLAGS has_warning(CreatureEntity &creature);
BIT_FLAGS has_anti_magic(CreatureEntity &creature);
BIT_FLAGS has_anti_tele(CreatureEntity &creature);
BIT_FLAGS has_sh_fire(CreatureEntity &creature);
BIT_FLAGS has_sh_elec(CreatureEntity &creature);
BIT_FLAGS has_sh_cold(CreatureEntity &creature);
BIT_FLAGS has_easy_spell(CreatureEntity &creature);
BIT_FLAGS has_hard_spell(CreatureEntity &creature);
BIT_FLAGS has_hold_exp(CreatureEntity &creature);
BIT_FLAGS has_see_inv(CreatureEntity &creature);
BIT_FLAGS has_magic_mastery(CreatureEntity &creature);
BIT_FLAGS has_free_act(CreatureEntity &creature);
BIT_FLAGS has_sustain_str(CreatureEntity &creature);
BIT_FLAGS has_sustain_int(CreatureEntity &creature);
BIT_FLAGS has_sustain_wis(CreatureEntity &creature);
BIT_FLAGS has_sustain_dex(CreatureEntity &creature);
BIT_FLAGS has_sustain_con(CreatureEntity &creature);
BIT_FLAGS has_sustain_chr(CreatureEntity &creature);
BIT_FLAGS has_levitation(CreatureEntity &creature);
bool has_can_swim(CreatureEntity &creature);
BIT_FLAGS has_slow_digest(CreatureEntity &creature);
BIT_FLAGS has_regenerate(CreatureEntity &creature);
void update_curses(CreatureEntity &creature);
BIT_FLAGS has_impact(CreatureEntity &creature);
BIT_FLAGS has_earthquake(CreatureEntity &creature);
void update_extra_blows(CreatureEntity &creature);
BIT_FLAGS has_resist_acid(CreatureEntity &creature);
BIT_FLAGS has_vuln_acid(CreatureEntity &creature);
BIT_FLAGS has_resist_elec(CreatureEntity &creature);
BIT_FLAGS has_vuln_elec(CreatureEntity &creature);
BIT_FLAGS has_resist_fire(CreatureEntity &creature);
BIT_FLAGS has_vuln_fire(CreatureEntity &creature);
BIT_FLAGS has_resist_cold(CreatureEntity &creature);
BIT_FLAGS has_vuln_cold(CreatureEntity &creature);
BIT_FLAGS has_resist_pois(CreatureEntity &creature);
BIT_FLAGS has_resist_conf(CreatureEntity &creature);
BIT_FLAGS has_resist_sound(CreatureEntity &creature);
BIT_FLAGS has_resist_lite(CreatureEntity &creature);
BIT_FLAGS has_vuln_lite(CreatureEntity &creature);
BIT_FLAGS has_resist_dark(CreatureEntity &creature);
BIT_FLAGS has_resist_chaos(CreatureEntity &creature);
BIT_FLAGS has_resist_disen(CreatureEntity &creature);
BIT_FLAGS has_resist_shard(CreatureEntity &creature);
BIT_FLAGS has_resist_nexus(CreatureEntity &creature);
BIT_FLAGS has_resist_blind(CreatureEntity &creature);
BIT_FLAGS has_resist_neth(CreatureEntity &creature);
BIT_FLAGS has_resist_time(CreatureEntity &creature);
BIT_FLAGS has_resist_water(CreatureEntity &creature);
BIT_FLAGS has_resist_curse(CreatureEntity &creature);
BIT_FLAGS has_vuln_curse(CreatureEntity &creature);
BIT_FLAGS has_heavy_vuln_curse(CreatureEntity &creature);
BIT_FLAGS has_resist_fear(CreatureEntity &creature);
BIT_FLAGS has_immune_acid(CreatureEntity &creature);
BIT_FLAGS has_immune_elec(CreatureEntity &creature);
BIT_FLAGS has_immune_fire(CreatureEntity &creature);
BIT_FLAGS has_immune_cold(CreatureEntity &creature);
BIT_FLAGS has_immune_dark(CreatureEntity &creature);
BIT_FLAGS has_immune_lite(CreatureEntity &creature);
bool can_attack_with_main_hand(CreatureEntity &creature);
bool can_attack_with_sub_hand(CreatureEntity &creature);
bool has_two_handed_weapons(CreatureEntity &creature);
BIT_FLAGS has_lite(CreatureEntity &creature);
bool has_disable_two_handed_bonus(CreatureEntity &creature, int i);
bool has_not_ninja_weapon(CreatureEntity &creature, int i);
bool has_not_monk_weapon(CreatureEntity &creature, int i);
bool is_wielding_icky_weapon(CreatureEntity &creature, int i);
bool is_wielding_icky_riding_weapon(CreatureEntity &creature, int i);
bool has_good_luck(CreatureEntity &creature);
bool has_pervert_attraction(CreatureEntity &creature);
BIT_FLAGS player_aggravate_state(CreatureEntity &creature);
melee_type player_melee_type(CreatureEntity &creature);
bool has_aggravate(CreatureEntity &creature);
bool has_aggravate_nasty(CreatureEntity &creature);
