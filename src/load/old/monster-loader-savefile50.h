#pragma once

#include "load/monster/monster-loader-base.h"

class MonsterLoader50 : public MonsterLoaderBase {
public:
    void rd_monster(CreatureEntity &monster) override;

private:
    void rd_monster_legacy(CreatureEntity &monster); //!< セーブデータバージョン <50 の旧フォーマット読込
    void rd_monster_v50(CreatureEntity &monster); //!< セーブデータバージョン >=50 の統合フォーマット読込
};
