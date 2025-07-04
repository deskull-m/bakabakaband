/*!
 * @file scene-table-monster.cpp
 * @brief モンスターの遭遇状況に応じたBGM設定処理実装
 */

#include "main/scene-table-monster.h"
#include "dungeon/quest.h"
#include "main/music-definitions-table.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"
#include <ctime>

struct scene_monster_info {
    MONSTER_IDX m_idx;
    MonraceDefinition *ap_r_ptr;
    GAME_TURN last_seen; //!< 最後に対象モンスター見たゲームターン
    uint32_t mute_until; //!< この時間に到達するまでモンスターBGMは設定しない
};

scene_monster_info scene_target_monster;

void clear_scene_target_monster()
{
    scene_target_monster.ap_r_ptr = nullptr;
}

static int get_game_turn()
{
    const auto turns = AngbandWorld::get_instance().game_turn;
    if (turns == InnerGameData::get_instance().get_game_turn_limit()) {
        return 0;
    }

    return turns;
}

/*!
 * @brief モンスターBGMの制限期間を設定する
 * @details 指定の時間が経過するまでモンスターBGMの再生を制限する
 * @param msec 制限する時間（秒）
 */
void set_temp_mute_scene_monster(int sec)
{
    scene_target_monster.mute_until = (uint32_t)time(nullptr) + sec;
}

/*!
 * @brief モンスターBGMの制限期間か判定する
 * @details ダンジョンターン数がscene_target_monster.mute_untilの値になるまで制限期間。
 * @return モンスターBGMの制限期間の場合trueを返す
 */
inline static bool can_mute_scene_monster()
{
    return scene_target_monster.mute_until > time(nullptr);
}

/*!
 * @brief モンスターの優先判定
 * @details ユニーク、あやしい影、未知のモンスター、レベルの高さ、モンスターIDで優先を付ける。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx1 モンスターA（新参）
 * @param m_idx2 モンスターB（現対象）
 * @retval true モンスターAが優先
 * @retval false モンスターBが優先
 */
static bool is_high_rate(PlayerType *player_ptr, MONSTER_IDX m_idx1, MONSTER_IDX m_idx2)
{
    // FIXME 視界内モンスターリストの比較関数と同じ処理
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster1 = floor.m_list[m_idx1];
    const auto &monster2 = floor.m_list[m_idx2];
    auto ap_r_ptr1 = &monster1.get_appearance_monrace();
    auto ap_r_ptr2 = &monster2.get_appearance_monrace();

    /* Unique monsters first */
    if (ap_r_ptr1->kind_flags.has(MonsterKindType::UNIQUE) != ap_r_ptr2->kind_flags.has(MonsterKindType::UNIQUE)) {
        return ap_r_ptr1->kind_flags.has(MonsterKindType::UNIQUE);
    }

    /* Shadowers first (あやしい影) */
    if (monster1.mflag2.has(MonsterConstantFlagType::KAGE) != monster2.mflag2.has(MonsterConstantFlagType::KAGE)) {
        return monster1.mflag2.has(MonsterConstantFlagType::KAGE);
    }

    /* Unknown monsters first */
    if ((ap_r_ptr1->r_tkills == 0) != (ap_r_ptr2->r_tkills == 0)) {
        return ap_r_ptr1->r_tkills == 0;
    }

    /* Higher level monsters first (if known) */
    if (ap_r_ptr1->r_tkills && ap_r_ptr2->r_tkills && ap_r_ptr1->level != ap_r_ptr2->level) {
        return ap_r_ptr1->level > ap_r_ptr2->level;
    }

    /* Sort by index if all conditions are same */
    return monster1.ap_r_idx > monster2.ap_r_idx;
}

/*!
 * @brief BGM対象モンスター更新処理
 * @details 現在の対象と対象候補が同一モンスターの場合、最後に見たゲームターン情報を更新する。
 * 対象候補が現在の対象よりも上位であれば対象を入れ替える。
 * ユニーク、あやしい影、未知のモンスター、レベルの高さ、モンスターIDで優先を付ける。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx BGM対象候補のモンスター
 */
static void update_target_monster(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    if (scene_target_monster.ap_r_ptr && (scene_target_monster.m_idx == m_idx)) {
        // 同一モンスター。最後に見たゲームターンを更新。
        scene_target_monster.last_seen = get_game_turn();
    } else {
        bool do_dwap = false;
        if (!scene_target_monster.ap_r_ptr) {
            // 空席
            do_dwap = true;
        } else if (is_high_rate(player_ptr, m_idx, scene_target_monster.m_idx)) {
            // 入れ替え
            do_dwap = true;
        }

        if (do_dwap) {
            const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
            auto *ap_r_ptr = &monster.get_appearance_monrace();
            scene_target_monster.m_idx = m_idx;
            scene_target_monster.ap_r_ptr = ap_r_ptr;
            scene_target_monster.last_seen = get_game_turn();
        }
    }
}

using scene_monster_func = bool (*)(PlayerType *player_ptr, scene_type *value);

static bool scene_monster(PlayerType *player_ptr, scene_type *value)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[scene_target_monster.m_idx];

    if (monster.mflag2.has(MonsterConstantFlagType::KAGE)) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_SHADOWER;
        return true;
    } else {
        value->type = TERM_XTRA_MUSIC_MONSTER;
        value->val = enum2i(monster.ap_r_idx);
        return true;
    }
}

static bool scene_unique(PlayerType *player_ptr, scene_type *value)
{
    (void)player_ptr;

    if (scene_target_monster.ap_r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_UNIQUE;
        return true;
    }

    return false;
}

static bool scene_unknown(PlayerType *player_ptr, scene_type *value)
{
    (void)player_ptr;
    if (scene_target_monster.ap_r_ptr->r_tkills == 0) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_UNKNOWN_MONSTER;
        return true;
    }

    return false;
}

static bool scene_high_level(PlayerType *player_ptr, scene_type *value)
{
    if (scene_target_monster.ap_r_ptr->r_tkills > 0 && (scene_target_monster.ap_r_ptr->level >= player_ptr->lev)) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_HIGHER_LEVEL_MONSTER;
        return true;
    }

    return false;
}

/*! モンスターBGMのフォールバック設定。
 * 先頭から適用する（先にある方を優先する）。
 */
std::vector<scene_monster_func> scene_monster_def_list = {
    // scene_monster : あやしい影 or モンスターID
    scene_monster,
    // scene_unique : ユニークモンスター判定
    scene_unique,
    // scene_unkown : 未知のモンスター判定
    scene_unknown,
    // scene_high_level : 高レベルのモンスター判定
    scene_high_level,
};

int get_scene_monster_count()
{
    return scene_monster_def_list.size();
}

/*!
 * @brief 現在の条件でモンスターのBGM選曲をリストに設定する。
 * @details リストのfrom_indexの位置から、get_scene_monster_count()で得られる個数分設定する。
 * 視界内モンスターリスト先頭のモンスターを記憶し、以前のモンスターと比較してより上位のモンスターをBGM選曲の対象とする。
 * 記憶したモンスターが視界内に存在しない場合、一定のゲームターン経過で忘れる。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monster_list 視界内モンスターリスト
 * @param list BGM選曲リスト
 * @param from_index リストの更新開始位置
 */
void refresh_scene_monster(PlayerType *player_ptr, const std::vector<MONSTER_IDX> &monster_list, scene_type_list &list, int from_index)
{
    const bool mute = can_mute_scene_monster();

    if (mute) {
        // モンスターBGM制限中
        clear_scene_target_monster();
    } else {
        if (scene_target_monster.ap_r_ptr) {
            // BGM対象から外すチェック
            if (get_game_turn() - scene_target_monster.last_seen >= 200) {
                // 最後に見かけてから一定のゲームターンが経過した場合、BGM対象から外す
                clear_scene_target_monster();
            } else {
                const auto &monster = player_ptr->current_floor_ptr->m_list[scene_target_monster.m_idx];
                auto *ap_r_ptr = &monster.get_appearance_monrace();
                if (ap_r_ptr != scene_target_monster.ap_r_ptr) {
                    // 死亡、チェンジモンスター、etc.
                    clear_scene_target_monster();
                }
            }
        }

        if (!monster_list.empty()) {
            // 現在のBGM対象とモンスターリスト先頭を比較し、上位をBGM対象に設定する
            update_target_monster(player_ptr, monster_list.front());
        }
    }

    if (scene_target_monster.ap_r_ptr) {
        // BGM対象の条件で選曲リストを設定する
        for (auto func : scene_monster_def_list) {
            scene_type &item = list[from_index];
            if (!func(player_ptr, &item)) {
                // Note -- 特に定義を設けていないが、type = 0は無効な値とする。
                item.type = 0;
                item.val = 0;
            }
            ++from_index;
        }
    } else {
        // BGM対象なしの場合は0で埋める
        const int count = get_scene_monster_count();
        for (int i = 0; i < count; i++) {
            scene_type &item = list[from_index + i];
            // Note -- 特に定義を設けていないが、type = 0は無効な値とする。
            item.type = 0;
            item.val = 0;
        }
    }
}
