#include "system/floor/monster-corpse.h"
#include "inventory/inventory-slot-types.h"
#include "locale/language-switcher.h"
#include "object/tval-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/baseitem/baseitem-key.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

/*!
 * @brief 空の死体を作る
 * @details **モンスター用のプロファイルを必ず初期化すること**。
 *          FloorType のコンストラクタが m_list の各要素に対して行っているのと
 *          同じ初期化で、これを忘れるとセーブデータ読込 (rd_monster) が
 *          tl::bad_optional_access で落ちる。
 */
MonsterCorpse::MonsterCorpse()
    : remains_sval(SV_CORPSE)
{
    this->body.init_monster_profile();
}

/*!
 * @brief 死亡したモンスターの実体を複製して死体を作る
 * @param monster 死亡したモンスター
 * @details 所持品 (shared_ptr) も複製されるため、**呼出側は死体を作る時点で
 *          モンスター側の所持品の扱いを確定させておくこと**。床へ放出した後に
 *          死体を作れば死体は空になり、放出せずに死体を作れば死体が所持品を持つ。
 */
MonsterCorpse::MonsterCorpse(const CreatureEntity &monster)
    : body(monster)
    , remains_sval(SV_CORPSE)
{
    // CreatureEntity のコピーは inventory の shared_ptr を共有するため、
    // **実体を複製して共有を切る**。これを怠ると、モンスター側の
    // drop_all_inventory() が held.wipe() で ItemEntity を直接潰すので、
    // 死体の中身まで一緒に消える。
    for (auto &item_ptr : this->body.inventory) {
        if (item_ptr) {
            item_ptr = std::make_shared<ItemEntity>(item_ptr->clone());
        }
    }
}

/*!
 * @brief 有効な死体かどうか
 * @details 種族が引けない (= 破棄済み or 未初期化) なら無効とする。
 */
bool MonsterCorpse::is_valid() const
{
    return this->body.is_valid();
}

bool MonsterCorpse::is_skeleton() const
{
    return this->remains_sval == SV_SKELETON;
}

/*!
 * @brief 死体がまだ持ち物 (剥ぎ取れる品) を抱えているか
 * @details 生前の所持品・装備に加え、ソウル・鱗等の剥ぎ取れる素材も含む。
 *          持ち物が残っている死体は持ち去れない (食べる・壊すと持ち物は周りへ散らばる)。
 */
bool MonsterCorpse::has_belongings() const
{
    for (auto slot = 0; slot < INVEN_TOTAL; slot++) {
        const auto &item_ptr = this->body.inventory[slot];
        if (item_ptr && item_ptr->is_valid()) {
            return true;
        }
    }

    return false;
}

Pos2D MonsterCorpse::get_position() const
{
    return this->body.get_position();
}

/*!
 * @brief 死体の呼び名を返す
 * @details 個体名 (ユニーク・ペットのニックネーム) があればそれを使う。
 */
std::string MonsterCorpse::describe() const
{
    const auto &monrace = MonraceList::get_instance().get_monrace(this->body.get_r_idx());
    const auto &name = this->body.name.empty() ? monrace.name.string() : this->body.name;
    if (this->is_skeleton()) {
        return fmt::format(_("{}の骨", "skeleton of {}"), name);
    }

    return fmt::format(_("{}の死体", "corpse of {}"), name);
}

/*!
 * @brief 持ち運ぶための死体・骨のアイテムを作る
 * @details 種族は pval に入れる (従来の死体アイテムと同じ規約)。
 *          死体は鑑定の必要が無い品なので既知として作る。
 */
ItemEntity MonsterCorpse::make_remains_item() const
{
    ItemEntity item({ ItemKindType::MONSTER_REMAINS, this->remains_sval });
    item.pval = enum2i(this->body.get_r_idx());
    item.mark_as_known();
    return item;
}

/*!
 * @brief 床に置かれたら死体へ戻すべきアイテム (死体・骨) か
 */
bool MonsterCorpse::is_remains_item(const ItemEntity &item)
{
    if (!item.is_valid() || (item.bi_key.tval() != ItemKindType::MONSTER_REMAINS)) {
        return false;
    }

    const auto sval = item.bi_key.sval();
    return sval && ((*sval == SV_CORPSE) || (*sval == SV_SKELETON));
}
