#include "system/baseitem/civilization-level-types.h"
#include "locale/language-switcher.h"

std::string_view civilization_level_name(CivilizationLevel level)
{
    switch (level) {
    case CivilizationLevel::STONE:
        return _("石器", "Stone");
    case CivilizationLevel::BRONZE:
        return _("青銅器", "Bronze");
    case CivilizationLevel::IRON:
        return _("鉄器", "Iron");
    case CivilizationLevel::INDUSTRIAL:
        return _("工業化", "Industrial");
    case CivilizationLevel::ATOMIC:
        return _("原子力", "Atomic");
    case CivilizationLevel::INFORMATION:
        return _("情報化", "Information");
    case CivilizationLevel::SOLAR:
        return _("太陽系進出", "Solar Expansion");
    case CivilizationLevel::GALACTIC:
        return _("銀河系進出", "Galactic Expansion");
    case CivilizationLevel::NONE:
    case CivilizationLevel::MAX:
        return "";
    }

    return "";
}

bool is_civilization_usable(CivilizationLevel item_level, CivilizationLevel user_level)
{
    if ((item_level == CivilizationLevel::NONE) || (user_level == CivilizationLevel::NONE)) {
        return true;
    }

    return item_level <= user_level;
}
