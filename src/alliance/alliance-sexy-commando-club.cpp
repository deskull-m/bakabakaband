#include "alliance/alliance-sexy-commando-club.h"
#include "alliance/alliance.h"
#include "effect/effect-monster-util.h"
#include "effect/effect-monster.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "monster-floor/monster-summon.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "spell-kind/spells-teleport.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

#include "game-option/birth-options.h"
int AllianceSexyCommandoClub::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int point = 0;

    // 基本ステータス評価 - バランス重視だが特殊
    auto str = creature_ptr->stat_index[A_STR];
    auto dex = creature_ptr->stat_index[A_DEX];
    auto con = creature_ptr->stat_index[A_CON];
    auto cha = creature_ptr->stat_index[A_CHR];

    // 器用さを最重視（フラフープなど）
    if (dex >= 18) {
        point += 40;
    } else if (dex >= 16) {
        point += 25;
    } else if (dex >= 14) {
        point += 10;
    } else if (dex >= 12) {
        point += 5;
    } else if (dex >= 10) {
        point += 2;
    } else if (dex >= 8) {
        point -= 5;
    } else if (dex <= 6) {
        point -= 20;
    } else if (dex <= 10) {
        point -= 15;
    }

    // 魅力も重要（セクシーだから）
    if (cha >= 18) {
        point += 35;
    } else if (cha >= 16) {
        point += 20;
    } else if (cha >= 14) {
        point += 10;
    } else if (cha <= 10) {
        point -= 10;
    }

    // 筋力・耐久は中程度
    if (str >= 18) {
        point += 15;
    }
    if (con >= 18) {
        point += 15;
    }
    if (str <= 8 || con <= 8) {
        point -= 20;
    }

    // 種族による評価
    switch (creature_ptr->prace) {
    case PlayerRaceType::HUMAN:
        point += 20; // 人間らしさ
        break;
    case PlayerRaceType::HALF_ELF:
    case PlayerRaceType::HOBBIT:
        point += 15; // 親しみやすさ
        break;
    case PlayerRaceType::ELF:
    case PlayerRaceType::HIGH_ELF:
        point += 10; // 美しさ
        break;
    case PlayerRaceType::DWARF:
    case PlayerRaceType::GNOME:
        point += 25; // 意外性
        break;
    case PlayerRaceType::HALF_ORC:
    case PlayerRaceType::HALF_TROLL:
        point += 30; // ギャップ萌え
        break;
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SPECTRE:
        point -= 25; // セクシーじゃない
        break;
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ANDROID:
        point -= 15; // 無機物
        break;
    default:
        break;
    }

    // 職業による評価
    switch (creature_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::RANGER:
    case PlayerClassType::PALADIN:
        point += 25; // 体育会系
        break;
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        point += 35; // 格闘技系
        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA:
        point += 30; // 身体能力
        break;
    case PlayerClassType::TOURIST:
    case PlayerClassType::BEASTMASTER:
        point += 20; // ユニーク性
        break;
    case PlayerClassType::MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
        point += 10; // 頭脳派（フラフープの計算）
        break;
    case PlayerClassType::PRIEST:
    case PlayerClassType::MINDCRAFTER:
        point -= 10; // 真面目すぎ
        break;
    default:
        break;
    }

    // 性格による評価
    switch (creature_ptr->ppersonality) {
    case PERSONALITY_ORDINARY:
        point += 15; // 普通が一番
        break;
    case PERSONALITY_SEXY:
        point += 50; // 完璧
        break;
    case PERSONALITY_LUCKY:
    case PERSONALITY_PATIENT:
        point += 20;
        break;
    case PERSONALITY_CHARGEMAN:
    case PERSONALITY_COMBAT:
        point += 25; // ノリの良さ
        break;
    case PERSONALITY_LAZY:
        point += 30; // 脱力系
        break;
    case PERSONALITY_MUNCHKIN:
        point -= 30; // ガチ勢お断り
        break;
    default:
        break;
    }

    // レベルによる補正（青春補正）
    if (creature_ptr->level <= 20) {
        point += 10;
    } else if (creature_ptr->level >= 40) {
        point -= 5;
    }

    return point;
}

bool AllianceSexyCommandoClub::isAnnihilated()
{
    // セクシーコマンドー部は不滅（卒業はあっても壊滅しない）
    /*
    if (this->impression <= -250) {
        if (one_in_(20)) { // 5%の確率で卒業
            msg_print("セクシーコマンドー部から卒業通知が届いた...");
            msg_print("「青春は終わりを告げた。だが、セクシーは永遠なり！」");
            return true;
        }
    }
    */
    return false;
}

void AllianceSexyCommandoClub::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    /*
    if (this->impression >= -50) {
        // 軽微な制裁：フラフープ攻撃
        msg_print("どこからともなくフラフープが飛んできた！");
        msg_print("「これがセクシーコマンドー奥義、『フラフープ投げ』だ！」");

        // 軽いダメージと混乱
        take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(20) + 10, "フラフープ");
        if (one_in_(3)) {
            msg_print("フラフープの回転であなたは目を回した！");
            set_confused(creature_ptr, creature_ptr->confused + randint1(10));
        }
    } else if (this->impression >= -100) {
        // 中程度の制裁：けん玉攻撃
        msg_print("空中からけん玉が降ってきた！");
        msg_print("「奥義『けん玉乱舞』を食らうがよい！」");

        // 中程度のダメージとスタン
        take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(40) + 20, "けん玉乱舞");
        if (one_in_(2)) {
            msg_print("けん玉の複雑な動きについていけない！");
            set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint1(5));
        }

        // 部員召喚
        for (int i = 0; i < 2; i++) {
            summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x,
                creature_ptr->current_floor_ptr->dun_level + 5,
                SUMMON_HUMAN, PM_FORCE_PET | PM_NO_PET);
        }
    } else if (this->impression >= -150) {
        // 重い制裁：マサルさん本人登場
        msg_print("「やあ、僕はマサルだよ〜♪」");
        msg_print("マサルさん本人が現れた！");
        msg_print("「君にはセクシーコマンドー最終奥義を見せてあげよう！」");

        // 大ダメージと複数の状態異常
        take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(80) + 40, "セクシーコマンドー最終奥義");

        if (one_in_(2)) {
            msg_print("わけのわからない動きで頭が混乱した！");
            set_confused(creature_ptr, creature_ptr->confused + randint1(20));
        }
        if (one_in_(3)) {
            msg_print("あまりのセクシーさに立っていられない！");
            set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint1(8));
        }
        if (one_in_(4)) {
            msg_print("「愛の力で君を別の場所に送ってあげよう♪」");
            teleport_player(creature_ptr, 200, TELEPORT_NONMAGICAL);
        }

        // 部員大量召喚
        for (int i = 0; i < 4; i++) {
            summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x,
                creature_ptr->current_floor_ptr->dun_level + 10,
                SUMMON_HUMAN, PM_FORCE_PET | PM_NO_PET);
        }
    } else {
        // 最重の制裁：部活動総動員
        msg_print("「みんな〜、集合だよ〜♪」");
        msg_print("セクシーコマンドー部全員が現れた！");
        msg_print("「君にはセクシーの真髄を叩き込んであげる♪」");

        // 極大ダメージ
        take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(120) + 60, "部活動総攻撃");

        // 全状態異常のオンパレード
        msg_print("フラフープ、けん玉、缶蹴り、竹馬、コマ回し...あらゆる技が炸裂した！");
        set_confused(creature_ptr, creature_ptr->confused + randint1(30));
        set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint1(15));
        set_slow(creature_ptr, creature_ptr->slow + randint1(20), false);

        if (one_in_(2)) {
            msg_print("あまりのカオスぶりに気を失いそうだ！");
            set_image(creature_ptr, creature_ptr->image + randint1(50));
        }

        if (one_in_(3)) {
            msg_print("「愛と青春の転校生活動〜♪」");
            teleport_player(creature_ptr, 500, TELEPORT_NONMAGICAL);
        }

        // 部員軍団召喚
        for (int i = 0; i < 8; i++) {
            summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x,
                creature_ptr->current_floor_ptr->dun_level + 15,
                SUMMON_HUMAN, PM_FORCE_PET | PM_NO_PET);
        }

        // 装備品にも影響（セクシーに改造）
        if (one_in_(4)) {
            msg_print("あなたの装備がセクシーに改造された！");
            // 装備の名前変更や特殊効果追加の処理をここに
        }
    }
    */
}
