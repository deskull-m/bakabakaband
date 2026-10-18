#pragma once

#include "system/angband.h"

/*!
 * @brief 感知のメッセージを出す条件
 */
enum class DetectionNotice {
    ALWAYS, //!< 対象があれば出す
    NEW_ONLY, //!< 新しく感知した対象があるときだけ出す (感知を続ける効果用)
    NEVER, //!< 出さない
};

class CreatureEntity;
bool detect_traps(CreatureEntity &creature, POSITION range, bool known, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_doors(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_stairs(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_treasure(CreatureEntity &creature, POSITION range);
bool detect_objects_gold(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_objects_normal(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_objects_magic(CreatureEntity &creature, POSITION range);
bool detect_monsters_normal(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_monsters_invis(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_monsters_evil(CreatureEntity &creature, POSITION range, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_monsters_xxx(CreatureEntity &creature, POSITION range, uint32_t match_flag);
bool detect_monsters_string(CreatureEntity &creature, POSITION range, concptr, DetectionNotice notice = DetectionNotice::ALWAYS);
bool detect_monsters_nonliving(CreatureEntity &creature, POSITION range);
bool detect_monsters_mind(CreatureEntity &creature, POSITION range);
bool detect_all(CreatureEntity &creature, POSITION range);
void keep_detected_monsters(CreatureEntity &creature, POSITION range);
