/*!
 * @brief 施設で突然変異を指定して付加・削除するサービス
 * @details メリンライトの生体研究所で使う。ランダムに 1 つ治すだけの
 * BACT_LOSE_MUTATION と違い、プレイヤーが変異を選べる代わりに高額。
 */

#include "market/building-mutation.h"
#include "mutation/gain-mutation-switcher.h"
#include "mutation/lose-mutation-switcher.h"
#include "mutation/mutation-data.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "mutation/mutation-util.h"
#include "system/creature-entity.h"
#include "util/candidate-selector.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <vector>

namespace {

/*!
 * @brief 指定して付加できる突然変異を列挙する
 * @param creature クリーチャーへの参照
 * @details 既に持っている変異を除くほか、**このクリーチャーでは無効化される変異**
 *          (カオス・ウォリアーのパトロン / いかさま師の黒いオーラ /
 *          メスガキの変質者引きつけ) も除く。`switch_gain_mutation()` がこれらを
 *          `PlayerMutationType::MAX` に潰すため、候補に残すと代金を払って
 *          何も起きないことになる。指定ありの `switch_gain_mutation()` は
 *          乱数を消費しないので、ここで引き直しても抽選列は乱れない。
 */
std::vector<PlayerMutationType> collect_graftable_mutations(CreatureEntity &creature)
{
    std::vector<PlayerMutationType> candidates;
    for (auto i = 0; i < enum2i(PlayerMutationType::MAX); i++) {
        const auto type = i2enum<PlayerMutationType>(i);
        if (creature.has_mutation(type)) {
            continue;
        }

        glm_type tmp_gm;
        auto *gm_ptr = initialize_glm_type(&tmp_gm, i);
        switch_gain_mutation(creature, gm_ptr);
        if (gm_ptr->muta_which != type) {
            continue;
        }

        candidates.push_back(type);
    }

    return candidates;
}

/*!
 * @brief 指定して取り除ける突然変異を列挙する
 * @param creature クリーチャーへの参照
 * @details 現在持っている変異のうち、抽選表に対応する番号がある (=
 *          `switch_lose_mutation()` が取り除ける) ものだけを返す。
 *          いかさま師の白いオーラのように治療対象外の変異は落ちる。
 *          ランダム治療では当たらない番号 (貧弱な下半身 / 変質者を引きつける /
 *          肛門破壊 / 頭部喪失) も**指定すれば取り除ける**。
 */
std::vector<PlayerMutationType> collect_excisable_mutations(CreatureEntity &creature)
{
    std::vector<PlayerMutationType> candidates;
    for (auto i = 0; i < enum2i(PlayerMutationType::MAX); i++) {
        const auto type = i2enum<PlayerMutationType>(i);
        if (!creature.has_mutation(type)) {
            continue;
        }

        if (!find_lose_mutation_case(creature, type)) {
            continue;
        }

        candidates.push_back(type);
    }

    return candidates;
}

/*!
 * @brief 突然変異の一覧から 1 つ選ばせる
 * @return 選んだ突然変異。ESC でキャンセルした場合は tl::nullopt
 */
tl::optional<PlayerMutationType> select_mutation(const std::vector<PlayerMutationType> &candidates, const std::string &prompt)
{
    CandidateSelector selector(prompt);
    const auto it = selector.select(candidates, [](PlayerMutationType type) {
        return MutationDataManager::get_mutation_name(type);
    });
    if (it == candidates.end()) {
        return tl::nullopt;
    }

    return *it;
}

}

/*!
 * @brief 任意の突然変異を付加する
 * @param creature クリーチャーへの参照
 * @return 付加したか否か (代金の支払い判定に使う)
 */
bool gain_chosen_mutation(CreatureEntity &creature)
{
    const auto candidates = collect_graftable_mutations(creature);
    if (candidates.empty()) {
        msg_print(_("これ以上埋め込める突然変異が無い。", "There is no mutation left to graft onto you."));
        msg_erase();
        return false;
    }

    const auto chosen = select_mutation(candidates, _("どの突然変異を埋め込みますか？ ", "Graft which mutation? "));
    if (!chosen) {
        return false;
    }

    return gain_mutation(creature, *chosen);
}

/*!
 * @brief 任意の突然変異を削除する
 * @param creature クリーチャーへの参照
 * @return 削除したか否か (代金の支払い判定に使う)
 */
bool lose_chosen_mutation(CreatureEntity &creature)
{
    const auto candidates = collect_excisable_mutations(creature);
    if (candidates.empty()) {
        msg_print(_("取り除ける突然変異が無い。", "You have no mutation that can be excised."));
        msg_erase();
        return false;
    }

    const auto chosen = select_mutation(candidates, _("どの突然変異を取り除きますか？ ", "Excise which mutation? "));
    if (!chosen) {
        return false;
    }

    return lose_mutation(creature, *chosen);
}

/*!
 * @brief ランダムな治療で取り除ける突然変異を 1 つでも持っているかを返す
 * @param creature クリーチャーへの参照
 * @details ランダム治療 (BACT_LOSE_MUTATION) は当たりが出るまで引き直すため、
 *          1 つも当たり得ない状態で呼ぶと**無限ループになる**。その事前判定用。
 */
bool has_randomly_curable_mutation(CreatureEntity &creature)
{
    for (auto i = 0; i < enum2i(PlayerMutationType::MAX); i++) {
        const auto type = i2enum<PlayerMutationType>(i);
        if (creature.has_mutation(type) && can_lose_mutation_randomly(creature, type)) {
            return true;
        }
    }

    return false;
}
