#include "mind/mind-warrior-mage.h"
#include "hpmp/hp-mp-processor.h"
#include "player/player-damage.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

bool comvert_hp_to_mp(CreatureEntity &creature)
{
    constexpr auto mes = _("ＨＰからＭＰへの無謀な変換", "thoughtless conversion from HP to SP");
    auto gain_sp = take_hit(creature, DAMAGE_USELIFE, creature.get_level(), mes) / 5;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    if (!gain_sp) {
        msg_print(_("変換に失敗した。", "You failed to convert."));
        rfu.set_flags(flags);
        return true;
    }

    creature.add_current_mp(gain_sp);
    if (creature.get_current_mp() > creature.get_max_mp()) {
        creature.set_current_mp(creature.get_max_mp());
        creature.current_mp_frac = 0;
    }

    rfu.set_flags(flags);
    return true;
}

bool comvert_mp_to_hp(CreatureEntity &creature)
{
    if (creature.get_current_mp() >= creature.get_level() / 5) {
        creature.sub_current_mp(creature.get_level() / 5);
        hp_player(creature, creature.get_level());
    } else {
        msg_print(_("変換に失敗した。", "You failed to convert."));
    }

    static constexpr auto flags = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    return true;
}
