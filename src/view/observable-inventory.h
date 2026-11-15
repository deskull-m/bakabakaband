#pragma once

/*!
 * @file observable-inventory.h
 * @brief プレイヤーの持ち物・装備・メッセージの表示用の取り出し (端末以外の UI 用)
 * @details
 * 端末版のサブウィンドウ (持ち物・装備・メッセージ) と同じ内容を、文字の格子ではなく
 * 構造化したデータとして取り出す。名前・色・重さは端末版の一覧と同じ規則で決める
 * (describe_flavor() / tval_to_attr / 再充填中は暗い灰色) ので、端末版より多くの情報は含まない。
 *
 * 文字列は内部の文字コード (日本語版は EUC-JP / SJIS) のまま返す。外部へ渡す側で変換すること。
 */

#include <cstdint>
#include <string>
#include <vector>

class CreatureEntity;

/*!
 * @brief 持ち物・装備の 1 品
 */
struct ObservedItem {
    int slot = 0; //!< 持ち物の添字 (inventory_slot_type)
    char label = 0; //!< 選ぶときのラベル (a, b, ...)
    std::string name; //!< 端末版の一覧と同じ名前 (describe_flavor())
    int count = 0;
    int weight = 0; //!< 合計の重さ (1/10 ポンド単位)
    std::string weight_text; //!< 端末版と同じ重さの表記 ("  3.0 kg" など)
    uint8_t color = 0; //!< 名前の色 (tval_to_attr。再充填中は暗い灰色)
    uint8_t symbol_color = 0; //!< アイテムのシンボルの色
    char symbol_char = 0; //!< アイテムのシンボルの文字
    int tval = 0;
    bool recharging = false; //!< 再充填中 (timeout が残っている)

    bool operator==(const ObservedItem &) const = default;
};

/*!
 * @brief 装備の 1 部位
 */
struct ObservedEquipmentSlot {
    int slot = 0; //!< 持ち物の添字 (INVEN_MAIN_HAND 以降)
    char label = 0;
    std::string slot_name; //!< 部位の名前 (mention_use())
    bool can_equip = true; //!< 体構造で装備できる部位か
    bool has_item = false;
    ObservedItem item; //!< has_item のときだけ有効

    bool operator==(const ObservedEquipmentSlot &) const = default;
};

/*!
 * @brief 持ち物と装備の全体
 */
struct ObservedInventory {
    std::vector<ObservedItem> pack; //!< 持ち物 (空き枠は含まない)
    std::vector<ObservedEquipmentSlot> equipment; //!< 装備の全部位 (空きも含む)
    int total_weight = 0; //!< 持ち物と装備の合計 (1/10 ポンド単位)
    int weight_limit = 0; //!< 重量の上限 (これを超えると遅くなる。1/10 ポンド単位)

    bool operator==(const ObservedInventory &) const = default;
};

ObservedInventory observe_inventory(CreatureEntity &creature);

/// 直近のメッセージを古い順に返す (最大 count 件。message_str() と同じく連続した同じ文は "(xN)" でまとまる)
std::vector<std::string> observe_recent_messages(int count);
