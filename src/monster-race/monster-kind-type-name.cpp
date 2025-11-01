/*!
 * @brief MonsterKindTypeの名前取得
 * @date 2025/01/23
 */

#include "monster-race/monster-kind-type-name.h"
#include "locale/language-switcher.h"

std::string get_monster_kind_type_name(MonsterKindType kind)
{
    switch (kind) {
    case MonsterKindType::UNIQUE:
        return _("ユニーク", "unique");
    case MonsterKindType::NONLIVING:
        return _("無生物", "nonliving");
    case MonsterKindType::HUMAN:
        return _("人間", "human");
    case MonsterKindType::QUANTUM:
        return _("量子生物", "quantum");
    case MonsterKindType::ORC:
        return _("オーク", "orc");
    case MonsterKindType::TROLL:
        return _("トロル", "troll");
    case MonsterKindType::GIANT:
        return _("巨人", "giant");
    case MonsterKindType::DRAGON:
        return _("ドラゴン", "dragon");
    case MonsterKindType::DEMON:
        return _("デーモン", "demon");
    case MonsterKindType::AMBERITE:
        return _("アンバーの王族", "amberite");
    case MonsterKindType::ANGEL:
        return _("天使", "angel");
    case MonsterKindType::EVIL:
        return _("邪悪", "evil");
    case MonsterKindType::GOOD:
        return _("善良", "good");
    case MonsterKindType::ANIMAL:
        return _("動物", "animal");
    case MonsterKindType::UNDEAD:
        return _("アンデッド", "undead");
    case MonsterKindType::ELDRAZI:
        return _("エルドラージ", "eldrazi");
    case MonsterKindType::ELF:
        return _("エルフ", "elf");
    case MonsterKindType::DWARF:
        return _("ドワーフ", "dwarf");
    case MonsterKindType::HOBBIT:
        return _("ホビット", "hobbit");
    case MonsterKindType::QUYLTHLUG:
        return _("クイルスルグ", "quylthlug");
    case MonsterKindType::SPIDER:
        return _("蜘蛛", "spider");
    case MonsterKindType::NASTY:
        return _("クッソ汚い", "nasty");
    case MonsterKindType::JOKE:
        return _("ふざけた", "joke");
    case MonsterKindType::YAKUZA:
        return _("ヤクザ", "yakuza");
    case MonsterKindType::NINJA:
        return _("ニンジャ", "ninja");
    case MonsterKindType::SUMOU_WRESTLER:
        return _("スモトリ", "sumou wrestler");
    case MonsterKindType::WARRIOR:
        return _("戦士", "warrior");
    case MonsterKindType::ROGUE:
        return _("盗賊", "rogue");
    case MonsterKindType::MAGE:
        return _("メイジ", "mage");
    case MonsterKindType::PRIEST:
        return _("プリースト", "priest");
    case MonsterKindType::PALADIN:
        return _("パラディン", "paladin");
    case MonsterKindType::RANGER:
        return _("レンジャー", "ranger");
    case MonsterKindType::SAMURAI:
        return _("サムライ", "samurai");
    case MonsterKindType::KARATEKA:
        return _("カラテカ", "karateka");
    case MonsterKindType::HOMO_SEXUAL:
        return _("ホモ", "homo sexual");
    case MonsterKindType::TANK:
        return _("戦車", "tank");
    case MonsterKindType::HENTAI:
        return _("エロゲ世界", "hentai");
    case MonsterKindType::ELEMENTAL:
        return _("エレメンタル", "elemental");
    case MonsterKindType::GOLEM:
        return _("ゴーレム", "golem");
    case MonsterKindType::PUYO:
        return _("ぷよ", "puyo");
    case MonsterKindType::ROBOT:
        return _("ロボット", "robot");
    case MonsterKindType::YAZYU:
        return _("野獣先輩", "Beast Senior");
    case MonsterKindType::SKELETON:
        return _("スケルトン", "skeleton");
    case MonsterKindType::DOG:
        return _("犬", "dog");
    case MonsterKindType::CAT:
        return _("猫", "cat");
    case MonsterKindType::MONKEY_SPACE:
        return _("猿空間", "monkey space");
    case MonsterKindType::MALE:
        return _("雄", "male");
    case MonsterKindType::FEMALE:
        return _("雌", "female");
    case MonsterKindType::CHOASIAN:
        return _("混沌の王族", "chaosian");
    case MonsterKindType::ZOMBIE:
        return _("ゾンビ", "zombie");
    case MonsterKindType::INSECT:
        return _("昆虫", "insect");
    case MonsterKindType::CANCER:
        return _("カニ", "cancer");
    case MonsterKindType::FUNGAS:
        return _("菌類", "fungas");
    case MonsterKindType::TURTLE:
        return _("亀", "turtle");
    case MonsterKindType::MIMIC:
        return _("ミミック", "mimic");
    case MonsterKindType::IXITXACHITL:
        return _("イクシツザチトル", "ixitxachitl");
    case MonsterKindType::NAGA:
        return _("ナーガ", "naga");
    case MonsterKindType::PERVERT:
        return _("変質者", "pervert");
    case MonsterKindType::SOLDIER:
        return _("兵士", "soldier");
    case MonsterKindType::APE:
        return _("類人猿", "ape");
    case MonsterKindType::HORSE:
        return _("馬", "horse");
    case MonsterKindType::FROG:
        return _("カエル", "frog");
    case MonsterKindType::BEHOLDER:
        return _("ビホルダー", "beholder");
    case MonsterKindType::YEEK:
        return _("イーク", "yeek");
    case MonsterKindType::AQUATIC_MAMMAL:
        return _("水棲哺乳類", "aquatic mammal");
    case MonsterKindType::FISH:
        return _("魚類", "fish");
    case MonsterKindType::BIRD:
        return _("鳥", "bird");
    case MonsterKindType::WALL:
        return _("壁", "wall");
    case MonsterKindType::PLANT:
        return _("植物", "plant");
    case MonsterKindType::FUNGUS:
        return _("菌類", "fungus");
    case MonsterKindType::SNAKE:
        return _("蛇", "snake");
    case MonsterKindType::FAIRY:
        return _("妖精", "fairy");
    case MonsterKindType::VAMPIRE:
        return _("吸血鬼", "vampire");
    case MonsterKindType::BEAR:
        return _("熊", "bear");
    case MonsterKindType::VORTEX:
        return _("ボルテックス", "vortex");
    case MonsterKindType::OOZE:
        return _("ウーズ", "ooze");
    case MonsterKindType::DINOSAUR:
        return _("恐竜", "dinosaur");
    case MonsterKindType::LICH:
        return _("リッチ", "lich");
    case MonsterKindType::GHOST:
        return _("幽霊", "ghost");
    case MonsterKindType::BERSERK:
        return _("狂戦士", "berserk");
    case MonsterKindType::EXPLOSIVE:
        return _("爆発物", "explosive");
    case MonsterKindType::RAT:
        return _("ネズミ", "rat");
    case MonsterKindType::MINOTAUR:
        return _("ミノタウロス", "minotaur");
    case MonsterKindType::SKAVEN:
        return _("スケイヴン", "skaven");
    case MonsterKindType::KOBOLD:
        return _("コボルド", "kobold");
    case MonsterKindType::OGRE:
        return _("オーガ", "ogre");
    case MonsterKindType::BOVINE:
        return _("牛", "bovine");
    case MonsterKindType::MERFOLK:
        return _("マーフォーク", "merfolk");
    case MonsterKindType::SHARK:
        return _("サメ", "shark");
    case MonsterKindType::HYDRA:
        return _("ヒドラ", "hydra");
    case MonsterKindType::SHIP:
        return _("船舶", "ship");
    case MonsterKindType::SLUG:
        return _("ナメクジ", "slug");
    case MonsterKindType::EYE:
        return _("目", "eye");
    case MonsterKindType::ALIEN:
        return _("異星人", "alien");
    case MonsterKindType::GRANDMA:
        return _("ババア", "grandma");
    case MonsterKindType::MAX:
        return _("不明", "unknown");
    }
    return _("不明", "unknown");
}
