/*!
 * @file cmd-text-command.cpp
 * @brief 文章コマンド処理の実装
 */

#include "cmd-io/cmd-text-command.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-others.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "floor/floor-object.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object/tval-types.h"
#include "player-status/player-energy.h"
#include "player/player-damage.h"
#include "spell-kind/spells-sight.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "sv-definition/sv-junk-types.h"
#include "system/baseitem/baseitem-key.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "timed-effect/player-cut.h"
#include "util/int-char-converter.h"
#include "util/probability-table.h"
#include "view/display-messages.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// コマンドマッピング用の構造体
struct TextCommand {
    std::vector<std::string> keywords;
    std::function<void(CreatureEntity &)> action;
    std::string description;
};

/*!
 * @brief 文字列を小文字に変換し、空白を除去する
 */
static std::string normalize_string(const std::string &str)
{
    std::string result;
    for (char c : str) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
}

/*!
 * @brief 入力文字列がキーワードにマッチするかチェック
 */
static bool matches_keywords(const std::string &input, const std::vector<std::string> &keywords)
{
    std::string normalized_input = normalize_string(input);

    for (const auto &keyword : keywords) {
        std::string normalized_keyword = normalize_string(keyword);
        if (normalized_input.find(normalized_keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/*!
 * @brief 文章コマンドのマッピングテーブルを取得
 */
static std::vector<TextCommand> get_text_commands()
{
    return {
        { { "search", "探す", "探索" },
            [](CreatureEntity &creature) {
                do_cmd_search(creature);
            },
            _("周囲を探索する", "Search") },
        { { "suicide", "死ぬ", "自殺" },
            [](CreatureEntity &creature) {
                do_cmd_suicide(creature);
            },
            _("自殺する", "suicide") },
        { { "defecate", "脱糞", "うんち", "うんこ" },
            [](CreatureEntity &creature) {
                // 脱糞アクション
                msg_print(_("あなたは脱糞した！", "You defecated!"));

                // ランダムな脱糞メッセージ
                std::vector<std::string> defecate_messages_jp = {
                    "ブリブリブリブリュリュリュリュリュリュ！！！！！！ブツチチブブブチチチチブリリイリブブブブゥゥゅ！！！",
                    "スッキリした。",
                    "お腹が軽くなった感じがする。",
                    "なんてこった、ここは公共の場所なのに...",
                    "緊急事態だったんだ！",
                    "野生の本能が目覚めた。"
                };

                std::vector<std::string> defecate_messages_en = {
                    "Blibb blab bloobooroo, toogie urnf!",
                    "You feel refreshed.",
                    "Your stomach feels lighter.",
                    "Oh no, this is a public place...",
                    "It was an emergency!",
                    "Your wild instincts have awakened."
                };

                int choice = randint0(defecate_messages_jp.size());

#ifdef JP
                msg_print(defecate_messages_jp[choice].c_str());
#else
                msg_print(defecate_messages_en[choice].c_str());
#endif

                ItemEntity dung_item(BaseitemKey(ItemKindType::JUNK, SV_JUNK_FECES)); // ベースアイテム ID 672 = 糞便
                dung_item.number = 1;

                // プレイヤーの足下に落とす
                Pos2D pos(creature.y, creature.x);
                drop_near(creature, dung_item, pos);

                msg_print(_("糞便が足下に落ちた。", "Dung has dropped at your feet."));

                // 体力を少し消費
                if (creature.hp > 1) {
                    creature.hp--;
                    auto &rfu = RedrawingFlagsUpdater::get_instance();
                    rfu.set_flag(MainWindowRedrawingFlag::HP);
                }

                // 時間消費
                PlayerEnergy(creature).set_player_turn_energy(100);

                // 周囲のモンスターが嫌悪感を示す可能性
                if (one_in_(4)) {
                    msg_print(_("あなたの行為に周囲が困惑している。", "Your actions confuse those around you."));
                    aggravate_monsters(creature, 0);
                } else if (one_in_(8)) {
                    msg_print(_("なんとも言えない臭いが漂っている...", "An indescribable smell wafts through the air..."));
                }

                // 稀に状態異常
                /* TODO: 混乱は面白いが、あまりにも理不尽なので一旦コメントアウト
                if (one_in_(20)) {
                    msg_print(_("恥ずかしさで顔が真っ赤になった。", "Your face turns red with embarrassment."));
                    // 混乱状態を付与
                    auto &timed_effects = creature.effects();
                    timed_effects.confusion().set(timed_effects.confusion().current() + randint1(5));
                }
                */
            },
            _("脱糞する", "Defecate") },
        { { "dance", "踊る", "ダンス", "おどる" },
            [](CreatureEntity &creature) {
                // 踊るアクション
                msg_print(_("あなたは楽しそうに踊り始めた！", "You start dancing joyfully!"));

                // ランダムな踊りメッセージ
                std::vector<std::string> dance_messages_jp = {
                    "あなたは優雅にワルツを踊った。",
                    "あなたは情熱的にタンゴを踊った。",
                    "あなたは軽やかにスキップした。",
                    "あなたは力強くステップを踏んだ。",
                    "あなたはくるくると回転した。",
                    "あなたは陽気にジグを踊った。"
                };

                std::vector<std::string> dance_messages_en = {
                    "You dance a graceful waltz.",
                    "You dance a passionate tango.",
                    "You skip lightly.",
                    "You take powerful steps.",
                    "You spin around and around.",
                    "You dance a merry jig."
                };

                int choice = randint0(dance_messages_jp.size());

#ifdef JP
                msg_print(dance_messages_jp[choice].c_str());
#else
                msg_print(dance_messages_en[choice].c_str());
#endif

                // 少しの体力消費
                if (creature.get_current_mp() > 1) {
                    creature.sub_current_mp(1);
                    auto &rfu = RedrawingFlagsUpdater::get_instance();
                    rfu.set_flag(MainWindowRedrawingFlag::MP);
                }

                // 時間消費
                PlayerEnergy(creature).set_player_turn_energy(50);

                // 周囲のモンスターが反応する可能性
                if (one_in_(3)) {
                    msg_print(_("あなたの踊りに周囲が注目している。", "Your dancing attracts attention."));
                    aggravate_monsters(creature, 0);
                }

                // 稀にポジティブ効果
                if (one_in_(10)) {
                    msg_print(_("素晴らしい踊りで気分が高揚した！", "Your wonderful dance lifts your spirits!"));
                    set_hero(creature, creature.get_timed_effect(CreatureTimedEffect::BLESSED) + randint1(10), false);
                }
            },
            _("踊る", "Dance") },
        { { "headbutt", "頭突き", "ずつき", "あたまづき" },
            [](CreatureEntity &creature) {
                // 実際の頭突き攻撃処理を実行
                do_cmd_headbutt(creature);
            },
            _("頭突き", "Headbutt") },
        { { "bodyslam", "体当たり", "たいあたり", "ぼでぃすらむ", "tackle", "タックル" },
            [](CreatureEntity &creature) {
                // 実際の体当たり攻撃処理を実行
                do_cmd_body_slam(creature);
            },
            _("体当たり", "Body Slam") },
        { { "浣腸", "かんちょう", "enema", "カンチョー" },
            [](CreatureEntity &creature) {
                // 実際の浣腸攻撃処理を実行
                do_cmd_enema(creature);
            },
            _("浣腸", "Enema") },
        { { "ひでぶ", "hidebu", "ヒデブ", "HIDEBU" },
            [](CreatureEntity &creature) {
                // ひでぶコマンド - プレイヤーに重症の傷を与える
                msg_print(_("ひでぶ！", "Hidebu!"));
                msg_print(_("あなたは謎の力によって重傷を負った！", "You are seriously wounded by a mysterious force!"));

                // 重症の傷（現在のHPの半分のダメージ）
                int damage = creature.hp / 2;
                if (damage < 1)
                    damage = 1;

                // 最低でも50ポイントのダメージ、最大でも現HP-1まで
                damage = std::max(damage, 50);
                damage = std::min(damage, creature.hp - 1);

                take_hit(creature, DAMAGE_NOESCAPE, damage, _("ひでぶ", "Hidebu"));

                // HPが1未満にならないようにする
                if (creature.hp < 1) {
                    creature.hp = 1;
                }

                // 重傷状態を設定
                auto cut_plus = PlayerCut::get_accumulation(100, 200);
                if (cut_plus > 0) {
                    (void)BadStatusSetter(creature).mod_cut(cut_plus);
                }

                // 画面更新
                auto &rfu = RedrawingFlagsUpdater::get_instance();
                rfu.set_flag(MainWindowRedrawingFlag::HP);
                rfu.set_flag(MainWindowRedrawingFlag::CUT);
                handle_stuff(creature);

                // 警告メッセージ
                msg_print(_("何ということをしたのだ...", "What have you done..."));

                // 時間消費
                PlayerEnergy(creature).set_player_turn_energy(100);
            },
            _("ひでぶ", "Hidebu") },
        { { "しゃぶれよ", "shabureyо", "しゃぶれ", "shabare" },
            [](CreatureEntity &creature) {
                // しゃぶれよコマンド - 敵対的ホモを召喚
                msg_print(_("何がしゃぶれだあ、お前がしゃぶれよ", "What do you mean 'suck it', you suck it yourself!"));

                // 敵対的ホモを召喚（HOMO_SEXUALフラグを持つモンスターを敵対的に召喚）
                int count = 0;
                for (int k = 0; k < 2 + randint1(3); k++) {
                    // プレイヤー周辺に敵対的にホモを召喚
                    if (summon_specific(creature, creature.y, creature.x, creature.get_level(),
                            SUMMON_HOMO, PM_NO_PET)) {
                        count++;
                    }
                }

                if (count > 0) {
                    if (count == 1) {
                        msg_print(_("敵対的なホモが現れた！", "A hostile homosexual appears!"));
                    } else {
                        msg_print(_("敵対的なホモたちが現れた！", "Hostile homosexuals appear!"));
                    }
                    msg_print(_("彼らは非常に怒っているようだ...", "They seem very angry..."));
                } else {
                    msg_print(_("何かが起こりそうだったが、何も起こらなかった。", "Something was about to happen, but nothing did."));
                }

                // 時間消費
                PlayerEnergy(creature).set_player_turn_energy(100);
            },
            _("しゃぶれよ", "Suck it") }
    };
}

/*!
 * @brief 文章コマンド入力処理
 * @param creature クリーチャーへの参照
 */
void do_cmd_text_command(CreatureEntity &creature)
{
    screen_save();

    msg_erase();

    // 入力ループ
    tl::optional<std::string> buf;
    while (true) {
        buf = input_string(_("コマンド: ", "COMMAND: "), 1024);
        if (!buf.has_value()) {
            return;
        }
        if (buf->empty()) {
            continue;
        }
        break;
    }

    screen_load();

    // コマンドを検索して実行
    auto commands = get_text_commands();
    bool found = false;

    for (const auto &cmd : commands) {
        if (matches_keywords(buf.value(), cmd.keywords)) {
            cmd.action(creature);
            found = true;
            break;
        }
    }

    if (!found) {
        msg_format(_("'%s' は認識できないコマンドです。", "'%s' is not a recognized command."), buf.value().c_str());
        msg_print(_("'ヘルプ' と入力すると利用可能なコマンドが表示されます。", "Type 'help' to see available commands."));
    }
}
