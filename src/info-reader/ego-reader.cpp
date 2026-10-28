#include "info-reader/ego-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include <limits>
#include <nlohmann/json.hpp>

EgoReader::EgoReader(const nlohmann::json &ego_data)
    : ego_data(ego_data)
{
}

ParseErrorType EgoReader::read() const
{
    if (!this->ego_data.is_object()) {
        return ParseErrorType::INVALID_TYPE;
    }

    int id{};
    if (const auto err = info_set_integer(get_json_value(this->ego_data, "id"), id, true, Range(1, 32767)); !err) {
        return err.error();
    }
    if (egos_info.contains(i2enum<EgoType>(id))) {
        return ParseErrorType::NON_SEQUENTIAL_RECORDS;
    }

    EgoItemDefinition ego;
    ego.idx = i2enum<EgoType>(id);
    if (const auto err = info_set_string(get_json_value(this->ego_data, "name"), ego.name, true); !err) {
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->ego_data, "slot"), ego.slot, true, Range(0, 255)); !err) {
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->ego_data, "rating"), ego.rating, true, Range(0, std::numeric_limits<int>::max())); !err) {
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->ego_data, "level"), ego.level, true, Range(0, std::numeric_limits<DEPTH>::max())); !err) {
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->ego_data, "rarity"), ego.rarity, true, Range(0, 255)); !err) {
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->ego_data, "cost"), ego.cost, true, Range(0, std::numeric_limits<int>::max())); !err) {
        return err.error();
    }

    const auto &base = get_json_value(this->ego_data, "base_bonuses");
    if (!base.is_null()) {
        if (!base.is_object()) {
            return ParseErrorType::INVALID_TYPE;
        }
        if (const auto err = info_set_integer(get_json_value(base, "to_hit"), ego.base_to_h, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(base, "to_damage"), ego.base_to_d, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(base, "to_ac"), ego.base_to_a, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
    }

    const auto &maximum = get_json_value(this->ego_data, "maximum_bonuses");
    if (!maximum.is_null()) {
        if (!maximum.is_object()) {
            return ParseErrorType::INVALID_TYPE;
        }
        if (const auto err = info_set_integer(get_json_value(maximum, "to_hit"), ego.max_to_h, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(maximum, "to_damage"), ego.max_to_d, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(maximum, "to_ac"), ego.max_to_a, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(maximum, "pval"), ego.max_pval, true, Range(-32768, 32767)); !err) {
            return err.error();
        }
    }

    if (const auto err = this->set_activation(ego); err != ParseErrorType::NONE) {
        return err;
    }
    if (const auto err = this->set_flags(ego); err != ParseErrorType::NONE) {
        return err;
    }
    if (const auto err = this->set_extra_flags(ego); err != ParseErrorType::NONE) {
        return err;
    }

    egos_info.emplace(ego.idx, std::move(ego));
    return ParseErrorType::NONE;
}

bool EgoReader::grab_one_flag(EgoItemDefinition &ego, const nlohmann::json &flag) const
{
    if (!flag.is_string()) {
        return false;
    }
    const auto &name = flag.get_ref<const std::string &>();
    return TrFlags::grab_one_flag(ego.flags, baseitem_flags, name) ||
           EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(ego.gen_flags, baseitem_geneneration_flags, name);
}

bool EgoReader::grab_one_extra_flag(ego_generate_type &extra, const nlohmann::json &flag) const
{
    if (!flag.is_string()) {
        return false;
    }
    const auto &name = flag.get_ref<const std::string &>();
    if (const auto it = baseitem_flags.find(name); it != baseitem_flags.end()) {
        extra.tr_flags.push_back(it->second);
        return true;
    }
    if (const auto it = baseitem_geneneration_flags.find(name); it != baseitem_geneneration_flags.end()) {
        extra.trg_flags.push_back(it->second);
        return true;
    }
    return false;
}

ParseErrorType EgoReader::set_flags(EgoItemDefinition &ego) const
{
    const auto &flags = get_json_value(this->ego_data, "flags");
    if (!flags.is_array()) {
        return flags.is_null() ? ParseErrorType::NONE : ParseErrorType::INVALID_TYPE;
    }
    for (const auto &flag : flags) {
        if (!this->grab_one_flag(ego, flag)) {
            return flag.is_string() ? ParseErrorType::INVALID_FLAG : ParseErrorType::INVALID_TYPE;
        }
    }
    return ParseErrorType::NONE;
}

ParseErrorType EgoReader::set_extra_flags(EgoItemDefinition &ego) const
{
    const auto &extras = get_json_value(this->ego_data, "extra_flags");
    if (!extras.is_array()) {
        return extras.is_null() ? ParseErrorType::NONE : ParseErrorType::INVALID_TYPE;
    }
    for (const auto &extra_data : extras) {
        if (!extra_data.is_object()) {
            return ParseErrorType::INVALID_TYPE;
        }
        ego_generate_type extra;
        if (const auto err = info_set_integer(get_json_value(extra_data, "numerator"), extra.mul, true, Range(1, 32767)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(extra_data, "denominator"), extra.dev, true, Range(1, 32767)); !err) {
            return err.error();
        }
        const auto &flags = get_json_value(extra_data, "flags");
        if (!flags.is_array()) {
            return flags.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE;
        }
        if (flags.empty()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        for (const auto &flag : flags) {
            if (!this->grab_one_extra_flag(extra, flag)) {
                return flag.is_string() ? ParseErrorType::INVALID_FLAG : ParseErrorType::INVALID_TYPE;
            }
        }
        ego.xtra_flags.push_back(std::move(extra));
    }
    return ParseErrorType::NONE;
}

ParseErrorType EgoReader::set_activation(EgoItemDefinition &ego) const
{
    const auto &activation = get_json_value(this->ego_data, "activation");
    if (activation.is_null()) {
        return ParseErrorType::NONE;
    }
    if (!activation.is_string()) {
        return ParseErrorType::INVALID_TYPE;
    }
    ego.act_idx = grab_one_activation_flag(activation.get<std::string>());
    return ego.act_idx > RandomArtActType::NONE ? ParseErrorType::NONE : ParseErrorType::INVALID_FLAG;
}
