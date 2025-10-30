#include "alliance/alliance-nibelung.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "io/write-diary.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object/object-kind-hook.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-status/player-energy.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-crusade.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief ニーベルングの王国アライアンスの印象ポイント計算
 * @param creature_ptr プレイヤー情報
 * @return 印象ポイント
 * @details ドワーフ系種族や鍛冶関連要素を重視する
 */
int AllianceNibelung::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int point = 0;

    // 種族ボーナス
    switch (creature_ptr->prace) {
    case PlayerRaceType::DWARF:
        point += 50; // ドワーフは大幅ボーナス
        break;
    case PlayerRaceType::GNOME:
        point += 15; // ノームも親近感
        break;
    case PlayerRaceType::KOBOLD:
        point += 10; // コボルドも地下種族として少し優遇
        break;
    case PlayerRaceType::ELF:
    case PlayerRaceType::HIGH_ELF:
        point -= 30; // エルフ系は敵対的
        break;
    case PlayerRaceType::DARK_ELF:
        point -= 15; // ダークエルフも若干敵対的
        break;
    default:
        break;
    }

    // 職業ボーナス
    switch (creature_ptr->pclass) {
    case PlayerClassType::SMITH:
        point += 35; // 鍛冶師は高評価
        break;
    case PlayerClassType::WARRIOR:
        point += 20; // 戦士も好感
        break;
    case PlayerClassType::PALADIN:
        point += 15; // パラディンも評価
        break;
    case PlayerClassType::MAGIC_EATER:
        point -= 20; // 魔法を食べる者は嫌悪
        break;
    case PlayerClassType::SORCERER:
    case PlayerClassType::HIGH_MAGE:
        point -= 15; // 魔法使い系は若干嫌悪
        break;
    default:
        break;
    }

    // ステータスボーナス
    point += (creature_ptr->stat_index[A_STR] - 10) * 2; // 筋力重視
    point += (creature_ptr->stat_index[A_CON] - 10) * 2; // 耐久力重視
    point += (creature_ptr->stat_index[A_DEX] - 10) * 1; // 器用さも評価
    point -= (creature_ptr->stat_index[A_CHR] - 10) * 1; // 魅力はそれほど重要視しない

    // 性格ボーナス
    if (creature_ptr->ppersonality == PERSONALITY_ORDINARY) {
        point += 10; // 普通の性格は安定感で評価
    }
    if (creature_ptr->ppersonality == PERSONALITY_SHREWD) {
        point += 15; // 抜け目ない性格は商売上手で評価
    }
    if (creature_ptr->ppersonality == PERSONALITY_PATIENT) {
        point += 20; // 我慢強い性格は鍛冶に向く
    }
    if (creature_ptr->ppersonality == PERSONALITY_MIGHTY) {
        point += 15; // 豪快な性格も評価
    }

    // レベルボーナス
    point += creature_ptr->lev / 3;

    // ニーベルング族のメンバーを殺害した場合の減点
    const auto &monrace_list = MonraceList::get_instance();

    // ユニークモンスター
    if (monrace_list.get_monrace(MonraceId::MIME).r_pkills > 0) {
        point -= 240; // ニーベルング族の『ミーメ』を殺害 (レベル24 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::HAGEN).r_pkills > 0) {
        point -= 240; // アルベリヒの息子『ハーゲン』を殺害 (レベル24 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::ALBERICH).r_pkills > 0) {
        point -= 270; // ニーベルング族の王『アルベリヒ』を殺害 (レベル27 × 10)
    }

    // 一般モンスター（複数撃破の可能性があるため、撃破数に応じて減点）
    point -= monrace_list.get_monrace(MonraceId::NIBELUNG).r_pkills * 6; // ニーベルングを殺害 (レベル6 × 1 per kill)
    point -= monrace_list.get_monrace(MonraceId::NIBELUNG_ASSASSIN).r_pkills * 19; // ニーベルングの暗殺者を殺害 (レベル19 × 1 per kill)

    return point;
}

/*!
 * @brief ニーベルングの王国アライアンスの制裁処理
 * @param player_ptr プレイヤー情報
 * @details 段階的に制裁が厳しくなる
 */
void AllianceNibelung::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    /*
    auto impression = this->calcImpressionPoint(&player_ptr);
    if (impression >= -50) {
        // 軽微な制裁：工房の煙で視界を妨害
        msg_print("地下深くから黒い煙が立ち上り、あなたの視界を曇らせた！");
        (void)BadStatusSetter(&player_ptr).set_blindness(randint1(20) + 20);
        return;
    }

    if (impression >= -100) {
        // 中程度の制裁：鍛冶の槌音で混乱
        msg_print("遠くから響く無数の槌音があなたの精神を乱した！");
        (void)BadStatusSetter(&player_ptr).set_confusion(randint1(30) + 30);

        // ドワーフの戦士を召喚
        for (int i = 0; i < randint1(3) + 1; i++) {
            MONSTER_IDX m_idx = summon_specific(&player_ptr, 0, player_ptr.y, player_ptr.x,
                player_ptr.current_floor_ptr->dun_level + 10,
                SUMMON_DWARF, PM_FORCE_PET | PM_ALLOW_GROUP);
            if (m_idx) {
                MonsterEntity *m_ptr = &player_ptr.current_floor_ptr->m_list[m_idx];
                set_monster_hostile(m_ptr);
                msg_print("ニーベルングの戦士があなたを討伐しにやってきた！");
            }
        }
        return;
    }

    if (impression >= -150) {
        // 重い制裁：装備品の劣化
        msg_print("あなたの装備品が錆び付き、劣化していく！");

        // ランダムな装備品を劣化させる
        for (int i = 0; i < 6; i++) {
            int slot = randint0(INVEN_TOTAL - INVEN_MAIN_HAND) + INVEN_MAIN_HAND;
            ItemEntity *o_ptr = &player_ptr.inventory_list[slot];

            if (o_ptr->is_valid() && one_in_(3)) {
                if (o_ptr->to_h > 0) {
                    o_ptr->to_h--;
                    player_ptr.update |= PU_BONUS;
                }
                if (o_ptr->to_d > 0) {
                    o_ptr->to_d--;
                    player_ptr.update |= PU_BONUS;
                }
                if (o_ptr->to_a > 0) {
                    o_ptr->to_a--;
                    player_ptr.update |= PU_BONUS;
                }
            }
        }

        // より強力なドワーフ軍団を召喚
        for (int i = 0; i < randint1(4) + 2; i++) {
            MONSTER_IDX m_idx = summon_specific(&player_ptr, 0, player_ptr.y, player_ptr.x,
                player_ptr.current_floor_ptr->dun_level + 20,
                SUMMON_DWARF, PM_FORCE_PET | PM_ALLOW_GROUP);
            if (m_idx) {
                MonsterEntity *m_ptr = &player_ptr.current_floor_ptr->m_list[m_idx];
                set_monster_hostile(m_ptr);
            }
        }
        return;
    }

    // 最も重い制裁：ニーベルングの王の怒り
    msg_print("ニーベルングの王の怒りがあなたを襲う！");
    msg_print("地下王国の全軍があなたを包囲した！");

    // 強力な地属性攻撃
    project(&player_ptr, 0, 8, player_ptr.y, player_ptr.x,
        player_ptr.lev * 4, AttributeType::SHARDS,
        PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID);

    // 大量のドワーフ軍団召喚
    for (int i = 0; i < randint1(6) + 4; i++) {
        MONSTER_IDX m_idx = summon_specific(&player_ptr, 0, player_ptr.y, player_ptr.x,
            player_ptr.current_floor_ptr->dun_level + 30,
            SUMMON_DWARF, PM_FORCE_PET | PM_ALLOW_GROUP);
        if (m_idx) {
            MonsterEntity *m_ptr = &player_ptr.current_floor_ptr->m_list[m_idx];
            set_monster_hostile(m_ptr);
        }
    }

    // 装備品の大幅劣化
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr = &player_ptr.inventory_list[i];
        if (o_ptr->is_valid()) {
            if (o_ptr->to_h > -5)
                o_ptr->to_h -= randint1(3);
            if (o_ptr->to_d > -5)
                o_ptr->to_d -= randint1(3);
            if (o_ptr->to_a > -5)
                o_ptr->to_a -= randint1(3);
        }
    }

    player_ptr.update |= PU_BONUS;
    player_ptr.redraw |= PR_EQUIPPY;

    // 状態異常の重ね掛け
    (void)BadStatusSetter(&player_ptr).set_stun(randint1(50) + 50);
    (void)BadStatusSetter(&player_ptr).set_cut(randint1(100) + 100);
    */
}

/*!
 * @brief ニーベルングの王国アライアンスの壊滅判定
 * @return 壊滅しているかどうか
 */
bool AllianceNibelung::isAnnihilated()
{
    return false;
}
