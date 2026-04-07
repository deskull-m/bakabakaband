#include "floor/party-monsters.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "world/world.h"

PartyMonsters party_monsters;

MonsterEntity &PartyMonsters::operator[](int i)
{
    return monsters_[i];
}

const MonsterEntity &PartyMonsters::operator[](int i) const
{
    return monsters_[i];
}

void PartyMonsters::invalidate_all()
{
    for (auto &monster : monsters_) {
        monster.r_idx = MonraceList::empty_id();
    }
}

void PartyMonsters::wipe_all()
{
    for (auto &monster : monsters_) {
        monster.wipe();
    }
}

/*!
 * @brief フロアにいるペットの種族出現数カウンタを加算する
 */
void PartyMonsters::precalc_cur_num_of_pet() const
{
    const auto max_num = AngbandWorld::get_instance().is_wild_mode() ? 1 : MAX_SIZE;
    for (auto i = 0; i < max_num; i++) {
        const auto &monster = monsters_[i];
        if (!monster.is_valid()) {
            continue;
        }

        monster.get_real_monrace().increment_current_numbers();
    }
}
