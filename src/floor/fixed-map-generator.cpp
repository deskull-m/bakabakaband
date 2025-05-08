#include "floor/fixed-map-generator.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "floor/wild.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/tokenizer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "window/main-window-util.h"

// PARSE_ERROR_MAXが既にあり扱い辛いのでここでconst宣言.
static const int PARSE_CONTINUE = 255;

qtwg_type *initialize_quest_generator_type(qtwg_type *qtwg_ptr, int ymin, int xmin, int ymax, int xmax, int *y, int *x)
{
    qtwg_ptr->ymin = ymin;
    qtwg_ptr->xmin = xmin;
    qtwg_ptr->ymax = ymax;
    qtwg_ptr->xmax = xmax;
    qtwg_ptr->y = y;
    qtwg_ptr->x = x;
    return qtwg_ptr;
}

/*!
 * @brief フロアの所定のマスにオブジェクトを配置する
 * Place the object j_ptr to a grid
 * @param floor 現在フロアへの参照
 * @param item アイテムの参照
 * @param y 配置先Y座標
 * @param x 配置先X座標
 * @return エラーコード
 */
static void drop_here(FloorType &floor, ItemEntity &&item, POSITION y, POSITION x)
{
    const auto item_idx = floor.pop_empty_index_item();
    auto &dropped_item = *floor.o_list[item_idx];
    dropped_item = std::move(item);
    dropped_item.iy = y;
    dropped_item.ix = x;
    dropped_item.held_m_idx = 0;
    auto &grid = floor.grid_array[y][x];
    grid.o_idx_list.add(floor, item_idx);
}

static void generate_artifact(PlayerType *player_ptr, qtwg_type *qtwg_ptr, const FixedArtifactId a_idx)
{
    if (a_idx == FixedArtifactId::NONE) {
        return;
    }

    const auto &artifact = ArtifactList::get_instance().get_artifact(a_idx);
    if (!artifact.is_generated && create_named_art(player_ptr, a_idx, *qtwg_ptr->y, *qtwg_ptr->x)) {
        return;
    }

    ItemEntity item({ ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT });
    drop_here(*player_ptr->current_floor_ptr, std::move(item), *qtwg_ptr->y, *qtwg_ptr->x);
}

/**
 * @note 馬鹿馬鹿では固定配置モンスターのグループ生成はクビだ！クビだ！クビだ！
 */
static void parse_qtw_D(PlayerType *player_ptr, qtwg_type *qtwg_ptr, char *s)
{
    *qtwg_ptr->x = qtwg_ptr->xmin;
    auto &floor = *player_ptr->current_floor_ptr;
    int len = strlen(s);
    auto &monraces = MonraceList::get_instance();
    const auto &dungeon = floor.get_dungeon_definition();
    for (auto i = 0; ((*qtwg_ptr->x < qtwg_ptr->xmax) && (i < len)); (*qtwg_ptr->x)++, s++, i++) {
        auto &grid = floor.grid_array[*qtwg_ptr->y][*qtwg_ptr->x];
        int idx = s[0];
        const auto item_index = letter[idx].object;
        auto monster_index = letter[idx].monster;
        const auto random = letter[idx].random;
        grid.feat = dungeon.convert_terrain_id(letter[idx].feature);
        if (init_flags & INIT_ONLY_FEATURES) {
            continue;
        }

        grid.info = letter[idx].cave_info;
        if (random & RANDOM_MONSTER) {
            floor.monster_level = floor.base_level + monster_index;
            place_random_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_NO_QUEST));
            floor.monster_level = floor.base_level;
        } else if (monster_index) {
            auto clone = false;
            if (monster_index < 0) {
                monster_index = -monster_index;
                clone = true;
            }

            const auto monrace_id = i2enum<MonraceId>(monster_index);
            auto &monrace = monraces.get_monrace(monrace_id);
            const auto old_cur_num = monrace.cur_num;
            const auto old_mob_num = monrace.mob_num;
            const auto old_max_num = monrace.max_num;
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                monrace.reset_current_numbers();
                monrace.mob_num = MAX_UNIQUE_NUM;
                monrace.max_num = MAX_UNIQUE_NUM;
            } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
                if (monrace.cur_num == monrace.mob_num) {
                    monrace.mob_num++;
                }
                monrace.max_num = monrace.mob_num;
            }

            const auto m_idx = place_specific_monster(player_ptr, *qtwg_ptr->y, *qtwg_ptr->x, monrace_id, (PM_ALLOW_SLEEP | PM_NO_KAGE));
            if (clone && m_idx) {
                floor.m_list[*m_idx].mflag2.set(MonsterConstantFlagType::CLONED);
                monrace.cur_num = old_cur_num;
                monrace.cur_num = old_mob_num;
                monrace.max_num = old_max_num;
            }
        }

        if ((random & RANDOM_OBJECT) && (random & RANDOM_TRAP)) {
            floor.object_level = floor.base_level + item_index;

            /*
             * Random trap and random treasure defined
             * 25% chance for trap and 75% chance for object
             */
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            if (evaluate_percent(75)) {
                place_object(player_ptr, pos, 0);
            } else {
                place_trap(floor, pos);
            }

            floor.object_level = floor.base_level;
        } else if (random & RANDOM_OBJECT) {
            floor.object_level = floor.base_level + item_index;
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            if (evaluate_percent(75)) {
                place_object(player_ptr, pos, 0);
            } else if (evaluate_percent(80)) {
                place_object(player_ptr, pos, AM_GOOD);
            } else {
                place_object(player_ptr, pos, AM_GOOD | AM_GREAT);
            }

            floor.object_level = floor.base_level;
        } else if (random & RANDOM_TRAP) {
            const Pos2D pos(*qtwg_ptr->y, *qtwg_ptr->x);
            place_trap(floor, pos);
        } else if (letter[idx].trap) {
            grid.mimic = grid.feat;
            grid.feat = dungeon.convert_terrain_id(letter[idx].trap);
        } else if (item_index) {
            ItemEntity item(item_index);
            if (item.bi_key.tval() == ItemKindType::GOLD) {
                item = floor.make_gold(item.bi_key);
            }

            ItemMagicApplier(player_ptr, &item, floor.base_level, AM_NO_FIXED_ART | AM_GOOD).execute();
            drop_here(floor, std::move(item), *qtwg_ptr->y, *qtwg_ptr->x);
        }

        generate_artifact(player_ptr, qtwg_ptr, letter[idx].artifact);
        grid.special = letter[idx].special;
    }
}

static bool parse_qtw_QQ(QuestType *q_ptr, char **zz, int num)
{
    if (zz[1][0] != 'Q') {
        return false;
    }

    if ((init_flags & INIT_ASSIGN) == 0) {
        return true;
    }

    if (num < 9) {
        return true;
    }

    q_ptr->type = i2enum<QuestKindType>(atoi(zz[2]));
    q_ptr->num_mon = (MONSTER_NUMBER)atoi(zz[3]);
    q_ptr->cur_num = (MONSTER_NUMBER)atoi(zz[4]);
    q_ptr->max_num = (MONSTER_NUMBER)atoi(zz[5]);
    q_ptr->level = (DEPTH)atoi(zz[6]);
    q_ptr->r_idx = i2enum<MonraceId>(atoi(zz[7]));
    const auto fa_id = i2enum<FixedArtifactId>(atoi(zz[8]));
    q_ptr->reward_fa_id = fa_id;
    q_ptr->dungeon = i2enum<DungeonId>(std::atoi(zz[9]));

    if (num > 10) {
        q_ptr->flags = atoi(zz[10]);
    }

    auto &monrace = q_ptr->get_bounty();
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        monrace.misc_flags.set(MonsterMiscType::QUESTOR);
    }

    if (fa_id == FixedArtifactId::NONE) {
        return true;
    }

    auto &artifact = q_ptr->get_reward();
    artifact.gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    return true;
}

/*!
 * @todo 処理がどうなっているのかいずれチェックする
 */
static bool parse_qtw_QR(QuestType *q_ptr, char **zz, int num)
{
    if (zz[1][0] != 'R') {
        return false;
    }

    if ((init_flags & INIT_ASSIGN) == 0) {
        return true;
    }

    int count = 0;
    auto reward_idx = FixedArtifactId::NONE;
    auto &artifacts = ArtifactList::get_instance();
    for (auto idx = 2; idx < num; idx++) {
        const auto fa_id = i2enum<FixedArtifactId>(atoi(zz[idx]));
        if (fa_id == FixedArtifactId::NONE) {
            continue;
        }

        if (artifacts.get_artifact(fa_id).is_generated) {
            continue;
        }

        count++;
        if (one_in_(count)) {
            reward_idx = fa_id;
        }
    }

    if (reward_idx != FixedArtifactId::NONE) {
        q_ptr->reward_fa_id = reward_idx;
        artifacts.get_artifact(reward_idx).gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    } else {
        q_ptr->type = QuestKindType::KILL_ALL;
    }

    return true;
}

/*!
 * @brief t_info、q_info、w_infoにおけるQトークンをパースする
 * @param qtwg_ptr トークンパース構造体への参照ポインタ
 * @param zz トークン保管文字列
 * @return エラーコード、但しPARSE_CONTINUEの時は処理続行
 */
static int parse_qtw_Q(qtwg_type *qtwg_ptr, char **zz)
{
    if (qtwg_ptr->buf[0] != 'Q') {
        return PARSE_CONTINUE;
    }

#ifdef JP
    if (qtwg_ptr->buf[2] == '$') {
        return PARSE_ERROR_NONE;
    }
#else
    if (qtwg_ptr->buf[2] != '$') {
        return PARSE_ERROR_NONE;
    }
#endif

    int num = tokenize(qtwg_ptr->buf + _(2, 3), 33, zz, 0);
    if (num < 3) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    auto &quests = QuestList::get_instance();
    auto &quest = quests.get_quest(i2enum<QuestId>(atoi(zz[0])));
    if (parse_qtw_QQ(&quest, zz, num)) {
        return PARSE_ERROR_NONE;
    }

    if (parse_qtw_QR(&quest, zz, num)) {
        return PARSE_ERROR_NONE;
    }

    if (zz[1][0] == 'N') {
        if (init_flags & (INIT_ASSIGN | INIT_SHOW_TEXT | INIT_NAME_ONLY)) {
            quest.name = zz[2];
        }

        return PARSE_ERROR_NONE;
    }

    if (zz[1][0] == 'T') {
        if ((init_flags & INIT_SHOW_TEXT) && (std::ssize(quest_text_lines) < QUEST_TEST_LINES_MAX)) {
            quest_text_lines.emplace_back(zz[2]);
        }

        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_GENERIC;
}

static bool parse_qtw_P(PlayerType *player_ptr, qtwg_type *qtwg_ptr, char **zz)
{
    if (qtwg_ptr->buf[0] != 'P') {
        return false;
    }

    if ((init_flags & INIT_CREATE_DUNGEON) == 0) {
        return true;
    }

    if (tokenize(qtwg_ptr->buf + 2, 2, zz, 0) != 2) {
        return true;
    }

    int panels_y = (*qtwg_ptr->y / SCREEN_HGT);
    if (*qtwg_ptr->y % SCREEN_HGT) {
        panels_y++;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    floor.height = panels_y * SCREEN_HGT;
    int panels_x = (*qtwg_ptr->x / SCREEN_WID);
    if (*qtwg_ptr->x % SCREEN_WID) {
        panels_x++;
    }

    floor.width = panels_x * SCREEN_WID;
    panel_row_min = floor.height;
    panel_col_min = floor.width;
    if (floor.is_in_quest()) {
        POSITION py = atoi(zz[0]);
        POSITION px = atoi(zz[1]);
        player_ptr->y = py;
        player_ptr->x = px;
        delete_monster(player_ptr, player_ptr->get_position());
        return true;
    }

    if (!player_ptr->oldpx && !player_ptr->oldpy) {
        player_ptr->oldpy = atoi(zz[0]);
        player_ptr->oldpx = atoi(zz[1]);
    }

    return true;
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)をフロアに生成する
 * Parse a sub-file of the "extra info"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf 文字列
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @param y 詳細不明
 * @param x 詳細不明
 * @return エラーコード
 * @todo クエスト情報のみを読み込む手段と実際にフロアデータまで読み込む処理は分離したい
 */
parse_error_type generate_fixed_map_floor(PlayerType *player_ptr, qtwg_type *qtwg_ptr, process_dungeon_file_pf parse_fixed_map)
{
    char *zz[33];
    if (!qtwg_ptr->buf[0]) {
        return PARSE_ERROR_NONE;
    }

    if (iswspace(qtwg_ptr->buf[0])) {
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[0] == '#') {
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[1] != ':') {
        return PARSE_ERROR_GENERIC;
    }

    if (qtwg_ptr->buf[0] == '%') {
        return (*parse_fixed_map)(player_ptr, qtwg_ptr->buf + 2, qtwg_ptr->ymin, qtwg_ptr->xmin, qtwg_ptr->ymax, qtwg_ptr->xmax);
    }

    /* Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid */
    if (qtwg_ptr->buf[0] == 'F') {
        return parse_line_feature(*player_ptr->current_floor_ptr, qtwg_ptr->buf);
    }

    if (qtwg_ptr->buf[0] == 'D') {
        char *s = qtwg_ptr->buf + 2;
        if (init_flags & INIT_ONLY_BUILDINGS) {
            return PARSE_ERROR_NONE;
        }

        parse_qtw_D(player_ptr, qtwg_ptr, s);
        (*qtwg_ptr->y)++;
        return PARSE_ERROR_NONE;
    }

    parse_error_type parse_result_Q = i2enum<parse_error_type>(parse_qtw_Q(qtwg_ptr, zz));
    if (parse_result_Q != PARSE_CONTINUE) {
        return parse_result_Q;
    }

    if (qtwg_ptr->buf[0] == 'W') {
        const Pos2D pos_initial(*qtwg_ptr->y, *qtwg_ptr->x);
        const auto &[parse_result_W, pos] = parse_line_wilderness(qtwg_ptr->buf, qtwg_ptr->xmin, qtwg_ptr->xmax, pos_initial);
        if (parse_result_W == PARSE_ERROR_NONE) {
            *qtwg_ptr->y = pos->y;
            *qtwg_ptr->x = pos->x;
        }

        return parse_result_W;
    }

    if (parse_qtw_P(player_ptr, qtwg_ptr, zz)) {
        return PARSE_ERROR_NONE;
    }

    if (qtwg_ptr->buf[0] == 'B') {
        return parse_line_building(qtwg_ptr->buf);
    }

    if (qtwg_ptr->buf[0] == 'V') {
        return parse_line_vault(player_ptr->current_floor_ptr, qtwg_ptr->buf);
    }

    if (qtwg_ptr->buf[0] == 'A') {
        return parse_line_alliance(player_ptr->current_floor_ptr, qtwg_ptr->buf);
    }

    if (qtwg_ptr->buf[0] == 'M') {
        // skip
        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_GENERIC;
}
