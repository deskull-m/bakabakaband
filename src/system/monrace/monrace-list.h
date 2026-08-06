/*!
 * @brief モンスター種族の集合論的処理定義
 * @author Hourier
 * @date 2024/12/03
 */

#pragma once

#include "util/abstract-map-wrapper.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tl/optional.hpp>
#include <utility>
#include <vector>

enum class MonraceId : short;

class LocalizedString;
class MonraceDefinition;
class MonraceList : public util::AbstractMapWrapper<MonraceId, std::shared_ptr<MonraceDefinition>> {
public:
    MonraceList(MonraceList &&) = delete;
    MonraceList(const MonraceList &) = delete;
    MonraceList &operator=(const MonraceList &) = delete;
    MonraceList &operator=(MonraceList &&) = delete;

    static bool is_valid(MonraceId monrace_id);
    static const std::map<MonraceId, std::set<MonraceId>> &get_unified_uniques();
    static MonraceList &get_instance();
    static MonraceId empty_id();
    static bool is_tsuchinoko(MonraceId monrace_id);
    static bool is_dark_elf(MonraceId monrace_id);
    static bool is_chapel(MonraceId monrace_id);
    MonraceDefinition &emplace(MonraceId monrace_id);
    MonraceDefinition &get_monrace(MonraceId monrace_id);
    const MonraceDefinition &get_monrace(MonraceId monrace_id) const;
    std::shared_ptr<MonraceDefinition> get_monrace_shared(MonraceId monrace_id);
    std::shared_ptr<const MonraceDefinition> get_monrace_shared(MonraceId monrace_id) const;
    const std::vector<MonraceId> &get_valid_monrace_ids() const;
    bool is_angel(MonraceId monrace_id) const;
    bool can_unify_separate(MonraceId monrace_id) const;
    void kill_unified_unique(MonraceId monrace_id);
    bool is_selectable(MonraceId monrace_id) const;
    bool is_unified(MonraceId monrace_id) const;
    bool exists_separates(MonraceId monrace_id) const;
    bool is_separated(MonraceId monrace_id) const;
    bool can_select_separate(MonraceId morace_id, const int hp, const int maxhp) const;
    MonraceId select_random_separated_unique_of(MonraceId monrace_id) const;
    bool order(MonraceId id1, MonraceId id2, bool is_detailed = false) const;
    bool order_level(MonraceId id1, MonraceId id2) const;
    bool order_level_unique(MonraceId id1, MonraceId id2) const;
    MonraceId pick_id_at_random() const;
    const MonraceDefinition &pick_monrace_at_random() const;
    int calc_defeat_count() const;
    MonraceId select_figurine(int max_level) const;
    const LocalizedString &get_name(MonraceId monrace_id) const;
    const std::vector<std::pair<MonraceId, LocalizedString>> &get_normal_monster_names() const;
    const std::vector<std::pair<MonraceId, LocalizedString>> &get_unique_monster_names() const;

    void reset_current_numbers();
    void reset_all_visuals();
    tl::optional<std::string> probe_lore(MonraceId monrace_id);
    void kill_unique_monster(MonraceId monrace_id);

private:
    MonraceList() = default;

    //! @brief get_monrace() の結果を MonraceId 添字で引く O(1) フラットキャッシュ
    //! @details monraces (std::map, 約 2300 要素) の .at() は O(log n) のツリー探索で、
    //!          描画・AI・呪文判定のホットパスで多発する。monraces のエントリはロード時
    //!          (emplace) にのみ追加され、以降 shared_ptr の指す MonraceDefinition は差し替え
    //!          られず in-place 更新のみのため、生ポインタはセッション中安定。範囲内かつ非 null
    //!          なら即返し、未キャッシュ時は .at() (無効 id はここで例外) で引いて充填する。
    //!          存在しない id は例外となり負エントリを残さないため、追加 emplace による無効化は不要。
    MonraceDefinition &get_monrace_cached(MonraceId monrace_id) const;

    static MonraceList instance;
    std::map<MonraceId, std::shared_ptr<MonraceDefinition>> monraces;
    mutable std::vector<MonraceDefinition *> monrace_flat_cache;

    const static std::map<MonraceId, std::set<MonraceId>> unified_uniques;

    std::map<MonraceId, std::shared_ptr<MonraceDefinition>> &get_inner_container() override
    {
        return this->monraces;
    }
};
