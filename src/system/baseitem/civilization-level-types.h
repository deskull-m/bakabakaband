/*!
 * @brief 装備品の文明水準列挙
 * @details その装備が「どの文明段階の産物か」を表す分類。シムアースの文明段階に倣い、
 *          石器時代から銀河系進出時代までの 8 段階を持つ。
 *          相互排他なので 8 個のフラグではなく単一の enum フィールドとして持たせる
 *          (将来モンスター側にも同じ水準を付与し、一致した品のみを装備させる判定に使う)。
 *
 *          **enum 値の順序がそのまま文明の進み具合を表す**ので、中間への挿入はせず
 *          末尾 (MAX の直前) へ追加すること。水準同士の比較を行う処理が将来入りうる。
 *          本列挙はベースアイテム定義 (JSON 由来) にのみ存在しセーブデータには載らないため、
 *          セーブデータバージョンの更新は不要。
 */
#pragma once

#include <cstdint>
#include <string_view>

enum class CivilizationLevel : uint8_t {
    NONE = 0, //!< 未指定 (既定)。魔法的な品・装備品以外・分類しない品
    STONE = 1, //!< 石器時代: 石・木・骨・毛皮を打ち削って作る品
    BRONZE = 2, //!< 青銅器時代: 銅・青銅の鋳造、織物、初期の冶金
    IRON = 3, //!< 鉄器時代: 鉄・鋼の鍛造。中世の武具一般
    INDUSTRIAL = 4, //!< 工業化時代: 機械加工・火薬・規格化された量産品
    ATOMIC = 5, //!< 原子力時代: 合成樹脂・特殊合金・核技術
    INFORMATION = 6, //!< 情報化時代: 電子制御・計算機・サイバネティクス
    SOLAR = 7, //!< 太陽系進出時代: 恒星間に至らない宇宙開発技術
    GALACTIC = 8, //!< 銀河系進出時代: 恒星間航行を前提とする超技術
    MAX,
};

/*!
 * @brief 文明水準の表示名を返す (「石器」「青銅器」等。「時代」は含まない)
 * @param level 文明水準
 * @return 表示名。NONE / MAX では空文字列
 */
std::string_view civilization_level_name(CivilizationLevel level);

/*!
 * @brief その文明水準の存在が、その文明水準の品を扱えるか
 * @param item_level 品の文明水準 (BaseitemDefinition::civilization)
 * @param user_level 扱う側の文明水準 (MonraceDefinition::civilization)
 * @return 扱えるなら true
 * @details **「以下なら可」判定**。本列挙は品側では「作れる最低の技術段階」を
 *          表すので、鉄器文明の兵士は棍棒 (STONE) も鉄剣 (IRON) も扱える。
 *          完全一致にすると鉄器水準の射手が STONE の矢を撃てない等の穴が開く。
 *
 *          **どちらかが NONE なら常に true**。品側の NONE は「一般的な
 *          ファンタジーの中でもレアな品」(ミスリル・竜素材・魔法の装具等) で
 *          どの文明の産物でもないため水準で縛らず、扱う側の NONE は
 *          「文明に属さない存在」(獣・悪魔・竜・アンデッド等) で従来どおり
 *          制限しない。これにより既定の挙動を変えるのは「水準を持つ存在 ×
 *          水準を持つ品」の組み合わせだけになる。
 */
bool is_civilization_usable(CivilizationLevel item_level, CivilizationLevel user_level);
