#include "mind/mind-chaos-warrior.h"
#include "floor/floor-object.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include <span>

void acquire_chaos_weapon(PlayerType *player_ptr)
{
    constexpr static auto weapons = {
        SV_DAGGER,
        SV_DAGGER,
        SV_MAIN_GAUCHE,
        SV_MAIN_GAUCHE,
        SV_TANTO,
        SV_RAPIER,
        SV_RAPIER,
        SV_SMALL_SWORD,
        SV_SMALL_SWORD,
        SV_BASILLARD, // LV10
        SV_BASILLARD,
        SV_SHORT_SWORD,
        SV_SHORT_SWORD,
        SV_SHORT_SWORD,
        SV_SABRE,
        SV_SABRE,
        SV_CUTLASS,
        SV_CUTLASS,
        SV_WAKIZASHI,
        SV_KHOPESH, // LV20
        SV_TULWAR,
        SV_BROAD_SWORD,
        SV_LONG_SWORD,
        SV_LONG_SWORD,
        SV_SCIMITAR,
        SV_SCIMITAR,
        SV_NINJATO,
        SV_KATANA,
        SV_BASTARD_SWORD,
        SV_BASTARD_SWORD, // LV30
        SV_GREAT_SCIMITAR,
        SV_CLAYMORE,
        SV_ESPADON,
        SV_TWO_HANDED_SWORD,
        SV_FLAMBERGE,
        SV_NO_DACHI,
        SV_EXECUTIONERS_SWORD,
        SV_ZWEIHANDER,
        SV_HAYABUSA,
        SV_BLADE_OF_CHAOS, // LV40
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS,
        SV_BLADE_OF_CHAOS, // LV50
    };

    const auto candidates = std::span(weapons).first(player_ptr->lev);
    const auto sval = rand_choice(candidates);

    ItemEntity item({ ItemKindType::SWORD, sval });
    item.to_h = 3 + randint1(player_ptr->current_floor_ptr->dun_level) % 10;
    item.to_d = 3 + randint1(player_ptr->current_floor_ptr->dun_level) % 10;
    one_resistance(&item);
    item.ego_idx = EgoType::CHAOTIC;
    (void)drop_near(player_ptr, item, player_ptr->get_position());
}

void acquire_hafted_weapon(PlayerType *player_ptr)
{
    ItemEntity item(BaseitemKey(ItemKindType::HAFTED, SV_GREAT_HAMMER));
    {
        item.pval = 1;
        item.to_h = 3 + randint1(18);
        item.to_d = 3 + randint1(18);
        item.to_a = randint1(5);

        switch (randint1(21)) {
        case 1:
        case 2:
        case 3:
            item.ego_idx = EgoType::CHAOTIC;
            break;
        case 4:
        case 5:
        case 6:
            item.ego_idx = EgoType::EARTHQUAKES;
            break;
        case 7:
        case 8:
        case 9:
            item.ego_idx = EgoType::DEMON;
            break;
        case 10:
        case 11:
            item.ego_idx = EgoType::KILL_HUMAN;
            break;
        case 12:
        case 13:
        case 14:
        case 15:
            item.ego_idx = EgoType::MORGUL;
            break;
        default:
            // エゴなしの場合はそのまま
            break;
        }
    }
    (void)drop_near(player_ptr, item, player_ptr->get_position());
}
