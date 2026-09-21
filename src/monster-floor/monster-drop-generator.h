/*!
 * @file monster-drop-generator.h
 * @brief モンスター生成時にドロップ品を所持品として生成する処理
 */

#pragma once

#include <set>
#include <tl/optional.hpp>

class CreatureEntity;
class ItemEntity;
class MonraceDropKind;

/*!
 * @brief モンスターの一般ドロップ品 (drop_flags に基づくアイテム/金) を
 *        生成し、そのモンスターの所持品 (inventory) に追加する。
 * @param player プレイヤーへの参照 (生成基準・フロア取得用)
 * @param monster ドロップ品を持たせる対象モンスター
 * @details 従来は monster_death() 時に生成・床散布していた一般ドロップを、
 *          モンスター生成時に所持品として前生成する方式へ移行したもの。
 *          死亡時は drop_all_inventory() により所持品ごと床へ放出される。
 *          固定アーティファクト・クエスト品・死体ドロップ等の特殊処理は
 *          引き続き死亡時に行う。
 */
void generate_monster_drop_items(CreatureEntity &player, CreatureEntity &monster);

/*!
 * @brief SOLDIER / WARRIOR フラグを持つモンスターに初期装備の近接武器を持たせる。
 * @param monster 対象モンスター
 * @details 兵士・戦士は得物を携えているのが自然なため、生成時に種族レベル相応の
 *          近接武器を 1 本与えて利き手に装備させる。体構造的に武器を持てない
 *          個体 (四足・不定形・非実体等) と、既に利き手が埋まっている個体は対象外。
 *          **モンスターの装備武器は近接ダメージに加算される**
 *          (`calc_weapon_melee_damage`) ため、これは SOLDIER / WARRIOR 持ち
 *          モンスターへのバランス変更を伴う。武器の格は
 *          `decide_initial_weapon()` のレベル帯テーブルで調整すること。
 *          一般ドロップ (`generate_monster_drop_items`) より先に呼び、
 *          利き手を初期武器が確保できるようにする。
 */
void equip_armed_monster_initial_weapon(CreatureEntity &monster);

/*!
 * @brief ARCHER / RANGER フラグを持つモンスターに初期装備の弓を持たせる。
 * @param monster 対象モンスター
 * @details 射手は得物として弓を携えているのが自然なため、生成時に種族レベル相応の
 *          弓を 1 張り与えて射撃スロットに装備させる。体構造的に弓を構えられない
 *          個体と、既に射撃スロットが埋まっている個体は対象外。
 *          **モンスターの装備弓の倍率は射撃 (MonsterAbilityType::SHOOT) ダメージに
 *          反映される** (`monspell_damage`) ため、これは ARCHER / RANGER 持ち
 *          モンスターへのバランス変更を伴う。弓の格は `decide_initial_bow()` の
 *          レベル帯テーブルで、倍率の効き方は `monspell_damage()` の
 *          `MONSTER_SHOOT_BASELINE_MAGNIFICATION` で調整すること。
 *          近接武器と別スロットのため、WARRIOR と ARCHER を併せ持つ個体は
 *          近接武器・弓の双方を装備する。
 */
void equip_ranged_monster_initial_bow(CreatureEntity &monster);

/*!
 * @brief MAGE フラグを持つモンスターに初期装備の軽装 (胴体防具) を着せる。
 * @param monster 対象モンスター
 * @details 魔術師が裸同然でいるのは不自然なため、生成時に種族レベル相応の
 *          軽装 (ローブ〜軽い革鎧) を 1 着与えて胴体に装備させる。体構造的に
 *          胴体防具を着られない個体と、既に胴体スロットが埋まっている個体は
 *          対象外。**モンスターの装備防具の AC は `CreatureEntity::get_ac()`
 *          で集計される**ため、これは MAGE 持ちモンスターへのバランス変更
 *          (AC 最大 +5) を伴う。防具の格は `decide_initial_robe()` のレベル帯
 *          テーブルで調整すること。
 */
void equip_spellcaster_monster_initial_robe(CreatureEntity &monster);

/*!
 * @brief 種族の武装度 (`MonraceDefinition::get_armament_level()`) を予算として
 *        モンスターの装備スロットを埋める。
 * @param monster 対象モンスター
 * @details 武装度は「生成時にどれだけ上質な装備を与えるか」を表す種族固有値
 *          (未指定なら `monrace.level * 50`)。これを購買力とみなし、体構造的に
 *          装備でき、かつまだ空いているスロットへ予算内で最も上等な品を与える。
 *          予算は残りスロット数で等分して割り当て、余りは次のスロットへ繰り越す
 *          ため、武装度が高いほど全身が上等になる。
 *          役割装備 (SOLDIER / WARRIOR の近接武器、ARCHER / RANGER の弓、
 *          MAGE の軽装) で既に埋まったスロットはそのまま残し、その価値を予算から
 *          差し引くため二重取りにならない。よって**役割装備の 3 関数より後**、
 *          一般ドロップ (`generate_monster_drop_items`) より前に呼ぶこと。
 *          **モンスターの装備は AC (`CreatureEntity::get_ac()`) と近接ダメージ
 *          (`calc_weapon_melee_damage`) に反映される**ため、これは装備可能な体構造を
 *          持つ全モンスターへのバランス変更を伴う。調整は武装度そのもの
 *          (JSON `armament_level` / `ARMAMENT_LEVEL_PER_LEVEL`) と
 *          `get_armament_slot_entries()` の対象スロット表で行うこと。
 */
void equip_monster_by_armament_budget(CreatureEntity &monster);

/*!
 * @brief 固定アイテム指定から実際に生成するベースアイテムIDを決める
 * @param creature 生成基準となるクリーチャへの参照 (深度加重抽選時の階取得用)
 * @param entry 固定アイテム指定
 * @param is_itemkind entry がアイテム種別指定 (`*_tvals`) なら true
 * @return ベースアイテムID。生成すべき候補が無い場合は 0
 * @details ベースアイテムID指定 (`*_kinds`) はその値をそのまま返す。アイテム種別指定
 *          (`*_tvals`) は既定で当該種別の全 sval から一様に 1 つ選ぶ。候補が 1 つも
 *          無い種別は reader (`RaceReader::set_mon_equip_tvals` 等) が読込時に弾いているため、
 *          ここで例外が飛ぶことはない。
 *          `use_allocation_table` が立っていれば代わりに `make_object` と同じ深度加重
 *          アロケーションテーブルで選び、生成階に候補が無ければ 0 を返す。
 */
short resolve_fixed_item_bi_id(CreatureEntity &creature, const MonraceDropKind &entry, bool is_itemkind);

/*!
 * @brief 固定アイテムの等級 (grade) に応じたアイテム魔法を適用する
 * @param creature 生成基準となるクリーチャー (フロア階層の取得に使う)
 * @param item 対象アイテム
 * @param entry 固定アイテム指定 (等級 -2:呪い〜2:優良、3:特別と固定アーティファクト許可を読む)
 * @details 生成時装備 (`equip_*`) と死亡時ドロップ (`drop_*`) で品質が変わらないよう、
 *          両経路で本関数を共用する。
 */
void apply_drop_kind_magic(CreatureEntity &creature, ItemEntity &item, const MonraceDropKind &entry);

/*!
 * @brief 固定アイテム指定 1 件分のアイテムを生成する
 * @param creature 生成基準となるクリーチャー (フロア階層の取得に使う)
 * @param entry 固定アイテム指定
 * @param is_itemkind entry がアイテム種別指定 (`*_tvals`) なら true
 * @return 生成したアイテム。生成できなかった場合は tl::nullopt
 * @details 生成時装備 (`equip_*`) と死亡時ドロップ (`drop_*`) で品質が変わらないよう、
 *          ベースアイテムの決定と魔法的強化の適用を両経路で本関数に集約する。
 *          `apply_magic` が false の指定では `grade` を無視してベースアイテムのまま返す。
 */
tl::optional<ItemEntity> generate_fixed_item(CreatureEntity &creature, const MonraceDropKind &entry, bool is_itemkind);

/*!
 * @brief 排他グループ (`exclusive_group`) を考慮して 1 件分の発火可否を判定する
 * @param entry 固定アイテム指定
 * @param fired_groups 既に発火したグループ番号の集合 (発火時に更新する)
 * @param dun_level 生成階 (`min_dun_level` の浅階ガード判定に使う)
 * @return 発火させるなら true
 * @details 同じグループ番号のエントリはリスト順に評価し、最初に確率抽選が当たった
 *          ものだけを採用する (先勝ちカスケード)。グループ番号 0 は従来どおり
 *          エントリごとに独立して抽選する。
 *          `min_dun_level` で弾かれたエントリはグループを消費しない。
 */
bool roll_fixed_item_entry(const MonraceDropKind &entry, std::set<int> &fired_groups, int dun_level);

/*!
 * @brief 生成時装備指定 (`equip_kinds` / `equip_tvals`) のアイテムをモンスターに持たせる。
 * @param player プレイヤーへの参照 (アイテム生成基準)
 * @param monster 対象モンスター
 * @details 「その装備を身に着けている」と定義されたモンスターに、生成時点で
 *          materialize したアイテムを渡す。装備できるスロットが空いていれば装備し、
 *          体構造的に装備できない・スロットが埋まっている場合は所持品に入る。
 *          いずれにせよ**撃破すれば `drop_all_inventory()` で床へ落ちる**。
 *          対して `drop_kinds` / `drop_tvals` は生成時には存在せず、死亡時に初めて
 *          生成される (死体から剥ぐ素材など、本人が所持しているとは限らないもの)。
 *          種族固有の「らしさ」を最優先するため、**役割装備・武装度充填より前**に
 *          呼ぶこと。装備した分の価値は武装度予算から差し引かれる。
 */
void equip_monster_fixed_items(CreatureEntity &player, CreatureEntity &monster);
