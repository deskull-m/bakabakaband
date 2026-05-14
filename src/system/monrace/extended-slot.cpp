#include "system/monrace/extended-slot.h"
#include "system/angband.h"

std::string_view get_extended_slot_name(ExtendedSlotType type)
{
    switch (type) {
    case ExtendedSlotType::TAIL_RING:
        return _("尾の指輪", "Tail Ring");
    case ExtendedSlotType::SECOND_NECK:
        return _("第二の首", "Second Neck");
    case ExtendedSlotType::THIRD_HEAD:
        return _("第三の頭", "Third Head");
    case ExtendedSlotType::WING_LEFT:
        return _("左翼", "Left Wing");
    case ExtendedSlotType::WING_RIGHT:
        return _("右翼", "Right Wing");
    case ExtendedSlotType::MAX:
        return "";
    }
    return "";
}
