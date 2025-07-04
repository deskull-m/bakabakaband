/*!
 * @brief 元素使いの魔法系統
 */

#include "mind/mind-elementalist.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-gameoption.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-monster-util.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-util.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-option-page.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-mindcrafter.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "racial/racial-util.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/grid-selector.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"
#include <array>
#include <string>
#include <unordered_map>

/*!
 * @brief 元素魔法呪文のID定義
 */
enum class ElementSpells {
    BOLT_1ST = 0,
    MON_DETECT = 1,
    PERCEPT = 2,
    CURE = 3,
    BOLT_2ND = 4,
    MAG_DETECT = 5,
    BALL_3RD = 6,
    BALL_1ST = 7,
    BREATH_2ND = 8,
    ANNIHILATE = 9,
    BOLT_3RD = 10,
    WAVE_1ST = 11,
    BALL_2ND = 12,
    BURST_1ST = 13,
    STORM_2ND = 14,
    BREATH_1ST = 15,
    STORM_3ND = 16,
    MAX
};

/*!
 * @brief 元素魔法タイプ構造体
 */
struct element_type {
    std::string title; //!< 領域名
    std::array<AttributeType, 3> type; //!< 属性タイプリスト
    std::array<std::string, 3> name; //!< 属性名リスト
    std::unordered_map<AttributeType, AttributeType> extra; //!< 追加属性タイプ
};

/*!
 * @brief 元素魔法難易度構造体
 */
struct element_power {
    int elem; //!< 使用属性番号
    mind_type info; //!< 難易度構造体
};

using element_type_list = const std::unordered_map<ElementRealmType, element_type>;
using element_power_list = const std::unordered_map<ElementSpells, element_power>;
using element_tip_list = const std::unordered_map<ElementSpells, std::string>;
using element_text_list = const std::unordered_map<ElementRealmType, std::string>;

// clang-format off
/*!
 * @brief 元素魔法タイプ定義
 */
static element_type_list element_types = {
    {
        ElementRealmType::FIRE, {
            _("炎", "Fire"),
            { { AttributeType::FIRE, AttributeType::HELL_FIRE, AttributeType::PLASMA } },
            { { _("火炎", "Fire"), _("業火", "Hell Fire"), _("プラズマ", "Plasma") } },
            { },
        }
    },
    {
        ElementRealmType::ICE, {
            _("氷", "Ice"),
            { { AttributeType::COLD, AttributeType::INERTIAL, AttributeType::TIME } },
            { { _("冷気", "Ice"), _("遅鈍", "Inertia"), _("時間逆転", "Time Stream") } },
            { { AttributeType::COLD, AttributeType::ICE} },
        }
    },
    {
        ElementRealmType::SKY, {
            _("空", "Sky"),
            { { AttributeType::ELEC, AttributeType::LITE, AttributeType::MANA } },
            { { _("電撃", "Lightning"), _("光", "Light"), _("魔力", "Mana") } },
            { },
        }
    },
    {
        ElementRealmType::SEA, {
            _("海", "Sea"),
            { { AttributeType::ACID, AttributeType::WATER, AttributeType::DISINTEGRATE } },
            { { _("酸", "Acid"), _("水", "Water"), _("分解", "Disintegration") } },
            { },
        }
    },
    {
        ElementRealmType::DARKNESS, {
            _("闇", "Darkness"),
            { { AttributeType::DARK, AttributeType::NETHER, AttributeType::VOID_MAGIC } },
            { { _("暗黒", "Darkness"), _("地獄", "Nether"), _("虚無", "void") } },
            { { AttributeType::DARK, AttributeType::ABYSS } },
        }
    },
    {
        ElementRealmType::CHAOS, {
            _("混沌", "Chaos"),
            { { AttributeType::CONFUSION, AttributeType::CHAOS, AttributeType::NEXUS } },
            { { _("混乱", "Confusion"), _("カオス", "Chaos"), _("因果混乱", "Nexus") } },
            { },
        }
    },
    {
        ElementRealmType::EARTH, {
            _("地", "Earth"),
            { { AttributeType::SHARDS, AttributeType::FORCE, AttributeType::METEOR } },
            { { _("破片", "Shards"), _("フォース", "Force"), _("隕石", "Meteor") } },
            { },
        }
    },
    {
        ElementRealmType::DEATH, {
            _("瘴気", "Death"),
            { { AttributeType::POIS, AttributeType::HYPODYNAMIA, AttributeType::DISENCHANT } },
            { { _("毒", "Poison"), _("吸血", "Drain Life"), _("劣化", "Disenchantment") } },
            { },
        }
    },
};

/*!
 * @brief 元素魔法呪文定義
 */
static const element_power_list element_powers = {
    { ElementSpells::BOLT_1ST,   { 0, {  1,  1,  15, _("%sの矢", "%s Bolt") }}},
    { ElementSpells::MON_DETECT, { 0, {  2,  2,  20, _("モンスター感知", "Detect Monsters") }}},
    { ElementSpells::PERCEPT,    { 0, {  5,  5,  50, _("擬似鑑定", "Psychometry") }}},
    { ElementSpells::CURE,       { 0, {  6,  5,  35, _("傷の治癒", "Cure Wounds") }}},
    { ElementSpells::BOLT_2ND,   { 1, {  8,  6,  35, _("%sの矢", "%s Bolt") }}},
    { ElementSpells::MAG_DETECT, { 0, { 10,  8,  60, _("魔法感知", "Detect Magical Objs") }}},
    { ElementSpells::BALL_3RD,   { 2, { 15, 10,  55, _("%s放射", "%s Spout") }}},
    { ElementSpells::BALL_1ST,   { 0, { 18, 13,  65, _("%sの球", "%s Ball") }}},
    { ElementSpells::BREATH_2ND, { 1, { 21, 20,  70, _("%sのブレス", "Breath of %s") }}},
    { ElementSpells::ANNIHILATE, { 0, { 24, 20,  75, _("モンスター消滅", "Annihilation") }}},
    { ElementSpells::BOLT_3RD,   { 2, { 25, 15,  60, _("%sの矢", "%s Bolt") }}},
    { ElementSpells::WAVE_1ST,   { 0, { 28, 30,  75, _("元素の波動", "Elemental Wave") }}},
    { ElementSpells::BALL_2ND,   { 1, { 28, 22,  75, _("%sの球", "%s Ball") }}},
    { ElementSpells::BURST_1ST,  { 0, { 33, 35,  75, _("精気乱射", "%s Blast") }}},
    { ElementSpells::STORM_2ND,  { 1, { 35, 30,  75, _("%sの嵐", "%s Storm") }}},
    { ElementSpells::BREATH_1ST, { 0, { 42, 48,  75, _("%sのブレス", "Breath of %s") }}},
    { ElementSpells::STORM_3ND,  { 2, { 45, 60,  80, _("%sの嵐", "%s Storm") }}},
};

/*!
 * @brief 元素魔法呪文説明文定義
 */
static element_tip_list element_tips = {
    { ElementSpells::BOLT_1ST,
    _("弱い%sの矢を放つ。", "Fire a weak bolt of %s.") },
    { ElementSpells::MON_DETECT,
    _("近くの全てのモンスターを感知する。", "Detects monsters.") },
    { ElementSpells::PERCEPT,
    _("アイテムの雰囲気を知る。", "Gives feeling of an item.") },
    { ElementSpells::CURE,
    _("怪我と体力を少し回復させる。", "Heals HP and wounds a bit.") },
    { ElementSpells::BOLT_2ND,
    _("%sの矢を放つ。", "Fire a bolt of %s.") },
    { ElementSpells::MAG_DETECT,
    _("近くの魔法のアイテムを感知する。", "Detects magic devices.") },
    { ElementSpells::BALL_3RD,
    _("高威力で射程が短い%sの球を放つ。", "Fire a strong, short-range, ball of %s.") },
    { ElementSpells::BALL_1ST,
    _("%sの球を放つ。",  "Fire a ball of %s.") },
    { ElementSpells::BREATH_2ND,
    _("%sのブレスを吐く。", "Fire a breath of %s.") },
    { ElementSpells::ANNIHILATE,
    _("%s耐性のないモンスターを1体抹殺する。", "Erase a monster unless it resists %s.") },
    { ElementSpells::BOLT_3RD,
    _("%sの矢を放つ。", "Fire a bolt of %s.") },
    { ElementSpells::WAVE_1ST,
    _("視界内の全ての敵に%sによるダメージを与える。", "Inflict %s damage on all monsters in sight.") },
    { ElementSpells::BALL_2ND,
    _("%sの球を放つ。",  "Fire a ball of %s.") },
    { ElementSpells::BURST_1ST,
    _("ランダムな方向に%sの矢を放つ。", "Fire some bolts of %s in random direction.") },
    { ElementSpells::STORM_2ND,
    _("%sの巨大な球を放つ。", "Fire a large ball of %s.") },
    { ElementSpells::BREATH_1ST,
    _("%sのブレスを吐く。", "Fire a breath of %s.") },
    { ElementSpells::STORM_3ND,
    _("%sの巨大な球を放つ。", "Fire a large ball of %s.") },
};

/*!
 * @brief 元素魔法選択時説明文定義
 */
static element_text_list element_texts = {
    { ElementRealmType::FIRE,
    _("炎系統は巨大なエネルギーで灼熱を生み出し、全ての敵を燃やし尽くそうとします。",
        "Great energy of Fire system will be able to burn out all of your enemies.")},
    { ElementRealmType::ICE,
    _("氷系統の魔法はその冷たさで敵の動きを奪い尽くし、魂すらも止めてしまうでしょう。",
        "Ice system will freeze your enemies, even their souls.")},
    { ElementRealmType::SKY,
    _("空系統は大いなる天空のエネルギーを駆使して敵の全てを撃滅できます。",
        "Sky system can terminate all of your enemies powerfully with the energy of the great sky.")},
    { ElementRealmType::SEA,
    _("海系統はその敵の全てを溶かし、大いなる海へと返してしまいます。",
        "Sea system melts all of your enemies and returns them to the great ocean.")},
    { ElementRealmType::DARKNESS,
    _("闇系統は恐るべき力を常闇から引き出し、敵を地獄へと叩き落とすでしょう。",
        "Dark system draws terrifying power from the darkness and knocks your enemies into hell.")},
    { ElementRealmType::CHAOS,
    _("混沌系統は敵の意識も条理も捻じ曲げ、その存在をあの世に送ってしまいます。",
        "Chaos system twists and wraps your enemies, even their souls, and scatters them as dust in the wind.")},
    { ElementRealmType::EARTH,
    _("地系統は偉大なる大地の力を呼び出して、数多の敵のことごとくを粉砕しようとします。",
        "Earth system smashes all of your enemies massively using its huge powers.")},
    { ElementRealmType::DEATH,
    _("瘴気系統は全ての生ける者にとって途轍もない毒です。",
        "Death system is a tremendous poison for all living enemies.")},
};

// clang-format on

/*!
 * @brief 元素魔法の領域名を返す
 * @param realm 領域
 * @return 領域名
 */
const std::string &get_element_title(ElementRealmType realm)
{
    return element_types.at(realm).title;
}

/*!
 * @brief 元素魔法領域の属性リストを返す
 * @param realm 領域
 * @return 領域で使用できる属性リスト
 */
static const std::array<AttributeType, 3> &get_element_types(ElementRealmType realm)
{
    return element_types.at(realm).type;
}

/*!
 * @brief 元素魔法領域のn番目の属性を返す
 * @param realm 領域
 * @param n 属性の何番目か
 * @return 属性タイプ
 */
AttributeType get_element_type(ElementRealmType realm, int n)
{
    return get_element_types(realm).at(n);
}

/*!
 * @brief 元素魔法領域のn番目の呪文用の属性を返す
 * @param n 属性の何番目か
 * @return 属性タイプ
 */
static AttributeType get_element_spells_type(PlayerType *player_ptr, int n)
{
    const auto &realm = element_types.at(player_ptr->element_realm);
    const auto t = realm.type.at(n);
    if (realm.extra.find(t) != realm.extra.end()) {
        if (evaluate_percent(player_ptr->lev * 2)) {
            return realm.extra.at(t);
        }
    }
    return t;
}

/*!
 * @brief 元素魔法領域の属性名リストを返す
 * @param realm 領域
 * @return 領域で使用できる属性の名称リスト
 */
static const std::array<std::string, 3> &get_element_names(ElementRealmType realm)
{
    return element_types.at(realm).name;
}

/*!
 * @brief 元素魔法領域のn番目の属性名を返す
 * @param realm 領域
 * @param n 属性の何番目か
 * @return 属性名
 */
const std::string &get_element_name(ElementRealmType realm, int n)
{
    return get_element_names(realm).at(n);
}

/*!
 * @brief 元素魔法の説明文を取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 説明文
 */
static std::string get_element_tip(PlayerType *player_ptr, int spell_idx)
{
    auto realm = player_ptr->element_realm;
    auto spell = i2enum<ElementSpells>(spell_idx);
    auto elem = element_powers.at(spell).elem;
    return format(element_tips.at(spell).data(), element_types.at(realm).name[elem].data());
}

/*!
 * @brief 元素魔法の説明文を取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 説明文
 */
static int get_elemental_elem(PlayerType *player_ptr, int spell_idx)
{
    (void)player_ptr;
    auto spell = i2enum<ElementSpells>(spell_idx);
    return element_powers.at(spell).elem;
}

/*!
 * @brief 元素魔法呪文の難易度データを取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 説明文
 */
static mind_type get_elemental_info(PlayerType *player_ptr, int spell_idx)
{
    (void)player_ptr;
    auto spell = i2enum<ElementSpells>(spell_idx);
    return element_powers.at(spell).info;
}

/*!
 * @brief 元素魔法呪文の効果表示文字列を取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return std::string 魔法の効果を表す文字列
 */
static std::string get_element_effect_info(PlayerType *player_ptr, int spell_idx)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    auto spell = i2enum<ElementSpells>(spell_idx);
    int dam = 0;

    switch (spell) {
    case ElementSpells::BOLT_1ST:
        return format(" %s%dd%d", KWD_DAM, 3 + ((plev - 1) / 5), 4);
    case ElementSpells::CURE:
        return format(" %s%dd%d", KWD_HEAL, 2, 8);
    case ElementSpells::BOLT_2ND:
        return format(" %s%dd%d", KWD_DAM, 8 + ((plev - 5) / 4), 8);
    case ElementSpells::BALL_3RD:
        return format(" %s%d", KWD_DAM, (50 + plev * 2));
    case ElementSpells::BALL_1ST:
        return format(" %s%d", KWD_DAM, 55 + plev);
    case ElementSpells::BREATH_2ND:
        dam = p_ptr->chp / 2;
        return format(" %s%d", KWD_DAM, (dam > 150) ? 150 : dam);
    case ElementSpells::ANNIHILATE:
        return format(" %s%d", _("効力:", "pow "), 50 + plev);
    case ElementSpells::BOLT_3RD:
        return format(" %s%dd%d", KWD_DAM, 12 + ((plev - 5) / 4), 8);
    case ElementSpells::WAVE_1ST:
        return format(" %s50+d%d", KWD_DAM, plev * 3);
    case ElementSpells::BALL_2ND:
        return format(" %s%d", KWD_DAM, 75 + plev * 3 / 2);
    case ElementSpells::BURST_1ST:
        return format(" %s%dd%d", KWD_DAM, 6 + plev / 8, 7);
    case ElementSpells::STORM_2ND:
        return format(" %s%d", KWD_DAM, 115 + plev * 5 / 2);
    case ElementSpells::BREATH_1ST:
        return format(" %s%d", KWD_DAM, p_ptr->chp * 2 / 3);
    case ElementSpells::STORM_3ND:
        return format(" %s%d", KWD_DAM, 300 + plev * 5);
    default:
        return std::string();
    }
}

/*!
 * @brief 元素魔法呪文を実行する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 実行したらTRUE、キャンセルならFALSE
 */
static bool cast_element_spell(PlayerType *player_ptr, SPELL_IDX spell_idx)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto spell = i2enum<ElementSpells>(spell_idx);
    const auto &power = element_powers.at(spell);
    const auto plev = player_ptr->lev;
    switch (spell) {
    case ElementSpells::BOLT_1ST: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = Dice::roll(3 + ((plev - 1) / 5), 4);
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_bolt(player_ptr, typ, dir, dam);
        return true;
    }
    case ElementSpells::MON_DETECT:
        (void)detect_monsters_normal(player_ptr, DETECT_RAD_DEFAULT);
        (void)detect_monsters_invis(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case ElementSpells::PERCEPT:
        return psychometry(player_ptr);
    case ElementSpells::CURE:
        (void)hp_player(player_ptr, Dice::roll(2, 8));
        (void)BadStatusSetter(player_ptr).mod_cut(-10);
        return true;
    case ElementSpells::BOLT_2ND: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = Dice::roll(8 + ((plev - 5) / 4), 8);
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_bolt_or_beam(player_ptr, plev, typ, dir, dam)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }

        return true;
    }
    case ElementSpells::MAG_DETECT:
        (void)detect_objects_magic(player_ptr, DETECT_RAD_DEFAULT);
        return true;
    case ElementSpells::BALL_3RD: {
        project_length = 4;
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto typ = get_element_spells_type(player_ptr, power.elem);
        const auto dam = 50 + plev * 2;
        (void)fire_ball(player_ptr, typ, dir, dam, 1);
        project_length = 0;
        return true;
    }
    case ElementSpells::BALL_1ST: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = 55 + plev;
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_ball(player_ptr, typ, dir, dam, 2);
        return true;
    }
    case ElementSpells::BREATH_2ND: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = std::min(150, player_ptr->chp / 2);
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_breath(player_ptr, typ, dir, dam, 3)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }

        return true;
    }
    case ElementSpells::ANNIHILATE: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        fire_ball_hide(player_ptr, AttributeType::E_GENOCIDE, dir, plev + 50, 0);
        return true;
    }
    case ElementSpells::BOLT_3RD: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = Dice::roll(12 + ((plev - 5) / 4), 8);
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        fire_bolt_or_beam(player_ptr, plev, typ, dir, dam);
        return true;
    }
    case ElementSpells::WAVE_1ST: {
        const auto dam = 50 + randint1(plev * 3);
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        project_all_los(player_ptr, typ, dam);
        return true;
    }
    case ElementSpells::BALL_2ND: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = 75 + plev * 3 / 2;
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_ball(player_ptr, typ, dir, dam, 3)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }

        return true;
    }
    case ElementSpells::BURST_1ST: {
        const auto p_pos = player_ptr->get_position();
        auto pos = p_pos;
        const auto num = Dice::roll(4, 3);
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        for (auto k = 0; k < num; k++) {
            auto attempts = 1000;
            while (attempts--) {
                pos = scatter(player_ptr, p_pos, 4, PROJECT_NONE);
                if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
                    continue;
                }

                if (!player_ptr->is_located_at(pos)) {
                    break;
                }
            }

            constexpr auto flag = PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL;
            project(player_ptr, 0, 0, pos.y, pos.x, Dice::roll(6 + plev / 8, 7), typ, flag);
        }

        return true;
    }
    case ElementSpells::STORM_2ND: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = 115 + plev * 5 / 2;
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_ball(player_ptr, typ, dir, dam, 4)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }

        return true;
    }
    case ElementSpells::BREATH_1ST: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = player_ptr->chp * 2 / 3;
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_breath(player_ptr, typ, dir, dam, 3);
        return true;
    }
    case ElementSpells::STORM_3ND: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto dam = 300 + plev * 5;
        const auto typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_ball(player_ptr, typ, dir, dam, 5);
        return true;
    }
    default:
        return false;
    }
}

/*!
 * @brief 元素魔法呪文の失敗率を計算
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 失敗率
 */
static PERCENTAGE decide_element_chance(PlayerType *player_ptr, mind_type spell)
{
    PERCENTAGE chance = spell.fail;

    chance -= 3 * (player_ptr->lev - spell.min_lev);
    chance += player_ptr->to_m_chance;
    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[A_WIS]] - 1);

    PERCENTAGE minfail = adj_mag_fail[player_ptr->stat_index[A_WIS]];
    if (chance < minfail) {
        chance = minfail;
    }

    chance += player_ptr->effects()->stun().get_magic_chance_penalty();
    if (heavy_armor(player_ptr)) {
        chance += 5;
    }

    if (player_ptr->is_icky_wield[0]) {
        chance += 5;
    }

    if (player_ptr->is_icky_wield[1]) {
        chance += 5;
    }

    if (chance > 95) {
        chance = 95;
    }

    return chance;
}

/*!
 * @brief 元素魔法呪文の消費MPを計算
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 消費MP
 */
static MANA_POINT decide_element_mana_cost(PlayerType *player_ptr, mind_type spell)
{
    (void)player_ptr;
    return spell.mana_cost;
}

/*!
 * @brief 元素魔法呪文を選択して取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param sn 呪文番号
 * @param only_browse 閲覧モードかどうか
 * @return 選んだらTRUE、選ばなかったらFALSE
 */
bool get_element_power(PlayerType *player_ptr, SPELL_IDX *sn, bool only_browse)
{
    SPELL_IDX i;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 10;
    PLAYER_LEVEL plev = player_ptr->lev;
    bool flag, redraw;
    int menu_line = (use_menu ? 1 : 0);

    *sn = -1;
    if (const auto code = repeat_pull(); code) {
        *sn = *code;
        if (get_elemental_info(player_ptr, *sn).min_lev <= plev) {
            return true;
        }
    }

    concptr p = _("元素魔法", "power");
    flag = false;
    redraw = false;

    for (i = 0; i < static_cast<SPELL_IDX>(ElementSpells::MAX); i++) {
        if (get_elemental_info(player_ptr, i).min_lev <= plev) {
            num++;
        }
    }

    std::string fmt;
    if (only_browse) {
        fmt = _("(%s^ %c-%c, '*'で一覧, ESC) どの%sについて知りますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? ");
    } else {
        fmt = _("(%s^ %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? ");
    }

    const auto prompt = format(fmt.data(), p, I2A(0), I2A(num - 1), p);
    if (use_menu && !only_browse) {
        screen_save();
    }

    auto choice = (always_show_list || use_menu) ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else {
            const auto new_choice = input_command(prompt, true);
            if (!new_choice.has_value()) {
                break;
            }

            choice = *new_choice;
        }

        auto should_redraw_cursor = true;
        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0':
                if (!only_browse) {
                    screen_load();
                }
                return false;
            case '8':
            case 'k':
            case 'K':
                menu_line += (num - 1);
                break;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case 'x':
            case 'X':
            case '\r':
            case '\n':
                i = menu_line - 1;
                should_redraw_cursor = false;
                break;
            }

            if (menu_line > num) {
                menu_line -= num;
            }
        }

        constexpr auto spell_max = enum2i(ElementSpells::MAX);
        if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && should_redraw_cursor)) {
            if (!redraw || use_menu) {
                redraw = true;
                if (!only_browse && !use_menu) {
                    screen_save();
                }

                display_element_spell_list(player_ptr, y, x);
                for (i = 0; i < spell_max && use_menu; i++) {
                    const auto spell = get_elemental_info(player_ptr, i);
                    if (spell.min_lev > plev) {
                        break;
                    }
                    const auto cursor = (i == (menu_line - 1)) ? _("  》 ", "  >  ") : "     ";
                    put_str(cursor, y + i + 1, x);
                }

            } else if (!only_browse) {
                redraw = false;
                screen_load();
            }

            continue;
        }

        if (!use_menu) {
            i = A2I(choice);
        }

        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        flag = true;
    }

    if (redraw && !only_browse) {
        screen_load();
    }

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(player_ptr);
    if (!flag) {
        return false;
    }

    *sn = i;
    repeat_push((COMMAND_CODE)i);
    return true;
}

/*!
 * @brief 元素魔法呪文をMPがなくても挑戦するか確認する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param mana_cost 消費MP
 * @return 詠唱するならTRUE、しないならFALSE
 */
static bool check_element_mp_sufficiency(PlayerType *player_ptr, int mana_cost)
{
    if (mana_cost <= player_ptr->csp) {
        return true;
    }

    msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
    if (!over_exert) {
        return false;
    }

    return input_check(_("それでも挑戦しますか? ", "Attempt it anyway? "));
}

/*!
 * @brief 元素魔法呪文の詠唱を試み、成功なら詠唱し、失敗ならファンブルする
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @param chance 失敗率
 * @return 詠唱して実行したらTRUE、されなかったらFALSE
 */
static bool try_cast_element_spell(PlayerType *player_ptr, SPELL_IDX spell_idx, PERCENTAGE chance)
{
    if (!evaluate_percent(chance)) {
        sound(SoundKind::ZAP);
        return cast_element_spell(player_ptr, spell_idx);
    }

    if (flush_failure) {
        flush();
    }

    msg_format(_("魔力の集中に失敗した！", "You failed to focus the elemental power!"));
    sound(SoundKind::FAIL);

    if (randint1(100) < chance / 2) {
        int plev = player_ptr->lev;
        msg_print(_("元素の力が制御できない氾流となって解放された！", "The elemental power surges from you in an uncontrollable torrent!"));
        const auto element = get_element_types(player_ptr->element_realm)[0];
        constexpr auto flags = PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM;
        project(player_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + plev / 10, player_ptr->y, player_ptr->x, plev * 2, element, flags);
        player_ptr->csp = std::max(0, player_ptr->csp - player_ptr->msp * 10 / (20 + randint1(10)));

        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::MP);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::PLAYER,
            SubWindowRedrawingFlag::SPELL,
        };
        rfu.set_flags(flags_swrf);
        return false;
    }

    return true;
}

/*!
 * @brief 元素魔法コマンドのメインルーチン
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void do_cmd_element(PlayerType *player_ptr)
{
    SPELL_IDX i;
    if (cmd_limit_confused(player_ptr) || !get_element_power(player_ptr, &i, false)) {
        return;
    }

    mind_type spell = get_elemental_info(player_ptr, i);
    PERCENTAGE chance = decide_element_chance(player_ptr, spell);
    int mana_cost = decide_element_mana_cost(player_ptr, spell);

    if (!check_element_mp_sufficiency(player_ptr, mana_cost)) {
        return;
    }

    if (!try_cast_element_spell(player_ptr, i, chance)) {
        return;
    }

    if (mana_cost <= player_ptr->csp) {
        player_ptr->csp -= mana_cost;
    } else {
        int oops = mana_cost;
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)BadStatusSetter(player_ptr).mod_paralysis(randnum1<short>(5 * oops + 1));
        chg_virtue(player_ptr, Virtue::KNOWLEDGE, -10);
        if (one_in_(2)) {
            const auto perm = one_in_(4);
            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));
            (void)dec_stat(player_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags_swrf);
}

/*!
 * @brief 現在プレイヤーが使用可能な元素魔法の一覧表示
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void do_cmd_element_browse(PlayerType *player_ptr)
{
    SPELL_IDX n = 0;

    screen_save();
    while (true) {
        if (!get_element_power(player_ptr, &n, true)) {
            screen_load();
            return;
        }

        term_erase(12, 21);
        term_erase(12, 20);
        term_erase(12, 19);
        term_erase(12, 18);
        term_erase(12, 17);
        term_erase(12, 16);
        display_wrap_around(get_element_tip(player_ptr, n), 62, 17, 15);

        prt(_("何かキーを押して下さい。", "Hit any key."), 0, 0);
        (void)inkey();
    }
}

/*!
 * @brief 元素魔法の一覧を表示する
 */
void display_element_spell_list(PlayerType *player_ptr, int y, int x)
{
    prt("", y, x);
    put_str(_("名前", "Name"), y, x + 5);
    put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

    constexpr auto spell_max = enum2i(ElementSpells::MAX);
    int i;
    for (i = 0; i < spell_max; i++) {
        const auto spell = get_elemental_info(player_ptr, i);
        if (spell.min_lev > player_ptr->lev) {
            break;
        }

        const auto elem = get_elemental_elem(player_ptr, i);
        const auto name = format(spell.name, get_element_name(player_ptr->element_realm, elem).data());

        const auto mana_cost = decide_element_mana_cost(player_ptr, spell);
        const auto chance = decide_element_chance(player_ptr, spell);
        const auto comment = get_element_effect_info(player_ptr, i);

        constexpr auto fmt = "  %c) %-30s%2d %4d %3d%%%s";
        const auto info_str = format(fmt, I2A(i), name.data(), spell.min_lev, mana_cost, chance, comment.data());
        const auto color = mana_cost > player_ptr->csp ? TERM_ORANGE : TERM_WHITE;
        c_prt(color, info_str, y + i + 1, x);
    }
    prt("", y + i + 1, x);
}

/*!
 * @brief 元素魔法の単体抹殺が有効か確認する
 * @param r_ptr モンスター種族への参照ポインタ
 * @param type 魔法攻撃属性
 * @return 効果があるならTRUE、なければFALSE
 */
static bool is_elemental_genocide_effective(const MonraceDefinition &monrace, AttributeType type)
{
    switch (type) {
    case AttributeType::FIRE:
        if (monrace.resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
            return false;
        }
        break;
    case AttributeType::COLD:
        if (monrace.resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
            return false;
        }
        break;
    case AttributeType::ELEC:
        if (monrace.resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
            return false;
        }
        break;
    case AttributeType::ACID:
        if (monrace.resistance_flags.has(MonsterResistanceType::IMMUNE_ACID)) {
            return false;
        }
        break;
    case AttributeType::DARK:
        if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_DARK) || monrace.r_resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
            return false;
        }
        break;
    case AttributeType::CONFUSION:
        if (monrace.resistance_flags.has(MonsterResistanceType::NO_CONF)) {
            return false;
        }
        break;
    case AttributeType::SHARDS:
        if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_SHARDS)) {
            return false;
        }
        break;
    case AttributeType::POIS:
        if (monrace.resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

/*!
 * @brief 元素魔法の単体抹殺の効果を発動する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 */
ProcessResult effect_monster_elemental_genocide(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    const auto &name = get_element_name(player_ptr->element_realm, 0);
    if (em_ptr->seen_msg) {
        msg_format(_("%sが%sを包み込んだ。", "The %s surrounds %s."), name.data(), em_ptr->m_name);
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto type = get_element_type(player_ptr->element_realm, 0);
    const auto is_effective = is_elemental_genocide_effective(*em_ptr->r_ptr, type);
    if (!is_effective) {
        if (em_ptr->seen_msg) {
            msg_format(_("%sには効果がなかった。", "%s^ is unaffected."), em_ptr->m_name);
        }
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_TRUE;
    }

    if (genocide_aux(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, em_ptr->is_player(), (em_ptr->r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One"))) {
        if (em_ptr->seen_msg) {
            msg_format(_("%sは消滅した！", "%s^ disappeared!"), em_ptr->m_name);
        }
        em_ptr->dam = 0;
        chg_virtue(player_ptr, Virtue::VITALITY, -1);
        return ProcessResult::PROCESS_TRUE;
    }

    em_ptr->skipped = true;
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 元素領域とレベルの条件に見合うかチェックする
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param realm 領域
 * @param lev プレイヤーレベル
 * @return 見合うならTRUE、そうでなければFALSE
 * @details
 * レベルに応じて取得する耐性などの判定に使用する
 */
bool has_element_resist(PlayerType *player_ptr, ElementRealmType realm, PLAYER_LEVEL lev)
{
    if (!PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        return false;
    }

    return (player_ptr->element_realm == realm) && (player_ptr->lev >= lev);
}

/*!
 * @brief 領域選択時のカーソル表示(シンボル＋領域名)
 * @param i 位置
 * @param n 最後尾の位置
 * @param color 表示色
 */
static void display_realm_cursor(int i, int n, term_color_type color)
{
    char sym;
    std::string name;
    if (i == n) {
        sym = '*';
        name = _("ランダム", "Random");
    } else {
        sym = I2A(i);
        name = element_types.at(i2enum<ElementRealmType>(i + 1)).title.data();
    }

    c_put_str(color, format("%c) %s", sym, name.data()), 12 + (i / 5), 2 + 15 * (i % 5));
}

/*!
 * @brief 領域選択時の移動キー処理
 * @param cs 現在位置
 * @param n 最後尾の位置
 * @param c 入力キー
 * @return 新しい位置
 */
static int interpret_realm_select_key(int cs, int n, char c)
{
    if (c == 'Q') {
        quit("");
    }

    if (c == '8') {
        if (cs >= 5) {
            return cs - 5;
        }
    }

    if (c == '4') {
        if (cs > 0) {
            return cs - 1;
        }
    }

    if (c == '6') {
        if (cs < n) {
            return cs + 1;
        }
    }

    if (c == '2') {
        if (cs + 5 <= n) {
            return cs + 5;
        }
    }

    return cs;
}

/*!
 * @brief 領域選択ループ処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param realm 選択中の領域
 * @param n 最後尾の位置
 * @return 領域番号
 */
static tl::optional<ElementRealmType> get_element_realm(PlayerType *player_ptr, ElementRealmType realm, int n)
{
    int cs = std::max(0, enum2i(realm) - 1);
    int os = cs;
    int k;

    const auto buf = format(_("領域を選んで下さい(%c-%c) ('='初期オプション設定): ", "Choose a realm (%c-%c) ('=' for options): "), I2A(0), I2A(n - 1));

    while (true) {
        display_realm_cursor(os, n, TERM_WHITE);
        display_realm_cursor(cs, n, TERM_YELLOW);
        put_str(buf, 10, 10);
        os = cs;

        char c = inkey();
        cs = interpret_realm_select_key(cs, n, c);

        if (c == 'S') {
            return tl::nullopt;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == n) {
                display_realm_cursor(cs, n, TERM_WHITE);
                cs = randint0(n - 1);
            }
            break;
        }

        if (c == '*') {
            display_realm_cursor(cs, n, TERM_WHITE);
            cs = randint0(n - 1);
            break;
        }

        k = islower(c) ? A2I(c) : -1;
        if (k >= 0 && k < n) {
            display_realm_cursor(cs, n, TERM_WHITE);
            cs = k;
            break;
        }

        k = isupper(c) ? (26 + c - 'A') : -1;
        if (k >= 26 && k < n) {
            display_realm_cursor(cs, n, TERM_WHITE);
            cs = k;
            break;
        }

        if (c == '=') {
            screen_save();
            do_cmd_options_aux(player_ptr, GameOptionPage::BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8') {
            bell();
        }
    }

    display_realm_cursor(cs, n, TERM_YELLOW);
    return i2enum<ElementRealmType>(cs + 1);
}

/*!
 * @brief 領域選択
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 領域番号
 */
tl::optional<ElementRealmType> select_element_realm(PlayerType *player_ptr)
{
    clear_from(10);

    constexpr auto realm_max = enum2i(ElementRealmType::MAX);
    auto realm = tl::make_optional(ElementRealmType::FIRE);
    int row = 16;
    while (1) {
        put_str(
            _("注意：元素系統の選択によりあなたが習得する呪文のタイプが決まります。", "Note: The system of element will determine which spells you can learn."),
            23, 5);

        for (int i = 0; i < realm_max; i++) {
            display_realm_cursor(i, realm_max - 1, TERM_WHITE);
        }

        realm = get_element_realm(player_ptr, *realm, realm_max - 1);
        if (!realm) {
            break;
        }

        display_wrap_around(element_texts.at(*realm), 74, row, 3);

        if (input_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), UserCheck::DEFAULT_Y)) {
            break;
        }

        clear_from(row);
    }

    clear_from(10);
    return realm;
}

/*!
 * @brief クラスパワー情報を追加
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 */
void switch_element_racial(PlayerType *player_ptr, rc_type *rc_ptr)
{
    auto plev = player_ptr->lev;
    rpi_type rpi;
    switch (player_ptr->element_realm) {
    case ElementRealmType::FIRE:
        rpi = rpi_type(_("ライト・エリア", "Light area"));
        rpi.text = _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");
        rpi.min_level = 3;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::ICE:
        rpi = rpi_type(_("周辺フリーズ", "Sleep monsters"));
        rpi.info = format("%s%d", KWD_POWER, 20 + plev * 3 / 2);
        rpi.text = _("視界内の全てのモンスターを眠らせる。抵抗されると無効。", "Attempts to put all monsters in sight to sleep.");
        rpi.min_level = 10;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::SKY:
        rpi = rpi_type(_("魔力充填", "Recharging"));
        rpi.info = format("%s%d", KWD_POWER, 120);
        rpi.text = _("杖/魔法棒の充填回数を増やすか、充填中のロッドの充填時間を減らす。", "Recharges staffs, wands or rods.");
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::SEA:
        rpi = rpi_type(_("岩石溶解", "Stone to mud"));
        rpi.text = _("壁を溶かして床にする。", "Turns one rock square to mud.");
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::DARKNESS:
        rpi = rpi_type(format(_("闇の扉", "Door to darkness"), 15 + plev / 2));
        rpi.info = format("%s%d", KWD_SPHERE, 15 + plev / 2);
        rpi.min_level = 5;
        rpi.cost = 5 + plev / 7;
        rpi.stat = A_WIS;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::CHAOS:
        rpi = rpi_type(_("現実変容", "Alter reality"));
        rpi.text = _("現在の階を再構成する。", "Recreates current dungeon level.");
        rpi.min_level = 35;
        rpi.cost = 30;
        rpi.stat = A_WIS;
        rpi.fail = 40;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::EARTH:
        rpi = rpi_type(_("地震", "Earthquake"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.text = _("周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        rpi.min_level = 25;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::DEATH:
        rpi = rpi_type(_("増殖阻止", "Sterilization"));
        rpi.text = _("この階の増殖するモンスターが増殖できなくなる。", "Prevents any breeders on current level from breeding.");
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    default:
        break;
    }
}

/*!
 * @brief 指定したマスが暗いかどうか
 * @param floor フロアへの参照
 * @param pos 指定の座標
 * @return 暗いならTRUE、そうでないならFALSE
 */
static bool is_target_grid_dark(const FloorType &floor, const Pos2D &pos)
{
    const auto &grid = floor.get_grid(pos);
    if (any_bits(grid.info, CAVE_MNLT)) {
        return false;
    }

    auto is_dark = false;
    auto is_lite = any_bits(grid.info, CAVE_GLOW | CAVE_LITE);
    for (auto dx = pos.x - 2; dx <= pos.x + 2; dx++) {
        for (auto dy = pos.y - 2; dy <= pos.y + 2; dy++) {
            const Pos2D pos_neighbor(dy, dx);
            if (pos == pos_neighbor) {
                continue;
            }
            if (!floor.contains(pos_neighbor)) {
                continue;
            }

            const auto m_idx = floor.get_grid(pos_neighbor).m_idx;
            if (m_idx == 0) {
                continue;
            }

            const auto d = Grid::calc_distance(pos, pos_neighbor);
            const auto &monrace = floor.m_list[m_idx].get_monrace();
            if (d <= 1 && monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_1, MonsterBrightnessType::SELF_LITE_1 })) {
                return false;
            }

            if (d <= 2 && monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_2, MonsterBrightnessType::SELF_LITE_2 })) {
                return false;
            }

            if (d <= 1 && monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::SELF_DARK_1 })) {
                is_dark = true;
            }

            if (d <= 2 && monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_2, MonsterBrightnessType::SELF_DARK_2 })) {
                is_dark = true;
            }
        }
    }

    return !is_lite || is_dark;
}

/*!
 * @breif 暗いところ限定での次元の扉
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
static bool door_to_darkness(PlayerType *player_ptr, int distance)
{
    const auto p_pos_orig = player_ptr->get_position();
    auto p_pos = tl::make_optional(player_ptr->get_position());
    const auto &floor = *player_ptr->current_floor_ptr;
    for (auto i = 0; i < 3; i++) {
        p_pos = point_target(player_ptr);
        if (!p_pos) {
            return false;
        }

        if (Grid::calc_distance(*p_pos, p_pos_orig) > distance) {
            msg_print(_("遠すぎる！", "That is too far!"));
            continue;
        }

        if (!floor.is_empty_at(*p_pos) || floor.get_grid(*p_pos).is_icky()) {
            msg_print(_("そこには移動できない。", "Can not teleport to there."));
            continue;
        }

        break;
    }

    const auto flag = cave_player_teleportable_bold(player_ptr, p_pos->y, p_pos->x, TELEPORT_SPONTANEOUS) && is_target_grid_dark(floor, *p_pos);
    if (flag) {
        teleport_player_to(player_ptr, p_pos->y, p_pos->x, TELEPORT_SPONTANEOUS);
    } else {
        msg_print(_("闇の扉は開かなかった！", "The door to darkness does not open!"));
    }

    return true;
}

/*!
 * @brief クラスパワーを実行
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 実行したらTRUE、しなかったらFALSE
 */
bool switch_element_execution(PlayerType *player_ptr)
{
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (player_ptr->element_realm) {
    case ElementRealmType::FIRE:
        (void)lite_area(player_ptr, Dice::roll(2, plev / 2), plev / 10);
        return true;
    case ElementRealmType::ICE:
        (void)project(player_ptr, 0, 5, player_ptr->y, player_ptr->x, 1, AttributeType::COLD, PROJECT_ITEM);
        (void)project_all_los(player_ptr, AttributeType::OLD_SLEEP, 20 + plev * 3 / 2);
        return true;
    case ElementRealmType::SKY:
        (void)recharge(player_ptr, 120);
        return true;
    case ElementRealmType::SEA: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        (void)wall_to_mud(player_ptr, dir, plev * 3 / 2);
        return true;
    }
    case ElementRealmType::DARKNESS:
        return door_to_darkness(player_ptr, 15 + plev / 2);
    case ElementRealmType::CHAOS:
        reserve_alter_reality(player_ptr, randint0(21) + 15);
        return true;
    case ElementRealmType::EARTH:
        (void)earthquake(player_ptr, player_ptr->get_position(), 10);
        return true;
    case ElementRealmType::DEATH:
        if (player_ptr->current_floor_ptr->num_repro <= MAX_REPRODUCTION) {
            player_ptr->current_floor_ptr->num_repro += MAX_REPRODUCTION;
        }

        return true;
    default:
        return false;
    }
}
