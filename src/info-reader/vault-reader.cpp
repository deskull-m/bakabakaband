#include "info-reader/vault-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "room/rooms-vault.h"
#include "util/string-processor.h"

/*!
 * @brief Vault定義 (VaultDefinitions)のパース関数
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_vaults_info(std::string_view buf, angband_header *)
{
    static vault_type *v_ptr = nullptr;
    const auto &tokens = str_split(buf, ':', false, 5);

    if (tokens[0] == "N") {
        // N:index:name
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        if (tokens[1].size() == 0 || tokens[2].size() == 0) {
            return PARSE_ERROR_GENERIC;
        }

        auto i = std::stoi(tokens[1]);
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }
        if (i >= static_cast<int>(vaults_info.size())) {
            vaults_info.resize(i + 1);
        }

        error_idx = i;
        v_ptr = &vaults_info[i];
        v_ptr->idx = static_cast<int16_t>(i);
        v_ptr->name = std::string(tokens[2]);
    } else if (!v_ptr) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    } else if (tokens[0] == "D") {
        // D:MapText
        if (tokens.size() < 2 || buf.length() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        v_ptr->text.append(buf.substr(2));
    } else if (tokens[0] == "X") {
        // X:type:rate:height:width
        if (tokens.size() < 5) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(v_ptr->typ, tokens[1]);
        info_set_value(v_ptr->rat, tokens[2]);
        info_set_value(v_ptr->hgt, tokens[3]);
        info_set_value(v_ptr->wid, tokens[4]);
    } else if (tokens[0] == "F") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        char c;
        FEAT_IDX feat_idx;
        if (tokens.size() >= 3) {
            c = tokens[1].c_str()[0];
            info_set_value(feat_idx, tokens[2]);
            v_ptr->feature_list[c] = feat_idx;
            v_ptr->feature_ap_list[c] = feat_idx;
        }
        if (tokens.size() >= 4) {
            info_set_value(feat_idx, tokens[3]);
            v_ptr->feature_ap_list[c] = feat_idx;
        }
        if (tokens.size() == 5) {
            const auto &flags = str_split(tokens[4], '|', true, 10);
            for (const auto &f : flags) {
                if (f.size() == 0) {
                    continue;
                }

                const auto &s_tokens = str_split(f, '_', false);
                if (s_tokens.size() == 2 && s_tokens[0] == "MONSTER") {
                    break;
                    /*
                    for (const auto &[r_idx, r_ref] : monraces_info) {
                        if (s_tokens[1] == r_ref.tag) {
                            v_ptr->place_monster_list[c] = r_idx;
                            break;
                        }
                    }
                    if (v_ptr->place_monster_list.count(c) == 0) {
                        info_set_value(v_ptr->place_monster_list[c], s_tokens[1]);
                    }
                    continue;
                    */
                }

                return PARSE_ERROR_INVALID_FLAG;
            }
        }

    } else if (tokens[0] == "T") {
        // T:traits
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true, 10);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto &s_tokens = str_split(f, '_', false);

            if (s_tokens.size() == 2 && s_tokens[0] == "MAXDEPTH") {
                info_set_value(v_ptr->max_depth, s_tokens[1]);
                continue;
            }

            if (s_tokens.size() == 2 && s_tokens[0] == "MINDEPTH") {
                info_set_value(v_ptr->min_depth, s_tokens[1]);
                continue;
            }

            if (s_tokens.size() == 2 && s_tokens[0] == "RARITY") {
                info_set_value(v_ptr->rarity, s_tokens[1]);
                continue;
            }

            return PARSE_ERROR_INVALID_FLAG;
        }
    } else {
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }
    return PARSE_ERROR_NONE;
}
