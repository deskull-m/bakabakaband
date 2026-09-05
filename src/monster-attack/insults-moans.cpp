#include "monster-attack/insults-moans.h"

/*! モンスターの侮辱行為メッセージテーブル / Hack -- possible "insult" messages */
concptr desc_insult[MAX_INSULTS] = {
    _("があなたを侮辱した！", "insults you!"),
    _("があなたの母を侮辱した！", "insults your mother!"),
    _("があなたを軽蔑した！", "gives you the finger!"),
    _("があなたを辱めた！", "humiliates you!"),
    _("があなたを汚した！", "defiles you!"),
    _("があなたの回りで踊った！", "dances around you!"),
    _("が猥褻な身ぶりをした！", "makes obscene gestures!"),
    _("があなたをぼんやりと見た！！！", "moons you!!!"),
    _("があなたをパラサイト呼ばわりした！", "calls you a parasite!"),
    _("があなたをサイボーグ扱いした！", "calls you a cyborg!"),
};

/*! マゴットのぼやきメッセージテーブル / Hack -- possible "insult" messages */
concptr desc_moan[MAX_MOANS] = {
    _("は何かを悲しんでいるようだ。", "seems sad about something."),
    _("が彼の飼い犬を見なかったかと尋ねている。", "asks if you have seen his dogs."),
    _("が縄張りから出て行けと言っている。", "tells you to get off his land."),
    _("はキノコがどうとか呟いている。", "mumbles something about mushrooms."),
};

/*! 百合の間に挟まる男メッセージテーブル  */
concptr desc_moan2[MAX_MOANS] = {
    "「あー、美少女二人いっぺんに手に入れて両手に花してえ」",
    "「百合カップルいいよなあ、間に挟まりてえ」",
    "「百合好きなんて一部の声のでかいやつだけだろ？」",
    "「子を成してこそだろ」",
};

/*! ミスミメッセージテーブル  */
concptr desc_moan3[MAX_MOANS] = {
    "「今日より明日なんじゃ！」",
    "「あ…明日が……明日が……」",
    "「種モミじゃ～種モミじゃ～」",
    "「は…半年、半年の間探し回ってようやくこれだけ…」",
};

/*! 撮り鉄メッセージテーブル / Trainspotter moan messages */
concptr desc_moan_toritetsu[MAX_MOANS] = {
    _("は「どけええええ！写り込んでんだよ！」と怒鳴っている。", "screams 'GET OUT OF THE WAY! You're in my shot!'"),
    _("は「ここは俺の撮影地だ、勝手に入るな！」と喚いている。", "yells 'This is MY photo spot! Stay out of it!'"),
    _("は「邪魔な柵は引っこ抜いておいた」と得意げに語っている。", "brags 'I pulled out that fence; it was ruining the composition.'"),
    _("は三脚を振り回しながら列車の時刻を叫んでいる。", "waves a tripod around while shouting train timetables."),
};
