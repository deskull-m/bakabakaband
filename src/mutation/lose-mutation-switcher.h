#pragma once

#include <tl/optional.hpp>

struct glm_type;
class CreatureEntity;
enum class PlayerMutationType;

/*!
 * @brief switch_lose_mutation() の重み付き抽選表の case 番号の最大値
 * @details 1 から LOSE_MUTATION_CASE_MAX までの各番号が 1 つの突然変異に対応する
 * (同じ突然変異に複数の番号が割り当てられており、その個数が抽選の重みになる)。
 */
constexpr auto LOSE_MUTATION_CASE_MAX = 210;

/*!
 * @brief ランダムな突然変異治療で抽選される case 番号の最大値
 * @details LOSE_MUTATION_CASE_MAX より小さいので、これを超える番号に対応する
 * 突然変異 (貧弱な下半身 / 変質者を引きつける / 肛門破壊 / 頭部喪失) は
 * **ランダム治療では決して治らない**。番号を指定する治療でのみ取り除ける。
 */
constexpr auto LOSE_MUTATION_RANDOM_CASE_MAX = 205;

void switch_lose_mutation(CreatureEntity &creature, glm_type *glm_ptr);
tl::optional<int> find_lose_mutation_case(CreatureEntity &creature, PlayerMutationType type);
bool can_lose_mutation_randomly(CreatureEntity &creature, PlayerMutationType type);
