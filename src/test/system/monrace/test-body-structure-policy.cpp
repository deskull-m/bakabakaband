/*!
 * @brief 体構造ごとの装備スロットポリシーと表示名のテスト
 *
 * 体構造 (BodyStructureType) ごとに、どの装備スロットが許可されるか、
 * どの拡張スロットを持つか、および表示名・表示色が定義されているかを検証する。
 * c コマンドのステータス表示と r コマンドの思い出表示は同じ表示名を共用するため、
 * 全ての体構造について名前が空でないことを保証する。
 */

#include "system/monrace/body-structure-policy.h"

#include "inventory/inventory-slot-types.h"
#include "system/monrace/body-structure-types.h"
#include "system/monrace/extended-slot.h"
#include "term/term-color-types.h"
#include "util/enum-converter.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

namespace {

//! 全ての体構造 (MAX を除く)
constexpr BodyStructureType ALL_BODY_STRUCTURES[] = {
    BodyStructureType::HUMANOID,
    BodyStructureType::BIPEDAL,
    BodyStructureType::QUADRUPED,
    BodyStructureType::SERPENTINE,
    BodyStructureType::AMORPHOUS,
    BodyStructureType::INCORPOREAL,
    BodyStructureType::DRACONIC,
};

//! 装備スロットのうち許可されているものを列挙する
std::vector<int> collect_allowed_slots(BodyStructureType type)
{
    const auto &policy = get_body_slot_policy(type);
    std::vector<int> allowed;
    for (int slot = INVEN_MAIN_HAND; slot < INVEN_TOTAL; slot++) {
        if (policy.is_allowed(slot)) {
            allowed.push_back(slot);
        }
    }

    return allowed;
}

bool contains_slot(const std::vector<int> &slots, int slot)
{
    return std::find(slots.begin(), slots.end(), slot) != slots.end();
}

}

TEST_CASE("HUMANOID allows every equipment slot")
{
    // HUMANOID は既定の体構造であり、通常のプレイヤーもこれとして扱われる。
    // 全 13 スロットが許可されていないと、プレイヤーの装備が壊れる。
    const auto allowed = collect_allowed_slots(BodyStructureType::HUMANOID);
    CHECK(allowed.size() == static_cast<size_t>(INVEN_TOTAL - INVEN_MAIN_HAND));
    for (int slot = INVEN_MAIN_HAND; slot < INVEN_TOTAL; slot++) {
        CAPTURE(slot);
        CHECK(get_body_slot_policy(BodyStructureType::HUMANOID).is_allowed(slot));
    }
}

TEST_CASE("INCORPOREAL allows no equipment slot")
{
    CHECK(collect_allowed_slots(BodyStructureType::INCORPOREAL).empty());
}

TEST_CASE("restricted body structures allow only their own slots")
{
    SUBCASE("BIPEDAL cannot wield weapons but wears neck/lite/body/head/feet")
    {
        const auto allowed = collect_allowed_slots(BodyStructureType::BIPEDAL);
        CHECK(contains_slot(allowed, INVEN_NECK));
        CHECK(contains_slot(allowed, INVEN_LITE));
        CHECK(contains_slot(allowed, INVEN_BODY));
        CHECK(contains_slot(allowed, INVEN_HEAD));
        CHECK(contains_slot(allowed, INVEN_FEET));
        CHECK_FALSE(contains_slot(allowed, INVEN_MAIN_HAND));
        CHECK_FALSE(contains_slot(allowed, INVEN_BOW));
    }

    SUBCASE("QUADRUPED wears only neck/body/head")
    {
        const auto allowed = collect_allowed_slots(BodyStructureType::QUADRUPED);
        CHECK(allowed.size() == 3);
        CHECK(contains_slot(allowed, INVEN_NECK));
        CHECK(contains_slot(allowed, INVEN_BODY));
        CHECK(contains_slot(allowed, INVEN_HEAD));
    }

    SUBCASE("SERPENTINE wears only neck/body")
    {
        const auto allowed = collect_allowed_slots(BodyStructureType::SERPENTINE);
        CHECK(allowed.size() == 2);
        CHECK(contains_slot(allowed, INVEN_NECK));
        CHECK(contains_slot(allowed, INVEN_BODY));
    }

    SUBCASE("AMORPHOUS wears only two rings")
    {
        const auto allowed = collect_allowed_slots(BodyStructureType::AMORPHOUS);
        CHECK(allowed.size() == 2);
        CHECK(contains_slot(allowed, INVEN_MAIN_RING));
        CHECK(contains_slot(allowed, INVEN_SUB_RING));
    }
}

TEST_CASE("extended slots are defined per body structure")
{
    SUBCASE("HUMANOID has no extended slot")
    {
        CHECK(get_body_slot_policy(BodyStructureType::HUMANOID).get_extended_slots().empty());
    }

    SUBCASE("SERPENTINE has a tail ring")
    {
        const auto &slots = get_body_slot_policy(BodyStructureType::SERPENTINE).get_extended_slots();
        REQUIRE(slots.size() == 1);
        CHECK(slots[0] == ExtendedSlotType::TAIL_RING);
    }

    SUBCASE("DRACONIC keeps humanoid slots and adds tail and wings")
    {
        // DRACONIC は HUMANOID と同じ全スロットに加えて拡張スロットを持つ
        CHECK(collect_allowed_slots(BodyStructureType::DRACONIC) == collect_allowed_slots(BodyStructureType::HUMANOID));

        const auto &slots = get_body_slot_policy(BodyStructureType::DRACONIC).get_extended_slots();
        REQUIRE(slots.size() == 3);
        CHECK(slots[0] == ExtendedSlotType::TAIL_RING);
        CHECK(slots[1] == ExtendedSlotType::WING_LEFT);
        CHECK(slots[2] == ExtendedSlotType::WING_RIGHT);
    }
}

TEST_CASE("out of range body structure falls back to HUMANOID policy")
{
    const auto &fallback = get_body_slot_policy(BodyStructureType::MAX);
    for (int slot = INVEN_MAIN_HAND; slot < INVEN_TOTAL; slot++) {
        CAPTURE(slot);
        CHECK(fallback.is_allowed(slot));
    }
}

TEST_CASE("slots outside the equipment range are never allowed")
{
    for (const auto type : ALL_BODY_STRUCTURES) {
        CAPTURE(enum2i(type));
        const auto &policy = get_body_slot_policy(type);
        CHECK_FALSE(policy.is_allowed(INVEN_MAIN_HAND - 1));
        CHECK_FALSE(policy.is_allowed(INVEN_TOTAL));
        CHECK_FALSE(policy.is_allowed(-1));
    }
}

TEST_CASE("every body structure has a display name and color")
{
    // c コマンドのステータス表示と r コマンドの思い出表示が共用するため、
    // 体構造を追加したときに名前の定義漏れがあると空欄が表示されてしまう。
    for (const auto type : ALL_BODY_STRUCTURES) {
        CAPTURE(enum2i(type));
        CHECK_FALSE(body_structure_name(type).empty());
        CHECK(body_structure_color(type) != TERM_DARK);
    }

    SUBCASE("distinct body structures have distinct names")
    {
        std::vector<std::string_view> names;
        for (const auto type : ALL_BODY_STRUCTURES) {
            names.push_back(body_structure_name(type));
        }

        std::sort(names.begin(), names.end());
        CHECK(std::adjacent_find(names.begin(), names.end()) == names.end());
    }

    SUBCASE("incorporeal and draconic are colored differently from the default")
    {
        CHECK(body_structure_color(BodyStructureType::INCORPOREAL) == TERM_L_DARK);
        CHECK(body_structure_color(BodyStructureType::DRACONIC) == TERM_ORANGE);
        CHECK(body_structure_color(BodyStructureType::HUMANOID) == TERM_L_BLUE);
    }
}
