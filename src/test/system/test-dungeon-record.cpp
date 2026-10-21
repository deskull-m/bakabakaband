/*!
 * @brief DungeonRecord の帰還深度と訪問履歴のテスト
 * @details hengband#5730 相当。帰還の解放 (has_entered) と実際の訪問 (has_visited) を
 * 区別することを検証する。上流のセーブ往復・ダンプ分割テストは未移植のテスト基盤
 * (dungeon-list-test-access.h) に依存するため、ここでは DungeonRecord 単体の挙動のみ検証する。
 */

#include "system/dungeon/dungeon-record.h"

#include <doctest/doctest.h>

TEST_CASE("DungeonRecord distinguishes unlocked recall from a visit")
{
    DungeonRecord record;
    CHECK_FALSE(record.has_entered());
    CHECK_FALSE(record.has_visited());

    record.set_max_level(20);
    CHECK(record.has_entered());
    CHECK_FALSE(record.has_visited());
    CHECK(record.get_max_level() == 20);

    // Visiting the unlocked depth must not require a new maximum depth.
    record.mark_visited();
    CHECK(record.has_visited());
    CHECK(record.get_max_level() == 20);
}

TEST_CASE("DungeonRecord preserves visits when recall depth changes")
{
    DungeonRecord record;
    record.set_max_level(30);
    record.mark_visited();
    record.set_max_level(10);
    CHECK(record.has_visited());
    CHECK(record.get_max_level() == 10);
    CHECK(record.get_max_max_level() == 30);
}

TEST_CASE("DungeonRecord reset clears both recall availability and visits")
{
    DungeonRecord record;
    record.set_max_level(20);
    record.mark_visited();
    record.reset();
    CHECK_FALSE(record.has_entered());
    CHECK_FALSE(record.has_visited());
    CHECK(record.get_max_level() == 0);
    CHECK(record.get_max_max_level() == 0);
    record.set_max_level(20);
    CHECK_FALSE(record.has_visited());
}
