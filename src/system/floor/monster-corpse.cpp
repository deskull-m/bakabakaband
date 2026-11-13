#include "system/floor/monster-corpse.h"
#include "locale/language-switcher.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include <fmt/format.h>

/*!
 * @brief 空の死体を作る
 * @details **モンスター用のプロファイルを必ず初期化すること**。
 *          FloorType のコンストラクタが m_list の各要素に対して行っているのと
 *          同じ初期化で、これを忘れるとセーブデータ読込 (rd_monster) が
 *          tl::bad_optional_access で落ちる。
 */
MonsterCorpse::MonsterCorpse()
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
{
}

/*!
 * @brief 有効な死体かどうか
 * @details 種族が引けない (= 破棄済み or 未初期化) なら無効とする。
 */
bool MonsterCorpse::is_valid() const
{
    return this->body.is_valid();
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
    return fmt::format(_("{}の死体", "corpse of {}"), name);
}
