/*!
 * @file monster-drop-generator.cpp
 * @brief モンスター生成時にドロップ品を所持品として生成する処理の実装
 */

#include "monster-floor/monster-drop-generator.h"
#include "artifact/random-art-generator.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/trg-types.h"
#include "object/object-info.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-system.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/civilization-level-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/services/baseitem-monrace-service.h"
#include "term/z-rand.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace {
/*!
 * @brief drop_flags からアイテム生成品質モード (AM_GOOD 等) を決める
 */
BIT_FLAGS decide_drop_quality_mode(const MonraceDefinition &monrace)
{
    BIT_FLAGS mode = 0L;
    if (monrace.drop_flags.has(MonsterDropType::DROP_GOOD)) {
        mode |= AM_GOOD;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_GREAT)) {
        mode |= (AM_GOOD | AM_GREAT);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_NASTY)) {
        mode |= AM_NASTY;
    }
    return mode;
}

/*!
 * @brief drop_flags からドロップ個数を決める
 * @details monster_death() 時の decide_drop_numbers() と同等。生成時点で
 *          確定しているクローン/ペット/アリーナ等の抑制条件も反映する。
 */
int decide_drop_numbers(const CreatureEntity &monster, const MonraceDefinition &monrace, bool inside_arena)
{
    int drop_numbers = 0;
    if (monrace.drop_flags.has(MonsterDropType::DROP_60) && evaluate_percent(60)) {
        drop_numbers++;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_90) && evaluate_percent(90)) {
        drop_numbers++;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_1D2)) {
        drop_numbers += Dice::roll(1, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_2D2)) {
        drop_numbers += Dice::roll(2, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_3D2)) {
        drop_numbers += Dice::roll(3, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_4D2)) {
        drop_numbers += Dice::roll(4, 2);
    }

    const auto cloned = monster.has_constant_flag(MonsterConstantFlagType::CLONED);
    if (cloned && monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        drop_numbers = 0;
    }
    if (monster.is_pet() || AngbandSystem::get_instance().is_phase_out() || inside_arena) {
        drop_numbers = 0;
    }
    if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY) && (monrace.r_akills > 1024)) {
        drop_numbers = 0;
    }
    return drop_numbers;
}

/*!
 * @brief 武装モンスターのレベル帯ごとの近接武器候補
 * @details 先頭から順に `monrace.level >= min_level` で最初に一致した段を使い、
 *          その候補から 1 つを等確率で選ぶ。格が上がるほど上等な得物になる。
 *          **装備武器の打撃ダイスはモンスターの近接ダメージへ加算される**ため、
 *          この表がそのまま SOLDIER / WARRIOR 持ちモンスターの強化量になる。
 *          バランス調整はここで行うこと。
 */
struct InitialWeaponTier {
    int min_level; //!< この段が適用される最低種族レベル
    std::vector<BaseitemKey> candidates; //!< 等確率で選ぶ武器候補
};

const std::vector<InitialWeaponTier> &get_initial_weapon_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 60, { { ItemKindType::SWORD, SV_EXECUTIONERS_SWORD }, { ItemKindType::POLEARM, SV_HEAVY_LANCE }, { ItemKindType::HAFTED, SV_GREAT_HAMMER } } },
        { 40, { { ItemKindType::SWORD, SV_TWO_HANDED_SWORD }, { ItemKindType::POLEARM, SV_LOCHABER_AXE }, { ItemKindType::POLEARM, SV_GREAT_AXE } } },
        { 30, { { ItemKindType::SWORD, SV_KATANA }, { ItemKindType::POLEARM, SV_HALBERD }, { ItemKindType::POLEARM, SV_BATTLE_AXE } } },
        { 20, { { ItemKindType::SWORD, SV_LONG_SWORD }, { ItemKindType::POLEARM, SV_BROAD_SPEAR }, { ItemKindType::POLEARM, SV_BROAD_AXE } } },
        { 10, { { ItemKindType::SWORD, SV_TULWAR }, { ItemKindType::POLEARM, SV_AWL_PIKE }, { ItemKindType::HAFTED, SV_MACE } } },
        { 5, { { ItemKindType::SWORD, SV_SHORT_SWORD }, { ItemKindType::POLEARM, SV_SPEAR }, { ItemKindType::HAFTED, SV_WHIP } } },
        { 0, { { ItemKindType::SWORD, SV_DAGGER }, { ItemKindType::HAFTED, SV_CLUB }, { ItemKindType::POLEARM, SV_SICKLE } } },
    };

    return tiers;
}

/*!
 * @brief 和風武器を使うモンスター (SAMURAI) のレベル帯ごとの近接武器候補
 * @details 刀・薙刀・鉄棒といった和風の得物だけで構成した、
 *          `get_initial_weapon_tiers()` と対になる表。段の切れ目と各段の打撃
 *          ダイス期待値を西洋武器の表へ揃えてあるため、**和風化による強弱の
 *          変化はほとんど無い** (段ごとの期待値: 3.0 / 4.0 / 5.3 / 6.5 / 7.5 /
 *          12.5 に対し、西洋側は 3.0 / 3.7 / 4.8 / 6.0 / 8.5 / 11.3)。
 *          バランス調整はここで行うこと。
 *
 *          **候補は刀・薙刀・鉄棒といった侍の得物に絞り、ヌンチャク・錫杖・
 *          三節棍のような武術家・僧の得物は和風でも含めない** (虎眼流の入門者が
 *          ヌンチャクを振るような不整合を避ける)。
 *
 *          **最上段が Lv40 で止まるのは、和風武器の打撃ダイスが野太刀 (5d4) で
 *          打ち止めになるため**。西洋側の Lv60 段 (首切りソード 4d5 /
 *          ヘヴィ・ランス 4d8 / グレート・ハンマー 4d6、期待値 14.7) に相当する
 *          和風武器が実データに無いので、Lv60 以上の個体も野太刀を持つ
 *          (隼の剣 1d6 は速度特化で打撃ダイスが小さく、青龍刀 4d5 は中華風の
 *          ため候補に含めない)。
 */
const std::vector<InitialWeaponTier> &get_initial_samurai_weapon_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 40, { { ItemKindType::SWORD, SV_NO_DACHI } } },
        { 30, { { ItemKindType::SWORD, SV_KATANA }, { ItemKindType::POLEARM, SV_NAGINATA }, { ItemKindType::HAFTED, SV_TETSUBO } } },
        { 20, { { ItemKindType::SWORD, SV_KATANA }, { ItemKindType::SWORD, SV_WAKIZASHI }, { ItemKindType::POLEARM, SV_NAGINATA } } },
        { 10, { { ItemKindType::SWORD, SV_WAKIZASHI }, { ItemKindType::SWORD, SV_NINJATO }, { ItemKindType::HAFTED, SV_BO_STAFF } } },
        { 5, { { ItemKindType::SWORD, SV_TANTO }, { ItemKindType::SWORD, SV_WAKIZASHI } } },
        { 0, { { ItemKindType::SWORD, SV_TANTO } } },
    };

    return tiers;
}

/*!
 * @brief 忍者系モンスター (NINJA) のレベル帯ごとの近接武器候補
 * @details **職業忍者が使える武器だけで構成した表**。職業忍者の武器熟練度
 *          (`lib/edit/ClassSkillDefinitions.jsonc` の id 26) で SKILLED 以上に
 *          なれるのは ダガー / 短刀 / 忍者刀 / 毒針 (MASTER) と マン・ゴーシュ /
 *          レイピア / スモール・ソード / ショート・ソード / 脇差し / 隼の剣
 *          (SKILLED) で、**いずれも刀剣 (SWORD) の軽量な短刀類**。それ以外は
 *          `is_icky_wield()` 相当の不得手な得物になる。
 *
 *          **鈍器・長柄 (HAFTED / POLEARM) は入れない**。`heavy_armor()`
 *          (`src/player-info/equipment-info.cpp`) は忍者の場合「刀剣より重い
 *          分類の手持ち武器の重量」も防具重量に合算するため、鈍器や長柄を持つと
 *          それだけで重装備判定に近付き、職業忍者なら加速・隠密のペナルティを
 *          受ける組み合わせになる。
 *
 *          **最上段が Lv20 で止まるのは、職業忍者が扱える武器の打撃ダイスが
 *          忍者刀 (1d9) / 脇差し (2d4) = 期待値 5.0 で打ち止めになるため**。
 *          隼の剣 (1d6) は Lv55 の忍者の象徴的な得物だが打撃ダイスはさらに小さく、
 *          価値 3500G が武装度予算を丸ごと食い潰すので候補に含めない。
 *          毒針 (1d1) も打撃ダイスが無いので除く。
 */
const std::vector<InitialWeaponTier> &get_initial_ninja_weapon_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 20, { { ItemKindType::SWORD, SV_NINJATO }, { ItemKindType::SWORD, SV_WAKIZASHI } } },
        { 10, { { ItemKindType::SWORD, SV_TANTO }, { ItemKindType::SWORD, SV_SHORT_SWORD }, { ItemKindType::SWORD, SV_WAKIZASHI } } },
        { 5, { { ItemKindType::SWORD, SV_TANTO }, { ItemKindType::SWORD, SV_MAIN_GAUCHE }, { ItemKindType::SWORD, SV_SHORT_SWORD } } },
        { 0, { { ItemKindType::SWORD, SV_DAGGER }, { ItemKindType::SWORD, SV_TANTO } } },
    };

    return tiers;
}

/*!
 * @brief 役割装備で使う近接武器の系統
 */
enum class InitialWeaponStyle {
    WESTERN, //!< SOLDIER / WARRIOR の西洋武器
    SAMURAI, //!< SAMURAI の和風武器
    NINJA, //!< NINJA の軽量な短刀類
};

/*!
 * @brief 種族フラグから近接武器の系統を決める
 * @details 併せ持つ個体は **NINJA > SAMURAI > SOLDIER / WARRIOR** の順で決まる。
 *          NINJA を最優先にするのは、忍者の得物制限が「重い武器を持つと重装備
 *          判定に近付く」という**制約**であり、侍の和風武器が単なる**好み**で
 *          あるのに対して破ると噛み合わなくなるため。
 */
InitialWeaponStyle decide_initial_weapon_style(const MonraceDefinition &monrace)
{
    if (monrace.kind_flags.has(MonsterKindType::NINJA)) {
        return InitialWeaponStyle::NINJA;
    }

    if (monrace.kind_flags.has(MonsterKindType::SAMURAI)) {
        return InitialWeaponStyle::SAMURAI;
    }

    return InitialWeaponStyle::WESTERN;
}

/*!
 * @brief レベル帯テーブルから種族レベルに応じた 1 品を選ぶ
 * @param tiers レベル帯テーブル (先頭から順に `level >= min_level` で判定)
 * @param level モンスター種族のレベル
 * @param fallback どの段にも一致しなかった場合に返すベースアイテムキー
 * @return 選ばれたベースアイテムキー
 * @details 近接武器・弓・軽装・和風武器の 4 テーブルで共通の走査処理。
 *          どのテーブルも最下段の `min_level` が 0 なので通常 fallback は使わない。
 */
BaseitemKey decide_initial_item(const std::vector<InitialWeaponTier> &tiers, int level, const BaseitemKey &fallback)
{
    for (const auto &tier : tiers) {
        if (level < tier.min_level) {
            continue;
        }

        return rand_choice(tier.candidates);
    }

    return fallback;
}

/*!
 * @brief 種族レベルに応じた初期武器を 1 つ選ぶ
 * @param level モンスター種族のレベル
 * @param style 使う武器の系統 (`decide_initial_weapon_style()` が決める)
 * @return 選ばれた武器のベースアイテムキー
 */
BaseitemKey decide_initial_weapon(int level, InitialWeaponStyle style)
{
    switch (style) {
    case InitialWeaponStyle::SAMURAI:
        return decide_initial_item(get_initial_samurai_weapon_tiers(), level, { ItemKindType::SWORD, SV_TANTO });
    case InitialWeaponStyle::NINJA:
        return decide_initial_item(get_initial_ninja_weapon_tiers(), level, { ItemKindType::SWORD, SV_DAGGER });
    case InitialWeaponStyle::WESTERN:
    default:
        return decide_initial_item(get_initial_weapon_tiers(), level, { ItemKindType::SWORD, SV_DAGGER });
    }
}

/*!
 * @brief 射手モンスターのレベル帯ごとの射撃武器 (弓) 候補
 * @details 近接武器と同じく、先頭から順に `monrace.level >= min_level` で最初に
 *          一致した段の候補から 1 つを等確率で選ぶ。
 *          **装備した弓の倍率はモンスターの射撃ダメージに反映される**
 *          (`monspell_damage()`) ため、この表がそのまま ARCHER / RANGER 持ち
 *          モンスターの射撃強化量になる。バランス調整はここで行うこと。
 */
const std::vector<InitialWeaponTier> &get_initial_bow_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 40, { { ItemKindType::BOW, SV_HEAVY_XBOW }, { ItemKindType::BOW, SV_LONG_BOW } } },
        { 25, { { ItemKindType::BOW, SV_LONG_BOW }, { ItemKindType::BOW, SV_LIGHT_XBOW } } },
        { 10, { { ItemKindType::BOW, SV_SHORT_BOW } } },
        { 0, { { ItemKindType::BOW, SV_SLING } } },
    };

    return tiers;
}

/*!
 * @brief 種族レベルに応じた初期弓を 1 つ選ぶ
 * @param level モンスター種族のレベル
 * @return 選ばれた弓のベースアイテムキー
 */
BaseitemKey decide_initial_bow(int level)
{
    return decide_initial_item(get_initial_bow_tiers(), level, { ItemKindType::BOW, SV_SLING });
}

/*!
 * @brief 魔術師モンスターのレベル帯ごとの軽装 (胴体防具) 候補
 * @details 近接武器・弓と同じく、先頭から順に `monrace.level >= min_level` で
 *          最初に一致した段の候補から 1 つを等確率で選ぶ。
 *          魔術師が裸同然でいるのは不自然なため、ローブを基本としつつ格に応じて
 *          軽い革鎧までを与える。**モンスターの装備防具の AC は `get_ac()` で
 *          集計される**ため、この表がそのまま MAGE 持ちモンスターの AC 上昇量
 *          (最大 +5) になる。バランス調整はここで行うこと。
 */
const std::vector<InitialWeaponTier> &get_initial_robe_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 40, { { ItemKindType::SOFT_ARMOR, SV_SOFT_STUDDED_LEATHER }, { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR } } },
        { 15, { { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR }, { ItemKindType::SOFT_ARMOR, SV_ROBE } } },
        { 0, { { ItemKindType::SOFT_ARMOR, SV_ROBE } } },
    };

    return tiers;
}

/*!
 * @brief 種族レベルに応じた初期軽装を 1 つ選ぶ
 * @param level モンスター種族のレベル
 * @return 選ばれた胴体防具のベースアイテムキー
 */
BaseitemKey decide_initial_robe(int level)
{
    return decide_initial_item(get_initial_robe_tiers(), level, { ItemKindType::SOFT_ARMOR, SV_ROBE });
}

/*!
 * @brief 武装度予算で埋める装備スロットと、そこへ装備できるアイテム種別
 * @details 先頭の要素から順に予算を割り当てるため、**重要な部位ほど先に**並べる。
 *          指輪 (INVEN_MAIN_RING / SUB_RING)・首飾り (INVEN_NECK) は呪い付きや
 *          効果がランダムな品が多く、光源 (INVEN_LITE) は燃料の設定が必要なため
 *          初版では対象外。射撃 (INVEN_BOW) も、射撃能力を持たない個体に弓だけ
 *          持たせても無意味なので ARCHER / RANGER の役割装備に任せて対象外とする。
 */
struct ArmamentSlotEntry {
    int slot; //!< 装備スロット (INVEN_*)
    std::vector<ItemKindType> tvals; //!< そのスロットへ装備できるアイテム種別
};

const std::vector<ArmamentSlotEntry> &get_armament_slot_entries()
{
    static const std::vector<ArmamentSlotEntry> entries = {
        { INVEN_BODY, { ItemKindType::SOFT_ARMOR, ItemKindType::HARD_ARMOR } },
        { INVEN_MAIN_HAND, { ItemKindType::SWORD, ItemKindType::POLEARM, ItemKindType::HAFTED } },
        { INVEN_HEAD, { ItemKindType::HELM, ItemKindType::CROWN } },
        { INVEN_SUB_HAND, { ItemKindType::SHIELD } },
        { INVEN_FEET, { ItemKindType::BOOTS } },
        { INVEN_ARMS, { ItemKindType::GLOVES } },
        { INVEN_OUTER, { ItemKindType::CLOAK } },
    };

    return entries;
}

/*!
 * @brief 忍者系モンスター (NINJA) が武装度予算で埋める装備スロット
 * @details 職業忍者の装備制約に合わせて既定の表から 2 点を変える。
 *
 *          **モンスターにペナルティが生じるわけではない**点に注意。
 *          `CreatureClass::equals()` は非プレイヤーで常に false を返すため、
 *          下記の `heavy_armor()` / `has_ninja_speed()` はモンスターでは発火しない。
 *          それでも職業忍者の基準に揃えるのは、見た目の不整合 (忍者が大盾と重鎧で
 *          固めている) を避け、C1 の `player_class` 付与や
 *          `player_birth_as_monster` で同じ基準が効くようにしておくため。
 *
 *          **必ず `get_armament_slot_entries()` の部分集合であること**
 *          (購入候補のキャッシュ `get_armament_candidates()` は既定の表から
 *          作るため、既定に無いスロットや種別を足しても候補が引けない)。
 *
 *          - **逆手 (盾) を外す**: 盾を持つと `has_ninja_speed()`
 *            (`src/player-base/player-class.cpp`) が偽になり、職業忍者なら
 *            加速・隠密のボーナスを失う。忍者は盾を使わない。
 *          - **胴は軽装 (SOFT_ARMOR) のみ**: `heavy_armor()` の重量判定
 *            (防具重量 > 100 + レベル×4) に引っ掛かると、職業忍者は加速・隠密に
 *            ペナルティを受ける。重鎧 (HARD_ARMOR) は買わせない。
 *          - **利き手は刀剣のみ**: 役割装備で既に埋まるため通常は使われないが、
 *            万一空いていても鈍器・長柄を買わないようにする
 *            (理由は `get_initial_ninja_weapon_tiers()` 参照)。
 */
const std::vector<ArmamentSlotEntry> &get_ninja_armament_slot_entries()
{
    static const std::vector<ArmamentSlotEntry> entries = {
        { INVEN_BODY, { ItemKindType::SOFT_ARMOR } },
        { INVEN_MAIN_HAND, { ItemKindType::SWORD } },
        { INVEN_HEAD, { ItemKindType::HELM, ItemKindType::CROWN } },
        { INVEN_FEET, { ItemKindType::BOOTS } },
        { INVEN_ARMS, { ItemKindType::GLOVES } },
        { INVEN_OUTER, { ItemKindType::CLOAK } },
    };

    return entries;
}

/*!
 * @brief 修行僧系モンスター (KARATEKA) が武装度予算で埋める装備スロット
 * @details 既定の表から**利き手・逆手を外した**もの。素手で戦う者に武器を
 *          買わせないための表で、他の 5 部位 (胴・頭・脚・腕・体の上) は
 *          既定どおり充填する。
 *
 *          **必ず `get_armament_slot_entries()` の部分集合であること**
 *          (理由は `get_ninja_armament_slot_entries()` のコメント参照)。
 *
 *          **根拠はプレイヤーの `MONK` 側の実装にある**。
 *          `is_martial_arts_mode()` (`src/player/player-status.cpp`) は
 *          `empty_hands()` の `EMPTY_HAND_MAIN` を要求するため、**利き手に
 *          何かを持つと素手打撃 (martial arts) を失う**。モンスターの近接は
 *          `CreatureEntity::select_melee_weapon_slot()` が `PUNCH` でも手の
 *          武器を使うので、利き手を空けておかないと素手連撃すべてに武器
 *          ダイスが乗ってしまう。
 *
 *          - **利き手を外す**: 上記のとおりプレイヤー `MONK` の素手打撃条件が
 *            「利き手が空」なので、修行僧は利き手を空けておく。
 *          - **逆手も外す**: プレイヤー側は `can_attack_with_sub_hand()` が
 *            近接武器のみを見るため**盾は素手打撃を壊さない**が、修行僧が盾を
 *            構えているのは見た目の不整合なので併せて外す
 *            (忍者が盾を外すのと同じ扱い。忍者は `has_ninja_speed()` という
 *            機械的な理由もあるが、こちらはフレーバー上の判断)。
 *          - **胴は軽装 (SOFT_ARMOR) のみ**。プレイヤー `MONK` も
 *            `heavy_armor()` の重量判定 (5 部位の防具重量合計 >
 *            100 + レベル×4) に掛かると加速 (Lv10 以降) と耐麻痺
 *            (Lv25 以降) のボーナスを失い、`PlayerStealth` でも隠密に
 *            ペナルティを受けるため、忍者と同じ扱いで重鎧を外す。
 *            `heavy_armor()` 自体はモンスターでは発火しない
 *            (`CreatureClass::equals()` が常に false) ので、これも忍者の
 *            盾・重鎧制限と同様「整合性のための縛り」になる。
 */
const std::vector<ArmamentSlotEntry> &get_monk_armament_slot_entries()
{
    static const std::vector<ArmamentSlotEntry> entries = {
        { INVEN_BODY, { ItemKindType::SOFT_ARMOR } },
        { INVEN_HEAD, { ItemKindType::HELM, ItemKindType::CROWN } },
        { INVEN_FEET, { ItemKindType::BOOTS } },
        { INVEN_ARMS, { ItemKindType::GLOVES } },
        { INVEN_OUTER, { ItemKindType::CLOAK } },
    };

    return entries;
}

/*!
 * @brief 武装度予算で購入できるベースアイテムか判定する
 * @details 通常のフロア生成に乗らない品 (生成確率が全て 0)・固定アーティファクト
 *          化する品・クエスト専用品・呪い付きの品は除外する。価値 0 の品は
 *          予算の意味が無くなるため除外する。
 */
bool is_purchasable_armament(const BaseitemDefinition &baseitem)
{
    if (!baseitem.is_valid() || (baseitem.cost <= 0)) {
        return false;
    }

    const auto &tables = baseitem.alloc_tables;
    const auto has_allocation = std::any_of(tables.begin(), tables.end(), [](const auto &table) { return table.chance > 0; });
    if (!has_allocation) {
        return false;
    }

    return baseitem.gen_flags.has_none_of({
        ItemGenerationTraitType::INSTA_ART,
        ItemGenerationTraitType::QUESTITEM,
        ItemGenerationTraitType::CURSED,
        ItemGenerationTraitType::HEAVY_CURSE,
        ItemGenerationTraitType::PERMA_CURSE,
    });
}

//! 武装度予算で購入できるベースアイテムの候補 (価値・性能・文明水準付き)
struct ArmamentCandidate {
    BaseitemKey bi_key;
    int cost; //!< 予算から差し引く価値
    int quality; //!< 選定に使う性能 (防具は基本AC、武器は打撃ダイスの最大値)
    CivilizationLevel civilization; //!< 品の文明水準 (扱う側の水準との突き合わせに使う)
};

/*!
 * @brief ベースアイテムの「装備としての性能」を求める
 * @details 装備しても効果が無い品 (AC 0 の衣装や打撃ダイスを持たない品) を
 *          候補から除くために使う。防具は基本AC、武器は打撃ダイスの最大値。
 *          **選定基準には使わない** (理由は `pick_armament` の説明を参照)。
 */
int calc_armament_quality(const BaseitemDefinition &baseitem)
{
    switch (baseitem.bi_key.tval()) {
    case ItemKindType::SWORD:
    case ItemKindType::POLEARM:
    case ItemKindType::HAFTED:
        return baseitem.damage_dice.maxroll() + baseitem.to_d;
    default:
        return baseitem.ac + baseitem.to_a;
    }
}

/*!
 * @brief スロットごとの購入候補一覧を得る (初回呼出時にベースアイテム表から構築)
 * @details ベースアイテム表はゲーム初期化時に確定するため、初回の呼出で構築して
 *          以降は使い回す。モンスター生成のたびに全ベースアイテムを走査しない。
 */
const std::map<int, std::vector<ArmamentCandidate>> &get_armament_candidates()
{
    static const auto candidates = [] {
        std::map<int, std::vector<ArmamentCandidate>> result;
        const auto &baseitems = BaseitemList::get_instance();
        for (const auto &entry : get_armament_slot_entries()) {
            auto &slot_candidates = result[entry.slot];
            for (const auto &baseitem : baseitems) {
                const auto tval = baseitem.bi_key.tval();
                if (std::find(entry.tvals.begin(), entry.tvals.end(), tval) == entry.tvals.end()) {
                    continue;
                }
                if (!is_purchasable_armament(baseitem)) {
                    continue;
                }

                const auto quality = calc_armament_quality(baseitem);
                if (quality <= 0) {
                    continue;
                }

                slot_candidates.push_back({ baseitem.bi_key, baseitem.cost, quality, baseitem.civilization });
            }
        }

        return result;
    }();

    return candidates;
}

/*!
 * @brief 予算内で最も上等な装備を 1 つ選ぶ
 * @param entry 対象の装備スロットと、そこへ装備できるアイテム種別
 * @param budget このスロットに使える上限価値
 * @param civilization 装備する側の文明水準 (この水準を超える品は候補から除く)
 * @return 選ばれた候補。予算内の候補が無ければ tl::nullopt
 * @details 予算内で最も高価な品の価値を基準とし、その半額以上の候補から等確率で
 *          選ぶ。価値を選定基準にすることで「武装度が高いほど上質な装備」が
 *          単調に成り立ち (実データで防具AC計 17→42 / 武器ダイス 5→20)、同レベル帯の
 *          個体が全て同じ装備になることも避けられる。
 *          **性能 (`calc_armament_quality`) を選定基準にしてはならない。**
 *          本データには安価で高性能なフレーバー品 (鉄の甲羅 AC22 が 1G 等) が
 *          あるため、性能で選ぶと予算が効かず低レベル個体まで防具AC計 41〜86 に
 *          達してしまう。性能値は「装備しても無意味な AC0 の品」を候補から
 *          除くためだけに使う。
 */
tl::optional<ArmamentCandidate> pick_armament(const ArmamentSlotEntry &entry, int budget, CivilizationLevel civilization)
{
    const auto &candidates_map = get_armament_candidates();
    const auto it = candidates_map.find(entry.slot);
    if (it == candidates_map.end()) {
        return tl::nullopt;
    }

    // キャッシュは既定のスロット表から作るため、忍者用の表のように種別を狭めた
    // 呼出では、そのスロットの候補をさらに絞り込む。
    // 文明水準の絞り込みも**ここ (best_cost の算出より前) で行う**。後段で弾くと
    // 「予算内で最高価な品の半額以上」の帯が空になり、そのスロットが裸になってしまう。
    // 前段で除けば帯が 1 つ下の品へ自動的に落ちる。
    const auto is_allowed = [&entry, civilization](const ArmamentCandidate &candidate) {
        if (std::find(entry.tvals.begin(), entry.tvals.end(), candidate.bi_key.tval()) == entry.tvals.end()) {
            return false;
        }

        return is_civilization_usable(candidate.civilization, civilization);
    };

    auto best_cost = 0;
    for (const auto &candidate : it->second) {
        if (is_allowed(candidate) && (candidate.cost <= budget) && (candidate.cost > best_cost)) {
            best_cost = candidate.cost;
        }
    }

    if (best_cost <= 0) {
        return tl::nullopt;
    }

    std::vector<ArmamentCandidate> affordable;
    for (const auto &candidate : it->second) {
        if (is_allowed(candidate) && (candidate.cost <= budget) && (candidate.cost * 2 >= best_cost)) {
            affordable.push_back(candidate);
        }
    }

    return rand_choice(affordable);
}

/*!
 * @brief 既に装備しているアイテムの価値の合計を求める
 * @details 役割装備 (SOLDIER / WARRIOR の近接武器、ARCHER / RANGER の弓、
 *          MAGE の軽装) で既に埋まっているスロットの分を武装度予算から差し引き、
 *          二重取りにならないようにする。
 */
int calc_equipped_armament_cost(const CreatureEntity &monster)
{
    auto total = 0;
    const auto &baseitems = BaseitemList::get_instance();
    for (auto slot = static_cast<int>(INVEN_MAIN_HAND); slot < static_cast<int>(INVEN_TOTAL); slot++) {
        const auto &item = *monster.inventory[slot];
        if (!item.is_valid()) {
            continue;
        }

        total += baseitems.get_baseitem(item.bi_id).cost;
    }

    return total;
}

/*!
 * @brief モンスターの持ち物として品を与える (正当な所有権を刻んでから渡す)
 * @param monster 持たせる対象のモンスター
 * @param item 与える品
 * @return acquire_item() の戻り値 (格納されたスロット)
 * @details 生成時にそのモンスターの持ち物として用意された品は、その種族が
 *          **正当な所有者**である。プレイヤーが撃破して略奪してもこの刻印は
 *          消えないため、*鑑定* で「本来誰の物か」が判る。
 *          金銭には刻まない (通貨に所有権の記録は意味を持たないため)。
 *          床から拾った品・プレイヤーから盗んだ品を渡す経路 (monster-object.cpp /
 *          monster-eating.cpp) は本ヘルパを通さない。拾得・窃盗で「正当な」
 *          所有権は移らないので、刻印は付けないのが正しい。
 */
int16_t give_item_to_monster(CreatureEntity &monster, ItemEntity &item)
{
    if (item.bi_key.tval() != ItemKindType::GOLD) {
        item.set_rightful_owner(monster.get_r_idx());
    }

    return monster.acquire_item(item);
}
}

void generate_monster_drop_items(CreatureEntity &player, CreatureEntity &monster)
{
    auto &floor = *player.get_floor();
    const auto &monrace = monster.get_monrace();

    const auto do_gold = monrace.drop_flags.has_none_of({
        MonsterDropType::ONLY_ITEM,
        MonsterDropType::DROP_GOOD,
        MonsterDropType::DROP_GREAT,
    });
    auto do_item = monrace.drop_flags.has_not(MonsterDropType::ONLY_GOLD);
    do_item |= monrace.drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });

    auto drop_numbers = decide_drop_numbers(monster, monrace, floor.inside_arena);

    // 金も品物も生成できない指定 (ONLY_GOLD と ONLY_ITEM の同時指定等の矛盾データ) ならドロップ枠を潰す。
    // 旧実装はここが `!do_item && !symbol_char_is_any_of("$")` となっており、ONLY_GOLD を持つ
    // 非 `$` 種族のドロップ個数指定を丸ごと無効化していた (判定対象とシンボル条件がどちらも反転していた)。
    if (!do_gold && !do_item) {
        drop_numbers = 0;
    }
    if (drop_numbers <= 0) {
        return;
    }

    const auto mo_mode = decide_drop_quality_mode(monrace);

    // 死亡時 (monster_death) と同様に、生成基準階をモンスター種族レベルで底上げする。
    const auto backup_object_level = floor.object_level;
    floor.object_level = (floor.dun_level + monrace.level) / 2;

    for (auto i = 0; i < drop_numbers; i++) {
        if (do_gold && (!do_item || one_in_(2))) {
            const auto bi_key = BaseitemMonraceService::lookup_fixed_gold_drop(monrace.drop_flags);
            auto item = floor.make_gold(bi_key);
            // 構成材質に応じて金銭額を増減させる (貴金属系は増、紙・糞は減)。
            const auto gold_percent = monster.get_material_gold_drop_percent();
            if (gold_percent != 100) {
                const auto scaled = static_cast<int>(item.pval) * gold_percent / 100;
                item.pval = static_cast<PARAMETER_VALUE>(std::clamp(scaled, 1, static_cast<int>(std::numeric_limits<PARAMETER_VALUE>::max())));
            }
            give_item_to_monster(monster, item);
        } else {
            if (auto item = make_object(player, mo_mode)) {
                give_item_to_monster(monster, *item);
            }
        }
    }

    floor.object_level = backup_object_level;
}

void equip_armed_monster_initial_weapon(CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();

    // SAMURAI は和風の得物、NINJA は職業忍者と同じ軽量な短刀類を持つ。
    // SOLDIER / WARRIOR と併せ持つ個体でもそれぞれの表を優先する
    // (「侍なのに西洋剣」「忍者なのに大槌」を避ける)。
    const auto style = decide_initial_weapon_style(monrace);
    const auto has_armed_role = monrace.kind_flags.has_any_of({ MonsterKindType::SOLDIER, MonsterKindType::WARRIOR });
    if ((style == InitialWeaponStyle::WESTERN) && !has_armed_role) {
        return;
    }

    // 体構造的に武器を持てない個体 (四足・不定形・非実体等) には持たせない。
    if (!monster.can_equip_to(INVEN_MAIN_HAND)) {
        return;
    }

    // 既に利き手が埋まっているなら何もしない (生成直後は通常空)。
    // 種族定義の equip_kind / equip_tval は本関数より先に走るため、
    // 製作者が名指しした得物はそのまま残る。
    if (monster.inventory[INVEN_MAIN_HAND]->is_valid()) {
        return;
    }

    ItemEntity weapon(decide_initial_weapon(monrace.level, style));
    weapon.number = 1;

    // エゴ・アーティファクト化や強化値は付けない。素の打撃ダイスのみを加える
    // ことで、強化量をレベル帯テーブルの範囲に収める。
    (void)give_item_to_monster(monster, weapon);
}

void equip_ranged_monster_initial_bow(CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    if (monrace.kind_flags.has_none_of({ MonsterKindType::ARCHER, MonsterKindType::RANGER })) {
        return;
    }

    // 体構造的に弓を構えられない個体 (四足・不定形・非実体等) には持たせない。
    if (!monster.can_equip_to(INVEN_BOW)) {
        return;
    }

    // 既に射撃スロットが埋まっているなら何もしない (生成直後は通常空)。
    if (monster.inventory[INVEN_BOW]->is_valid()) {
        return;
    }

    ItemEntity bow(decide_initial_bow(monrace.level));
    bow.number = 1;

    // 近接武器と同様、エゴ・アーティファクト化や強化値は付けない。
    (void)give_item_to_monster(monster, bow);
}

void equip_spellcaster_monster_initial_robe(CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::MAGE)) {
        return;
    }

    // 体構造的に胴体防具を着られない個体 (不定形・非実体等) には持たせない。
    if (!monster.can_equip_to(INVEN_BODY)) {
        return;
    }

    // 既に胴体スロットが埋まっているなら何もしない (生成直後は通常空)。
    if (monster.inventory[INVEN_BODY]->is_valid()) {
        return;
    }

    ItemEntity robe(decide_initial_robe(monrace.level));
    robe.number = 1;

    // 近接武器・弓と同様、エゴ・アーティファクト化や強化値は付けない。
    (void)give_item_to_monster(monster, robe);
}

void equip_monster_by_armament_budget(CreatureEntity &monster)
{
    // 武具を自ら調えない体 (獣・鳥・蟲・蠕虫・不定形・非実体等) は対象外。
    // これらは首・胴・頭のスロットが構造上あっても、自分で防具を買い揃えて
    // 着込む生き物ではないため、武装度由来の装備を一切持たせない
    // (獣が安物の鎧や兜を身に着けて落とすのを防ぐ)。種族定義で明示した
    // equip_kind / equip_tval と役割装備は製作者の明示指定なので別経路で残る。
    if (!monster.is_armament_capable()) {
        return;
    }

    // 職業を持たない個体も対象外。グレムリンのように「武具を使うとは思えない
    // 人型」が武装度だけで鎧兜を着込むのを防ぐため、武装度由来の装備は
    // 「自ら武具を調える職業」(MonraceDefinition::has_armed_profession()) を
    // 持つ個体に限る。JSON で armament_level を明示指定した個体は「その武装度で
    // 武装する」という製作者の意思表示として職業の有無に依らず充填する
    // (レベル由来の既定値しか持たない個体とはここで区別する)。
    const auto &monrace = monster.get_monrace();
    if (!monrace.has_armed_profession() && !monrace.armament_level) {
        return;
    }

    // 役割装備で既に使った分を差し引いた残額が、この個体の購買力になる。
    auto budget = monrace.get_armament_level() - calc_equipped_armament_cost(monster);
    if (budget <= 0) {
        return;
    }

    // 忍者は盾を持たず重鎧も着ないため、修行僧は素手で戦うため、それぞれ専用の
    // スロット表を使う。**優先順位は NINJA > KARATEKA** で、これは
    // decide_initial_weapon_style() の得物の優先順位と揃えてある (実データで
    // 両方を持つのは巨大サイバーワイアーム天使悪魔リッチ 929 の 1 体だけで、
    // 忍者の役割装備が利き手を埋めるため修行僧表との差は逆手だけになる)。
    const auto &slot_entries = [&monrace]() -> const std::vector<ArmamentSlotEntry> & {
        if (monrace.kind_flags.has(MonsterKindType::NINJA)) {
            return get_ninja_armament_slot_entries();
        }

        if (monrace.kind_flags.has(MonsterKindType::KARATEKA)) {
            return get_monk_armament_slot_entries();
        }

        return get_armament_slot_entries();
    }();

    // 体構造的に装備でき、かつまだ空いているスロットだけを対象にする。
    std::vector<const ArmamentSlotEntry *> targets;
    for (const auto &entry : slot_entries) {
        if (!monster.can_equip_to(entry.slot) || monster.inventory[entry.slot]->is_valid()) {
            continue;
        }

        targets.push_back(&entry);
    }

    // 残りスロット数で等分した額を各スロットの上限とし、余りは次のスロットへ繰り越す。
    // これにより 1 部位に予算を使い切らず、武装度が高いほど全身が上等になる。
    for (auto i = 0U; i < targets.size(); i++) {
        const auto slot_budget = budget / static_cast<int>(targets.size() - i);
        const auto candidate = pick_armament(*targets[i], slot_budget, monrace.civilization);
        if (!candidate) {
            continue;
        }

        ItemEntity item(candidate->bi_key);
        item.number = 1;

        // 役割装備と同様、エゴ・アーティファクト化や強化値は付けない。
        (void)give_item_to_monster(monster, item);
        budget -= candidate->cost;
    }
}

void apply_drop_kind_magic(CreatureEntity &creature, ItemEntity &item, const MonraceDropKind &entry)
{
    const auto level = creature.get_floor()->dun_level;

    // allows_fixed_artifact が立つ指定では AM_NO_FIXED_ART を外し、通常の
    // アイテム生成 (make_object) と同様に固定アーティファクト化を許す。
    // grade 3 は元から AM_NO_FIXED_ART を付けないため影響しない。
    const BIT_FLAGS no_fixed_art = entry.allows_fixed_artifact ? 0U : AM_NO_FIXED_ART;
    switch (entry.grade) {
    case -2:
        ItemMagicApplier(creature, &item, level, no_fixed_art | AM_GOOD | AM_GREAT | AM_CURSED).execute();
        return;
    case -1:
        ItemMagicApplier(creature, &item, level, no_fixed_art | AM_GOOD | AM_CURSED).execute();
        return;
    case 0:
        ItemMagicApplier(creature, &item, level, no_fixed_art).execute();
        return;
    case 1:
        ItemMagicApplier(creature, &item, level, no_fixed_art | AM_GOOD).execute();
        return;
    case 2:
        ItemMagicApplier(creature, &item, level, no_fixed_art | AM_GOOD | AM_GREAT).execute();
        return;
    case 3:
        ItemMagicApplier(creature, &item, level, AM_GOOD | AM_GREAT | AM_SPECIAL).execute();
        if (!item.is_fixed_artifact()) {
            become_random_artifact(creature, &item, false);
        }
        return;
    default:
        return;
    }
}

short resolve_fixed_item_bi_id(CreatureEntity &creature, const MonraceDropKind &entry, bool is_itemkind)
{
    if (!is_itemkind) {
        return entry.id;
    }

    // 分類指定は tval を跨ぐので、tval 一致ではなく述語で絞る。
    // kind_is_book / kind_is_good_book と同じ判定で、必ず深度加重抽選になる。
    if (entry.category != MonraceDropCategory::NONE) {
        const auto category = entry.category;
        auto &table = BaseitemAllocationTable::get_instance();
        table.set_restriction([category](short bi_id) {
            const auto &bi_key = BaseitemList::get_instance().get_baseitem(bi_id).bi_key;
            switch (category) {
            case MonraceDropCategory::SPELL_BOOK:
                return bi_key.is_spell_book();
            case MonraceDropCategory::HIGH_LEVEL_BOOK:
                return bi_key.is_high_level_book();
            default:
                return false;
            }
        });
        const auto &floor = *creature.get_floor();
        const auto bi_id = floor.select_baseitem_id(floor.object_level, 0);
        table.reset_restriction();
        return bi_id;
    }

    const auto tval = i2enum<ItemKindType>(entry.id);
    const auto sval_min = entry.sval_min;
    if (entry.use_allocation_table) {
        // 通常のアイテム生成 (make_object) と同じ深度加重抽選を使う。
        // 生成階を超える深度の品は選ばれないため、浅い階で高級品が出ない。
        // 候補が 1 つも無い深度では 0 (= 生成しない) が返る。
        auto &table = BaseitemAllocationTable::get_instance();
        table.set_restriction([tval, sval_min](short bi_id) {
            const auto &bi_key = BaseitemList::get_instance().get_baseitem(bi_id).bi_key;
            return (bi_key.tval() == tval) && (bi_key.sval() >= sval_min);
        });
        const auto &floor = *creature.get_floor();
        const auto bi_id = floor.select_baseitem_id(floor.object_level, 0);
        table.reset_restriction();
        return bi_id;
    }

    if (sval_min > 0) {
        // 下限付きの一様抽選。lookup_baseitem_id() は種別内の全 sval から選ぶため
        // 使えず、候補を自前で絞ってから等確率で選ぶ。
        const auto &baseitems = BaseitemList::get_instance();
        std::vector<short> candidates;
        for (const auto bi_id : baseitems.collect_valid_bi_ids()) {
            const auto &bi_key = baseitems.get_baseitem(bi_id).bi_key;
            if ((bi_key.tval() == tval) && (bi_key.sval() >= sval_min)) {
                candidates.push_back(bi_id);
            }
        }

        // 候補ゼロは reader が弾いているため、ここで空にはならない。
        return candidates.empty() ? 0 : rand_choice(candidates);
    }

    // sval を省略 (tl::nullopt) すると当該種別の中から無作為にベースアイテムが選ばれる。
    // 0 を渡すと「sval 0 のベースアイテム」の完全一致検索になり、SWORD 等
    // sval 0 が存在しない種別で例外を投げるので注意。
    // 候補ゼロの種別は reader が弾いているため、ここで例外にはならない。
    return BaseitemList::get_instance().lookup_baseitem_id(BaseitemKey(tval));
}

tl::optional<ItemEntity> generate_fixed_item(CreatureEntity &creature, const MonraceDropKind &entry, bool is_itemkind)
{
    // 深度加重抽選 (use_allocation_table) で候補が無かった場合は 0 が返るので生成しない。
    const auto bi_id = resolve_fixed_item_bi_id(creature, entry, is_itemkind);
    if (bi_id == 0) {
        return tl::nullopt;
    }

    ItemEntity item;
    item.generate(bi_id);

    // apply_magic: false はハードコーディング由来の「素のアイテム」指定。
    // grade を無視し、エゴ・アーティファクト化・呪いのいずれも起こさない。
    if (entry.apply_magic) {
        apply_drop_kind_magic(creature, item, entry);
    }

    return item;
}

bool roll_fixed_item_entry(const MonraceDropKind &entry, std::set<int> &fired_groups, int dun_level)
{
    // 浅階ガード。弾かれたエントリは抽選自体を行わないのでグループも消費しない。
    if (dun_level < entry.min_dun_level) {
        return false;
    }

    // 同グループの先行エントリが既に当たっていれば、このエントリは抽選せず見送る。
    if ((entry.exclusive_group > 0) && fired_groups.contains(entry.exclusive_group)) {
        return false;
    }

    if (randint1(entry.denominator) > entry.numerator) {
        return false;
    }

    if (entry.exclusive_group > 0) {
        fired_groups.insert(entry.exclusive_group);
    }

    return true;
}

/*!
 * @brief 固定装備指定 1 リスト分をモンスターに持たせる
 * @param player プレイヤーへの参照 (アイテム生成基準)
 * @param monster 対象モンスター
 * @param entries 装備指定のリスト
 * @param is_itemkind entries がアイテム種別指定 (equip_tvals) なら true
 */
static void equip_monster_fixed_entries(CreatureEntity &player, CreatureEntity &monster, const std::vector<MonraceDropKind> &entries, bool is_itemkind)
{
    // 排他グループ (exclusive_group) は 1 リストの中で閉じる。
    std::set<int> fired_groups;
    const auto dun_level = player.get_floor()->dun_level;
    for (const auto &entry : entries) {
        if (!roll_fixed_item_entry(entry, fired_groups, dun_level)) {
            continue;
        }

        const auto item_nums = entry.dice.roll();
        for (auto i = 0; i < item_nums; i++) {
            auto item = generate_fixed_item(player, entry, is_itemkind);
            if (!item) {
                continue;
            }

            // 装備できるスロットが空いていれば装備し、無理なら所持品に入る。
            // いずれにせよ死亡時は drop_all_inventory() で床へ落ちる。
            (void)give_item_to_monster(monster, *item);
        }
    }
}

void equip_monster_fixed_items(CreatureEntity &player, CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    equip_monster_fixed_entries(player, monster, monrace.equip_kinds, false);
    equip_monster_fixed_entries(player, monster, monrace.equip_tvals, true);
}
