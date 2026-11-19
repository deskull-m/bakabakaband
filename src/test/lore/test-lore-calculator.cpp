/*!
 * @brief 思い出へ写すフラグの計算のテスト
 */

#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-kind-flags.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/monrace-records-test-access.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include <doctest/doctest.h>

namespace {
constexpr auto TEST_MONRACE_ID = static_cast<MonraceId>(1);

//! 種族分類を 1 つだけ持つ種族の思い出を作る
EnumClassFlagGroup<MonsterKindType> kind_flags_of_lore(MonsterKindType kind_flag)
{
    auto &monrace = MonraceList::get_instance().get_monrace(TEST_MONRACE_ID);
    monrace.kind_flags.clear();
    monrace.kind_flags.set(kind_flag);
    lore_type lore(TEST_MONRACE_ID, MONSTER_LORE_NORMAL);
    set_known_kind_flags(&lore);
    return lore.kind_flags;
}
}

TEST_CASE("set_known_kind_flags copies every kind flag of the monrace")
{
    test::MonraceListTestAccess monraces;
    test::MonraceRecordsTestAccess records;
    MonraceList::get_instance().emplace(TEST_MONRACE_ID);

    SUBCASE("No kind flag is left unwired")
    {
        // 種族分類をフラグごとに写していた頃は、写し漏れた分類の表示処理
        // (display_monster_kind() 等) がデッドコードになっていた。
        // MonsterKindType を追加したときに同じ漏れが起きないよう全列挙を確かめる。
        for (const auto kind_flag : EnumRange(MonsterKindType::UNIQUE, MonsterKindType::MAX)) {
            // CAPTURE() に enum をそのまま渡すと文字列化できないため整数で受ける
            const auto kind_flag_index = enum2i(kind_flag);
            CAPTURE(kind_flag_index);
            CHECK(kind_flags_of_lore(kind_flag).has(kind_flag));
        }
    }

    SUBCASE("Only the kind flags of the monrace are copied")
    {
        // 種族が持たない分類まで立つと、無関係な分類が思い出に出てしまう
        const auto kind_flags = kind_flags_of_lore(MonsterKindType::ROGUE);
        CHECK(kind_flags.count() == 1);
        CHECK(kind_flags.has(MonsterKindType::ROGUE));
    }
}
