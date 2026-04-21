#include "player/temporary-resistances.h"
#include "object-enchant/tr-types.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの一時的魔法効果による耐性を返す
 * Prints ratings on certain abilities
 * @param creature クリーチャーへの参照
 * @param flags フラグを保管する配列
 */
void tim_player_flags(CreatureEntity &creature, TrFlags &flags)
{
    BIT_FLAGS tmp_effect_flag = FLAG_CAUSE_MAGIC_TIME_EFFECT;
    set_bits(tmp_effect_flag, FLAG_CAUSE_STANCE);
    BIT_FLAGS race_class_flag = FLAG_CAUSE_CLASS;
    set_bits(race_class_flag, FLAG_CAUSE_RACE);

    flags.clear();

    if (is_oppose_acid(creature) && none_bits(has_immune_acid(creature), (race_class_flag | tmp_effect_flag))) {
        flags.set(TR_RES_ACID);
    }
    if (is_oppose_elec(creature) && none_bits(has_immune_elec(creature), (race_class_flag | tmp_effect_flag))) {
        flags.set(TR_RES_ELEC);
    }
    if (is_oppose_fire(creature) && none_bits(has_immune_fire(creature), (race_class_flag | tmp_effect_flag))) {
        flags.set(TR_RES_FIRE);
    }
    if (is_oppose_cold(creature) && none_bits(has_immune_cold(creature), (race_class_flag | tmp_effect_flag))) {
        flags.set(TR_RES_COLD);
    }
    if (is_oppose_pois(creature)) {
        flags.set(TR_RES_POIS);
    }

    for (int test_flag = 0; test_flag < TR_FLAG_MAX; test_flag++) {
        if (any_bits(get_player_flags(creature, i2enum<tr_type>(test_flag)), tmp_effect_flag)) {
            flags.set(i2enum<tr_type>(test_flag));
        }
    }
}
