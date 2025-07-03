/*!
 * @brief これまでの行いを表示する
 * @date 2021/06/21
 * @author Deskull
 */

#include "core/show-file.h"
#include "io-dump/dump-util.h"
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

    if (player_ptr->incident.count(INCIDENT::WALK)) {
        fprintf(fff, _("あなたはこれまで%d歩進んだ。\n", "You walked %d steps\n"), player_ptr->incident[INCIDENT::WALK]);
    }
    if (player_ptr->incident.count(INCIDENT::LEAVE_FLOOR)) {
        fprintf(fff, _("あなたはこれまで%d回フロアを移動した。\n", "You moved %d floors\n"), player_ptr->incident[INCIDENT::LEAVE_FLOOR]);
    }
    if (player_ptr->incident.count(INCIDENT::TRAPPED)) {
        fprintf(fff, _("あなたはこれまで%d回罠にかかった。\n", "You have been trapped %d times.\n"), player_ptr->incident[INCIDENT::TRAPPED]);
    }
    if (player_ptr->incident.count(INCIDENT::ATTACK_ACT_COUNT)) {
        fprintf(fff, _("あなたはこれまで%d回攻撃動作を行い、%d回攻撃を実行した。\n", "You have prepared %d attack action and execute %d attacks. \n"),
            player_ptr->incident[INCIDENT::ATTACK_ACT_COUNT], player_ptr->incident[INCIDENT::ATTACK_EXE_COUNT]);
    }
    if (player_ptr->incident.count(INCIDENT::SHOOT)) {
        fprintf(fff, _("あなたはこれまで%d回射撃を行った。\n", "You have fired %d times. \n"), player_ptr->incident[INCIDENT::SHOOT]);
    }
    if (player_ptr->incident.count(INCIDENT::THROW)) {
        fprintf(fff, _("あなたはこれまで%d回投擲を行った。\n", "You have thrown %d times. \n"), player_ptr->incident[INCIDENT::THROW]);
    }
    if (player_ptr->incident.count(INCIDENT::READ_SCROLL)) {
        fprintf(fff, _("あなたはこれまで%d回巻物を読んだ。\n", "You have read scroll %d times. \n"), player_ptr->incident[INCIDENT::READ_SCROLL]);
    }
    if (player_ptr->incident.count(INCIDENT::ZAP_STAFF)) {
        fprintf(fff, _("あなたはこれまで%d回魔法の杖を振るった。\n", "You have zapped magic staff %d times. \n"), player_ptr->incident[INCIDENT::ZAP_STAFF]);
    }
    if (player_ptr->incident.count(INCIDENT::ZAP_WAND)) {
        fprintf(fff, _("あなたはこれまで%d回魔法棒を振るった。\n", "You have zapped magic wand %d times. \n"), player_ptr->incident[INCIDENT::ZAP_WAND]);
    }
    if (player_ptr->incident.count(INCIDENT::ZAP_ROD)) {
        fprintf(fff, _("あなたはこれまで%d回ロッドを振るった。\n", "You have zapped magic rod %d times. \n"), player_ptr->incident[INCIDENT::ZAP_ROD]);
    }
    if (player_ptr->incident.count(INCIDENT::STORE_BUY)) {
        fprintf(fff, _("あなたはこれまで%d回アイテムを購入した。\n", "You have buy item %d times. \n"), player_ptr->incident[INCIDENT::STORE_BUY]);
    }
    if (player_ptr->incident.count(INCIDENT::STORE_SELL)) {
        fprintf(fff, _("あなたはこれまで%d回アイテムを売却した。\n", "You have sold item %d times. \n"), player_ptr->incident[INCIDENT::STORE_SELL]);
    }
    if (player_ptr->incident.count(INCIDENT::STAY_INN)) {
        fprintf(fff, _("あなたはこれまで%d回宿屋に宿泊した。\n", "You have stayed inn %d times. \n"), player_ptr->incident[INCIDENT::STAY_INN]);
    }
    if (player_ptr->incident.count(INCIDENT::EAT)) {
        fprintf(fff, _("あなたはこれまで%d回食事を摂った。\n", "You have eaten %d times.\n"), player_ptr->incident[INCIDENT::EAT]);
        if (player_ptr->incident.count(INCIDENT::EAT_FECES)) {
            fprintf(fff, _("    うち%d回は糞便や汚物を食った。\n", "    of which %d times ate feces and dirts\n"), player_ptr->incident[INCIDENT::EAT_FECES]);
        }
        if (player_ptr->incident.count(INCIDENT::EAT_POISON)) {
            fprintf(fff, _("    うち%d回中毒症状を起こした。\n", "    of which %d times had addiction symptoms\n"), player_ptr->incident[INCIDENT::EAT_POISON]);
        }
    }
    if (player_ptr->incident.count(INCIDENT::QUAFF)) {
        fprintf(fff, _("あなたはこれまで薬を%d回服用した。\n", "You have taken the drug %d times.\n"), player_ptr->incident[INCIDENT::QUAFF]);
    }
    angband_fclose(fff);

    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("これまでの出来事", "Incidents"));
    fd_kill(file_name);
}
