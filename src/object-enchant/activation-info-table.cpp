#include "object-enchant/activation-info-table.h"
#include "artifact/random-art-effects.h"
#include "locale/language-switcher.h"
#include <sstream>

/*!
 * @brief アイテムの発動効果テーブル
 */
const std::vector<ActivationType> activation_info = {
    { "SUNLIGHT", RandomArtActType::SUNLIGHT, 10, 250, 10, 0, _("太陽光線", "beam of sunlight") },
    { "BO_MISS_1", RandomArtActType::BO_MISS_1, 10, 250, 2, 0, _("マジック・ミサイル(2d6)", "magic missile (2d6)") },
    { "BA_POIS_1", RandomArtActType::BA_POIS_1, 10, 300, 4, 0, _("悪臭雲(12)", "stinking cloud (12)") },
    { "BO_ELEC_1", RandomArtActType::BO_ELEC_1, 20, 250, 5, 0, _("サンダー・ボルト(4d8)", "lightning bolt (4d8)") },
    { "BO_ACID_1", RandomArtActType::BO_ACID_1, 20, 250, 6, 0, _("アシッド・ボルト(5d8)", "acid bolt (5d8)") },
    { "BO_COLD_1", RandomArtActType::BO_COLD_1, 20, 250, 7, 0, _("アイス・ボルト(6d8)", "frost bolt (6d8)") },
    { "BO_FIRE_1", RandomArtActType::BO_FIRE_1, 20, 250, 8, 0, _("ファイア・ボルト(9d8)", "fire bolt (9d8)") },
    { "BA_COLD_1", RandomArtActType::BA_COLD_1, 30, 750, 6, 0, _("アイス・ボール(48)", "ball of cold (48)") },
    { "BA_COLD_2", RandomArtActType::BA_COLD_2, 40, 1000, 12, 0, _("アイス・ボール(100)", "ball of cold (100)") },
    { "BA_COLD_3", RandomArtActType::BA_COLD_3, 70, 2500, 50, 0, _("巨大アイス・ボール(400)", "ball of cold (400)") },
    { "BA_FIRE_1", RandomArtActType::BA_FIRE_1, 30, 1000, 9, 0, _("ファイア・ボール(72)", "ball of fire (72)") },
    { "BA_FIRE_2", RandomArtActType::BA_FIRE_2, 40, 1500, 15, 0, _("巨大ファイア・ボール(120)", "large fire ball (120)") },
    { "BA_FIRE_3", RandomArtActType::BA_FIRE_3, 60, 1750, 40, 0, _("巨大ファイア・ボール(300)", "fire ball (300)") },
    { "BA_FIRE_4", RandomArtActType::BA_FIRE_4, 40, 1000, 12, 0, _("ファイア・ボール(100)", "fire ball (100)") },
    { "BA_ELEC_2", RandomArtActType::BA_ELEC_2, 40, 1000, 12, 0, _("サンダー・ボール(100)", "ball of lightning (100)") },
    { "BA_ELEC_3", RandomArtActType::BA_ELEC_3, 70, 2500, 70, 0, _("巨大サンダー・ボール(500)", "ball of lightning (500)") },
    { "BA_ACID_1", RandomArtActType::BA_ACID_1, 30, 1000, 12, 0, _("アシッド・ボール(100)", "ball of acid (100)") },
    { "BA_NUKE_1", RandomArtActType::BA_NUKE_1, 50, 1000, 12, 0, _("放射能球(100)", "ball of nuke (100)") },
    { "HYPODYNAMIA_1", RandomArtActType::HYPODYNAMIA_1, 30, 500, 12, 0, _("窒息攻撃(100)", "a strangling attack (100)") },
    { "HYPODYNAMIA_2", RandomArtActType::HYPODYNAMIA_2, 40, 750, 15, 0, _("衰弱の矢(120)", "hypodynamic bolt (120)") },
    { "DRAIN_1", RandomArtActType::DRAIN_1, 40, 1000, 20, 0, _("吸収の矢(3*50)", "drain bolt (3*50)") },
    { "BO_MISS_2", RandomArtActType::BO_MISS_2, 40, 1000, 20, 0, _("矢(150)", "arrows (150)") },
    { "WHIRLWIND", RandomArtActType::WHIRLWIND, 50, 7500, 25, 0, _("カマイタチ", "whirlwind attack") },
    { "DRAIN_2", RandomArtActType::DRAIN_2, 50, 2500, 40, 0, _("吸収の矢(3*100)", "drain bolt (3*100)") },
    { "CALL_CHAOS", RandomArtActType::CALL_CHAOS, 70, 5000, 35, 0, _("混沌召来", "call chaos") },
    { "ROCKET", RandomArtActType::ROCKET, 70, 5000, 20, 0, _("ロケット(120+レベル)", "launch rocket (120+level)") },
    { "DISP_EVIL", RandomArtActType::DISP_EVIL, 50, 4000, 50, 0, _("邪悪退散(x5)", "dispel evil (x5)") },
    { "BA_MISS_3", RandomArtActType::BA_MISS_3, 50, 1500, 50, 0, _("エレメントのブレス(300)", "elemental breath (300)") },
    { "DISP_GOOD", RandomArtActType::DISP_GOOD, 50, 3500, 50, 0, _("善良退散(x5)", "dispel good (x5)") },
    { "BO_MANA", RandomArtActType::BO_MANA, 40, 1500, 20, 0, _("魔法の矢(150)", "a magical arrow (150)") },
    { "BA_WATER", RandomArtActType::BA_WATER, 50, 2000, 25, 0, _("ウォーター・ボール(200)", "water ball (200)") },
    { "BA_STAR", RandomArtActType::BA_STAR, 50, 2200, 25, 0, _("巨大スター・ボール(200)", "large star ball (200)") },

    { "BA_DARK", RandomArtActType::BA_DARK, 50, 2200, 30, 0, _("暗黒の嵐(250)", "darkness storm (250)") },
    { "BA_MANA", RandomArtActType::BA_MANA, 70, 2500, 30, 0, _("魔力の嵐(250)", "a mana storm (250)") },
    { "PESTICIDE", RandomArtActType::PESTICIDE, 10, 500, 10, 0, _("害虫の駆除", "dispel pests") },
    { "BLINDING_LIGHT", RandomArtActType::BLINDING_LIGHT, 30, 5000, 40, 0, _("眩しい光", "blinding light") },
    { "BIZARRE", RandomArtActType::BIZARRE, 90, 10000, 50, 0, _("信じ難いこと", "bizarre things") },
    { "CAST_BA_STAR", RandomArtActType::CAST_BA_STAR, 70, 7500, 100, 0, _("スター・ボール・ダスト(150)", "cast star balls (150)") },
    { "BLADETURNER", RandomArtActType::BLADETURNER, 80, 20000, 80, 0,
        _("エレメントのブレス(300), 士気高揚、祝福、耐性", "breathe elements (300), hero, bless, and resistance") },
    { "BR_FIRE", RandomArtActType::BR_FIRE, 50, 5000, tl::nullopt, 0, _("火炎のブレス (200)", "fire breath (200)") },
    { "BR_COLD", RandomArtActType::BR_COLD, 50, 5000, tl::nullopt, 0, _("冷気のブレス (200)", "cold breath (200)") },
    { "BR_DRAGON", RandomArtActType::BR_DRAGON, 70, 10000, 30, 0, "" /* built by item_activation_dragon_breath() */ },
    { "CONFUSE", RandomArtActType::CONFUSE, 10, 500, 10, 0, _("パニック・モンスター", "confuse monster") },
    { "SLEEP", RandomArtActType::SLEEP, 10, 750, 15, 0, _("周囲のモンスターを眠らせる", "sleep nearby monsters") },
    { "QUAKE", RandomArtActType::QUAKE, 30, 600, 20, 0, _("地震", "earthquake") },
    { "TERROR", RandomArtActType::TERROR, 20, 2500, tl::nullopt, 0, _("恐慌", "terror") },
    { "TELE_AWAY", RandomArtActType::TELE_AWAY, 20, 2000, 15, 0, _("テレポート・アウェイ", "teleport away") },
    { "BANISH_EVIL", RandomArtActType::BANISH_EVIL, 40, 2000, 250, 0, _("邪悪消滅", "banish evil") },
    { "GENOCIDE", RandomArtActType::GENOCIDE, 50, 10000, 500, 0, _("抹殺", "genocide") },
    { "MASS_GENO", RandomArtActType::MASS_GENO, 50, 10000, 1000, 0, _("周辺抹殺", "mass genocide") },
    { "SCARE_AREA", RandomArtActType::SCARE_AREA, 20, 2500, 20, 0, _("モンスター恐慌", "frighten monsters") },
    { "AGGRAVATE", RandomArtActType::AGGRAVATE, 0, 100, 0, 0, _("モンスターを怒らせる", "aggravate monsters") },
    { "CHARM_ANIMAL", RandomArtActType::CHARM_ANIMAL, 40, 7500, 200, 0, _("動物魅了", "charm animal") },
    { "CHARM_UNDEAD", RandomArtActType::CHARM_UNDEAD, 40, 10000, 333, 0, _("アンデッド従属", "enslave undead") },
    { "CHARM_OTHER", RandomArtActType::CHARM_OTHER, 40, 10000, 400, 0, _("モンスター魅了", "charm monster") },
    { "CHARM_ANIMALS", RandomArtActType::CHARM_ANIMALS, 40, 12500, 500, 0, _("動物友和", "animal friendship") },
    { "CHARM_OTHERS", RandomArtActType::CHARM_OTHERS, 40, 17500, 750, 0, _("周辺魅了", "mass charm") },
    { "SUMMON_ANIMAL", RandomArtActType::SUMMON_ANIMAL, 50, 10000, 200, 300, _("動物召喚", "summon animal") },
    { "SUMMON_PHANTOM", RandomArtActType::SUMMON_PHANTOM, 50, 12000, 200, 200, _("幻霊召喚", "summon phantasmal servant") },
    { "SUMMON_ELEMENTAL", RandomArtActType::SUMMON_ELEMENTAL, 50, 15000, 750, 0, _("エレメンタル召喚", "summon elemental") },
    { "SUMMON_DEMON", RandomArtActType::SUMMON_DEMON, 50, 20000, 666, 0, _("悪魔召喚", "summon demon") },
    { "SUMMON_UNDEAD", RandomArtActType::SUMMON_UNDEAD, 50, 20000, 666, 0, _("アンデッド召喚", "summon undead") },
    { "SUMMON_HOUND", RandomArtActType::SUMMON_HOUND, 50, 15000, 300, 0, _("ハウンド召喚", "summon hound") },
    { "SUMMON_DAWN", RandomArtActType::SUMMON_DAWN, 50, 15000, 500, 0, _("暁の師団召喚", "summon the Legion of the Dawn") },

    { "SUMMON_OCTOPUS", RandomArtActType::SUMMON_OCTOPUS, 50, 15000, 300, 0, _("蛸の大群召喚", "summon octopus") },
    { "CHOIR_SINGS", RandomArtActType::CHOIR_SINGS, 60, 20000, 300, 0, _("回復(777)、癒し、士気高揚", "heal 777 hit points, curing and HEROism") },
    { "CURE_LW", RandomArtActType::CURE_LW, 10, 500, 10, 0, _("恐怖除去/体力回復(30)", "remove fear and heal 30 hp") },
    { "CURE_MW", RandomArtActType::CURE_MW, 20, 750, 3, 3, _("傷回復(4d8)", "heal 4d8 and wounds") },
    { "CURE_POISON", RandomArtActType::CURE_POISON, 10, 1000, 5, 0, _("恐怖除去/毒消し", "remove fear and cure poison") },
    { "REST_LIFE", RandomArtActType::REST_EXP, 40, 7500, 450, 0, _("経験値復活", "restore experience") },
    { "REST_ALL", RandomArtActType::REST_ALL, 30, 15000, 750, 0, _("全ステータスと経験値復活", "restore stats and experience") },
    { "CURE_700", RandomArtActType::CURE_700, 40, 10000, 250, 0, _("体力回復(700)", "heal 700 hit points") },
    { "CURE_1000", RandomArtActType::CURE_1000, 50, 15000, 888, 0, _("体力回復(1000)", "heal 1000 hit points") },
    { "CURING", RandomArtActType::CURING, 30, 5000, 100, 0, _("癒し", "curing") },
    { "CURE_MANA_FULL", RandomArtActType::CURE_MANA_FULL, 60, 20000, 777, 0, _("魔力復活", "restore mana") },
    { "ESP", RandomArtActType::ESP, 30, 1500, 100, 0, _("テレパシー(期間 25+d30)", "telepathy (dur 25+d30)") },
    { "BERSERK", RandomArtActType::BERSERK, 10, 800, 75, 0, _("狂戦士化(25+d25ターン)", "berserk (25+d25 turns)") },
    { "PROT_EVIL", RandomArtActType::PROT_EVIL, 30, 5000, 100, 0, _("対邪悪結界(期間 3*レベル+d25)", "protect evil (dur level*3 + d25)") },
    { "RESIST_ALL", RandomArtActType::RESIST_ALL, 30, 5000, 111, 0, _("全耐性(期間 20+d20)", "resist elements (dur 20+d20)") },
    { "SPEED", RandomArtActType::SPEED, 40, 15000, 250, 0, _("加速(期間 20+d20)", "speed (dur 20+d20)") },
    { "MID_SPEED", RandomArtActType::MID_SPEED, 40, 20000, 250, 0, _("加速(期間 50+d50)", "speed (dur 50+d50)") },
    { "XTRA_SPEED", RandomArtActType::XTRA_SPEED, 40, 25000, 200, 200, _("加速(期間 75+d75)", "speed (dur 75+d75)") },
    { "WRAITH", RandomArtActType::WRAITH, 90, 25000, 1000, 0, _("幽体化(期間 (レベル/2)+d(レベル/2))", "wraith form (dur level/2 + d(level/2))") },
    { "INVULN", RandomArtActType::INVULN, 90, 25000, 1000, 0, _("無敵化(期間 8+d8)", "invulnerability (dur 8+d8)") },
    { "HERO", RandomArtActType::HERO, 10, 500, 30, 30, _("士気高揚", "heroism") },
    { "HERO_SPEED", RandomArtActType::HERO_SPEED, 30, 20000, 100, 200, _("士気高揚, スピード(期間 50+d50ターン)", "hero and +10 to speed (dur 50+d50)") },
    { "ACID_BALL_AND_RESISTANCE", RandomArtActType::ACID_BALL_AND_RESISTANCE, 20, 2000, 40, 40, _("酸の球と酸への耐性", "acid ball and resist") },
    { "FIRE_BALL_AND_RESISTANCE", RandomArtActType::FIRE_BALL_AND_RESISTANCE, 20, 2000, 40, 40, _("火炎の球と火炎への耐性", "fire ball and resist") },
    { "COLD_BALL_AND_RESISTANCE", RandomArtActType::COLD_BALL_AND_RESISTANCE, 20, 2000, 40, 40, _("冷気の球と冷気への耐性", "cold ball and resist") },
    { "ELEC_BALL_AND_RESISTANCE", RandomArtActType::ELEC_BALL_AND_RESISTANCE, 20, 2000, 40, 40, _("電撃の球と電撃への耐性", "elec ball and resist") },
    { "POIS_BALL_AND_RESISTANCE", RandomArtActType::POIS_BALL_AND_RESISTANCE, 20, 2000, 40, 40, _("毒の球と毒への耐性", "pois ball and resist") },
    { "RESIST_ACID", RandomArtActType::RESIST_ACID, 20, 2000, 40, 40, _("酸への耐性(期間 20+d20)", "resist acid (dur 20+d20)") },
    { "RESIST_FIRE", RandomArtActType::RESIST_FIRE, 20, 2000, 40, 40, _("火炎への耐性(期間 20+d20)", "resist fire (dur 20+d20)") },
    { "RESIST_COLD", RandomArtActType::RESIST_COLD, 20, 2000, 40, 40, _("冷気への耐性(期間 20+d20)", "resist cold (dur 20+d20)") },
    { "RESIST_ELEC", RandomArtActType::RESIST_ELEC, 20, 2000, 40, 40, _("電撃への耐性(期間 20+d20)", "resist elec (dur 20+d20)") },
    { "RESIST_POIS", RandomArtActType::RESIST_POIS, 20, 2000, 40, 40, _("毒への耐性(期間 20+d20)", "resist poison (dur 20+d20)") },
    { "LIGHT", RandomArtActType::LIGHT, 10, 150, 10, 10, _("イルミネーション", "light area (dam 2d15)") },

    { "MAP_LIGHT", RandomArtActType::MAP_LIGHT, 30, 500, 50, 50, _("魔法の地図と光", "light (dam 2d15) & map area") },
    { "DETECT_ALL", RandomArtActType::DETECT_ALL, 30, 1000, 55, 55, _("全感知", "detection") },
    { "DETECT_XTRA", RandomArtActType::DETECT_XTRA, 50, 12500, 100, 0, _("全感知、探索、*鑑定*", "detection, probing and identify true") },
    { "ID_FULL", RandomArtActType::ID_FULL, 50, 10000, 75, 0, _("*鑑定*", "identify true") },
    { "ID_PLAIN", RandomArtActType::ID_PLAIN, 20, 1250, 10, 0, _("鑑定", "identify spell") },
    { "RUNE_EXPLO", RandomArtActType::RUNE_EXPLO, 40, 4000, 200, 0, _("爆発のルーン", "explosive rune") },
    { "RUNE_PROT", RandomArtActType::RUNE_PROT, 60, 10000, 400, 0, _("守りのルーン", "rune of protection") },
    { "SATIATE", RandomArtActType::SATIATE, 10, 2000, 200, 0, _("空腹充足", "satisfy hunger") },
    { "DEST_DOOR", RandomArtActType::DEST_DOOR, 10, 100, 10, 0, _("ドア破壊", "destroy doors") },
    { "STONE_MUD", RandomArtActType::STONE_MUD, 20, 1000, 3, 0, _("岩石溶解", "stone to mud") },
    { "RECHARGE", RandomArtActType::RECHARGE, 30, 1000, 70, 0, _("魔力充填", "recharging") },
    { "ALCHEMY", RandomArtActType::ALCHEMY, 50, 10000, 500, 0, _("錬金術", "alchemy") },
    { "DIM_DOOR", RandomArtActType::DIM_DOOR, 50, 10000, 100, 0, _("次元の扉", "dimension door") },
    { "TELEPORT", RandomArtActType::TELEPORT, 10, 2000, 25, 0, _("テレポート", "teleport") },
    { "RECALL", RandomArtActType::RECALL, 30, 7500, 200, 0, _("帰還の詔", "word of recall") },
    { "JUDGE", RandomArtActType::JUDGE, 90, 50000, 20, 20, _("体力と引き替えに千里眼と帰還", "clairvoyance and recall, draining you") },
    { "TELEKINESIS", RandomArtActType::TELEKINESIS, 20, 5500, 25, 25, _("物体を引き寄せる(重量25kgまで)", "a telekinesis (50 lb)") },
    { "DETECT_UNIQUE", RandomArtActType::DETECT_UNIQUE, 40, 10000, 200, 0, _("この階にいるユニークモンスターを表示", "list of the uniques on the level") },
    { "ESCAPE", RandomArtActType::ESCAPE, 10, 3000, 35, 0, _("逃走", "a getaway") },
    { "DISP_CURSE_XTRA", RandomArtActType::DISP_CURSE_XTRA, 40, 30000, 0, 0, _("*解呪*と調査", "dispel curse and probing") },
    { "BRAND_FIRE_BOLTS", RandomArtActType::BRAND_FIRE_BOLTS, 40, 20000, 999, 0, _("刃先のファイア・ボルト", "fire branding of bolts") },
    { "RECHARGE_XTRA", RandomArtActType::RECHARGE_XTRA, 70, 30000, 200, 0, _("魔力充填", "recharge item") },
    { "LORE", RandomArtActType::LORE, 10, 30000, 0, 0, _("危険を伴う鑑定", "perilous identify") },
    { "SHIKOFUMI", RandomArtActType::SHIKOFUMI, 10, 10000, 100, 100, _("四股踏み", "shiko") },
    { "PHASE_DOOR", RandomArtActType::PHASE_DOOR, 10, 1500, 10, 0, _("ショート・テレポート", "blink") },
    { "DETECT_ALL_MONS", RandomArtActType::DETECT_ALL_MONS, 30, 3000, 150, 0, _("全モンスター感知", "detect all monsters") },
    { "ULTIMATE_RESIST", RandomArtActType::ULTIMATE_RESIST, 90, 20000, 777, 0, _("士気高揚、祝福、究極の耐性", "hero, bless, and ultimate resistance") },
    { "CAST_OFF", RandomArtActType::CAST_OFF, 30, 15000, 100, 0, _("脱衣と小宇宙燃焼", "cast it off and cosmic heroism") },
    { "FISHING", RandomArtActType::FISHING, 0, 100, 0, 0, _("釣りをする", "fishing") },
    { "INROU", RandomArtActType::INROU, 40, 15000, 150, 150, _("例のアレ", "reveal your identity") },
    { "MURAMASA", RandomArtActType::MURAMASA, 0, 0, tl::nullopt, 0, _("腕力の上昇", "increase STR") },
    { "BLOODY_MOON", RandomArtActType::BLOODY_MOON, 0, 0, 3333, 0, _("属性変更", "change zokusei") },

    { "CRIMSON", RandomArtActType::CRIMSON, 0, 50000, 15, 0, _("ファイア！", "fire!") },
    { "STRAIN_HASTE", RandomArtActType::STRAIN_HASTE, 10, 1000, 120, 100, _("体力と引き換えに加速", "haste with strain") },
    { "GRAND_CROSS", RandomArtActType::GRAND_CROSS, 30, 15000, 250, 200, _("グランド・クロス", "grand cross") },
    { "TELEPORT_LEVEL", RandomArtActType::TELEPORT_LEVEL, 10, 1500, 100, 200, _("テレポート・レベル", "teleport level") },
    { "ARTS_FALLING_STAR", RandomArtActType::FALLING_STAR, 20, 5500, 30, 50, _("魔剣・流れ星", "blade arts 'falling star'") },
    { "ANIM_DEAD", RandomArtActType::ANIM_DEAD, 30, 2000, 10, 10, _("死者復活", "animate dead") },
    { "TREE_CREATION", RandomArtActType::TREE_CREATION, 50, 25000, 1000, 0, _("森林生成", "tree creation") },
    { "ELBERETH", RandomArtActType::ELBERETH, 10, 30000, 75, 0, _("エルベレスの結界", "Rune of Elbereth") },
    { "DETECT_TREASURE", RandomArtActType::DETECT_TREASURE, 10, 3000, 35, 0, _("財宝感知", "detect treasure") },
    { "HERO_BLESS", RandomArtActType::HERO_BLESS, 10, 500, 30, 30, _("士気高揚、祝福", "hero, bless") },
    { "CREATE_AMMO", RandomArtActType::CREATE_AMMO, 10, 30000, 200, 0, _("弾/矢の製造", "Create Ammo") },
    { "DISPEL_MAGIC", RandomArtActType::DISPEL_MAGIC, 10, 10000, 50, 50, _("魔力消去", "Dispel Magic") },
    { "DESTRUCTION", RandomArtActType::DESTRUCTION, 10, 30000, 50, 50, _("*破壊*", "*Destruction*") },
    { "HUGE_STINKING_STORM", RandomArtActType::HUGE_STINKING_STORM, 40, 25000, 200, 200, _("超巨大悪臭嵐", "huge stinking storm") },
    { "WHISTLE", RandomArtActType::WHISTLE, 0, 0, 100, 100, _("ペット呼び寄せ", "call pet") },
    { "CAPTURE_MONSTER", RandomArtActType::CAPTURE_MONSTER, 0, 0, 0, 0, _("モンスターを捕える、又は解放する。", "captures or releases a monster.") },
};

tl::optional<std::string> ActivationType::build_timeout_description() const
{
    if ((this->constant == 0) && (this->dice == 0)) {
        return _("いつでも", "every turn");
    }

    if (!this->constant) {
        return tl::nullopt;
    }

    std::stringstream ss;
    ss << _("", "every ");
    if (this->constant > 0) {
        ss << *this->constant;
        if (this->dice > 0) {
            ss << '+';
        }
    }

    if (this->dice > 0) {
        ss << 'd' << this->dice;
    }

    ss << _(" ターン毎", " turns");
    return ss.str();
}
