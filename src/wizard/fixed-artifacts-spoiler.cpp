﻿#include "wizard/fixed-artifacts-spoiler.h"
#include "io/files-util.h"
#include "object/object-kind-hook.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/artifact-analyzer.h"
#include "wizard/spoiler-util.h"
#include <sstream>

/*!
 * @brief フラグ名称を出力する汎用関数
 * @param header ヘッダに出力するフラグ群の名前
 * @param descriptions フラグ名リスト
 * @param separator フラグ表示の区切り記号
 */
void spoiler_outlist(std::string_view header, const std::vector<std::string> &descriptions, char separator)
{
    if (descriptions.empty()) {
        return;
    }

    std::string line = spoiler_indent;
    if (!header.empty()) {
        line.append(header).append(" ");
    }

    for (size_t i = 0; i < descriptions.size(); i++) {
        auto elem = descriptions[i];
        if (i < descriptions.size() - 1) {
            elem.push_back(separator);
            elem.push_back(' ');
        }

        if (line.length() + elem.length() <= MAX_LINE_LEN) {
            line.append(elem);
            continue;
        }

        if (line.length() > 1 && line[line.length() - 1] == ' ' && line[line.length() - 2] == list_separator) {
            line[line.length() - 2] = '\0';
            fprintf(spoiler_file, "%s\n", line.data());
            line = spoiler_indent;
            line.append(elem);
        } else {
            fprintf(spoiler_file, "%s\n", line.data());
            line = "      ";
            line.append(elem);
        }
    }

    fprintf(spoiler_file, "%s\n", line.data());
}

/*!
 * @brief フラグ名称を出力する汎用関数
 * @param header ヘッダに出力するフラグ群の名前
 * @param list フラグ名リスト
 * @param separator フラグ表示の区切り記号
 * @todo 固定アーティファクトとランダムアーティファクトで共用、ここに置くべきかは要調整.
 * @todo いずれ上で定義したオーバーロードに吸収合併させてこれは消滅させる予定
 */
void spoiler_outlist(concptr header, concptr *list, char separator)
{
    char line[MAX_LINE_LEN + 20], buf[80];
    if (*list == nullptr) {
        return;
    }

    strcpy(line, spoiler_indent);
    if (header && (header[0])) {
        strcat(line, header);
        strcat(line, " ");
    }

    int buf_len;
    int line_len = strlen(line);
    while (true) {
        strcpy(buf, *list);
        buf_len = strlen(buf);
        if (list[1]) {
            sprintf(buf + buf_len, "%c ", separator);
            buf_len += 2;
        }

        if (line_len + buf_len <= MAX_LINE_LEN) {
            strcat(line, buf);
            line_len += buf_len;
        } else {
            if (line_len > 1 && line[line_len - 1] == ' ' && line[line_len - 2] == list_separator) {
                line[line_len - 2] = '\0';
                fprintf(spoiler_file, "%s\n", line);
                sprintf(line, "%s%s", spoiler_indent, buf);
            } else {
                fprintf(spoiler_file, "%s\n", line);
                concptr ident2 = "      ";
                sprintf(line, "%s%s", ident2, buf);
            }

            line_len = strlen(line);
        }

        if (!*++list) {
            break;
        }
    }

    fprintf(spoiler_file, "%s\n", line);
}

/*!
 * @brief アーティファクト情報を出力するためにダミー生成を行う /
 * Hack -- Create a "forged" artifact
 * @param o_ptr 一時生成先を保管するオブジェクト構造体
 * @param fixed_artifact_idx 生成するアーティファクトID
 * @return 生成が成功した場合TRUEを返す
 */
static bool make_fake_artifact(ItemEntity *o_ptr, FixedArtifactId fixed_artifact_idx)
{
    const auto &artifact = ArtifactsInfo::get_instance().get_artifact(fixed_artifact_idx);
    if (artifact.name.empty()) {
        return false;
    }

    const auto bi_id = lookup_baseitem_id(artifact.bi_key);
    if (bi_id == 0) {
        return false;
    }

    o_ptr->prep(bi_id);
    o_ptr->fixed_artifact_idx = fixed_artifact_idx;
    o_ptr->pval = artifact.pval;
    o_ptr->ac = artifact.ac;
    o_ptr->dd = artifact.dd;
    o_ptr->ds = artifact.ds;
    o_ptr->to_a = artifact.to_a;
    o_ptr->to_h = artifact.to_h;
    o_ptr->to_d = artifact.to_d;
    o_ptr->weight = artifact.weight;
    return true;
}

/*!
 * @brief アーティファクト一件をスポイラー出力する /
 * Create a spoiler file entry for an artifact
 * @param art_ptr アーティファクト情報をまとめた構造体の参照ポインタ
 */
static void spoiler_print_art(obj_desc_list *art_ptr)
{
    const auto *pval_ptr = &art_ptr->pval_info;
    fprintf(spoiler_file, "%s\n", art_ptr->description);
    if (!pval_ptr->pval_desc.empty()) {
        std::stringstream ss;
        ss << pval_ptr->pval_desc << _("の修正:", " to");
        spoiler_outlist(ss.str(), pval_ptr->pval_affects, item_separator);
    }

    spoiler_outlist(_("対:", "Slay"), art_ptr->slays, item_separator);
    spoiler_outlist(_("武器属性:", ""), art_ptr->brands, list_separator);
    spoiler_outlist(_("免疫:", "Immunity to"), art_ptr->immunities, item_separator);
    spoiler_outlist(_("耐性:", "Resist"), art_ptr->resistances, item_separator);
    spoiler_outlist(_("弱点:", "Vulnerable"), art_ptr->vulnerabilities, item_separator);
    spoiler_outlist(_("維持:", "Sustain"), art_ptr->sustains, item_separator);
    spoiler_outlist("", art_ptr->misc_magic, list_separator);

    if (art_ptr->addition[0]) {
        fprintf(spoiler_file, _("%s追加: %s\n", "%sAdditional %s\n"), spoiler_indent, art_ptr->addition);
    }

    if (art_ptr->activation) {
        fprintf(spoiler_file, _("%s発動: %s\n", "%sActivates for %s\n"), spoiler_indent, art_ptr->activation);
    }

    fprintf(spoiler_file, "%s%s\n\n", spoiler_indent, art_ptr->misc_desc);
}

/*!
 * @brief アーティファクト情報のスポイラー出力を行うメインルーチン /
 * Create a spoiler file for artifacts
 * @param fname 生成ファイル名
 */
SpoilerOutputResultType spoil_fixed_artifact(concptr fname)
{
    const auto &path = path_build(ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(path, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    spoiler_underline(std::string("Artifact Spoilers for Hengband Version ").append(get_version()).data());
    for (const auto &[tval_list, name] : group_artifact_list) {
        spoiler_blanklines(2);
        spoiler_underline(name);
        spoiler_blanklines(1);

        for (auto tval : tval_list) {
            for (const auto &[a_idx, artifact] : artifacts_info) {
                if (artifact.bi_key.tval() != tval) {
                    continue;
                }

                ItemEntity item;
                if (!make_fake_artifact(&item, a_idx)) {
                    continue;
                }

                PlayerType dummy;
                obj_desc_list artifact_descriptions;
                object_analyze(&dummy, &item, &artifact_descriptions);
                spoiler_print_art(&artifact_descriptions);
            }
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
