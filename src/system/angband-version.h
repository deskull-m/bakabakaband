#pragma once

#include <cstdint>
#include <string>

/*!
 * @brief 現在のバリアント名
 */
constexpr std::string_view VARIANT_NAME("Bakabaka");

/*!
 * @brief セーブファイル上のバージョン定義
 * @details v1.1.1以上にのみ適用.
 * angband.rc に影響があるため、constexpr ではなくdefine 定数のままにしておくこと.
 */
#define H_VER_MAJOR 0 //!< ゲームのバージョン定義(メジャー番号)
#define H_VER_MINOR 0 //!< ゲームのバージョン定義(マイナー番号)
#define H_VER_PATCH 2 //!< ゲームのバージョン定義(パッチ番号)
#define H_VER_EXTRA 0 //!< ゲームのバージョン定義(エクストラ番号)

/*!
 * @brief セーブファイルのバージョン(3.0.0から導入)
 * @details
 * 47: XOR 鎖チェックサム修復 (hengband#1372 相当)。本バージョン以降は
 * verify_checksum() / verify_encoded_checksum() の失敗をエラーとして扱う。
 * 48: モンスタープレイヤーの種族 ID (r_idx / ap_r_idx) を保存。
 * 49: 最大プレイヤーレベルを 50 → 60 に拡張。クイックスタート情報の
 *     player_hp 配列が PY_MAX_LEVEL (件数非保持) で保存されるため、
 *     旧バージョン (<49) は 50 件として読む差分処理が必要。
 * 54: prace / pclass を CreatureEntity 共通ブロック (wr_creature_common) へ集約。
 *     従来プレイヤー writer (byte) / モンスター writer (s16b) で個別保存していた
 *     ものを統一 (s16b)。旧バージョンは各固有経路で読む (rd_creature_common /
 *     rd_base_info / rd_monster_v50 に older_than(54) ガード)。
 * 55: 世界情報の訪問済フラグを s16b から bool へ変更 (world-loader に
 *     older_than(55) ガード)。
 * 56: モンスターが視認されているか (visible_on_map / 旧 ml) を保存 (hengband#6060 相当)。
 *     旧バージョン (<56) のモンスターは視認されていない状態で読み込む
 *     (rd_monster_v50 に older_than(56) ガード)。
 * 57: アイテムに「正当な所有権」(ItemEntity::rightful_owner, モンスター種族 ID) を
 *     追加。wr_item() の末尾へ s16b で無条件追記し、rd_item() は
 *     older_than(57) なら MonraceId::PLAYER (= 記録なし) で埋める。
 *     **注意: アイテムのビットマスク (SaveDataItemFlagType) は 0x80000000 のみ
 *     未使用で実質満杯のため、本フィールドはフラグビットを使わず末尾無条件
 *     書込 + バージョンガードの形を採った。以降のアイテムフィールド追加も
 *     同じ形にすること。**
 * 58: 最後にターゲットしたもの (Target の last_target) とペット・乗馬の攻撃対象
 *     (pet_t_m_idx / riding_t_m_idx) を保存 (hengband#6069 相当)。wr_player() 末尾へ
 *     追記し、rd_extra() は older_than(58) なら未ターゲット状態で埋める。
 */
constexpr uint32_t SAVEFILE_VERSION = 58;

/*!
 * @brief バージョンが開発版が安定版かを返す(廃止予定)
 */
constexpr bool IS_STABLE_VERSION = (H_VER_MINOR % 2 == 0 && H_VER_EXTRA == 0);

enum class VersionStatusType {
    ALPHA,
    BETA,
    RELEASE_CANDIDATE,
    RELEASE,
};

/*!
 * @brief バージョンの立ち位置
 */
constexpr VersionStatusType VERSION_STATUS = VersionStatusType::ALPHA;

enum class VersionExpression {
    WITHOUT_EXTRA,
    WITH_EXTRA,
    FULL,
};

/*!
 * @brief バージョン番号
 * @details
 * OpenBSD等のsys/types.hはmajor()/minor()を関数形式マクロとして定義しており、
 * ソース中に "major(" や "minor(" というトークン列があるとそこで展開されてしまう。
 * コンストラクタを持たない集成体とすることでその形の記述を避けているので、
 * メンバ初期化子リスト等でmajor/minorを関数呼び出しの形で書かないこと。
 */
class AngbandVersion {
public:
    uint8_t major = 0; //!< 変愚蛮怒バージョン(メジャー番号)
    uint8_t minor = 0; //!< 変愚蛮怒バージョン(マイナー番号)
    uint8_t patch = 0; //!< 変愚蛮怒バージョン(パッチ番号)
    uint8_t extra = 0; //!< 変愚蛮怒バージョン(エクストラ番号)

    std::string build_expression(VersionExpression expression) const;
};
