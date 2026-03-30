/*!
 * @brief プレイヤーの種族に基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details PlayerRaceからPlayerClassへの依存はあるが、逆は依存させないこと.
 */
#include "player-base/player-race.h"
#include "player-base/player-class.h"
#include "player-info/mimic-info-table.h"
#include "player/race-info-table.h"
#include "system/angband-exceptions.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief Construct a new Creature Race:: Creature Race object
 *
 * @param base_race true の場合、仮に変身している場合でも元の種族について扱う。 false の場合変身している種族について扱う。
 * 引数を省略した場合は false
 */
CreatureRace::CreatureRace(CreatureEntity *creature_ptr, bool base_race)
    : creature_ptr(creature_ptr)
    , base_race(base_race)
{
}

/*!
 * @brief 種族固有の特性フラグを取得する
 * @return
 */
TrFlags CreatureRace::tr_flags() const
{
    TrFlags flags;

    auto race_ptr = this->get_info();
    if (race_ptr->infra > 0) {
        flags.set(TR_INFRA);
    }

    for (auto &cond : race_ptr->extra_flags) {
        if (this->creature_ptr->level < cond.level) {
            continue;
        }
        if (cond.pclass) {
            auto is_class_equal = CreatureClass(*this->creature_ptr).equals(*cond.pclass);
            if (cond.not_class && is_class_equal) {
                continue;
            }
            if (!cond.not_class && !is_class_equal) {
                continue;
            }
        }

        flags.set(cond.type);
    }

    return flags;
}

/*!
 * @brief プレイヤーの種族情報テーブルへのポインタを取得する
 *
 * @return プレイヤーの種族情報テーブルへのポインタ
 */
const player_race_info *CreatureRace::get_info() const
{
    if (this->base_race) {
        return &race_info[enum2i(this->creature_ptr->prace)];
    }

    switch (this->creature_ptr->mimic_form) {
    case MimicKindType::NONE:
        return &race_info[enum2i(this->creature_ptr->prace)];
    case MimicKindType::DEMON:
    case MimicKindType::DEMON_LORD:
    case MimicKindType::VAMPIRE:
    case MimicKindType::ANGEL:
    case MimicKindType::DEMIGOD:
        return &mimic_info.at(this->creature_ptr->mimic_form);
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid MimicKindType was specified!");
    }
}

/*!
 * @brief 種族の生命形態を返す
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @return 生命形態
 */
PlayerRaceLifeType CreatureRace::life() const
{
    return this->get_info()->life;
}

/*!
 * @brief 種族の食料形態を返す
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param base_race ミミック中も元種族の情報を返すならtrue
 * @return 食料形態
 */
PlayerRaceFoodType CreatureRace::food() const
{
    return this->get_info()->food;
}

bool CreatureRace::is_mimic_nonliving() const
{
    constexpr int nonliving_flag = 1;
    return any_bits(mimic_info.at(this->creature_ptr->mimic_form).choice, nonliving_flag);
}

bool CreatureRace::has_cut_immunity() const
{
    auto cut_immunity = this->equals(PlayerRaceType::GOLEM);
    cut_immunity |= this->equals(PlayerRaceType::SKELETON);
    cut_immunity |= this->equals(PlayerRaceType::SPECTRE);
    cut_immunity |= this->equals(PlayerRaceType::ZOMBIE) && (this->creature_ptr->level > 11);
    return cut_immunity;
}

bool CreatureRace::has_stun_immunity() const
{
    return this->equals(PlayerRaceType::GOLEM);
}

bool CreatureRace::equals(PlayerRaceType prace) const
{
    return (this->creature_ptr->mimic_form == MimicKindType::NONE) && (this->creature_ptr->prace == prace);
}

/*!
 * @brief 速度計算 - 種族
 * @return 速度値の増減分
 * @details
 * ** クラッコンと妖精に加算(+レベル/10)
 * ** 悪魔変化/吸血鬼変化で加算(+3)
 * ** 魔王変化で加算(+5)
 * ** マーフォークがFF_WATER地形にいれば加算(+2+レベル/10)
 * ** そうでなく浮遊を持っていないなら減算(-2)
 */
int16_t CreatureRace::speed() const
{
    int16_t result = 0;

    FloorType *floor_ptr = this->creature_ptr->current_floor_ptr;
    if (creature_ptr->x > 0 && creature_ptr->y > 0 && creature_ptr->x <= floor_ptr->width - 1 && creature_ptr->y <= floor_ptr->height - 1) {
        const auto &f_ptr = TerrainList::get_instance().get_terrain(floor_ptr->grid_array[this->creature_ptr->y][this->creature_ptr->x].feat);
        if (f_ptr.flags.has(TerrainCharacteristics::SLOW)) {
            result -= 10;
        }
        if (this->equals(PlayerRaceType::KLACKON) || this->equals(PlayerRaceType::SPRITE)) {
            result += (this->creature_ptr->level) / 10;
        }
    }

    if (this->equals(PlayerRaceType::MERFOLK)) {
        const auto &floor = *this->creature_ptr->current_floor_ptr;
        const auto &terrain = floor.get_grid(this->creature_ptr->get_position()).get_terrain();
        if (terrain.flags.has(TerrainCharacteristics::WATER)) {
            result += (2 + this->creature_ptr->level / 10);
        } else if (!this->creature_ptr->levitation) {
            result -= 2;
        }
    }

    switch (this->creature_ptr->mimic_form) {
    case MimicKindType::NONE:
        return result;
    case MimicKindType::DEMON:
        return result + 3;
    case MimicKindType::DEMON_LORD:
        return result + 5;
    case MimicKindType::VAMPIRE:
        return result + 3;
    case MimicKindType::ANGEL:
        return result + 3;
    case MimicKindType::DEMIGOD:
        return result + 5;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid MimicKindType was specified!");
    }
}

/*!
 * @brief 腕力補正計算 - 種族
 * @return 腕力補正値
 * @details
 * * 種族による腕力修正値。
 * * エントはレベル26,41,46到達ごとに加算(+1)
 */
int16_t CreatureRace::additional_strength() const
{
    int16_t result = 0;

    if (this->equals(PlayerRaceType::ENT)) {
        if (this->creature_ptr->level > 25) {
            result++;
        }
        if (this->creature_ptr->level > 40) {
            result++;
        }
        if (this->creature_ptr->level > 45) {
            result++;
        }
    }

    return result;
}

/*!
 * @brief 器用さ補正計算 - 種族
 * @return 器用さ補正値
 * @details
 * * 種族による器用さ修正値。
 * * エントはレベル26,41,46到達ごとに減算(-1)
 */
int16_t CreatureRace::additional_dexterity() const
{
    int16_t result = 0;

    if (this->equals(PlayerRaceType::ENT)) {
        if (this->creature_ptr->level > 25) {
            result--;
        }
        if (this->creature_ptr->level > 40) {
            result--;
        }
        if (this->creature_ptr->level > 45) {
            result--;
        }
    }

    return result;
}

/*!
 * @brief 耐久力補正計算 - 種族
 * @return 耐久力補正値
 * @details
 * * 種族による耐久力修正値。
 * * エントはレベル26,41,46到達ごとに加算(+1)
 */
int16_t CreatureRace::additional_constitution() const
{
    int16_t result = 0;

    if (CreatureRace(this->creature_ptr).equals(PlayerRaceType::ENT)) {
        if (this->creature_ptr->level > 25) {
            result++;
        }
        if (this->creature_ptr->level > 40) {
            result++;
        }
        if (this->creature_ptr->level > 45) {
            result++;
        }
    }

    return result;
}

/*!
 * @brief 救援召喚時のモンスターシンボルを返す
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @return シンボル文字
 */
char CreatureRace::get_summon_symbol() const
{
    auto symbol = 'N';
    auto mmc_ptr = this->get_info();

    auto l = strlen(mmc_ptr->symbol);
    auto mul = 1;
    for (size_t i = 0; i < l; i++) {
        if (one_in_(mul)) {
            symbol = mmc_ptr->symbol[i];
        }
        mul *= 13;
    }
    return symbol;
}
