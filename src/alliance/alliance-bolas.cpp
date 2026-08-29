#include "alliance/alliance-bolas.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-kind-flags.h"
#include "spell/summon-types.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "view/display-messages.h"
#include <algorithm>

/*!
 * @brief ボーラスの印象値を計算する
 * @param creature クリーチャーへの参照
 * @return 印象値
 * @details 龍神『ニコル・ボーラス』は他者を盤上の駒としか見ない。力ある者ほど
 * 利用価値のある駒として高く評価される一方、善なる者は御しがたい邪魔者として嫌われる。
 * 首領たるボーラス本人や、その手駒である戦慄衆・配下のプレインズウォーカーを
 * 殺害している場合は大幅な減点を受ける。
 */
int AllianceBolas::calcImpressionPoint(const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    // 力ある者は駒として重用される
    impression += Alliance::calcPlayerPower(creature, 12, 30);

    // 善性は御しがたい邪魔者として嫌われる
    impression += (creature.alignment < 0) ? -creature.alignment : -creature.alignment * 2;

    // 首領および配下を殺害した場合の減点
    const auto &monrace_list = MonraceList::get_instance();
    if (monrace_list.get_monrace(MonraceId::NICOL_BOLAS).r_pkills > 0) {
        impression -= 8700; // 龍神『ニコル・ボーラス』を殺害 (レベル87 × 100)
    }
    if (monrace_list.get_monrace(MonraceId::DAVRIEL).r_pkills > 0) {
        impression -= 410; // 魂の仲介人『ダブリエル』を殺害 (レベル41 × 10)
    }
    if (monrace_list.get_monrace(MonraceId::TIBALT).r_pkills > 0) {
        impression -= 220; // 悪鬼の血脈『ティボルト』を殺害 (レベル22 × 10)
    }

    // 一般モンスター（複数撃破の可能性があるため、撃破数に応じて減点）
    impression -= monrace_list.get_monrace(MonraceId::DREADHORDE_WARRIOR).r_pkills * 23; // 戦慄衆の戦士 (レベル23 × 1 per kill)
    impression -= monrace_list.get_monrace(MonraceId::TIBALTS_RAGER).r_pkills * 15; // ティボルトの憤怒鬼 (レベル15 × 1 per kill)

    return impression;
}

/*!
 * @brief ボーラスの制裁処理
 * @param creature クリーチャーへの参照
 * @details 盤上の駒が意に沿わぬ動きをすれば、死者の軍団『永遠衆』が差し向けられる。
 */
void AllianceBolas::panishment(CreatureEntity &creature)
{
    auto impression = calcImpressionPoint(creature);
    if (isAnnihilated() || impression > -40) {
        return;
    }

    if (!creature.is_player()) {
        return;
    }

    if (!one_in_(20)) {
        return;
    }

    Pos2D m_pos(creature.get_position());
    m_pos = scatter(*creature.get_floor(), m_pos, 12, PROJECT_NONE);
    const auto m_idx = place_monster_one(creature, m_pos.y, m_pos.x, MonraceId::DREADHORDE_WARRIOR, PM_ALLOW_GROUP);
    if (!m_idx) {
        return;
    }

    msg_print(_("「盤上の駒が身の程を弁えぬか」戦慄衆があなたを狩り立てに現れた！",
        "\"Does a pawn forget its place?\" The Dreadhorde comes to hunt you down!"));
    disturb(creature, true, true);
    for (int k = 0; k < 4; k++) {
        summon_specific(creature, m_pos.y, m_pos.x, std::max(creature.get_floor()->monster_level, 5), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
    }
}

/*!
 * @brief ボーラスが壊滅しているかを判定する
 * @return 首領たる龍神『ニコル・ボーラス』が絶滅していればtrue
 * @details 全ては首領一体の謀略の上に成り立っており、彼が失われれば軍団は瓦解する。
 */
bool AllianceBolas::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::NICOL_BOLAS });
}

/*!
 * @brief ボーラスがモンスターに敵意を抱くかどうかを判定する
 * @param creature_other 敵意を抱くか調べる他のモンスターの参照
 * @param monrace モンスター種族情報の参照
 * @return 敵意を持つならばtrue
 * @details 死者の軍団『永遠衆』は次元を渡る者を狩るために造られた。配下でない
 * プレインズウォーカーは、灯を奪うべき獲物として無条件に敵対対象となる。
 * それ以外は通常の善悪属性による判定に従う。
 */
bool AllianceBolas::is_hostile_to(const CreatureEntity &creature_other, const MonraceDefinition &monrace) const
{
    if (monrace.kind_flags.has(MonsterKindType::PLANESWALKER) && (monrace.alliance_idx != AllianceType::BOLAS)) {
        return true;
    }

    return Alliance::is_hostile_to(creature_other, monrace);
}

/*!
 * @brief 襲撃時に出現するモンスターのリストを取得する
 * @param creature クリーチャーへの参照
 * @param impression_point 印象値
 * @return ボーラスのモンスターIDのリスト（印象値が低い場合は戦慄衆）
 */
std::vector<MonraceId> AllianceBolas::get_ambush_monsters([[maybe_unused]] CreatureEntity &creature, int impression_point) const
{
    std::vector<MonraceId> monsters;

    // 印象値が低い場合のみ襲撃を行う（-150以下）
    if (impression_point >= -150) {
        return monsters;
    }

    // 死者の軍団『永遠衆』による襲撃
    monsters.push_back(MonraceId::DREADHORDE_WARRIOR); // 戦慄衆の戦士
    monsters.push_back(MonraceId::DREADHORDE_WARRIOR); // 複数体を追加
    monsters.push_back(MonraceId::TIBALTS_RAGER); // ティボルトの憤怒鬼

    // 印象値が非常に低い場合は配下のプレインズウォーカーも差し向けられる
    if (impression_point < -500) {
        monsters.push_back(MonraceId::TIBALT); // 悪鬼の血脈『ティボルト』
        monsters.push_back(MonraceId::DAVRIEL); // 魂の仲介人『ダブリエル』
    }

    return monsters;
}

/*!
 * @brief 襲撃時のメッセージを取得する
 * @return ボーラス固有の襲撃メッセージ
 */
std::string AllianceBolas::get_ambush_message() const
{
    return _("戦慄衆があなたを取り囲んだ！ 黄金の眼が全てを見据えている！",
        "The Dreadhorde surrounds you! The golden gaze sees everything!");
}
