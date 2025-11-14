#include "monster-attack/monster-attack-table.h"
#include "effect/attribute-types.h"
#include "monster-attack/monster-attack-effect.h"

/*!
 * @brief モンスターの打撃効力テーブル /
 * The table of monsters' blow effects
 */
const mbe_info_type mbe_info[static_cast<int>(RaceBlowEffectType::MAX)] = {
    {
        0,
        AttributeType::NONE,
    }, /* None      */

    {
        60,
        AttributeType::MONSTER_MELEE,
    }, /* HURT      */

    {
        5,
        AttributeType::POIS,
    }, /* POISON    */

    {
        20,
        AttributeType::DISENCHANT,
    }, /* UN_BONUS  */

    {
        15,
        AttributeType::MONSTER_MELEE,
    },
    /* UN_POWER  */ /* ToDo: Apply the correct effects */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EAT_GOLD  */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EAT_ITEM  */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EAT_FOOD  */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EAT_LITE  */

    {
        0,
        AttributeType::ACID,
    }, /* ACID      */

    {
        10,
        AttributeType::ELEC,
    }, /* ELEC      */

    {
        10,
        AttributeType::FIRE,
    }, /* FIRE      */

    {
        10,
        AttributeType::COLD,
    }, /* COLD      */

    {
        2,
        AttributeType::MONSTER_MELEE,
    }, /* BLIND     */

    {
        10,
        AttributeType::CONFUSION,
    }, /* CONFUSE   */

    {
        10,
        AttributeType::MONSTER_MELEE,
    }, /* TERRIFY   */

    {
        2,
        AttributeType::MONSTER_MELEE,
    }, /* PARALYZE  */

    {
        0,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_STR  */

    {
        0,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_INT  */

    {
        0,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_WIS  */

    {
        0,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_DEX  */

    {
        0,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_CON  */

    {
        0,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_CHR  */

    {
        2,
        AttributeType::MONSTER_MELEE,
    }, /* LOSE_ALL  */

    {
        60,
        AttributeType::ROCKET,
    }, /* SHATTER   */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EXP_10    */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EXP_20    */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EXP_40    */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EXP_80    */

    {
        5,
        AttributeType::POIS,
    }, /* DISEASE   */

    {
        5,
        AttributeType::TIME,
    }, /* TIME      */

    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* EXP_VAMP  */

    {
        5,
        AttributeType::MANA,
    }, /* DR_MANA   */

    {
        60,
        AttributeType::MONSTER_MELEE,
    }, /* SUPERHURT */
    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* INERTIA */
    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* STUN */
    {
        5,
        AttributeType::MONSTER_MELEE,
    }, /* HUNGRY */
    {
        0,
        AttributeType::NONE,
    }, /* FLAVOR */
};

/*!
 * @brief RaceBlowMethodTypeから日本語名を取得する
 * @param method RaceBlowMethodType
 * @return 日本語名
 */
std::string get_blow_method_name(RaceBlowMethodType method)
{
    switch (method) {
    case RaceBlowMethodType::NONE:
        return "無し";
    case RaceBlowMethodType::HIT:
        return "殴打";
    case RaceBlowMethodType::TOUCH:
        return "接触";
    case RaceBlowMethodType::PUNCH:
        return "パンチ";
    case RaceBlowMethodType::KICK:
        return "蹴り";
    case RaceBlowMethodType::CLAW:
        return "ひっかき";
    case RaceBlowMethodType::BITE:
        return "噛みつき";
    case RaceBlowMethodType::STING:
        return "刺突";
    case RaceBlowMethodType::SLASH:
        return "斬撃";
    case RaceBlowMethodType::BUTT:
        return "角突き";
    case RaceBlowMethodType::CRUSH:
        return "体当たり";
    case RaceBlowMethodType::ENGULF:
        return "飲み込み";
    case RaceBlowMethodType::CHARGE:
        return "請求";
    case RaceBlowMethodType::CRAWL:
        return "這い回り";
    case RaceBlowMethodType::DROOL:
        return "よだれ";
    case RaceBlowMethodType::SPIT:
        return "唾吐き";
    case RaceBlowMethodType::EXPLODE:
        return "爆発";
    case RaceBlowMethodType::GAZE:
        return "睨み";
    case RaceBlowMethodType::WAIL:
        return "泣き叫び";
    case RaceBlowMethodType::SPORE:
        return "胞子";
    case RaceBlowMethodType::XXX4:
        return "未定義";
    case RaceBlowMethodType::BEG:
        return "物乞い";
    case RaceBlowMethodType::INSULT:
        return "侮辱";
    case RaceBlowMethodType::MOAN:
        return "うめき";
    case RaceBlowMethodType::SHOW:
        return "歌唱";
    case RaceBlowMethodType::ENEMA:
        return "浣腸";
    case RaceBlowMethodType::BIND:
        return "拘束";
    case RaceBlowMethodType::WHISPER:
        return "囁き";
    case RaceBlowMethodType::STAMP:
        return "踏みつけ";
    case RaceBlowMethodType::FECES:
        return "糞塗り";
    case RaceBlowMethodType::PUTAWAY:
        return "収納";
    case RaceBlowMethodType::CHOKE:
        return "絞殺";
    case RaceBlowMethodType::MAX:
        return "不明";
    }
    return "不明";
}

/*!
 * @brief RaceBlowMethodTypeからタグ文字列を取得する
 * @param method RaceBlowMethodType
 * @return タグ文字列
 */
std::string get_blow_method_tag(RaceBlowMethodType method)
{
    switch (method) {
    case RaceBlowMethodType::NONE:
        return "NONE";
    case RaceBlowMethodType::HIT:
        return "HIT";
    case RaceBlowMethodType::TOUCH:
        return "TOUCH";
    case RaceBlowMethodType::PUNCH:
        return "PUNCH";
    case RaceBlowMethodType::KICK:
        return "KICK";
    case RaceBlowMethodType::CLAW:
        return "CLAW";
    case RaceBlowMethodType::BITE:
        return "BITE";
    case RaceBlowMethodType::STING:
        return "STING";
    case RaceBlowMethodType::SLASH:
        return "SLASH";
    case RaceBlowMethodType::BUTT:
        return "BUTT";
    case RaceBlowMethodType::CRUSH:
        return "CRUSH";
    case RaceBlowMethodType::ENGULF:
        return "ENGULF";
    case RaceBlowMethodType::CHARGE:
        return "CHARGE";
    case RaceBlowMethodType::CRAWL:
        return "CRAWL";
    case RaceBlowMethodType::DROOL:
        return "DROOL";
    case RaceBlowMethodType::SPIT:
        return "SPIT";
    case RaceBlowMethodType::EXPLODE:
        return "EXPLODE";
    case RaceBlowMethodType::GAZE:
        return "GAZE";
    case RaceBlowMethodType::WAIL:
        return "WAIL";
    case RaceBlowMethodType::SPORE:
        return "SPORE";
    case RaceBlowMethodType::XXX4:
        return "XXX4";
    case RaceBlowMethodType::BEG:
        return "BEG";
    case RaceBlowMethodType::INSULT:
        return "INSULT";
    case RaceBlowMethodType::MOAN:
        return "MOAN";
    case RaceBlowMethodType::SHOW:
        return "SHOW";
    case RaceBlowMethodType::ENEMA:
        return "ENEMA";
    case RaceBlowMethodType::BIND:
        return "BIND";
    case RaceBlowMethodType::WHISPER:
        return "WHISPER";
    case RaceBlowMethodType::STAMP:
        return "STAMP";
    case RaceBlowMethodType::FECES:
        return "FECES";
    case RaceBlowMethodType::PUTAWAY:
        return "PUTAWAY";
    case RaceBlowMethodType::CHOKE:
        return "CHOKE";
    case RaceBlowMethodType::MAX:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}

/*!
 * @brief RaceBlowEffectTypeから日本語名を取得する
 * @param effect RaceBlowEffectType
 * @return 日本語名
 */
std::string get_blow_effect_name(RaceBlowEffectType effect)
{
    switch (effect) {
    case RaceBlowEffectType::NONE:
        return "なし";
    case RaceBlowEffectType::HURT:
        return "殴られた";
    case RaceBlowEffectType::POISON:
        return "毒をくらわされた";
    case RaceBlowEffectType::UN_BONUS:
        return "劣化させた";
    case RaceBlowEffectType::UN_POWER:
        return "充填魔力を吸収された";
    case RaceBlowEffectType::EAT_GOLD:
        return "金を盗まれた";
    case RaceBlowEffectType::EAT_ITEM:
        return "アイテムを盗まれた";
    case RaceBlowEffectType::EAT_FOOD:
        return "食糧を食べられた";
    case RaceBlowEffectType::EAT_LITE:
        return "明かりを吸収された";
    case RaceBlowEffectType::ACID:
        return "酸を飛ばされた";
    case RaceBlowEffectType::ELEC:
        return "感電させられた";
    case RaceBlowEffectType::FIRE:
        return "燃やされた";
    case RaceBlowEffectType::COLD:
        return "凍らされた";
    case RaceBlowEffectType::BLIND:
        return "盲目にされた";
    case RaceBlowEffectType::CONFUSE:
        return "混乱させられた";
    case RaceBlowEffectType::TERRIFY:
        return "恐怖させられた";
    case RaceBlowEffectType::PARALYZE:
        return "麻痺させられた";
    case RaceBlowEffectType::LOSE_STR:
        return "腕力を減少させた";
    case RaceBlowEffectType::LOSE_INT:
        return "知能を減少させた";
    case RaceBlowEffectType::LOSE_WIS:
        return "賢さを減少させた";
    case RaceBlowEffectType::LOSE_DEX:
        return "器用さを減少させた";
    case RaceBlowEffectType::LOSE_CON:
        return "耐久力を減少させた";
    case RaceBlowEffectType::LOSE_CHR:
        return "魅力を減少させた";
    case RaceBlowEffectType::LOSE_ALL:
        return "全ステータスを減少させた";
    case RaceBlowEffectType::SHATTER:
        return "粉砕された";
    case RaceBlowEffectType::EXP_10:
        return "経験値を減少(10d6+)させられた";
    case RaceBlowEffectType::EXP_20:
        return "経験値を減少(20d6+)させられた";
    case RaceBlowEffectType::EXP_40:
        return "経験値を減少(40d6+)させられた";
    case RaceBlowEffectType::EXP_80:
        return "経験値を減少(80d6+)させられた";
    case RaceBlowEffectType::DISEASE:
        return "病気にさせられた";
    case RaceBlowEffectType::TIME:
        return "時間を逆戻りさせられた";
    case RaceBlowEffectType::DR_LIFE:
        return "生命力を吸収された";
    case RaceBlowEffectType::DR_MANA:
        return "魔力を奪われた";
    case RaceBlowEffectType::SUPERHURT:
        return "強力に攻撃された";
    case RaceBlowEffectType::INERTIA:
        return "減速させられた";
    case RaceBlowEffectType::STUN:
        return "朦朧とさせられた";
    case RaceBlowEffectType::FLAVOR:
        return "ブラフだった";
    case RaceBlowEffectType::HUNGRY:
        return "空腹を進行させられた";
    case RaceBlowEffectType::DEFECATE:
        return "浣腸された";
    case RaceBlowEffectType::SANITY_BLAST:
        return "狂気へ誘われた";
    case RaceBlowEffectType::CHAOS:
        return "カオスを呼び起こされた";
    case RaceBlowEffectType::LOCKUP:
        return "閉じ込められた";
    case RaceBlowEffectType::MAX:
        return "訳が分からなかった";
    }
    return "訳が分からなかった";
}

/*!
 * @brief RaceBlowEffectTypeからタグ文字列を取得する
 * @param effect RaceBlowEffectType
 * @return タグ文字列
 */
std::string get_blow_effect_tag(RaceBlowEffectType effect)
{
    switch (effect) {
    case RaceBlowEffectType::NONE:
        return "NONE";
    case RaceBlowEffectType::HURT:
        return "HURT";
    case RaceBlowEffectType::POISON:
        return "POISON";
    case RaceBlowEffectType::UN_BONUS:
        return "UN_BONUS";
    case RaceBlowEffectType::UN_POWER:
        return "UN_POWER";
    case RaceBlowEffectType::EAT_GOLD:
        return "EAT_GOLD";
    case RaceBlowEffectType::EAT_ITEM:
        return "EAT_ITEM";
    case RaceBlowEffectType::EAT_FOOD:
        return "EAT_FOOD";
    case RaceBlowEffectType::EAT_LITE:
        return "EAT_LITE";
    case RaceBlowEffectType::ACID:
        return "ACID";
    case RaceBlowEffectType::ELEC:
        return "ELEC";
    case RaceBlowEffectType::FIRE:
        return "FIRE";
    case RaceBlowEffectType::COLD:
        return "COLD";
    case RaceBlowEffectType::BLIND:
        return "BLIND";
    case RaceBlowEffectType::CONFUSE:
        return "CONFUSE";
    case RaceBlowEffectType::TERRIFY:
        return "TERRIFY";
    case RaceBlowEffectType::PARALYZE:
        return "PARALYZE";
    case RaceBlowEffectType::LOSE_STR:
        return "LOSE_STR";
    case RaceBlowEffectType::LOSE_INT:
        return "LOSE_INT";
    case RaceBlowEffectType::LOSE_WIS:
        return "LOSE_WIS";
    case RaceBlowEffectType::LOSE_DEX:
        return "LOSE_DEX";
    case RaceBlowEffectType::LOSE_CON:
        return "LOSE_CON";
    case RaceBlowEffectType::LOSE_CHR:
        return "LOSE_CHR";
    case RaceBlowEffectType::LOSE_ALL:
        return "LOSE_ALL";
    case RaceBlowEffectType::SHATTER:
        return "SHATTER";
    case RaceBlowEffectType::EXP_10:
        return "EXP_10";
    case RaceBlowEffectType::EXP_20:
        return "EXP_20";
    case RaceBlowEffectType::EXP_40:
        return "EXP_40";
    case RaceBlowEffectType::EXP_80:
        return "EXP_80";
    case RaceBlowEffectType::DISEASE:
        return "DISEASE";
    case RaceBlowEffectType::TIME:
        return "TIME";
    case RaceBlowEffectType::DR_LIFE:
        return "DR_LIFE";
    case RaceBlowEffectType::DR_MANA:
        return "DR_MANA";
    case RaceBlowEffectType::SUPERHURT:
        return "SUPERHURT";
    case RaceBlowEffectType::INERTIA:
        return "INERTIA";
    case RaceBlowEffectType::STUN:
        return "STUN";
    case RaceBlowEffectType::FLAVOR:
        return "FLAVOR";
    case RaceBlowEffectType::HUNGRY:
        return "HUNGRY";
    case RaceBlowEffectType::DEFECATE:
        return "DEFECATE";
    case RaceBlowEffectType::SANITY_BLAST:
        return "SANITY_BLAST";
    case RaceBlowEffectType::CHAOS:
        return "CHAOS";
    case RaceBlowEffectType::LOCKUP:
        return "LOCKUP";
    case RaceBlowEffectType::MAX:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}
