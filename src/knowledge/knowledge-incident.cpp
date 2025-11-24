/*!
 * @brief これまでの行いを表示する
 * @date 2021/06/21
 * @author Deskull
 */

#include "alliance/alliance.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-kind-type-name.h"
#include "monster-race/race-kind-flags.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <string>
#include <vector>

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

        // 脱糞させた回数を表示
        if (player_ptr->incident_tree.count("ATTACK_EXE_COUNT/DEFECATION")) {
            fprintf(fff, _("    うち%d回モンスターを脱糞させた。\n", "    of which %d times caused monsters to defecate.\n"), player_ptr->incident_tree["ATTACK_EXE_COUNT/DEFECATION"]);
        }
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

    // 店舗に入った回数を表示
    if (player_ptr->incident_tree.count("STORE/ENTER")) {
        fprintf(fff, _("\nあなたはこれまで%d回店舗に入った。\n", "\nYou have entered stores %d times.\n"), player_ptr->incident_tree["STORE/ENTER"]);

        // 各店舗への入場回数を表示
        static const struct {
            const char *tag;
            const char *name;
        } store_types[] = {
            { "GENERAL", _("雑貨屋", "General Store") },
            { "ARMOURY", _("防具屋", "Armoury") },
            { "WEAPON", _("武器屋", "Weapon Smiths") },
            { "TEMPLE", _("寺院", "Temple") },
            { "ALCHEMIST", _("錬金術の店", "Alchemy Shop") },
            { "MAGIC", _("魔道具屋", "Magic Shop") },
            { "BLACK", _("ブラック・マーケット", "Black Market") },
            { "HOME", _("我が家", "Home") },
            { "BOOK", _("書店", "Book Store") },
            { "MUSEUM", _("博物館", "Museum") },
            { "MELINLAITO", _("メリンライトの薬品庫", "Melinlaito's Apothecary") },
            { "HUNAHYANDA", _("フナヒヤンダの武器蔵", "Hunahyanda's Armory") },
        };

        for (const auto &store : store_types) {
            const std::string key = "STORE/ENTER/" + std::string(store.tag);
            if (player_ptr->incident_tree.count(key)) {
                fprintf(fff, _("    %s: %d回\n", "    %s: %d times\n"), store.name, player_ptr->incident_tree[key]);
            }
        }
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
    if (player_ptr->incident_tree.count("OPEN_DOOR")) {
        fprintf(fff, _("あなたはこれまで%d回扉を開いた。\n", "You have opened door %d times.\n"), player_ptr->incident_tree["OPEN_DOOR"]);
    }
    if (player_ptr->incident_tree.count("CLOSE_DOOR")) {
        fprintf(fff, _("あなたはこれまで%d回扉を閉じた。\n", "You have closed door %d times.\n"), player_ptr->incident_tree["CLOSE_DOOR"]);
    }
    if (player_ptr->incident_tree.count("TUNNEL")) {
        fprintf(fff, _("あなたはこれまで%d回掘削を行った。\n", "You have tunneled %d times.\n"), player_ptr->incident_tree["TUNNEL"]);
    }
    if (player_ptr->incident_tree.count("REST")) {
        fprintf(fff, _("あなたはこれまで%d回休憩を取った。\n", "You have rested %d times.\n"), player_ptr->incident_tree["REST"]);
    }
    if (player_ptr->incident_tree.count("CAST_SPELL")) {
        fprintf(fff, _("あなたはこれまで%d回魔法を唱えた。\n", "You have cast spell %d times.\n"), player_ptr->incident_tree["CAST_SPELL"]);
    }
    if (player_ptr->incident_tree.count("USE_MIND")) {
        fprintf(fff, _("あなたはこれまで%d回特殊能力を使用した。\n", "You have used mind power %d times.\n"), player_ptr->incident_tree["USE_MIND"]);
    }
    if (player_ptr->incident_tree.count("USE_HISSATSU")) {
        fprintf(fff, _("あなたはこれまで%d回必殺技を使用した。\n", "You have used hissatsu %d times.\n"), player_ptr->incident_tree["USE_HISSATSU"]);
    }
    if (player_ptr->incident_tree.count("USE_RACIAL")) {
        fprintf(fff, _("あなたはこれまで%d回種族能力を使用した。\n", "You have used racial power %d times.\n"), player_ptr->incident_tree["USE_RACIAL"]);
    }
    if (player_ptr->incident_tree.count("DEFECATE")) {
        fprintf(fff, _("あなたはこれまで%d回脱糞した。\n", "You have defecated %d times.\n"), player_ptr->incident_tree["DEFECATE"]);
    }
    if (player_ptr->incident_tree.count("HEADBUTT")) {
        fprintf(fff, _("あなたはこれまで%d回頭突きした。\n", "You have headbutted %d times.\n"), player_ptr->incident_tree["HEADBUTT"]);
    }
    if (player_ptr->incident_tree.count("HIT_BY_MONSTER")) {
        fprintf(fff, _("あなたはこれまで%d回モンスターの打撃を受けた。\n", "You have been hit by monsters %d times.\n"), player_ptr->incident_tree["HIT_BY_MONSTER"]);

        // Display hits by method
        for (int i = 0; i < static_cast<int>(RaceBlowMethodType::MAX); i++) {
            const auto method = static_cast<RaceBlowMethodType>(i);
            if (method == RaceBlowMethodType::NONE || method == RaceBlowMethodType::MAX) {
                continue;
            }
            const std::string tag = get_blow_method_tag(method);
            const std::string key = "HIT_BY_MONSTER/METHOD/" + tag;
            if (player_ptr->incident_tree.count(key)) {
                const std::string name = get_blow_method_name(method);
                fprintf(fff, _("    %d回 %s\n", "    %d times by %s\n"),
#ifdef JP
                    player_ptr->incident_tree[key], name.c_str()
#else
                    player_ptr->incident_tree[key], tag.c_str()
#endif
                );
            }
        }

        // Display hits by effect
        for (int i = 0; i < static_cast<int>(RaceBlowEffectType::MAX); i++) {
            const auto effect = static_cast<RaceBlowEffectType>(i);
            if (effect == RaceBlowEffectType::NONE || effect == RaceBlowEffectType::MAX) {
                continue;
            }
            const std::string tag = get_blow_effect_tag(effect);
            const std::string key = "HIT_BY_MONSTER/EFFECT/" + tag;
            if (player_ptr->incident_tree.count(key)) {
                const std::string name = get_blow_effect_name(effect);
                fprintf(fff, _("    %d回 %s\n", "    %d times by %s\n"),
#ifdef JP
                    player_ptr->incident_tree[key], name.c_str()
#else
                    player_ptr->incident_tree[key], tag.c_str()
#endif
                );
            }
        }
    }

    // 死亡回数を表示
    if (player_ptr->incident_tree.count("DEAD")) {
        fprintf(fff, _("\nあなたはこれまで%d回死亡した。\n", "\nYou have died %d times.\n"), player_ptr->incident_tree["DEAD"]);

        // 死因となったモンスターを表示
        const auto &monraces = MonraceList::get_instance();
        std::vector<std::pair<int, int>> death_list; // (MonraceId, count)のペア

        for (const auto &entry : player_ptr->incident_tree) {
            if (entry.first.find("DEAD/") == 0 && entry.first.length() > 5) {
                try {
                    int monrace_id = std::stoi(entry.first.substr(5));
                    death_list.push_back({ monrace_id, entry.second });
                } catch (...) {
                    // パース失敗は無視
                }
            }
        }

        if (!death_list.empty()) {
            // 死亡回数でソート（降順）
            std::sort(death_list.begin(), death_list.end(),
                [](const auto &a, const auto &b) { return a.second > b.second; });

            fprintf(fff, _("    死因:\n", "    Killed by:\n"));
            for (const auto &[monrace_id_int, count] : death_list) {
                const auto monrace_id = i2enum<MonraceId>(monrace_id_int);
                if (MonraceList::is_valid(monrace_id)) {
                    const auto &monrace = monraces.get_monrace(monrace_id);
                    fprintf(fff, _("        %s: %d回\n", "        %s: %d times\n"),
                        monrace.name.data(), count);
                }
            }
        }
    }

    angband_fclose(fff);

    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("これまでの出来事", "Incidents"));
    fd_kill(file_name);
}
