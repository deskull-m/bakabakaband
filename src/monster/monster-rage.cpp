/*!
 * @brief 植木鉢くんが破壊されたときの激怒と罪悪感の処理 (Issue #180)
 * @details `MonsterMiscType::UEKIBACHIKUN` を持つ種族が死亡すると、周囲の敵対
 *          モンスターへ `MonsterConstantFlagType::ENRAGED_BY_BROKEN_POT` を付け、
 *          速度・近接命中・近接ダメージ・AC・セーヴに常時ボーナスを乗せる。
 *          既存の「周囲のモンスターを怒らせる」(`aggravate_monsters()` の ANGER、
 *          一時加速のみ) とは別枠の、より大きく永続的な激怒。
 *          壊したのがプレイヤー自身なら、併せて時限バッドステータス
 *          `CreatureTimedEffect::BROKEN_POT_GUILT` (常時反感・AC低下・
 *          被クリティカル率向上) を背負う。
 */

#include "monster/monster-rage.h"
#include "grid/grid.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-status-setter.h"
#include "status/bad-status-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "target/projection-path-calculator.h"
#include "term/z-rand.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <array>
#include <string_view>

namespace {
/*!
 * @brief 激怒した人型が吐く罵りのセリフ (Issue #180)
 * @details 植木鉢くんを壊された虐待おじさんの口調。バランスではなく flavor なので
 *          追加・差し替えはこの表だけで済む。
 */
const std::array RAGE_SPEECHES = {
    _("これどうすんだよなぁ！", "What are you gonna do about this, huh!"),
    _("お前の攻撃が飛んで来てよぉなぁ、全部メチャメチャになってんだよなぁ…", "Your attacks come flying in, see, and now everything's wrecked, see..."),
    _("おい すいませんじゃねえよおい、なぁ、お前よぉこんな所でなぁ戦闘やってたらよぉなぁ……おい……", "Hey, 'sorry' doesn't cut it, hey, see, you, fighting in a place like this, see... hey..."),
    _("どっかのよぉダンジョンによぉなぁ、割れるとか考えねえのかよなぁ！？", "Somebody's dungeon, see, don't you ever think stuff might break, huh!?"),
    _("そうだろ？なぁ？ おい……戦闘やる所じゃねえだろここなぁ！どう考えたってよぉお前よぉなぁ……", "Right? See? Hey... this ain't a place to fight, see! Any way you slice it, you, see..."),
    _("人の縄張り入るなとかって思うだろなぁ普通よぉ！なぁこれだけ狭かったらよぉなぁ……", "Normally you'd think not to walk into somebody's turf, see! See, cramped as it is here..."),
    _("バカかおいなぁ？お前これどうすんだよそのへん！おい、お前、あれか？なぁ？", "You stupid or what, huh? What're you gonna do about all this! Hey, you, is that it? Huh?"),
    _("自分だけ矢返してくれてこれは放っておくのか、なぁ？なぁ？", "You take your own arrows back and leave this lying here, huh? Huh?"),
    _("おいこれどうするんだよこれ高かったんだよなぁ！植木鉢ィ！", "Hey, what're you gonna do, that was expensive, see! My plant pot!"),
    _("なぁ？花、大事に育ててたんだよなぁ！おい……なぁ？", "See? I was raising that flower with care, see! Hey... see?"),
    _("これぇ！これなんか見ろよこれ！……なぁこの無残な姿をよぉなぁ（半笑い）", "This! Just look at this! ...See, this pitiful state of it, see (half-laughing)"),
    _("おい……お前……なぁ……種から育ててたんだぞここまでなぁおい", "Hey... you... see... I raised it from a seed all the way to here, see, hey"),
    _("どう繋がっ、償うんだよなぁ！？", "How're you gonna make, make up for this, huh!?"),
    _("お前こっち来いよ！いいから、なぁ！", "You, get over here! Just come on, see!"),
};
}

void enrage_monsters_by_broken_pot(CreatureEntity &creature, MONSTER_IDX pot_m_idx, bool killed_by_player)
{
    auto &floor = *creature.get_floor();
    auto &pot = floor.get_monster(pot_m_idx);
    auto &pot_monrace = pot.get_monrace();
    if (pot_monrace.misc_flags.has_not(MonsterMiscType::UEKIBACHIKUN)) {
        return;
    }

    const auto pot_pos = pot.get_position();
    auto enraged_count = 0;
    auto is_any_visible = false;
    for (auto m_idx = 1; m_idx < floor.m_max; m_idx++) {
        if (m_idx == pot_m_idx) {
            continue;
        }

        auto &monster = floor.get_monster(m_idx);
        if (!monster.is_valid() || monster.is_pet() || monster.is_enraged_by_broken_pot()) {
            continue;
        }

        if (Grid::calc_distance(pot_pos, monster.get_position()) > RAGE_BROKEN_POT_RADIUS) {
            continue;
        }

        monster.set_constant_flag(MonsterConstantFlagType::ENRAGED_BY_BROKEN_POT);

        // 激怒した相手はもう手懐けられない (既存の ANGER と同じ扱い)。
        monster.set_constant_flag(MonsterConstantFlagType::NOPET);

        // 眠っていれば叩き起こし、怯えていれば立ち直らせる。
        (void)set_monster_csleep(floor, m_idx, 0);
        if (monster.is_fearful()) {
            (void)set_monster_monfear(floor, m_idx, 0);
        }

        enraged_count++;
        is_any_visible |= monster.is_visible_on_map();
    }

    if (enraged_count > 0) {
        // 目の前で起きたなら思い出に記録する (見えていなければ何が起きたか分からない)。
        if (is_any_visible || pot.is_visible_on_map()) {
            pot_monrace.r_misc_flags.set(MonsterMiscType::UEKIBACHIKUN);
        }

        if (is_any_visible) {
            msg_print(_("周囲のモンスターが植木鉢くんの無残な姿に激怒した！", "The monsters nearby are enraged by the pitiful remains of the plant pot!"));
        } else {
            msg_print(_("どこかで何かが激怒したような気配を感じた！", "You sense something nearby seething with rage!"));
        }
    }

    // 壊したのがプレイヤー自身なら罪悪感 (時間経過でのみ薄れる時限バッドステータス) を背負う。
    // 周囲に激怒する個体が居なくても壊した事実は変わらないので、`enraged_count` には依らない。
    // 複数壊せば加算されるので、ボール系や視界内魔法を撒き散らすほど重くなる。
    //
    // この setter は `handle_stuff()` を呼ぶため、`pot` や `pot_monrace` の参照を
    // 使い終えた関数末尾に置いてある (順序を入れ替えないこと)。
    if (killed_by_player) {
        const auto duration = BROKEN_POT_GUILT_DURATION_BASE + randint1(BROKEN_POT_GUILT_DURATION_DICE);
        (void)BadStatusSetter(creature).mod_broken_pot_guilt(static_cast<TIME_EFFECT>(duration));
    }
}

bool speak_rage_by_broken_pot(CreatureEntity &creature, MONSTER_IDX m_idx, const Pos2D &pos, bool aware)
{
    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    if (!monster.is_enraged_by_broken_pot()) {
        return false;
    }

    // CAN_SPEAK は要求せず、人型系の体構造であることだけを条件にする (Issue #180)。
    if (!monster.get_monrace().is_humanoid_body()) {
        return false;
    }

    // 罵る相手はプレイヤーなので、通常の喋りと同じく気付いていて声が届く状況に限る。
    if (!aware || !floor.has_los_at(pos) || !projectable(floor, pos, creature.get_position())) {
        return false;
    }

    if (!one_in_(RAGE_SPEECH_CHANCE)) {
        return false;
    }

    const auto m_name = monster.is_visible_on_map() ? monster_desc(creature, monster, 0) : std::string(_("それ", "It"));
    msg_print(_("{}は「{}」と凄んだ。", "{} snarls, \"{}\""), str_upcase_first(m_name), rand_choice(RAGE_SPEECHES));
    return true;
}
