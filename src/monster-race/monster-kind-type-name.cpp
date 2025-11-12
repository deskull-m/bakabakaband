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
    case MonsterKindType::RABBIT:
        return _("兎", "rabbit");
    case MonsterKindType::PEASANT:
        return _("農民", "peasant");
    case MonsterKindType::RABBLE:
        return _("賤民", "rabble");
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
    case MonsterKindType::PAPER:
        return _("紙で出来た", "made of paper");
    case MonsterKindType::DEEPONE:
        return _("深きもの", "deep one");
    case MonsterKindType::PHYREXIAN:
        return _("ファイレクシア人", "phyrexian");
    case MonsterKindType::HORROR:
        return _("ホラー", "horror");
    case MonsterKindType::WORM:
        return _("ワーム", "worm");
    case MonsterKindType::OCTOPUS:
        return _("タコ", "octopus");
    case MonsterKindType::SQUID:
        return _("イカ", "squid");
    case MonsterKindType::FACE:
        return _("顔面", "face");
    case MonsterKindType::HAND:
        return _("手", "hand");
    case MonsterKindType::MINDFLAYER:
        return _("マインドフレア", "mindflayer");
    case MonsterKindType::NIBELUNG:
        return _("ニーベルング", "nibelung");
    case MonsterKindType::GNOME:
        return _("ノーム", "gnome");
    case MonsterKindType::KRAKEN:
        return _("クラーケン", "kraken");
    case MonsterKindType::HARPY:
        return _("ハーピー", "harpy");
    case MonsterKindType::ALARM:
        return _("警報機", "alarm");
    case MonsterKindType::DEER:
        return _("鹿", "deer");
    case MonsterKindType::ELEPHANT:
        return _("象", "elephant");
    case MonsterKindType::LIZARD:
        return _("トカゲ", "lizard");
    case MonsterKindType::AVATAR:
        return _("アヴァター", "avatar");
    case MonsterKindType::NIGHTSHADE:
        return _("ナイトシェード", "nightshade");
    case MonsterKindType::HIPPO:
        return _("カバ", "hippo");
    case MonsterKindType::BAT:
        return _("コウモリ", "bat");
    case MonsterKindType::PLANESWALKER:
        return _("プレインズウォーカー", "planeswalker");
    case MonsterKindType::BOAR:
        return _("猪", "boar");
    case MonsterKindType::ARCHER:
        return _("アーチャー", "archer");
    case MonsterKindType::MAX:
        return _("不明", "unknown");
    }
    return _("不明", "unknown");
}

std::string get_monster_kind_type_tag(MonsterKindType kind)
{
    switch (kind) {
    case MonsterKindType::UNIQUE:
        return "UNIQUE";
    case MonsterKindType::NONLIVING:
        return "NONLIVING";
    case MonsterKindType::HUMAN:
        return "HUMAN";
    case MonsterKindType::QUANTUM:
        return "QUANTUM";
    case MonsterKindType::ORC:
        return "ORC";
    case MonsterKindType::TROLL:
        return "TROLL";
    case MonsterKindType::GIANT:
        return "GIANT";
    case MonsterKindType::DRAGON:
        return "DRAGON";
    case MonsterKindType::DEMON:
        return "DEMON";
    case MonsterKindType::AMBERITE:
        return "AMBERITE";
    case MonsterKindType::ANGEL:
        return "ANGEL";
    case MonsterKindType::EVIL:
        return "EVIL";
    case MonsterKindType::GOOD:
        return "GOOD";
    case MonsterKindType::ANIMAL:
        return "ANIMAL";
    case MonsterKindType::UNDEAD:
        return "UNDEAD";
    case MonsterKindType::ELDRAZI:
        return "ELDRAZI";
    case MonsterKindType::ELF:
        return "ELF";
    case MonsterKindType::DWARF:
        return "DWARF";
    case MonsterKindType::HOBBIT:
        return "HOBBIT";
    case MonsterKindType::QUYLTHLUG:
        return "QUYLTHLUG";
    case MonsterKindType::SPIDER:
        return "SPIDER";
    case MonsterKindType::NASTY:
        return "NASTY";
    case MonsterKindType::JOKE:
        return "JOKE";
    case MonsterKindType::YAKUZA:
        return "YAKUZA";
    case MonsterKindType::NINJA:
        return "NINJA";
    case MonsterKindType::SUMOU_WRESTLER:
        return "SUMOU_WRESTLER";
    case MonsterKindType::WARRIOR:
        return "WARRIOR";
    case MonsterKindType::ROGUE:
        return "ROGUE";
    case MonsterKindType::MAGE:
        return "MAGE";
    case MonsterKindType::PRIEST:
        return "PRIEST";
    case MonsterKindType::PALADIN:
        return "PALADIN";
    case MonsterKindType::RANGER:
        return "RANGER";
    case MonsterKindType::SAMURAI:
        return "SAMURAI";
    case MonsterKindType::KARATEKA:
        return "KARATEKA";
    case MonsterKindType::HOMO_SEXUAL:
        return "HOMO_SEXUAL";
    case MonsterKindType::TANK:
        return "TANK";
    case MonsterKindType::HENTAI:
        return "HENTAI";
    case MonsterKindType::ELEMENTAL:
        return "ELEMENTAL";
    case MonsterKindType::GOLEM:
        return "GOLEM";
    case MonsterKindType::PUYO:
        return "PUYO";
    case MonsterKindType::ROBOT:
        return "ROBOT";
    case MonsterKindType::YAZYU:
        return "YAZYU";
    case MonsterKindType::SKELETON:
        return "SKELETON";
    case MonsterKindType::DOG:
        return "DOG";
    case MonsterKindType::CAT:
        return "CAT";
    case MonsterKindType::RABBIT:
        return "RABBIT";
    case MonsterKindType::PEASANT:
        return "PEASANT";
    case MonsterKindType::RABBLE:
        return "RABBLE";
    case MonsterKindType::MONKEY_SPACE:
        return "MONKEY_SPACE";
    case MonsterKindType::MALE:
        return "MALE";
    case MonsterKindType::FEMALE:
        return "FEMALE";
    case MonsterKindType::CHOASIAN:
        return "CHOASIAN";
    case MonsterKindType::ZOMBIE:
        return "ZOMBIE";
    case MonsterKindType::INSECT:
        return "INSECT";
    case MonsterKindType::CANCER:
        return "CANCER";
    case MonsterKindType::FUNGAS:
        return "FUNGAS";
    case MonsterKindType::TURTLE:
        return "TURTLE";
    case MonsterKindType::MIMIC:
        return "MIMIC";
    case MonsterKindType::IXITXACHITL:
        return "IXITXACHITL";
    case MonsterKindType::NAGA:
        return "NAGA";
    case MonsterKindType::PERVERT:
        return "PERVERT";
    case MonsterKindType::SOLDIER:
        return "SOLDIER";
    case MonsterKindType::APE:
        return "APE";
    case MonsterKindType::HORSE:
        return "HORSE";
    case MonsterKindType::FROG:
        return "FROG";
    case MonsterKindType::BEHOLDER:
        return "BEHOLDER";
    case MonsterKindType::YEEK:
        return "YEEK";
    case MonsterKindType::AQUATIC_MAMMAL:
        return "AQUATIC_MAMMAL";
    case MonsterKindType::FISH:
        return "FISH";
    case MonsterKindType::BIRD:
        return "BIRD";
    case MonsterKindType::WALL:
        return "WALL";
    case MonsterKindType::PLANT:
        return "PLANT";
    case MonsterKindType::FUNGUS:
        return "FUNGUS";
    case MonsterKindType::SNAKE:
        return "SNAKE";
    case MonsterKindType::FAIRY:
        return "FAIRY";
    case MonsterKindType::VAMPIRE:
        return "VAMPIRE";
    case MonsterKindType::BEAR:
        return "BEAR";
    case MonsterKindType::VORTEX:
        return "VORTEX";
    case MonsterKindType::OOZE:
        return "OOZE";
    case MonsterKindType::DINOSAUR:
        return "DINOSAUR";
    case MonsterKindType::LICH:
        return "LICH";
    case MonsterKindType::GHOST:
        return "GHOST";
    case MonsterKindType::BERSERK:
        return "BERSERK";
    case MonsterKindType::EXPLOSIVE:
        return "EXPLOSIVE";
    case MonsterKindType::RAT:
        return "RAT";
    case MonsterKindType::MINOTAUR:
        return "MINOTAUR";
    case MonsterKindType::SKAVEN:
        return "SKAVEN";
    case MonsterKindType::KOBOLD:
        return "KOBOLD";
    case MonsterKindType::OGRE:
        return "OGRE";
    case MonsterKindType::BOVINE:
        return "BOVINE";
    case MonsterKindType::MERFOLK:
        return "MERFOLK";
    case MonsterKindType::SHARK:
        return "SHARK";
    case MonsterKindType::HYDRA:
        return "HYDRA";
    case MonsterKindType::SHIP:
        return "SHIP";
    case MonsterKindType::SLUG:
        return "SLUG";
    case MonsterKindType::EYE:
        return "EYE";
    case MonsterKindType::ALIEN:
        return "ALIEN";
    case MonsterKindType::GRANDMA:
        return "GRANDMA";
    case MonsterKindType::PAPER:
        return "PAPER";
    case MonsterKindType::DEEPONE:
        return "DEEPONE";
    case MonsterKindType::PHYREXIAN:
        return "PHYREXIAN";
    case MonsterKindType::HORROR:
        return "HORROR";
    case MonsterKindType::WORM:
        return "WORM";
    case MonsterKindType::OCTOPUS:
        return "OCTOPUS";
    case MonsterKindType::SQUID:
        return "SQUID";
    case MonsterKindType::FACE:
        return "FACE";
    case MonsterKindType::HAND:
        return "HAND";
    case MonsterKindType::MINDFLAYER:
        return "MINDFLAYER";
    case MonsterKindType::NIBELUNG:
        return "NIBELUNG";
    case MonsterKindType::GNOME:
        return "GNOME";
    case MonsterKindType::KRAKEN:
        return "KRAKEN";
    case MonsterKindType::HARPY:
        return "HARPY";
    case MonsterKindType::ALARM:
        return "ALARM";
    case MonsterKindType::DEER:
        return "DEER";
    case MonsterKindType::ELEPHANT:
        return "ELEPHANT";
    case MonsterKindType::LIZARD:
        return "LIZARD";
    case MonsterKindType::AVATAR:
        return "AVATAR";
    case MonsterKindType::NIGHTSHADE:
        return "NIGHTSHADE";
    case MonsterKindType::HIPPO:
        return "HIPPO";
    case MonsterKindType::BAT:
        return "BAT";
    case MonsterKindType::PLANESWALKER:
        return "PLANESWALKER";
    case MonsterKindType::BOAR:
        return "BOAR";
    case MonsterKindType::ARCHER:
        return "ARCHER";
    case MonsterKindType::MAX:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}
