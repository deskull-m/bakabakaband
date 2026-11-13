#include "mutation/mutation-data.h"
#include "player-base/player-class.h"
#include "system/angband.h"
#include <iterator>

const MutationData MutationDataManager::mutation_table[] = {
    // 有益な変異 (0-31)
    { PlayerMutationType::SPIT_ACID, "酸を吐く能力を得た。", "You gain the ability to spit acid.", 4 },
    { PlayerMutationType::BR_FIRE, "火を吐く能力を得た。", "You gain the ability to breathe fire.", 3 },
    { PlayerMutationType::HYPN_GAZE, "催眠眼の能力を得た。", "Your eyes look mesmerizing...", 2 },
    { PlayerMutationType::TELEKINES, "物体を念動力で動かす能力を得た。", "You gain the ability to move objects telekinetically.", 2 },
    { PlayerMutationType::VTELEPORT, "自分の意思でテレポートする能力を得た。", "You gain the power of teleportation at will.", 3 },
    { PlayerMutationType::MIND_BLST, "精神攻撃の能力を得た。", "You gain the power of Mind Blast.", 2 },
    { PlayerMutationType::RADIATION, "あなたは強い放射線を発生し始めた。", "You start emitting hard radiation.", 2 },
    { PlayerMutationType::VAMPIRISM, "生命力を吸収できるようになった。", "You become vampiric.", 2 },
    { PlayerMutationType::SMELL_MET, "金属の匂いを嗅ぎ分けられるようになった。", "You smell a metallic odor.", 3 },
    { PlayerMutationType::SMELL_MON, "モンスターの臭いを嗅ぎ分けられるようになった。", "You smell filthy monsters.", 4 },
    { PlayerMutationType::BLINK, "近距離テレポートの能力を得た。", "You gain the power of minor teleportation.", 3 },
    { PlayerMutationType::EAT_ROCK, "壁が美味しそうに見える。", "The walls look delicious.", 2 },
    { PlayerMutationType::SWAP_POS, "他人の靴で一マイル歩くような気分がする。", "You feel like walking a mile in someone else's shoes.", 2 },
    { PlayerMutationType::SHRIEK, "あなたの声は相当強くなった。", "Your vocal cords get much tougher.", 3 },
    { PlayerMutationType::ILLUMINE, "あなたは光り輝いて部屋を明るくするようになった。", "You can light up rooms with your presence.", 3 },
    { PlayerMutationType::DET_CURSE, "邪悪な魔法を感知できるようになった。", "You can feel evil magics.", 2 },
    { PlayerMutationType::BERSERK, "制御できる激情を感じる。", "You feel a controlled rage.", 3 },
    { PlayerMutationType::POLYMORPH, "体が変異しやすくなった。", "Your body seems mutable.", 1 },
    { PlayerMutationType::MIDAS_TCH, "「ミダス王の手」の能力を得た。", "You gain the Midas touch.", 2 },
    { PlayerMutationType::GROW_MOLD, "突然カビに親しみを覚えた。", "You feel a sudden affinity for mold.", 1 },
    { PlayerMutationType::RESIST, "あなたは自分自身を守れる気がする。", "You feel like you can protect yourself.", 3 },
    { PlayerMutationType::EARTHQUAKE, "ダンジョンを破壊する能力を得た。", "You gain the ability to wreck the dungeon.", 3 },
    { PlayerMutationType::EAT_MAGIC, "魔法のアイテムが美味そうに見える。", "Your magic items look delicious.", 1 },
    { PlayerMutationType::WEIGH_MAG, "あなたは周囲にある魔法をより良く理解できる気がする。", "You feel you can better understand the magic around you.", 2 },
    { PlayerMutationType::STERILITY, "周りの全ての者に頭痛を起こすことができる。", "You can give everything around you a headache.", 1 },
    { PlayerMutationType::HIT_AND_AWAY, "突然、泥棒の気分が分かるようになった。", "You suddenly understand how thieves feel.", 2 },
    { PlayerMutationType::DAZZLE, "眩い閃光を発する能力を得た。", "You gain the ability to emit dazzling lights.", 3 },
    { PlayerMutationType::LASER_EYE, "あなたの目は一瞬焼け付いた。", "Your eyes burn for a moment.", 3 },
    { PlayerMutationType::RECALL, "少しだけホームシックになったが、すぐ直った。", "You feel briefly homesick, but it passes.", 2 },
    { PlayerMutationType::BANISH, "神聖な怒りの力に満たされた。", "You feel a holy wrath fill you.", 1 },
    { PlayerMutationType::COLD_TOUCH, "あなたの両手はとても冷たくなった。", "Your hands get very cold.", 2 },
    { PlayerMutationType::LAUNCHER, "あなたの物を投げる手はかなり強くなった気がする。", "Your throwing arm feels much stronger.", 2 },

    // 有害な変異 (32-63)
    { PlayerMutationType::BERS_RAGE, "あなたは狂暴化の発作を起こすようになった！", "You become subject to fits of berserk rage!", 1 },
    { PlayerMutationType::COWARDICE, "信じられないくらい臆病になった！", "You become an incredible coward!", 1 },
    { PlayerMutationType::RTELEPORT, "あなたの位置は非常に不確定になった。", "Your position seems very uncertain...", 1 },
    { PlayerMutationType::ALCOHOL, "あなたはアルコールを分泌するようになった。", "Your body starts producing alcohol!", 1 },
    { PlayerMutationType::HALLU, "あなたは幻覚を引き起こす精神錯乱に侵された。", "You are afflicted by a hallucinatory insanity!", 1 },
    { PlayerMutationType::FLATULENT, "あなたは制御不能な強烈な屁をこくようになった。", "You become subject to uncontrollable flatulence.", 1 },
    { PlayerMutationType::SCOR_TAIL, "サソリの尻尾が生えてきた！", "You grow a scorpion tail!", 2 },
    { PlayerMutationType::HORNS, "額に角が生えた！", "Horns pop forth into your forehead!", 2 },
    { PlayerMutationType::BEAK, "口が鋭く強いクチバシに変化した！", "Your mouth turns into a sharp, powerful beak!", 2 },
    { PlayerMutationType::ATT_DEMON, "悪魔を引き付けるようになった。", "You start attracting demons.", 2 },
    { PlayerMutationType::PROD_MANA, "あなたは制御不能な魔法のエネルギーを発生するようになった。", "You start producing magical energy uncontrollably.", 1 },
    { PlayerMutationType::SPEED_FLUX, "あなたは躁鬱質になった。", "You become manic-depressive.", 2 },
    { PlayerMutationType::BANISH_ALL, "恐ろしい力があなたの背後に潜んでいる気がする。", "You feel a terrifying power lurking behind you.", 2 },
    { PlayerMutationType::EAT_LIGHT, "あなたはウンゴリアントに奇妙な親しみを覚えるようになった。", "You feel a strange kinship with Ungoliant.", 1 },
    { PlayerMutationType::TRUNK, "あなたの鼻は伸びて象の鼻のようになった。", "Your nose grows into an elephant-like trunk.", 2 },
    { PlayerMutationType::ATT_ANIMAL, "動物を引き付けるようになった。", "You start attracting animals.", 1 },
    { PlayerMutationType::TENTACLES, "邪悪な触手が体の両側に生えてきた。", "Evil-looking tentacles sprout from your sides.", 1 },
    { PlayerMutationType::RAW_CHAOS, "周囲の空間が不安定になった気がする。", "You feel the universe is less stable around you.", 1 },
    { PlayerMutationType::NORMALITY, "あなたは奇妙なほど普通になった気がする。", "You feel strangely normal.", 1 },
    { PlayerMutationType::WRAITH, "あなたは幽体化したり実体化したりするようになった。", "You start to fade in and out of the physical world.", 1 },
    { PlayerMutationType::POLY_WOUND, "あなたはカオスの力が古い傷に入り込んでくるのを感じた。", "You feel forces of chaos entering your old scars.", 1 },
    { PlayerMutationType::WASTING, "あなたは突然おぞましい衰弱病にかかった。", "You suddenly contract a horrible wasting disease.", 1 },
    { PlayerMutationType::ATT_DRAGON, "あなたはドラゴンを引きつけるようになった。", "You start attracting dragons.", 1 },
    { PlayerMutationType::WEIRD_MIND, "あなたの思考は突然おかしな方向に向き始めた。", "Your thoughts suddenly take off in strange directions.", 2 },
    { PlayerMutationType::NAUSEA, "胃袋がピクピクしはじめた。", "Your stomach starts to roil nauseously.", 1 },
    { PlayerMutationType::CHAOS_GIFT, "あなたはカオスの守護悪魔の注意を惹くようになった。", "You attract the notice of a chaos deity!", 2 },
    { PlayerMutationType::WALK_SHAD, "あなたは現実が紙のように薄いと感じるようになった。", "You feel like reality is as thin as paper.", 1 },
    { PlayerMutationType::WARNING, "あなたは突然パラノイアになった気がする。", "You suddenly feel paranoid.", 2 },
    { PlayerMutationType::INVULN, "あなたは祝福され、無敵状態になる発作を起こすようになった。", "You are blessed with fits of invulnerability.", 1 },
    { PlayerMutationType::SP_TO_HP, "魔法の治癒の発作を起こすようになった。", "You are subject to fits of magical healing.", 2 },
    { PlayerMutationType::HP_TO_SP, "痛みを伴う精神明瞭化の発作を起こすようになった。", "You are subject to fits of painful clarity.", 1 },
    { PlayerMutationType::DISARM, "あなたの脚は長さが四倍になった。", "Your feet grow to four times their former size.", 1 },

    // 能力値変異 (64-95)
    { PlayerMutationType::HYPER_STR, "超人的に強くなった！", "You turn into a superhuman he-man!", 3 },
    { PlayerMutationType::PUNY, "筋肉が弱ってしまった...", "Your muscles wither away...", 3 },
    { PlayerMutationType::HYPER_INT, "あなたの脳は生体コンピュータに進化した！", "Your brain evolves into a living computer!", 3 },
    { PlayerMutationType::MORONIC, "脳が萎縮してしまった...", "Your brain withers away...", 3 },
    { PlayerMutationType::RESILIENT, "並外れてタフになった。", "You become extraordinarily resilient.", 2 },
    { PlayerMutationType::XTRA_FAT, "あなたは気持ち悪いくらい太った！", "You become sickeningly fat!", 2 },
    { PlayerMutationType::ALBINO, "アルビノになった！弱くなった気がする...", "You turn into an albino! You feel frail...", 2 },
    { PlayerMutationType::FLESH_ROT, "あなたの肉体は腐敗する病気に侵された！", "Your flesh is afflicted by a rotting disease!", 3 },
    { PlayerMutationType::SILLY_VOI, "声が間抜けなキーキー声になった！", "Your voice turns into a ridiculous squeak!", 2 },
    { PlayerMutationType::BLANK_FAC, "のっぺらぼうになった！", "Your face becomes completely featureless!", 2 },
    { PlayerMutationType::ILL_NORM, "心の安らぐ幻影を映し出すようになった。", "You start projecting a reassuring image.", 1 },
    { PlayerMutationType::XTRA_EYES, "新たに二つの目が出来た！", "You grow an extra pair of eyes!", 3 },
    { PlayerMutationType::MAGIC_RES, "魔法への耐性がついた。", "You become resistant to magic.", 2 },
    { PlayerMutationType::XTRA_NOIS, "あなたは奇妙な音を立て始めた！", "You start making strange noise!", 3 },
    { PlayerMutationType::INFRAVIS, "赤外線視力が増した。", "Your infravision is improved.", 3 },
    { PlayerMutationType::XTRA_LEGS, "新たに二本の足が生えてきた！", "You grow an extra pair of legs!", 2 },
    { PlayerMutationType::SHORT_LEG, "足が短い突起になってしまった！", "Your legs turn into short stubs!", 2 },
    { PlayerMutationType::ELEC_TOUC, "血管を電流が流れ始めた！", "Electricity starts running through you!", 2 },
    { PlayerMutationType::FIRE_BODY, "あなたの体は炎につつまれている。", "Your body is enveloped in flames!", 2 },
    { PlayerMutationType::WART_SKIN, "気持ち悪いイボイボが体中にできた！", "Disgusting warts appear everywhere on you!", 3 },
    { PlayerMutationType::SCALES, "肌が黒い鱗に変わった！", "Your skin turns into black scales!", 3 },
    { PlayerMutationType::IRON_SKIN, "あなたの肌は鉄になった！", "Your skin turns to steel!", 2 },
    { PlayerMutationType::WINGS, "背中に羽が生えた。", "You grow a pair of wings.", 2 },
    { PlayerMutationType::FEARLESS, "完全に怖れ知らずになった。", "You become completely fearless.", 3 },
    { PlayerMutationType::REGEN, "急速に回復し始めた。", "You start regenerating.", 2 },
    { PlayerMutationType::ESP, "テレパシーの能力を得た！", "You develop a telepathic ability!", 2 },
    { PlayerMutationType::LIMBER, "筋肉がしなやかになった。", "Your muscles become limber.", 3 },
    { PlayerMutationType::ARTHRITIS, "関節が突然痛み出した。", "Your joints suddenly hurt.", 3 },
    { PlayerMutationType::BAD_LUCK, "悪意に満ちた黒いオーラがあなたをとりまいた...", "There is a malignant black aura surrounding you...", 1 },
    { PlayerMutationType::VULN_ELEM, "妙に無防備になった気がする。", "You feel strangely exposed.", 1 },
    { PlayerMutationType::MOTION, "体の動作がより正確になった。", "You move with new assurance.", 3 },
    { PlayerMutationType::GOOD_LUCK, "慈悲深い白いオーラがあなをとりまいた...", "There is a benevolent white aura surrounding you...", 1 },

    // バカバカ蛮怒特殊変異 (96-105)
    { PlayerMutationType::DEFECATION, "あなたは脱糞を制御できなくなった。", "You become subject to uncontrollable defecation.", 4 },
    { PlayerMutationType::ZEERO_VIRUS, "あなたはゼEROウイルスに感染した！DAAAAAAA！", "You are infected with ZEERO virus. DAAAAAAA!", 2 },
    { PlayerMutationType::HOMO_SEXUAL, "あなたは同性にしか欲情しなくなった。", "You are only lustful for the same sex.", 3 },
    { PlayerMutationType::BI_SEXUAL, "あなたは男女問わず欲情するようになった。", "You have become lustful for both men and women.", 3 },
    { PlayerMutationType::WEAK_LOWER_BODY, "これマジ？上半身に比べて下半身が貧弱すぎるだろ…", "It's really? Your under body too weak compared to your upper body.", 2 },
    { PlayerMutationType::IKISUGI, "あなたは時々イキすぎるようになった。", "You sometimes climax too much.", 1 },
    { PlayerMutationType::ATT_NASTY, "あなたはクッソ汚い輩を引きつけるようになった。", "You attract nasty creatures.", 1 },
    { PlayerMutationType::ATT_PERVERT, "あなたは変質者を引きつけるようになった。", "You attract perverts.", 2 },
    { PlayerMutationType::DESTROYED_ASSHOLE, "あなたの肛門は完全に破壊された！尻の穴に装備できなくなった。", "Your asshole has been completely destroyed! You can no longer equip items there.", 1 },
    { PlayerMutationType::LOST_HEAD, "あなたは頭部を失った！頭に装備できなくなった。", "You have lost your head! You can no longer equip items on your head.", 1 }
};

/*!
 * @brief 突然変異の短い表示名の一覧 (PlayerMutationType の列挙順)
 * @details 添字で引くので列挙順と完全に一致させること。enum に値を足したら
 * ここにも足す (get_mutation_name() の static_assert が要素数の食い違いを検出する)。
 */
const MutationName MutationDataManager::mutation_name_table[] = {
    { PlayerMutationType::SPIT_ACID, "酸の唾", "Spit Acid" },
    { PlayerMutationType::BR_FIRE, "炎のブレス", "Breathe Fire" },
    { PlayerMutationType::HYPN_GAZE, "催眠睨み", "Hypnotic Gaze" },
    { PlayerMutationType::TELEKINES, "念動力", "Telekinesis" },
    { PlayerMutationType::VTELEPORT, "テレポート", "Teleport" },
    { PlayerMutationType::MIND_BLST, "精神攻撃", "Mind Blast" },
    { PlayerMutationType::RADIATION, "放射能", "Emit Radiation" },
    { PlayerMutationType::VAMPIRISM, "吸血", "Vampiric Drain" },
    { PlayerMutationType::SMELL_MET, "金属嗅覚", "Smell Metal" },
    { PlayerMutationType::SMELL_MON, "敵臭嗅覚", "Smell Monsters" },
    { PlayerMutationType::BLINK, "ショート・テレポート", "Blink" },
    { PlayerMutationType::EAT_ROCK, "岩喰い", "Eat Rock" },
    { PlayerMutationType::SWAP_POS, "位置交換", "Swap Position" },
    { PlayerMutationType::SHRIEK, "叫び", "Shriek" },
    { PlayerMutationType::ILLUMINE, "照明", "Illuminate" },
    { PlayerMutationType::DET_CURSE, "呪い感知", "Detect Curses" },
    { PlayerMutationType::BERSERK, "狂戦士化", "Berserk" },
    { PlayerMutationType::POLYMORPH, "変身", "Polymorph" },
    { PlayerMutationType::MIDAS_TCH, "ミダスの手", "Midas Touch" },
    { PlayerMutationType::GROW_MOLD, "カビ発生", "Grow Mold" },
    { PlayerMutationType::RESIST, "エレメント耐性", "Resist Elements" },
    { PlayerMutationType::EARTHQUAKE, "地震", "Earthquake" },
    { PlayerMutationType::EAT_MAGIC, "魔力喰い", "Eat Magic" },
    { PlayerMutationType::WEIGH_MAG, "魔力感知", "Weigh Magic" },
    { PlayerMutationType::STERILITY, "増殖阻止", "Sterilize" },
    { PlayerMutationType::HIT_AND_AWAY, "ヒットアンドアウェイ", "Panic Hit" },
    { PlayerMutationType::DAZZLE, "眩惑", "Dazzle" },
    { PlayerMutationType::LASER_EYE, "レーザー・アイ", "Laser Eye" },
    { PlayerMutationType::RECALL, "帰還", "Recall" },
    { PlayerMutationType::BANISH, "邪悪消滅", "Banish Evil" },
    { PlayerMutationType::COLD_TOUCH, "凍結の手", "Cold Touch" },
    { PlayerMutationType::LAUNCHER, "アイテム投げ", "Throw Object" },
    { PlayerMutationType::BERS_RAGE, "狂戦士化の発作", "Berserk Fits" },
    { PlayerMutationType::COWARDICE, "臆病", "Cowardice" },
    { PlayerMutationType::RTELEPORT, "ランダムテレポート", "Random Teleport" },
    { PlayerMutationType::ALCOHOL, "アルコール分泌", "Alcohol" },
    { PlayerMutationType::HALLU, "幻覚", "Hallucination" },
    { PlayerMutationType::FLATULENT, "猛烈な屁", "Flatulence" },
    { PlayerMutationType::SCOR_TAIL, "サソリの尻尾", "Scorpion Tail" },
    { PlayerMutationType::HORNS, "ツノ", "Horns" },
    { PlayerMutationType::BEAK, "クチバシ", "Beak" },
    { PlayerMutationType::ATT_DEMON, "デーモンを引き付ける", "Attract Demons" },
    { PlayerMutationType::PROD_MANA, "制御できない魔力", "Produce Mana" },
    { PlayerMutationType::SPEED_FLUX, "ランダムな加減速", "Speed Flux" },
    { PlayerMutationType::BANISH_ALL, "ランダムなモンスター消滅", "Banish All" },
    { PlayerMutationType::EAT_LIGHT, "光源喰い", "Eat Light" },
    { PlayerMutationType::TRUNK, "象の鼻", "Trunk" },
    { PlayerMutationType::ATT_ANIMAL, "動物を引き寄せる", "Attract Animals" },
    { PlayerMutationType::TENTACLES, "邪悪な触手", "Evil Tentacles" },
    { PlayerMutationType::RAW_CHAOS, "純カオス", "Raw Chaos" },
    { PlayerMutationType::NORMALITY, "ランダムな変異の消滅", "Normality" },
    { PlayerMutationType::WRAITH, "ランダムな幽体化", "Wraith Form" },
    { PlayerMutationType::POLY_WOUND, "ランダムな傷の変化", "Polymorph Wounds" },
    { PlayerMutationType::WASTING, "衰弱", "Wasting Disease" },
    { PlayerMutationType::ATT_DRAGON, "ドラゴンを引き寄せる", "Attract Dragons" },
    { PlayerMutationType::WEIRD_MIND, "ランダムなテレパシー", "Weird Mind" },
    { PlayerMutationType::NAUSEA, "落ち着きの無い胃", "Nausea" },
    { PlayerMutationType::CHAOS_GIFT, "パトロン", "Chaos Patron" },
    { PlayerMutationType::WALK_SHAD, "ランダムな現実変容", "Walk Shadows" },
    { PlayerMutationType::WARNING, "警告", "Warning" },
    { PlayerMutationType::INVULN, "ランダムな無敵化", "Invulnerability Fits" },
    { PlayerMutationType::SP_TO_HP, "ランダムなMPからHPへの変換", "Convert MP to HP" },
    { PlayerMutationType::HP_TO_SP, "ランダムなHPからMPへの変換", "Convert HP to MP" },
    { PlayerMutationType::DISARM, "ランダムな武器落とし", "Disarm Self" },
    { PlayerMutationType::HYPER_STR, "超人的な力", "Hyper Strength" },
    { PlayerMutationType::PUNY, "虚弱", "Puny" },
    { PlayerMutationType::HYPER_INT, "生体コンピュータ", "Living Computer" },
    { PlayerMutationType::MORONIC, "精神薄弱", "Moronic" },
    { PlayerMutationType::RESILIENT, "弾力のある体", "Resilient Body" },
    { PlayerMutationType::XTRA_FAT, "異常な肥満", "Extremely Fat" },
    { PlayerMutationType::ALBINO, "アルビノ", "Albino" },
    { PlayerMutationType::FLESH_ROT, "腐敗した肉体", "Rotting Flesh" },
    { PlayerMutationType::SILLY_VOI, "間抜けなキーキー声", "Silly Voice" },
    { PlayerMutationType::BLANK_FAC, "のっぺらぼう", "Blank Face" },
    { PlayerMutationType::ILL_NORM, "幻影に覆われた体", "Illusion Normality" },
    { PlayerMutationType::XTRA_EYES, "第三の目", "Extra Eyes" },
    { PlayerMutationType::MAGIC_RES, "魔法防御", "Magic Resistance" },
    { PlayerMutationType::XTRA_NOIS, "騒音", "Extra Noise" },
    { PlayerMutationType::INFRAVIS, "赤外線視力", "Infravision" },
    { PlayerMutationType::XTRA_LEGS, "追加の脚", "Extra Legs" },
    { PlayerMutationType::SHORT_LEG, "短い脚", "Short Legs" },
    { PlayerMutationType::ELEC_TOUC, "電撃オーラ", "Electric Aura" },
    { PlayerMutationType::FIRE_BODY, "火炎オーラ", "Fiery Aura" },
    { PlayerMutationType::WART_SKIN, "イボ肌", "Warty Skin" },
    { PlayerMutationType::SCALES, "鱗肌", "Scaled Skin" },
    { PlayerMutationType::IRON_SKIN, "鉄の肌", "Steel Skin" },
    { PlayerMutationType::WINGS, "翼", "Wings" },
    { PlayerMutationType::FEARLESS, "恐れ知らず", "Fearless" },
    { PlayerMutationType::REGEN, "急回復", "Regeneration" },
    { PlayerMutationType::ESP, "テレパシー", "Telepathy" },
    { PlayerMutationType::LIMBER, "しなやかな肉体", "Limber Body" },
    { PlayerMutationType::ARTHRITIS, "関節の痛み", "Arthritis" },
    { PlayerMutationType::BAD_LUCK, "黒いオーラ(不運)", "Black Aura (Bad Luck)" },
    { PlayerMutationType::VULN_ELEM, "元素攻撃弱点", "Elemental Vulnerability" },
    { PlayerMutationType::MOTION, "正確で力強い動作", "Good Motion" },
    { PlayerMutationType::GOOD_LUCK, "白いオーラ(幸運)", "White Aura (Good Luck)" },
    { PlayerMutationType::DEFECATION, "脱糞癖", "Uncontrolled Defecation" },
    { PlayerMutationType::ZEERO_VIRUS, "ゼEROウイルス感染", "ZEERO Virus" },
    { PlayerMutationType::HOMO_SEXUAL, "同性愛", "Homosexuality" },
    { PlayerMutationType::BI_SEXUAL, "両性愛", "Bisexuality" },
    { PlayerMutationType::WEAK_LOWER_BODY, "貧弱な下半身", "Weak Lower Body" },
    { PlayerMutationType::IKISUGI, "イキすぎ", "Excessive Climax" },
    { PlayerMutationType::ATT_NASTY, "クッソ汚い輩を引きつける", "Attract Nasty Creatures" },
    { PlayerMutationType::ATT_PERVERT, "変質者を引きつける", "Attract Perverts" },
    { PlayerMutationType::DESTROYED_ASSHOLE, "肛門を完全破壊された", "Destroyed Asshole" },
    { PlayerMutationType::LOST_HEAD, "頭部を失った", "Lost Head" },
};

/*!
 * @brief 突然変異の短い表示名を返す
 */
const std::string &MutationDataManager::get_mutation_name(PlayerMutationType type)
{
    static_assert(std::size(mutation_name_table) == static_cast<size_t>(PlayerMutationType::MAX),
        "mutation_name_table must cover every PlayerMutationType");
    const auto &entry = mutation_name_table[static_cast<int>(type)];
    return _(entry.name_jp, entry.name_en);
}

const MutationData &MutationDataManager::get_mutation_data(PlayerMutationType type)
{
    return mutation_table[static_cast<int>(type)];
}

PlayerMutationType MutationDataManager::get_random_mutation(int class_modifier)
{
    // 全重みの合計を計算
    int total_weight = 0;
    for (int i = 0; i < static_cast<int>(PlayerMutationType::MAX); i++) {
        int weight = mutation_table[i].probability_weight;

        // バーサーカークラス向け調整（有害な変異を受けやすい）
        if (class_modifier > 0 && i >= 32) {
            weight += class_modifier;
        }

        total_weight += weight;
    }

    // ランダム値を生成
    int random_value = randint0(total_weight);

    // 累積重みで選択
    int current_weight = 0;
    for (int i = 0; i < static_cast<int>(PlayerMutationType::MAX); i++) {
        int weight = mutation_table[i].probability_weight;

        // バーサーカークラス向け調整
        if (class_modifier > 0 && i >= 32) {
            weight += class_modifier;
        }

        current_weight += weight;

        if (random_value < current_weight) {
            return static_cast<PlayerMutationType>(i);
        }
    }

    // フォールバック（通常は到達しない）
    return PlayerMutationType::SPIT_ACID;
}