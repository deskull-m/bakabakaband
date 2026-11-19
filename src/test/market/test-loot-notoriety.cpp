/*!
 * @brief 略奪による社会的問題 (loot-notoriety) のテスト
 *
 * 他者の正当な所有物 (ItemEntity::rightful_owner) を略奪した記録は
 * CreatureEntity::incident_tree に貯まり、各効果はそこから都度算出される。
 * 本テストは記録済みのインシデントから算出される派生値 (賞金首ランク・店舗
 * 価格ペナルティ・買取拒否・アライアンス印象値ペナルティ・街の住人の敵対)
 * の境界を検証する。
 */

#include "market/loot-notoriety.h"

#include "alliance/alliance.h"
#include "monster-race/race-wilderness-flags.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/store-sale-type.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/system/monrace-records-test-access.h"

#include <doctest/doctest.h>

namespace {
constexpr auto OWNER_MONRACE_ID = static_cast<MonraceId>(1);
constexpr auto TOWNSFOLK_MONRACE_ID = static_cast<MonraceId>(2);

//! 所有者となる種族 (アライアンス GOLAN 所属) と、街の住人の種族を用意する
MonraceDefinition &setup_monraces()
{
    auto &monraces = MonraceList::get_instance();
    auto &owner = monraces.emplace(OWNER_MONRACE_ID);
    owner.alliance_idx = AllianceType::GOLAN;
    auto &townsfolk = monraces.emplace(TOWNSFOLK_MONRACE_ID);
    townsfolk.wilderness_flags.set(MonsterWildernessType::WILD_TOWN);
    return owner;
}

//! 刻印付きのアイテムを作る
ItemEntity make_stolen_item()
{
    ItemEntity item;
    item.set_rightful_owner(OWNER_MONRACE_ID);
    return item;
}
}

TEST_CASE("略奪による社会的問題")
{
    test::MonraceRecordsTestAccess monraces_guard;
    setup_monraces();

    const auto stolen = make_stolen_item();
    const ItemEntity clean{};
    REQUIRE(stolen.has_rightful_owner());
    REQUIRE_FALSE(clean.has_rightful_owner());

    SUBCASE("賞金首ランクは略奪総額から算出され、上限で飽和する")
    {
        CreatureEntity creature;
        CHECK(get_loot_bounty_level(creature) == 0);

        // 1 ランク目の境界 (閾値の直前と直後)
        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 19999);
        CHECK(get_loot_bounty_level(creature) == 0);
        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 1);
        CHECK(get_loot_bounty_level(creature) == 1);

        // 上限で止まる (際限なく上がらない)
        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 100000000);
        CHECK(get_loot_bounty_level(creature) == get_loot_bounty_level_max());
    }

    SUBCASE("刻印の無い品は買い叩かれない")
    {
        CreatureEntity creature;
        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 1000000);
        REQUIRE(get_loot_bounty_level(creature) > 0);
        CHECK(get_loot_store_sale_discount_percent(creature, clean) == 0);
    }

    SUBCASE("刻印付きの品は賞金首ランクに応じて買い叩かれ、上限で飽和する")
    {
        CreatureEntity creature;
        const auto base = get_loot_store_sale_discount_percent(creature, stolen);
        CHECK(base > 0);

        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 20000);
        REQUIRE(get_loot_bounty_level(creature) == 1);
        const auto rank1 = get_loot_store_sale_discount_percent(creature, stolen);
        CHECK(rank1 > base);

        // 完全に換金不能にはしない (割引率には上限がある)
        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 100000000);
        REQUIRE(get_loot_bounty_level(creature) == get_loot_bounty_level_max());
        const auto saturated = get_loot_store_sale_discount_percent(creature, stolen);
        CHECK(saturated > rank1);
        CHECK(saturated < 100);
    }

    SUBCASE("盗品の買取拒否は賞金首になってから、かつ正業の店だけ")
    {
        CreatureEntity creature;

        // 賞金首でなければどこでも買ってくれる
        CHECK_FALSE(store_refuses_stolen_goods(creature, stolen, StoreSaleType::GENERAL));

        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 20000);
        REQUIRE(get_loot_bounty_level(creature) >= 1);

        // 正業の店は扱わない
        CHECK(store_refuses_stolen_goods(creature, stolen, StoreSaleType::GENERAL));
        CHECK(store_refuses_stolen_goods(creature, stolen, StoreSaleType::WEAPON));
        CHECK(store_refuses_stolen_goods(creature, stolen, StoreSaleType::ARMOURY));

        // 闇市は盗品でも買う (売却経路が完全に閉じないようにするため)
        CHECK_FALSE(store_refuses_stolen_goods(creature, stolen, StoreSaleType::BLACK));

        // 我が家・博物館は売買ではないので対象外
        CHECK_FALSE(store_refuses_stolen_goods(creature, stolen, StoreSaleType::HOME));
        CHECK_FALSE(store_refuses_stolen_goods(creature, stolen, StoreSaleType::MUSEUM));

        // 刻印の無い品は拒否されない
        CHECK_FALSE(store_refuses_stolen_goods(creature, clean, StoreSaleType::GENERAL));
    }

    SUBCASE("アライアンス印象値ペナルティは略奪数に比例し、上限で飽和する")
    {
        CreatureEntity creature;
        CHECK(get_loot_alliance_impression_penalty(creature, AllianceType::GOLAN) == 0);

        creature.plus_incident_tree(std::string(INCIDENT_LOOT_ALLIANCE_PREFIX) + get_alliance_type_tag(AllianceType::GOLAN), 1);
        const auto unit = get_loot_alliance_impression_penalty(creature, AllianceType::GOLAN);
        CHECK(unit > 0);

        creature.plus_incident_tree(std::string(INCIDENT_LOOT_ALLIANCE_PREFIX) + get_alliance_type_tag(AllianceType::GOLAN), 9);
        CHECK(get_loot_alliance_impression_penalty(creature, AllianceType::GOLAN) == unit * 10);

        // 剥いでいない別のアライアンスには波及しない
        CHECK(get_loot_alliance_impression_penalty(creature, AllianceType::AMBER) == 0);

        // 無所属は所有権の記録対象にならないので常に 0
        CHECK(get_loot_alliance_impression_penalty(creature, AllianceType::NONE) == 0);

        // 際限なく敵対度が振り切れないよう有界
        creature.plus_incident_tree(std::string(INCIDENT_LOOT_ALLIANCE_PREFIX) + get_alliance_type_tag(AllianceType::GOLAN), 1000000);
        const auto saturated = get_loot_alliance_impression_penalty(creature, AllianceType::GOLAN);
        creature.plus_incident_tree(std::string(INCIDENT_LOOT_ALLIANCE_PREFIX) + get_alliance_type_tag(AllianceType::GOLAN), 1000000);
        CHECK(get_loot_alliance_impression_penalty(creature, AllianceType::GOLAN) == saturated);
    }

    SUBCASE("街の住人が敵対するのは賞金首になってからで、街の住人に限る")
    {
        CreatureEntity creature;
        const auto &monraces = MonraceList::get_instance();
        const auto &townsfolk = monraces.get_monrace(TOWNSFOLK_MONRACE_ID);
        const auto &outsider = monraces.get_monrace(OWNER_MONRACE_ID);
        REQUIRE(townsfolk.wilderness_flags.has(MonsterWildernessType::WILD_TOWN));
        REQUIRE_FALSE(outsider.wilderness_flags.has(MonsterWildernessType::WILD_TOWN));

        // 賞金首でなければ誰も敵対しない
        CHECK_FALSE(is_wanted_by_townsfolk(creature, townsfolk));
        CHECK_FALSE(is_wanted_by_townsfolk(creature, outsider));

        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 20000);
        REQUIRE(get_loot_bounty_level(creature) >= 1);

        // 街の住人だけが敵対する (街の外の種族には波及しない)
        CHECK(is_wanted_by_townsfolk(creature, townsfolk));
        CHECK_FALSE(is_wanted_by_townsfolk(creature, outsider));
    }

    SUBCASE("所有者の種族定義が失われた品は「記録なし」に落ちる")
    {
        // 種族は JSON 由来で増減しうるため、旧セーブに残った未知の ID で
        // get_monrace() の例外を踏まないことを確認する。
        ItemEntity unknown_owner;
        unknown_owner.set_rightful_owner(static_cast<MonraceId>(9999));
        CHECK_FALSE(unknown_owner.has_rightful_owner());
        CHECK(unknown_owner.describe_rightful_owner().empty());

        CreatureEntity creature;
        creature.plus_incident_tree(INCIDENT_LOOT_VALUE, 1000000);
        CHECK(get_loot_store_sale_discount_percent(creature, unknown_owner) == 0);
        CHECK_FALSE(store_refuses_stolen_goods(creature, unknown_owner, StoreSaleType::GENERAL));
    }
}
