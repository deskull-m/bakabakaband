/*!
 * @brief アライアンス情報の表示を行う。
 * @date 2021/06/21
 * @author Deskull
 */

#include "alliance/alliance.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "system/monster-race-info.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include <sstream>
#include <string>

void do_cmd_knowledge_alliance(PlayerType *player_ptr, bool detail)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    const auto &monraces = MonraceList::get_instance();
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    if (!detail) {
        fprintf(fff, "状態 アライアンス名                 印象値     勢力指数\n");
    }

    for (auto a : alliance_list) {
        if (a.second->id == AllianceType::NONE) {
            continue;
        }
        std::stringstream st;
        st << a.second->calcCurrentPower();

        if (detail) {
            fprintf(fff, _("%-30s/ あなたへの印象値: %+5d \n", "%-30s/ Impression to you: %+5d \n"), a.second->name.c_str(), a.second->calcImpressionPoint(player_ptr));
            fprintf(fff, _("  勢力指数: %s \n", "  Power Value: %s \n"), st.str().c_str());
            fprintf(fff, _("  (%s) \n", "  (%s) \n"), (a.second->isAnnihilated() ? _("壊滅", "Annihilated") : _("健在", "Alive")));

            if (!a.second->isAnnihilated()) {
                fprintf(fff, _("所属戦力--\n", "Affiliation strength--\n"));
            } else {
                fprintf(fff, _("残存戦力--\n", "Remaining strength--\n"));
            }

            for (auto &[r_idx, r_ref] : monraces_info) {
                if (r_ref.alliance_idx == a.second->id) {
                    fprintf(fff, _("%s  %-40s レベル %3d 評価値 %9d", "%s  %-40s LEVEL %3d POW %9d"), r_ref.kind_flags.has(MonsterKindType::UNIQUE) ? "[U]" : "---", r_ref.name.data(), r_ref.level, monraces.get_monrace(r_idx).calc_power());
                    if (r_ref.kind_flags.has_not(MonsterKindType::UNIQUE)) {
                        if (r_ref.mob_num > 0) {
                            fprintf(fff, "x %d\n", r_ref.mob_num);
                        } else {
                            if (r_ref.max_num > 0) {
                                fprintf(fff, _(" 全滅\n", " Wiped\n"));
                            } else {
                                fprintf(fff, _(" ----\n", " ----\n"));
                            }
                        }
                    } else {
                        if (r_ref.mob_num > 0) {
                            fprintf(fff, _(" 生存\n", " Survived\n"));
                        } else {
                            fprintf(fff, _(" 死亡\n", " Dead\n"));
                        }
                    }
                }
            }
            fprintf(fff, "\n\n");
        } else {
            fprintf(fff, "%s %-30s %+6d %12s\n", (a.second->isAnnihilated() ? _("壊滅", "Annihilated") : _("健在", "Alive")), a.second->name.c_str(), a.second->calcImpressionPoint(player_ptr), st.str().c_str());
        }
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("各アライアンス情報", "Information of all alliances"));
    fd_kill(file_name);
}
