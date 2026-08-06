#pragma once

#include "object-enchant/tr-flags.h"
#include "system/creature-entity.h"
#include <initializer_list>
#include <memory>
#include <span>
#include <variant>

enum class SamuraiStanceType : uint8_t;
enum class MonkStanceType : uint8_t;
enum class PlayerClassType : short;
class CreatureClass {
public:
    CreatureClass(CreatureEntity &creature);
    /*!
     * @brief 読み取り専用（const）クリーチャーから構築するためのビュー用コンストラクタ
     * @details 構え問い合わせ等の const メソッド内から CreatureClass を組み立てるための窓口。
     * ゲーム状態としての CreatureEntity は実体が const になることが無い（const は
     * アクセサ修飾子の伝播に過ぎない）ため、内部で 1 箇所に const_cast を局所化する。
     * これにより呼び出し側（自然回復処理等）から const_cast を排除する。
     */
    CreatureClass(const CreatureEntity &creature);
    CreatureClass(const CreatureClass &) = default;
    CreatureClass(CreatureClass &&) = default;
    CreatureClass &operator=(const CreatureClass &) = delete;
    CreatureClass &operator=(CreatureClass &&) = delete;
    virtual ~CreatureClass() = default;

    bool equals(PlayerClassType type) const;
    TrFlags tr_flags() const;
    TrFlags stance_tr_flags() const;

    bool has_stun_immunity() const;
    bool has_poison_resistance() const;
    bool has_additional_speed() const;
    bool is_soldier() const;
    bool is_wizard() const;
    bool is_tamer() const;
    bool can_browse() const;
    bool has_listed_magics() const;
    bool is_tough() const;
    bool is_martial_arts_pro() const;
    bool is_every_magic() const;
    bool has_number_of_spells_learned() const;

    bool lose_balance();
    void break_samurai_stance(std::initializer_list<SamuraiStanceType> stance_list);
    SamuraiStanceType get_samurai_stance() const;
    bool samurai_stance_is(SamuraiStanceType stance) const;
    void set_samurai_stance(SamuraiStanceType stance) const;

    MonkStanceType get_monk_stance() const;
    bool monk_stance_is(MonkStanceType stance) const;
    void set_monk_stance(MonkStanceType stance) const;

    void init_specific_data();
    void init_monster_specific_data();
    template <typename T>
    std::shared_ptr<T> get_specific_data() const;

    bool has_ninja_speed() const;
    std::span<const std::string> get_subtitle_candidates() const;

private:
    CreatureEntity &creature;
};

/**
 * @brief 職業固有データへのアクセスを取得する
 * @details 事前条件: init_specifid_data を呼び出し職業固有データ領域の初期化を行っておくこと
 *
 * @tparam T 職業固有データの型
 * @return 職業固有データTへのアクセスができる std::shared_ptr<T> を返す。
 * プレイヤーが職業固有データTを使用できない職業の場合はなにも所有権を持たない std::shared_ptr<T> を返す。
 */
template <typename T>
std::shared_ptr<T> CreatureClass::get_specific_data() const
{
    if (!this->creature.is_player()) {
        return nullptr;
    }
    if (!std::holds_alternative<std::shared_ptr<T>>(this->creature.class_specific_data)) {
        return nullptr;
    }

    return std::get<std::shared_ptr<T>>(this->creature.class_specific_data);
}
