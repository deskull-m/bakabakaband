#include "object-use/item-use-checker.h"
#include "system/creature-entity.h"
#include "view/display-messages.h"

ItemUseChecker::ItemUseChecker(CreatureEntity &creature)
    : creature(creature)
{
}

bool ItemUseChecker::check_stun(std::string_view mes) const
{
    const auto penalty = this->creature.get_stun_item_chance_penalty();
    if (penalty >= randint1(100)) {
        msg_print(mes);
        return false;
    }

    return true;
}
