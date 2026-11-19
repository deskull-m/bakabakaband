/*!
 * @brief セーブデータ (フォーマット) のバージョン比較のテスト
 * @details bakabakaband のロード処理は、旧 h_older_than (ゲームバージョンの 4 要素比較) ではなく、
 *          セーブファイル形式のバージョン番号 (単一の uint32) を loading_savefile_version に持ち、
 *          loading_savefile_version_is_older_than() で「読み込み中のセーブが指定版より古いか」を判定する。
 *          分岐の基準となるこの比較の境界をテストで固定する。
 */

#include "load/load-util.h"
#include "test/scoped-restore.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>

TEST_CASE("loading_savefile_version_is_older_than compares against the loaded savefile version")
{
    // グローバルの loading_savefile_version を退避し、テスト後に元へ戻す。
    const auto restore = test::scoped_restore(loading_savefile_version);

    loading_savefile_version = 50;
    CHECK(loading_savefile_version_is_older_than(51)); // 現在 < 閾値 → 古い
    CHECK_FALSE(loading_savefile_version_is_older_than(50)); // 同値は古くない (境界)
    CHECK_FALSE(loading_savefile_version_is_older_than(49)); // 現在 > 閾値 → 古くない

    // 現在版を基準にした境界 (v55 = 現行 SAVEFILE_VERSION 相当)。
    loading_savefile_version = 54;
    CHECK(loading_savefile_version_is_older_than(55));
    CHECK_FALSE(loading_savefile_version_is_older_than(54));
}

TEST_CASE("loading_savefile_version_is_older_than handles the representable extremes")
{
    const auto restore = test::scoped_restore(loading_savefile_version);
    constexpr auto max_version = std::numeric_limits<uint32_t>::max();

    loading_savefile_version = 0;
    CHECK(loading_savefile_version_is_older_than(1));
    CHECK_FALSE(loading_savefile_version_is_older_than(0));
    CHECK(loading_savefile_version_is_older_than(max_version));

    loading_savefile_version = max_version;
    CHECK_FALSE(loading_savefile_version_is_older_than(max_version));
    CHECK_FALSE(loading_savefile_version_is_older_than(0));
}
