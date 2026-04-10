#include "spell/spells-execution.h"
#include "realm/realm-arcane.h"
#include "realm/realm-chaos.h"
#include "realm/realm-craft.h"
#include "realm/realm-crusade.h"
#include "realm/realm-death.h"
#include "realm/realm-demon.h"
#include "realm/realm-hex.h"
#include "realm/realm-hissatsu.h"
#include "realm/realm-life.h"
#include "realm/realm-nature.h"
#include "realm/realm-song.h"
#include "realm/realm-sorcery.h"
#include "realm/realm-trump.h"
#include "realm/realm-types.h"
#include "system/creature-entity.h"

/*!
 * @brief 魔法処理のメインルーチン
 * @param realm 魔法領域のID
 * @param spell 各領域の魔法ID
 * @param mode 求める処理
 * @return 各領域魔法に各種テキストを求めた場合は文字列参照ポインタ、そうでない場合はnullptrを返す。
 */
tl::optional<std::string> exe_spell(CreatureEntity &creature, RealmType realm, SPELL_IDX spell, SpellProcessType mode)
{
    switch (realm) {
    case RealmType::LIFE:
        return do_life_spell(creature, spell, mode);
    case RealmType::SORCERY:
        return do_sorcery_spell(creature, spell, mode);
    case RealmType::NATURE:
        return do_nature_spell(creature, spell, mode);
    case RealmType::CHAOS:
        return do_chaos_spell(creature, spell, mode);
    case RealmType::DEATH:
        return do_death_spell(creature, spell, mode);
    case RealmType::TRUMP:
        return do_trump_spell(creature, spell, mode);
    case RealmType::ARCANE:
        return do_arcane_spell(creature, spell, mode);
    case RealmType::CRAFT:
        return do_craft_spell(creature, spell, mode);
    case RealmType::DAEMON:
        return do_daemon_spell(creature, spell, mode);
    case RealmType::CRUSADE:
        return do_crusade_spell(creature, spell, mode);
    case RealmType::MUSIC:
        return do_music_spell(creature, spell, mode);
    case RealmType::HISSATSU:
        return do_hissatsu_spell(creature, spell, mode);
    case RealmType::HEX:
        return do_hex_spell(creature, i2enum<spell_hex_type>(spell), mode);
    default:
        return tl::nullopt;
    }
}
