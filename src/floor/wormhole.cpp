/*!
 * @file wormhole.cpp
 * @brief 同一フロアの別地点へ相互に繋がるワームホール地形
 * @details 階段と同じく `<` / `>` で乗り、同じフロアの対となるワームホールへ移動する。
 *          地形自体は `PERMANENT` なので *破壊* / 地震では壊れない。
 *
 *          **対の情報はどこにも保存しない。** フロア上のワームホールを座標順
 *          (y, x の昇順) に並べ、前半 i 番目と後半 i 番目を結ぶ規則で毎回導出する。
 *          これにより
 *          - セーブデータの拡張が不要 (地形配置自体は既に保存されている)
 *          - `Grid::special` を消費しないので階段・クエストの既存用途と衝突しない
 *          - ロード後も必ず同じ対になる
 *          という利点が得られる。代償として対は幾何的に決まる (北半分の個体が
 *          南半分の個体と結ばれる) が、フロアを縦断する近道になるので
 *          ワームホールとしてはむしろ望ましい。
 *
 *          規則が成立するのは個体数が偶数のときだけなので、生成は必ず対単位で行う
 *          (`alloc_wormholes()`)。何らかの理由で奇数個になった場合は座標順で
 *          最後の 1 個が相手を持たず、乗っても何も起きない。
 */

#include "floor/wormhole.h"
#include "core/disturbance.h"
#include "floor/floor-base-definitions.h"
#include "floor/object-allocator.h"
#include "game-option/cheat-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/z-rand.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include <algorithm>
#include <range/v3/view.hpp>
#include <utility>

/*!
 * @brief 面積に比例したワームホールの出現率を返す
 * @param height フロアの縦幅
 * @param width フロアの横幅
 * @return 出現率 [%] (0〜100)
 * @details ダンジョンフロアの最大面積 (MAX_DUNGEON_HGT * MAX_DUNGEON_WID) で
 *          WORMHOLE_MAX_AREA_CHANCE_PERCENT に達する線形比例。
 */
int calc_wormhole_appearance_chance(int height, int width)
{
    constexpr auto max_area = MAX_DUNGEON_HGT * MAX_DUNGEON_WID;
    const auto area = std::max(0, height) * std::max(0, width);
    const auto chance = WORMHOLE_MAX_AREA_CHANCE_PERCENT * area / max_area;
    return std::clamp(chance, 0, 100);
}

/*!
 * @brief 面積に応じたワームホールの対の数を返す
 * @param height フロアの縦幅
 * @param width フロアの横幅
 * @return 対の数 (1 以上)
 */
int calc_wormhole_pairs(int height, int width)
{
    const auto area = std::max(0, height) * std::max(0, width);
    return 1 + area / WORMHOLE_AREA_PER_PAIR;
}

/*!
 * @brief 座標順に並べたワームホールのうち、与えた添字の相手を返す
 * @param count フロア上のワームホールの総数
 * @param index 相手を知りたいワームホールの添字
 * @return 相手の添字。相手が居なければ nullopt
 * @details 前半 i 番目 <-> 後半 i 番目で結ぶ。個体数が奇数なら最後の 1 個は相手を持たない。
 */
tl::optional<int> calc_wormhole_partner_index(int count, int index)
{
    const auto half = count / 2;
    const auto paired_count = half * 2;
    if ((index < 0) || (index >= paired_count)) {
        return tl::nullopt;
    }

    return (index + half) % paired_count;
}

/*!
 * @brief フロア上のワームホールを座標順 (y, x の昇順) に集める
 * @param floor フロアへの参照
 * @return ワームホールの座標一覧
 */
std::vector<Pos2D> collect_wormholes(const FloorType &floor)
{
    std::vector<Pos2D> positions;
    for (const auto &pos : floor.get_area()) {
        if (floor.get_grid(pos).has(TerrainCharacteristics::WORMHOLE)) {
            positions.push_back(pos);
        }
    }

    return positions;
}

/*!
 * @brief 指定座標のワームホールの相手を返す
 * @param floor フロアへの参照
 * @param pos ワームホールの座標
 * @return 相手の座標。ワームホールでない / 相手が居ない場合は nullopt
 */
tl::optional<Pos2D> find_wormhole_partner(const FloorType &floor, const Pos2D &pos)
{
    const auto positions = collect_wormholes(floor);
    const auto it = std::find(positions.begin(), positions.end(), pos);
    if (it == positions.end()) {
        return tl::nullopt;
    }

    const auto index = static_cast<int>(std::distance(positions.begin(), it));
    const auto partner_index = calc_wormhole_partner_index(static_cast<int>(positions.size()), index);
    if (!partner_index) {
        return tl::nullopt;
    }

    return positions[*partner_index];
}

/*!
 * @brief ワームホールを対単位でフロアへ配置する
 * @param creature クリーチャーへの参照
 * @param pairs 配置したい対の数
 * @param walls 配置先の床に要求する隣接外壁数
 * @return 1 組でも配置できたらTRUE
 * @details 候補を先に全て集めてから 2 個ずつ書き込むので、**必ず偶数個**になる
 *          (奇数個になると相手の居ないワームホールが生まれてしまう)。
 *          候補が足りなければ `walls` を緩め、それでも足りなければ置ける範囲で
 *          対の数を減らす。ワームホールは階段と違いフロア生成の成否には関わらない。
 */
bool alloc_wormholes(CreatureEntity &creature, int pairs, int walls)
{
    if (pairs <= 0) {
        return false;
    }

    auto &floor = *creature.get_floor();
    const auto terrain_id = TerrainList::get_instance().get_terrain_id(TerrainTag::WORMHOLE);
    auto required = pairs * 2;
    while (true) {
        const auto can_alloc = [&](const Pos2D &pos) { return can_place_stair_at(creature, pos, walls); };
        auto candidates =
            floor.get_area(FloorBoundary::OUTER_WALL_INCLUSIVE) |
            ranges::views::filter(can_alloc) |
            ranges::to_vector;

        if (std::cmp_less(candidates.size(), required)) {
            if (walls > 0) {
                walls--;
                continue;
            }

            // 置ける床が足りないので、偶数個に収まるところまで対を減らす。
            required = (static_cast<int>(candidates.size()) / 2) * 2;
            if (required == 0) {
                return false;
            }
        }

        for (auto i = 0; i < required; i++) {
            // 選んだ候補を末尾と入れ替えて取り除く (重複して選ばないため)。
            const auto index = randint0(static_cast<int>(candidates.size()));
            const auto pos_wormhole = candidates[index];
            candidates[index] = candidates.back();
            candidates.pop_back();

            auto &grid = floor.get_grid(pos_wormhole);
            grid.mimic = 0;
            grid.feat = terrain_id;
            grid.info &= ~(CAVE_FLOOR);
        }

        return true;
    }
}

/*!
 * @brief 面積に応じてワームホールを生成する
 * @param creature クリーチャーへの参照
 * @details 広いフロアほど出やすく、かつ多く置かれる。
 *          配置に失敗してもフロア生成は続行する (階段と違い必須の地形ではない)。
 */
void alloc_wormholes_by_floor_area(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    if (!evaluate_percent(calc_wormhole_appearance_chance(floor.height, floor.width))) {
        return;
    }

    const auto pairs = calc_wormhole_pairs(floor.height, floor.width);
    if (!alloc_wormholes(creature, pairs, WORMHOLE_REQUIRED_WALLS)) {
        msg_print_wizard(creature, CHEAT_DUNGEON, _("ワームホール生成に失敗", "Failed to generate wormholes."));
    }
}

/*!
 * @brief ワームホールに乗って同じフロアの対の地点へ移動する
 * @param creature クリーチャーへの参照
 */
void do_cmd_enter_wormhole(CreatureEntity &creature)
{
    auto &floor = *creature.get_floor();
    const auto pos = creature.get_position();
    if (!floor.get_grid(pos).has(TerrainCharacteristics::WORMHOLE)) {
        msg_print(_("ここにはワームホールが見当たらない。", "I see no wormhole here."));
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });
    const auto pos_partner = find_wormhole_partner(floor, pos);
    if (!pos_partner) {
        msg_print(_("ワームホールは何の反応も示さなかった。", "The wormhole does not respond."));
        return;
    }

    msg_print(_("ワームホールに潜り込んだ。", "You slip into the wormhole."));
    sound(SoundKind::TELEPORT);
    (void)move_player_effect(creature, pos_partner->y, pos_partner->x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
    PlayerEnergy(creature).set_player_turn_energy(100);
    disturb(creature, false, true);
}
