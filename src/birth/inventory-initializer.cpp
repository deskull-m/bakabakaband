#include "birth/inventory-initializer.h"
#include "autopick/autopick.h"
#include "birth/initial-equipments-table.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/object-ego.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/enum-converter.h"
#include <tuple>

/*!
 * @brief 所持状態にあるアイテムの中から一部枠の装備可能なものを装備させる。
 */
void wield_all(CreatureEntity &creature)
{

    for (INVENTORY_IDX i_idx = INVEN_PACK - 1; i_idx >= 0; i_idx--) {
        const auto &item = *creature.inventory[i_idx];
        if (!item.is_valid()) {
            continue;
        }

        int slot = wield_slot(creature, item);
        if (slot < INVEN_MAIN_HAND) {
            continue;
        }
        if (slot == INVEN_LITE) {
            continue;
        }

        auto &wield_slot_item = *creature.inventory[slot];
        if (wield_slot_item.is_valid()) {
            continue;
        }

        wield_slot_item = item.clone();
        wield_slot_item.number = 1;

        if (i_idx >= 0) {
            inven_item_increase(creature, i_idx, -1);
            inven_item_optimize(creature, i_idx);
        } else {
            floor_item_increase(creature, 0 - i_idx, -1);
            floor_item_optimize(creature, 0 - i_idx);
        }

        creature.increment_equip_cnt();
    }
}

/*!
 * @brief 初期所持アイテムの処理 / Add an outfit object
 * @details アイテムを既知のものとした上でwield_all()関数により装備させる。
 * @param item 処理したいアイテムへの参照
 */
static void add_outfit(CreatureEntity &creature, ItemEntity &item)
{
    object_aware(creature, item);
    item.mark_as_known();
    const auto slot = creature.store_item(item);
    autopick_alter_item(creature, slot, false);
    wield_all(creature);
}

static void decide_initial_items(CreatureEntity &creature)
{
    switch (creature.prace) {
    case PlayerRaceType::VAMPIRE:
        /* Nothing! */
        /* Vampires can drain blood of creatures */
        return;
    case PlayerRaceType::BALROG:
        /* Demon can drain vitality from humanoid corpse */
        get_mon_num_prep_enum(creature, MonraceHook::HUMAN);
        for (int i = rand_range(3, 4); i > 0; i--) {
            ItemEntity item({ ItemKindType::MONSTER_REMAINS, SV_CORPSE });
            item.pval = enum2i(get_mon_num(creature, 0, 2, PM_NONE));
            if (item.pval) {
                item.number = 1;
                add_outfit(creature, item);
            }
        }

        return;
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE: {
        /* Staff (of Nothing) */
        ItemEntity item({ ItemKindType::STAFF, SV_STAFF_NOTHING });
        item.number = 1;
        add_outfit(creature, item);
        return;
    }
    case PlayerRaceType::ENT: {
        /* Potions of Water */
        ItemEntity item({ ItemKindType::POTION, SV_POTION_WATER });
        item.number = rand_range(15, 23);
        add_outfit(creature, item);
        return;
    }
    case PlayerRaceType::ANDROID: {
        /* Flasks of oil */
        ItemEntity item(ItemKindType::FLASK);
        ItemMagicApplier(creature, &item, 1, AM_NO_FIXED_ART).execute();
        item.number = rand_range(7, 12);
        add_outfit(creature, item);
        return;
    }
    default: {
        /* Food rations */
        ItemEntity item({ ItemKindType::FOOD, SV_FOOD_RATION });
        item.number = rand_range(3, 7);
        add_outfit(creature, item);
        return;
    }
    }
}

/*!
 * @brief 種族/職業/性格などに基づき初期所持アイテムを設定するメインセット関数。 / Init players with some belongings
 * @details Having an item makes the creature "aware" of its purpose.
 */
void player_outfit(CreatureEntity &creature)
{

    const auto &baseitems = BaseitemList::get_instance();
    ItemEntity parchment;
    decide_initial_items(creature);

    // アンナタールの羊皮紙
    parchment.generate(baseitems.lookup_baseitem_id({ ItemKindType::READING_MATTER, 0 }));
    parchment.number = 1;
    add_outfit(creature, parchment);

    // メルコールの羊皮紙
    parchment.generate(baseitems.lookup_baseitem_id({ ItemKindType::READING_MATTER, 3 }));
    parchment.number = 1;
    add_outfit(creature, parchment);

    CreatureClass pc(creature);
    CreatureRace pr(&creature);
    if (pr.equals(PlayerRaceType::VAMPIRE) && !pc.equals(PlayerClassType::NINJA)) {
        ItemEntity item({ ItemKindType::SCROLL, SV_SCROLL_DARKNESS });
        item.number = rand_range(2, 5);
        add_outfit(creature, item);
    } else if (!pc.equals(PlayerClassType::NINJA)) {
        ItemEntity item({ ItemKindType::LITE, SV_LITE_TORCH });
        item.number = rand_range(3, 7);
        item.fuel = rand_range(3, 7) * 500;
        add_outfit(creature, item);
    }

    if (pr.equals(PlayerRaceType::MERFOLK)) {
        ItemEntity item({ ItemKindType::RING, SV_RING_LEVITATION_FALL });
        item.number = 1;
        add_outfit(creature, item);
    }

    if (pc.equals(PlayerClassType::RANGER) || pc.equals(PlayerClassType::CAVALRY)) {
        ItemEntity item({ ItemKindType::ARROW, SV_AMMO_NORMAL });
        item.number = rand_range(15, 20);
        add_outfit(creature, item);
    }

    if (pc.equals(PlayerClassType::RANGER)) {
        ItemEntity item({ ItemKindType::BOW, SV_SHORT_BOW });
        add_outfit(creature, item);
    } else if (pc.equals(PlayerClassType::ARCHER)) {
        ItemEntity item({ ItemKindType::ARROW, SV_AMMO_NORMAL });
        item.number = rand_range(15, 20);
        add_outfit(creature, item);
    } else if (pc.equals(PlayerClassType::HIGH_MAGE) || pc.equals(PlayerClassType::ELEMENTALIST)) {
        ItemEntity item({ ItemKindType::WAND, SV_WAND_MAGIC_MISSILE });
        item.number = 1;
        item.pval = static_cast<short>(rand_range(25, 30));
        add_outfit(creature, item);
    } else if (pc.equals(PlayerClassType::SORCERER)) {
        for (auto book_tval = enum2i(ItemKindType::LIFE_BOOK); book_tval <= enum2i(ItemKindType::LIFE_BOOK) + MAX_MAGIC - 1; book_tval++) {
            ItemEntity item({ i2enum<ItemKindType>(book_tval), 0 });
            item.number = 1;
            add_outfit(creature, item);
        }
    } else if (pc.equals(PlayerClassType::TOURIST)) {
        if (creature.ppersonality != PERSONALITY_SEXY) {
            ItemEntity item({ ItemKindType::SHOT, SV_AMMO_LIGHT });
            item.number = rand_range(15, 20);
            add_outfit(creature, item);
        }

        ItemEntity item_biscuit({ ItemKindType::FOOD, SV_FOOD_BISCUIT });
        item_biscuit.number = rand_range(2, 4);
        add_outfit(creature, item_biscuit);

        ItemEntity item_waybread({ ItemKindType::FOOD, SV_FOOD_WAYBREAD });
        item_waybread.number = rand_range(2, 4);
        add_outfit(creature, item_waybread);

        ItemEntity item_jerky({ ItemKindType::FOOD, SV_FOOD_JERKY });
        item_jerky.number = rand_range(1, 3);
        add_outfit(creature, item_jerky);

        ItemEntity item_ale({ ItemKindType::FOOD, SV_FOOD_PINT_OF_ALE });
        item_ale.number = rand_range(2, 4);
        add_outfit(creature, item_ale);

        ItemEntity item_wine({ ItemKindType::FOOD, SV_FOOD_PINT_OF_WINE });
        item_wine.number = rand_range(2, 4);
        add_outfit(creature, item_wine);
    } else if (pc.equals(PlayerClassType::NINJA)) {
        ItemEntity item({ ItemKindType::SPIKE, 0 });
        item.number = rand_range(15, 20);
        add_outfit(creature, item);
    } else if (pc.equals(PlayerClassType::SNIPER)) {
        ItemEntity item({ ItemKindType::BOLT, SV_AMMO_NORMAL });
        item.number = rand_range(15, 20);
        add_outfit(creature, item);
    }

    // @todo 本来read-onlyであるべきプリセットテーブルを書き換えている. 良くないパターン.
    // 「状況によって特別に持たせたいアイテム」は別途定義すべき.
    if (!pc.equals(PlayerClassType::SORCERER)) {
        auto short_pclass = enum2i(creature.pclass);
        if (creature.ppersonality == PERSONALITY_SEXY) {
            player_init[short_pclass][2] = std::make_tuple(ItemKindType::HAFTED, SV_WHIP);
        } else if (pr.equals(PlayerRaceType::MERFOLK)) {
            player_init[short_pclass][2] = std::make_tuple(ItemKindType::POLEARM, SV_TRIDENT);
        }
    }

    for (auto i = 0; i < 3; i++) {
        auto &[tval, sval] = player_init[enum2i(creature.pclass)][i];
        if (pr.equals(PlayerRaceType::ANDROID) && ((tval == ItemKindType::SOFT_ARMOR) || (tval == ItemKindType::HARD_ARMOR))) {
            continue;
        }

        PlayerRealm prealm(creature);
        if (tval == ItemKindType::SORCERY_BOOK) {
            tval = prealm.realm1().get_book();
        } else if (tval == ItemKindType::DEATH_BOOK) {
            tval = prealm.realm2().get_book();
        } else if (tval == ItemKindType::RING && sval == SV_RING_RES_FEAR && pr.equals(PlayerRaceType::BARBARIAN)) {
            sval = SV_RING_SUSTAIN_STR;
        } else if (tval == ItemKindType::RING && sval == SV_RING_SUSTAIN_INT && pr.equals(PlayerRaceType::MIND_FLAYER)) {
            tval = ItemKindType::POTION;
            sval = SV_POTION_RESTORE_MANA;
        }

        /* Only assassins get a poisoned weapon */
        ItemEntity item({ tval, sval });
        if (((tval == ItemKindType::SWORD) || (tval == ItemKindType::HAFTED)) && (pc.equals(PlayerClassType::ROGUE) && prealm.realm1().equals(RealmType::DEATH))) {
            item.ego_idx = EgoType::BRAND_POIS;
        }

        add_outfit(creature, item);
    }

    BaseitemList::get_instance().mark_common_items_as_aware();
}
