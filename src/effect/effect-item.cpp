#include "effect/effect-item.h"
#include "autopick/autopick.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "monster-floor/monster-corpse-leaver.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-expendable.h"
#include "object/object-broken.h"
#include "object/object-mark-types.h"
#include "spell-kind/spells-perception.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"
#include "system/floor/floor-info.h"
#include "system/floor/monster-corpse.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <set>

namespace {
/*!
 * @brief 属性によってアイテムが壊れるかの判定結果
 */
struct ItemDestructionJudge {
    bool do_kill = false; //!< 壊れる属性か
    bool ignore = false; //!< 壊れる属性だが耐性で無事か
    concptr note_kill = nullptr; //!< 壊れたときの描写
};

/*!
 * @brief 属性によってアイテムが壊れるかを判定する
 * @details 床のアイテムと足元の死体 (MonsterCorpse) の双方に同じ規則を適用するため、
 *          affect_item() の属性分岐のうち「壊れるか」だけを決める部分を切り出したもの。
 *          死体は死体・骨のアイテム (MonsterCorpse::make_remains_item()) に見立てて判定する。
 */
ItemDestructionJudge judge_item_destruction(ItemEntity &item, AttributeType typ)
{
    ItemDestructionJudge judge;
    auto &do_kill = judge.do_kill;
    auto &ignore = judge.ignore;
    auto &note_kill = judge.note_kill;
#ifdef JP
#else
    bool plural = (item.number > 1);
#endif
    const auto flags = item.get_flags();
    switch (typ) {
    case AttributeType::ACID: {
        if (BreakerAcid().hates(&item)) {
            do_kill = true;
            note_kill = _("融けてしまった！", (plural ? " melt!" : " melts!"));
            if (flags.has(TR_IGNORE_ACID)) {
                ignore = true;
            }
        }

        break;
    }
    case AttributeType::ELEC: {
        if (BreakerElec().hates(&item)) {
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            if (flags.has(TR_IGNORE_ELEC)) {
                ignore = true;
            }
        }

        break;
    }
    case AttributeType::FIRE: {
        if (BreakerFire().hates(&item)) {
            do_kill = true;
            note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
            if (flags.has(TR_IGNORE_FIRE)) {
                ignore = true;
            }
        }

        break;
    }
    case AttributeType::COLD: {
        if (BreakerCold().hates(&item)) {
            note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
            do_kill = true;
            if (flags.has(TR_IGNORE_COLD)) {
                ignore = true;
            }
        }

        break;
    }
    case AttributeType::PLASMA: {
        if (BreakerFire().hates(&item)) {
            do_kill = true;
            note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
            if (flags.has(TR_IGNORE_FIRE)) {
                ignore = true;
            }
        }

        if (BreakerElec().hates(&item)) {
            ignore = false;
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
            if (flags.has(TR_IGNORE_ELEC)) {
                ignore = true;
            }
        }

        break;
    }
    case AttributeType::METEOR: {
        if (BreakerFire().hates(&item)) {
            do_kill = true;
            note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
            if (flags.has(TR_IGNORE_FIRE)) {
                ignore = true;
            }
        }

        if (BreakerCold().hates(&item)) {
            ignore = false;
            do_kill = true;
            note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
            if (flags.has(TR_IGNORE_COLD)) {
                ignore = true;
            }
        }

        break;
    }
    case AttributeType::ICE:
    case AttributeType::SHARDS:
    case AttributeType::FORCE:
    case AttributeType::SOUND: {
        if (BreakerCold().hates(&item)) {
            note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
            do_kill = true;
        }

        break;
    }
    case AttributeType::MANA:
    case AttributeType::SEEKER:
    case AttributeType::SUPER_RAY: {
        do_kill = true;
        note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
        break;
    }
    case AttributeType::DISINTEGRATE: {
        do_kill = true;
        note_kill = _("蒸発してしまった！", (plural ? " evaporate!" : " evaporates!"));
        break;
    }
    case AttributeType::CHAOS: {
        do_kill = true;
        note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
        if (flags.has(TR_RES_CHAOS)) {
            ignore = true;
        } else if (item.bi_key == BaseitemKey(ItemKindType::SCROLL, SV_SCROLL_CHAOS)) {
            ignore = true;
        }
        break;
    }
    case AttributeType::HOLY_FIRE:
    case AttributeType::HELL_FIRE: {
        if (item.is_cursed()) {
            do_kill = true;
            note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
        }

        break;
    }
    case AttributeType::VOID_MAGIC: {
        do_kill = true;
        note_kill = _("消滅してしまった！", (plural ? " vanish!" : " vanishes!"));
        break;
    }
    default:
        break;
    }

    return judge;
}

/*!
 * @brief 死体の持ち物を蘇ったモンスターへ返す
 * @details 返しきれなかった品は床へ出す (品を黙って消さないため)。
 */
void return_belongings_to_revived(CreatureEntity &creature, MonsterCorpse &corpse, CreatureEntity &revived)
{
    const auto pos = corpse.get_position();
    for (auto &item_ptr : corpse.body.inventory) {
        if (!item_ptr || !item_ptr->is_valid()) {
            continue;
        }

        auto item = item_ptr->clone();
        item_ptr->wipe();
        if (revived.acquire_item(item) < 0) {
            (void)drop_near(creature, item, pos, false);
        }
    }
}

/*!
 * @brief 足元に遺された死体 (MonsterCorpse) への効果処理
 * @return 何か一つでも効力が見えたら true
 * @details 床の亡骸は MonsterCorpse だけが表すので、床のアイテムとは別に処理する。
 *          **壊れるかの判定と死者復活の確率は床の死体・骨のアイテムと同じ**にしてある
 *          (死体・骨のアイテムに見立てて judge_item_destruction() に通す)。
 *          - 破壊: 死体が消え、残っていた持ち物は床へ出る
 *          - 死者復活: 蘇ったモンスターが持ち物を取り戻す。灰になった場合は持ち物が床へ出る
 *          床のアイテムの処理より後に呼ぶので、床へ出た持ち物は同じ効果を受けない。
 */
bool affect_corpses(CreatureEntity &creature, MONSTER_IDX src_idx, const Pos2D &pos, AttributeType typ, bool known)
{
    auto &floor = *creature.get_floor();
    auto is_affected = false;
    for (const auto corpse_idx : collect_corpses_at(floor, pos)) {
        auto &corpse = floor.get_corpse(corpse_idx);
        auto remains = corpse.make_remains_item();
        const auto corpse_name = corpse.describe();
        concptr note = nullptr;
        if (typ == AttributeType::ANIM_DEAD) {
            BIT_FLAGS mode = 0L;
            if (is_player(src_idx) || floor.get_monster(src_idx).is_pet()) {
                mode |= PM_FORCE_PET;
            }

            const auto revive_chance = corpse.is_skeleton() ? 60 : 80;
            tl::optional<MONSTER_IDX> revived_idx;
            if (randint1(100) <= revive_chance) {
                revived_idx = summon_named_creature(creature, src_idx, pos.y, pos.x, corpse.body.get_r_idx(), mode);
            }

            if (revived_idx) {
                return_belongings_to_revived(creature, corpse, floor.get_monster(*revived_idx));
                note = _("生き返った。", " revived.");
            } else {
                note = _("灰になった。", " becomes dust.");
            }
        } else {
            const auto judge = judge_item_destruction(remains, typ);
            if (!judge.do_kill || judge.ignore) {
                continue;
            }

            note = judge.note_kill;
        }

        if (known && note) {
            msg_print(_("{}は{}", "The {}{}"), corpse_name, note);
            is_affected = true;
        }

        remove_corpse(creature, corpse_idx, true);
        lite_spot(creature, pos);
    }

    return is_affected;
}
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるアイテムオブジェクトへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param creature クリーチャーへの参照
 * @param src_idx 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_item(CreatureEntity &creature, MONSTER_IDX src_idx, POSITION r, POSITION y, POSITION x, int dam, AttributeType typ)
{
    const auto &floor = *creature.get_floor();
    const Pos2D pos(y, x);
    const auto &grid = floor.get_grid(pos);

    auto is_item_affected = false;
    const auto known = grid.has_los();
    src_idx = is_monster(src_idx) ? src_idx : 0;
    dam = (dam + r) / (r + 1);
    std::vector<OBJECT_IDX> delete_i_idx_list;
    std::vector<short> affected_potions;
    for (const auto this_o_idx : grid.o_idx_list) {
        auto &item = *creature.get_floor()->o_list[this_o_idx];
        auto ignore = false;
        auto do_kill = false;
        concptr note_kill = nullptr;

#ifdef JP
#else
        bool plural = (item.number > 1);
#endif
        bool is_fixed_or_random_artifact = item.is_fixed_or_random_artifact();
        switch (typ) {
        case AttributeType::IDENTIFY: {
            identify_item(creature, &item);
            autopick_alter_item(creature, (-this_o_idx), false);
            break;
        }
        case AttributeType::KILL_TRAP:
        case AttributeType::KILL_DOOR: {
            if (item.bi_key.tval() != ItemKindType::CHEST) {
                break;
            }

            if (item.pval <= 0) {
                break;
            }

            item.pval = (0 - item.pval);
            item.mark_as_known();
            DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
            if (known && item.marked.has(OmType::FOUND)) {
                msg_print(_("カチッと音がした！", "Click!"));
                is_item_affected = true;
            }

            break;
        }
        case AttributeType::ANIM_DEAD: {
            if (item.bi_key.tval() != ItemKindType::MONSTER_REMAINS) {
                break;
            }

            if (item.bi_key.sval() == SV_SOUL) {
                break;
            }

            BIT_FLAGS mode = 0L;
            if (is_player(src_idx) || creature.get_floor()->get_monster(src_idx).is_pet()) {
                mode |= PM_FORCE_PET;
            }

            for (int i = 0; i < item.number; i++) {
                const auto &monrace = item.get_monrace();
                const auto sval = *item.bi_key.sval();
                if (((sval == SV_CORPSE) && (randint1(100) > 80)) || ((sval == SV_SKELETON) && (randint1(100) > 60))) {
                    if (!note_kill) {
                        note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
                    }

                    continue;
                } else if (summon_named_creature(creature, src_idx, y, x, monrace.idx, mode)) {
                    note_kill = _("生き返った。", " revived.");
                } else if (!note_kill) {
                    note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
                }
            }

            do_kill = true;
            is_item_affected = true;
            break;
        }
        default: {
            const auto judge = judge_item_destruction(item, typ);
            do_kill = judge.do_kill;
            ignore = judge.ignore;
            note_kill = judge.note_kill;
            break;
        }
        }

        if (!do_kill) {
            continue;
        }

        std::string item_name("");
        if (known && item.marked.has(OmType::FOUND)) {
            is_item_affected = true;
            item_name = describe_flavor(creature, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        }

        if ((is_fixed_or_random_artifact || ignore)) {
            if (known && item.marked.has(OmType::FOUND)) {
#ifdef JP
                msg_print("{}は影響を受けない！", item_name);
#else
                msg_print("The {} {} unaffected!", item_name, plural ? "are" : "is");
#endif
            }

            continue;
        }

        if (known && item.marked.has(OmType::FOUND) && note_kill) {
            msg_print(_("{}は{}", "The {}{}"), item_name, note_kill);
        }

        delete_i_idx_list.push_back(this_o_idx);
        if (item.is_potion()) {
            affected_potions.push_back(item.bi_id);
        }

        lite_spot(creature, pos);
    }

    delete_items(creature, std::move(delete_i_idx_list));
    for (const auto bi_id : affected_potions) {
        (void)potion_smash_effect(creature, src_idx, y, x, bi_id);
    }

    if (affect_corpses(creature, src_idx, pos, typ, known)) {
        is_item_affected = true;
    }

    return is_item_affected;
}
