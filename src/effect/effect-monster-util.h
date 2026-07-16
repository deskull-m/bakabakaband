#pragma once

#include "system/angband.h"
#include "system/creature-entity.h"
#include "util/point-2d.h"
#include <memory>
#include <string>

enum class AttributeType;
class Grid;
class MonraceDefinition;
class EffectMonster {
public:
    EffectMonster(CreatureEntity &creature, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg);

    char killer[MAX_MONSTER_NAME]{};
    bool obvious = false;
    bool skipped = false;
    bool get_angry = false;
    bool do_polymorph = false;
    int do_dist = 0;
    int do_conf = 0;
    int do_stun = 0;
    int do_sleep = 0;
    int do_fear = 0;
    int do_time = 0;
    bool heal_leper = false;
    GAME_TEXT m_name[MAX_NLEN]{};
    char m_poss[10]{};
    short photo = 0;
    std::string note = "";

    MONSTER_IDX src_idx;
    POSITION r;
    POSITION y;
    POSITION x;
    int dam;
    AttributeType attribute;
    BIT_FLAGS flag;
    bool see_s_msg;

    Grid *g_ptr;
    CreatureEntity *m_ptr;
    CreatureEntity *m_caster_ptr;
    std::shared_ptr<MonraceDefinition> monrace;
    bool seen;
    bool seen_msg;
    bool slept;
    bool known;
    std::string note_dies;
    int caster_lev;

    bool is_player() const;
    bool is_monster() const;
    Pos2D get_position() const;
};

enum tr_type : int;

/*!
 * @brief 対象モンスターが付与された player_race 由来の属性耐性を持つか (提案C1第2弾/第3弾/第4弾)
 * @details opt-in フラグ applies_player_race_resistances が立ち、かつ有効な player_race
 *          を持つ個体のみ、その種族の tr_flags に指定耐性 (res_flag) があれば true。
 *          既定 OFF のため誰にも反映されずバランス不変。prace==NONE は安全に false。
 *          属性ダメージ (effect-monster-resist-hurt) と状態異常 (fear 等) の両方から使う共通述語。
 */
bool target_race_resists_element(EffectMonster *em_ptr, tr_type res_flag);

/*!
 * @brief 状態異常/精神攻撃に対するモンスターのレベル基準セーヴ判定 (提案D4/C2第3弾セーヴ)
 * @param difficulty セーヴ難易度の基準値 (通常は em_ptr->dam。術者レベル基準の効果は caster_lev)
 * @return 抵抗に成功したら true
 * @details 実効レベル (monrace.level + WIS セーヴ補正) が randint1(max(1, difficulty-10)) + 10 を
 *          上回れば抵抗成功。oldies (poly/slow/sleep/conf/stun/stasis)・evil (turn 系)・
 *          spirit (mind_blast/brain_smash) に散在していた同一式を集約し、WIS 補正
 *          (applies_stat_combat_bonus、既定 OFF) の注入点を一元化する。randint1 の消費は
 *          1 回のみで、呼出側の短絡評価 (`UNIQUE || ...` 等) も保存される。
 */
bool monster_saves_status_by_level(EffectMonster *em_ptr, int difficulty);
