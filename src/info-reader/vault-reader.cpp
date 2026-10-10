#include "info-reader/vault-reader.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/vault-info-tokens-table.h"
#include "locale/character-encoding.h"
#include "room/rooms-vault.h"
#include "room/vault-flag-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <limits>
#include <utility>

VaultReader::VaultReader(const nlohmann::json &data)
    : data(data)
{
}

const std::optional<VaultReadError> &VaultReader::error() const
{
    return this->diagnostic;
}

int VaultReader::fail(int code, std::string_view path, std::string_view reason)
{
    const auto id = this->data.is_object() && this->data.contains("id") ? this->data["id"].dump() : "<unknown>";
    this->diagnostic = VaultReadError{ id, std::string(path), std::string(reason) };
    return code;
}

int VaultReader::read_integer(std::string_view key, int &value, int minimum, int maximum)
{
    if (const auto err = info_set_integer(this->data[key], value, true, Range(minimum, maximum))) {
        return this->fail(err, fmt::format("$.{}", key), fmt::format(_("{}以上{}以下の整数が必要です", "expected an integer in [{}, {}]"), minimum, maximum));
    }
    return PARSE_ERROR_NONE;
}

int VaultReader::read_optional_integer(std::string_view key, int &value, int minimum, int maximum)
{
    if (!this->data.contains(key)) {
        return PARSE_ERROR_NONE;
    }
    return this->read_integer(key, value, minimum, maximum);
}

int VaultReader::read_flags(vault_type &vault)
{
    if (!this->data.contains("flags")) {
        return PARSE_ERROR_NONE;
    }
    const auto &flags = this->data["flags"];
    if (!flags.is_array()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, "$.flags", _("配列が必要です", "expected an array"));
    }
    for (size_t i = 0; i < flags.size(); ++i) {
        const auto path = fmt::format("$.flags[{}]", i);
        if (!flags[i].is_string()) {
            return this->fail(PARSE_ERROR_INVALID_TYPE, path, _("文字列が必要です", "expected a string"));
        }
        const auto &name = flags[i].get_ref<const std::string &>();
        if (!EnumClassFlagGroup<VaultFeatureType>::grab_one_flag(vault.flags, vault_flags, name)) {
            return this->fail(PARSE_ERROR_INVALID_FLAG, path, _("未知のVaultフラグです", "unknown vault flag"));
        }
    }
    return PARSE_ERROR_NONE;
}

int VaultReader::read_features(vault_type &vault)
{
    if (!this->data.contains("features")) {
        return PARSE_ERROR_NONE;
    }
    const auto &features = this->data["features"];
    if (!features.is_array()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, "$.features", _("配列が必要です", "expected an array"));
    }
    const auto &monraces = MonraceList::get_instance();
    for (size_t i = 0; i < features.size(); ++i) {
        const auto path = fmt::format("$.features[{}]", i);
        const auto &feature = features[i];
        if (!feature.is_object()) {
            return this->fail(PARSE_ERROR_INVALID_TYPE, path, _("オブジェクトが必要です", "expected an object"));
        }
        const auto symbol_it = feature.find("symbol");
        if (symbol_it == feature.end() || !symbol_it->is_string() || symbol_it->get_ref<const std::string &>().size() != 1) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, fmt::format("{}.symbol", path), _("1文字の記号が必要です", "expected a single-character symbol"));
        }
        const auto symbol = symbol_it->get_ref<const std::string &>().front();
        const auto feat_it = feature.find("feat");
        // 次の if は `||` の短絡で info_set_integer() を呼ばない経路を持ち、
        // MSVC がその先の return を読み切れず C4701 を出すため初期値を与える
        int feat = 0;
        if (feat_it == feature.end() || info_set_integer(*feat_it, feat, true, Range(0, std::numeric_limits<short>::max()))) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, fmt::format("{}.feat", path), _("地形IDが必要です", "expected a terrain id"));
        }
        auto appearance = feat;
        if (const auto ap_it = feature.find("appearance"); ap_it != feature.end()) {
            if (info_set_integer(*ap_it, appearance, true, Range(0, std::numeric_limits<short>::max()))) {
                return this->fail(PARSE_ERROR_INVALID_VALUE, fmt::format("{}.appearance", path), _("地形IDが必要です", "expected a terrain id"));
            }
        }
        vault.feature_list[symbol] = static_cast<FEAT_IDX>(feat);
        vault.feature_ap_list[symbol] = static_cast<FEAT_IDX>(appearance);
        if (const auto mon_it = feature.find("monster"); mon_it != feature.end()) {
            if (!mon_it->is_string() || mon_it->get_ref<const std::string &>().empty()) {
                return this->fail(PARSE_ERROR_INVALID_TYPE, fmt::format("{}.monster", path), _("空でない文字列が必要です", "expected a nonempty string"));
            }
            const auto &monster = mon_it->get_ref<const std::string &>();
            auto resolved = false;
            for (const auto &[monrace_id, monrace] : monraces) {
                if (monrace->tag == monster) {
                    vault.place_monster_list[symbol] = monrace_id;
                    resolved = true;
                    break;
                }
            }
            if (!resolved) {
                // Fall back to a numeric monrace id. An unresolvable token (neither a
                // known tag nor a number) is silently ignored, matching the former
                // text reader which left such MONSTER_ directives without effect.
                try {
                    vault.place_monster_list[symbol] = i2enum<MonraceId>(std::stoi(monster));
                } catch (const std::exception &) {
                    // leave place_monster_list untouched for this symbol
                }
            }
        }
    }
    return PARSE_ERROR_NONE;
}

int VaultReader::read()
{
    this->diagnostic.reset();
    if (!this->data.is_object()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, "$", _("オブジェクトが必要です", "expected an object"));
    }
    constexpr auto keys = std::to_array<std::string_view>({ "id", "name", "type", "rating", "height", "width", "layout", "min_depth", "max_depth", "rarity", "flags", "features" });
    constexpr auto required_count = 7; // id, name, type, rating, height, width, layout
    for (auto i = 0; i < required_count; ++i) {
        if (!this->data.contains(keys[i])) {
            return this->fail(PARSE_ERROR_TOO_FEW_ARGUMENTS, fmt::format("$.{}", keys[i]), _("必須項目がありません", "missing required field"));
        }
    }
    for (const auto &entry : this->data.items()) {
        if (std::find(keys.begin(), keys.end(), entry.key()) == keys.end()) {
            return this->fail(PARSE_ERROR_UNDEFINED_DIRECTIVE, fmt::format("$.{}", entry.key()), _("未知の項目です", "unknown field"));
        }
    }
    int id;
    if (const auto err = this->read_integer("id", id, 0, std::numeric_limits<short>::max())) {
        return err;
    }
    if (id <= error_idx) {
        return this->fail(PARSE_ERROR_NON_SEQUENTIAL_RECORDS, "$.id", _("IDは重複せず昇順に並べてください", "IDs must be unique and in increasing order"));
    }
    vault_type vault;
    vault.idx = static_cast<short>(id);
    if (!this->data["name"].is_string() || this->data["name"].get_ref<const std::string &>().empty()) {
        return this->fail(PARSE_ERROR_INVALID_TYPE, "$.name", _("空でない文字列が必要です", "expected a nonempty string"));
    }
    vault.name = utf8_to_local(this->data["name"].get_ref<const std::string &>());
    int type;
    if (const auto err = this->read_integer("type", type, 0, 255)) {
        return err;
    }
    if (type != 7 && type != 8 && type != 17 && type != 18 && type != 19 && type != 20) {
        return this->fail(PARSE_ERROR_INVALID_VALUE, "$.type", _("Vault種別は7、8、17、18、19、20のいずれかです", "vault type must be 7, 8, 17, 18, 19 or 20"));
    }
    vault.typ = static_cast<uint8_t>(type);
    if (const auto err = this->read_integer("rating", vault.rat, 0, std::numeric_limits<int>::max())) {
        return err;
    }
    if (const auto err = this->read_integer("height", vault.hgt, 1, MAX_HGT)) {
        return err;
    }
    if (const auto err = this->read_integer("width", vault.wid, 1, MAX_WID)) {
        return err;
    }
    if (const auto err = this->read_optional_integer("min_depth", vault.min_depth, 0, std::numeric_limits<int>::max())) {
        return err;
    }
    if (const auto err = this->read_optional_integer("max_depth", vault.max_depth, 0, std::numeric_limits<int>::max())) {
        return err;
    }
    if (const auto err = this->read_optional_integer("rarity", vault.rarity, 0, std::numeric_limits<int>::max())) {
        return err;
    }
    if (const auto err = this->read_flags(vault)) {
        return err;
    }
    if (const auto err = this->read_features(vault)) {
        return err;
    }
    const auto &layout = this->data["layout"];
    if (!layout.is_array() || layout.size() != static_cast<size_t>(vault.hgt)) {
        return this->fail(PARSE_ERROR_INVALID_VALUE, "$.layout", _("行数がheightと一致しません", "row count must equal height"));
    }
    for (size_t i = 0; i < layout.size(); ++i) {
        const auto path = fmt::format("$.layout[{}]", i);
        if (!layout[i].is_string()) {
            return this->fail(PARSE_ERROR_INVALID_TYPE, path, _("文字列が必要です", "expected a string"));
        }
        const auto &row = layout[i].get_ref<const std::string &>();
        if (row.size() != static_cast<size_t>(vault.wid)) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, path, _("行のバイト数がwidthと一致しません", "row byte length must equal width"));
        }
        if (std::any_of(row.begin(), row.end(), [](unsigned char c) { return c < 0x20 || c > 0x7e; })) {
            return this->fail(PARSE_ERROR_INVALID_VALUE, path, _("配置図には印字可能なASCII文字を指定してください", "layout must contain printable ASCII characters"));
        }
        vault.text += row;
    }
    // Publish only a fully validated record; failed reads leave global state intact.
    if (static_cast<size_t>(id) >= vaults_info.size()) {
        vaults_info.resize(id + 1);
    }
    vaults_info[id] = std::move(vault);
    error_idx = id;
    return PARSE_ERROR_NONE;
}
