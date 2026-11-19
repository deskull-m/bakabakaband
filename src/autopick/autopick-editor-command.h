#pragma once

/*! APE means AutoPickEditor*/
enum ape_quittance {
    APE_QUIT = 0,
    APE_QUIT_WITHOUT_SAVE = 1,
    APE_QUIT_AND_SAVE = 2
};

class CreatureEntity;
class AutopickTextBody;
ape_quittance do_editor_command(CreatureEntity &creature, AutopickTextBody *tb, int com_id);
