/*!
 * @brief これまでの行いを表示する
 * @date 2021/06/21
 * @author Deskull
 */

#include "alliance/alliance.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "monster-race/monster-kind-type-name.h"
#include "monster-race/race-kind-flags.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include <string>

void do_cmd_knowledge_incident(PlayerType *player_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    if (player_ptr->incident_tree.count("WALK")) {
        fprintf(fff, _("あなたはこれまで%d歩進んだ。\n", "You walked %d steps\n"), player_ptr->incident_tree["WALK"]);

        // 8方向の歩数を表示
        static constexpr struct {
            const char *key;
            const char *name_jp;
            const char *name_en;
        } directions[8] = {
            { "WALK/N", "北", "north" },
            { "WALK/NE", "北東", "northeast" },
            { "WALK/E", "東", "east" },
            { "WALK/SE", "南東", "southeast" },
            { "WALK/S", "南", "south" },
            { "WALK/SW", "南西", "southwest" },
            { "WALK/W", "西", "west" },
            { "WALK/NW", "北西", "northwest" },
        };

        for (const auto &dir : directions) {
            if (player_ptr->incident_tree.count(dir.key)) {
                fprintf(fff, _("    %sに%d歩\n", "    %d steps to %s\n"),
#ifdef JP
                    dir.name_jp, player_ptr->incident_tree[dir.key]
#else
                    player_ptr->incident_tree[dir.key], dir.name_en
#endif
                );
            }
        }
    }

    // ダンジョンに入った回数を表示
    if (player_ptr->incident_tree.count("ENTER_DUNGEON")) {
        fprintf(fff, _("\nあなたはこれまで%d回ダンジョンに入った。\n", "\nYou have entered dungeons %d times.\n"), player_ptr->incident_tree["ENTER_DUNGEON"]);

        // 各ダンジョンへの入場回数を表示
        const auto &dungeons = DungeonList::get_instance();
        for (const auto &entry : player_ptr->incident_tree) {
            if (entry.first.find("ENTER_DUNGEON/") == 0 && entry.first.length() > 14) {
                const char *dungeon_tag = entry.first.c_str() + 14; // "ENTER_DUNGEON/" の後

                // タグからダンジョン名を検索
                std::string dungeon_name = _("不明なダンジョン", "Unknown Dungeon");
                for (const auto &[dungeon_id, dungeon_ptr] : dungeons) {
                    if (dungeon_ptr->tag == dungeon_tag) {
                        dungeon_name = dungeon_ptr->name;
                        break;
                    }
                }

                fprintf(fff, _("    %s: %d回\n", "    %s: %d times\n"), dungeon_name.c_str(), entry.second);
            }
        }
    }

    if (player_ptr->incident_tree.count("TRAPPED")) {
        fprintf(fff, _("あなたはこれまで%d回罠にかかった。\n", "You have been trapped %d times.\n"), player_ptr->incident_tree["TRAPPED"]);
    }
    if (player_ptr->incident_tree.count("ATTACK_ACT_COUNT")) {
        fprintf(fff, _("あなたはこれまで%d回攻撃動作を行い、%d回攻撃を実行した。\n", "You have prepared %d attack action and execute %d attacks. \n"),
            player_ptr->incident_tree["ATTACK_ACT_COUNT"], player_ptr->incident_tree["ATTACK_EXE_COUNT"]);
    }
    if (player_ptr->incident_tree.count("SHOOT")) {
        fprintf(fff, _("あなたはこれまで%d回射撃を行った。\n", "You have fired %d times. \n"), player_ptr->incident_tree["SHOOT"]);
    }
    if (player_ptr->incident_tree.count("THROW")) {
        fprintf(fff, _("あなたはこれまで%d回投擲を行った。\n", "You have thrown %d times. \n"), player_ptr->incident_tree["THROW"]);
    }
    if (player_ptr->incident_tree.count("READ_SCROLL")) {
        fprintf(fff, _("あなたはこれまで%d回巻物を読んだ。\n", "You have read scroll %d times. \n"), player_ptr->incident_tree["READ_SCROLL"]);
    }
    if (player_ptr->incident_tree.count("ZAP_STAFF")) {
        fprintf(fff, _("あなたはこれまで%d回魔法の杖を振るった。\n", "You have zapped magic staff %d times. \n"), player_ptr->incident_tree["ZAP_STAFF"]);
    }
    if (player_ptr->incident_tree.count("ZAP_WAND")) {
        fprintf(fff, _("あなたはこれまで%d回魔法棒を振るった。\n", "You have zapped magic wand %d times. \n"), player_ptr->incident_tree["ZAP_WAND"]);
    }
    if (player_ptr->incident_tree.count("ZAP_ROD")) {
        fprintf(fff, _("あなたはこれまで%d回ロッドを振るった。\n", "You have zapped magic rod %d times. \n"), player_ptr->incident_tree["ZAP_ROD"]);
    }
    if (player_ptr->incident_tree.count("STORE_BUY")) {
        fprintf(fff, _("あなたはこれまで%d回アイテムを購入した。\n", "You have buy item %d times. \n"), player_ptr->incident_tree["STORE_BUY"]);
    }
    if (player_ptr->incident_tree.count("STORE_SELL")) {
        fprintf(fff, _("あなたはこれまで%d回アイテムを売却した。\n", "You have sold item %d times. \n"), player_ptr->incident_tree["STORE_SELL"]);
    }
    if (player_ptr->incident_tree.count("STAY_INN")) {
        fprintf(fff, _("あなたはこれまで%d回宿屋に宿泊した。\n", "You have stayed inn %d times. \n"), player_ptr->incident_tree["STAY_INN"]);
    }
    if (player_ptr->incident_tree.count("KILL")) {
        fprintf(fff, _("あなたはこれまで%d体のモンスターを倒した。\n", "You have killed %d monsters.\n"), player_ptr->incident_tree["KILL"]);

        // Display kills by MonsterKindType
        for (size_t i = 0; i < static_cast<size_t>(MonsterKindType::MAX); i++) {
            const auto kind = static_cast<MonsterKindType>(i);
            const std::string tag = get_monster_kind_type_tag(kind);
            const std::string key = "KILL/RACE/" + tag;
            if (player_ptr->incident_tree.count(key)) {
                const auto kind_name = get_monster_kind_type_name(kind);
                fprintf(fff, _("    %sを%d体殺した\n", "    killed %d %s\n"),
#ifdef JP
                    kind_name.c_str(), player_ptr->incident_tree[key]
#else
                    player_ptr->incident_tree[key], kind_name.c_str()
#endif
                );
            }
        }

        // Display kills by Alliance
        fprintf(fff, _("\n", "\n"));
        bool has_alliance_kills = false;
        for (const auto &entry : player_ptr->incident_tree) {
            if (entry.first.find("KILL/ALLIANCE/") == 0) {
                has_alliance_kills = true;
                break;
            }
        }

        if (has_alliance_kills) {
            for (const auto &[alliance_type, alliance_ptr] : alliance_list) {
                if (alliance_type == AllianceType::NONE) {
                    continue;
                }
                const std::string tag = get_alliance_type_tag(alliance_type);
                const std::string key = "KILL/ALLIANCE/" + tag;
                if (player_ptr->incident_tree.count(key)) {
                    fprintf(fff, _("    %sに属する者を%d体殺した\n", "    killed %d members of %s\n"),
#ifdef JP
                        alliance_ptr->name.c_str(), player_ptr->incident_tree[key]
#else
                        player_ptr->incident_tree[key], alliance_ptr->name.c_str()
#endif
                    );
                }
            }
        }
    }
    if (player_ptr->incident_tree.count("EAT")) {
        fprintf(fff, _("あなたはこれまで%d回食事を摂った。\n", "You have eaten %d times.\n"), player_ptr->incident_tree["EAT"]);
        if (player_ptr->incident_tree.count("EAT_FECES")) {
            fprintf(fff, _("    うち%d回は糞便や汚物を食った。\n", "    of which %d times ate feces and dirts\n"), player_ptr->incident_tree["EAT_FECES"]);
        }
        if (player_ptr->incident_tree.count("EAT_POISON")) {
            fprintf(fff, _("    うち%d回中毒症状を起こした。\n", "    of which %d times had addiction symptoms\n"), player_ptr->incident_tree["EAT_POISON"]);
        }
    }
    if (player_ptr->incident_tree.count("QUAFF")) {
        fprintf(fff, _("あなたはこれまで薬を%d回服用した。\n", "You have taken the drug %d times.\n"), player_ptr->incident_tree["QUAFF"]);
    }
    angband_fclose(fff);

    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("これまでの出来事", "Incidents"));
    fd_kill(file_name);
}
