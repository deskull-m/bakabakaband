#include "target/target.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "target/target-preparation.h"
#include <variant>

namespace {
/// 最後にターゲットしたもの
Target last_target = Target::none();

/// 特定のマスをターゲットする
struct TargetGrid {
    Pos2D pos = { 0, 0 }; //<! マスの座標
};

/// モンスターをターゲットする
struct TargetMonster {
    short m_idx = 0; //<! モンスターの参照インデックス
};

class IsOkay {
public:
    IsOkay(CreatureEntity *creature_ptr)
        : creature_ptr(creature_ptr)
    {
    }
    bool operator()(std::monostate) const
    {
        return false;
    }
    bool operator()(const TargetGrid &) const
    {
        return true;
    }
    bool operator()(const TargetMonster &target_monster) const
    {
        return target_able(*this->creature_ptr, target_monster.m_idx);
    }

private:
    CreatureEntity *creature_ptr;
};

class PositionGettor {
public:
    PositionGettor(CreatureEntity *creature_ptr)
        : creature_ptr(creature_ptr)
    {
    }
    tl::optional<Pos2D> operator()(std::monostate) const
    {
        return tl::nullopt;
    }
    tl::optional<Pos2D> operator()(const TargetGrid &target_grid) const
    {
        return target_grid.pos;
    }
    tl::optional<Pos2D> operator()(const TargetMonster &target_monster) const
    {
        const auto &monster = this->creature_ptr->current_floor_ptr->get_monster(target_monster.m_idx);
        return monster.get_position();
    }

private:
    CreatureEntity *creature_ptr;
};

class MonsterIndexGetter {
public:
    MonsterIndexGetter() = default;

    tl::optional<short> operator()(std::monostate) const
    {
        return tl::nullopt;
    }
    tl::optional<short> operator()(const TargetGrid &) const
    {
        return tl::nullopt;
    }
    tl::optional<short> operator()(const TargetMonster &target_monster) const
    {
        return target_monster.m_idx;
    }
};
}

class Target::Impl {
public:
    Impl() = default;
    CreatureEntity *creature_ptr;
    std::variant<std::monostate, TargetGrid, TargetMonster> target;
};

Target::Target()
    : impl(std::make_unique<Impl>())
{
}

Target::Target(const Target &other)
    : impl(std::make_unique<Impl>(*other.impl))
{
}

Target &Target::operator=(const Target &other)
{
    if (this == &other) {
        return *this;
    }
    this->impl = std::make_unique<Impl>(*other.impl);
    return *this;
}

Target::~Target() = default;

/*!
 * @brief なにもターゲットしていないインスタンスを生成する
 */
Target Target::none()
{
    return Target();
}

/*!
 * @brief 特定のマスをターゲットするインスタンスを生成する
 *
 * @param pos ターゲットするマスの座標
 * @return 生成したインスタンス
 */
Target Target::create_grid_target(CreatureEntity &creature, const Pos2D &pos)
{
    Target target;
    target.impl->creature_ptr = &creature;
    target.impl->target = TargetGrid{ pos };
    return target;
}

/*!
 * @brief モンスターをターゲットするインスタンスを生成する
 *
 * @param m_idx ターゲットするモンスターの参照インデックス
 * @return 生成したインスタンス
 */
Target Target::create_monster_target(CreatureEntity &creature, short m_idx)
{
    Target target;
    target.impl->creature_ptr = &creature;
    target.impl->target = TargetMonster{ m_idx };
    return target;
}

/*!
 * @brief 最後にターゲットしたものを設定する
 * @param target 設定するターゲット
 */
void Target::set_last_target(const Target &target)
{
    last_target = target;
}

/*!
 * @brief 最後にターゲットしたものを取得する
 * @return 最後にターゲットしたもの
 */
Target Target::get_last_target()
{
    return last_target;
}

/*!
 * @brief 最後にターゲットしたものをクリアする
 */
void Target::clear_last_target()
{
    last_target = Target::none();
}

/*!
 * @brief ターゲットが有効かどうかを取得する
 * @return
 * なにもターゲットしていない場合はfalse
 * 特定のマスをターゲットしている場合はtrue
 * モンスターをターゲットしている場合target_able()で判定した結果
 */
bool Target::is_okay() const
{
    return std::visit(IsOkay(this->impl->creature_ptr), this->impl->target);
}

/*!
 * @brief ターゲットの座標を取得する
 * @return
 * なにもターゲットしていない場合はtl::nullopt
 * 特定のマスをターゲットしている場合はその座標
 * モンスターをターゲットしている場合target_able()==trueであればモンスターの座標、そうでなければtl::nullopt
 */
tl::optional<Pos2D> Target::get_position() const
{
    if (!this->is_okay()) {
        return tl::nullopt;
    }
    return std::visit(PositionGettor(this->impl->creature_ptr), this->impl->target);
}

/*!
 * @brief ターゲットのモンスター参照IDを取得する
 * @return
 * モンスターをターゲットしていない場合はtl::nullopt
 * モンスターをターゲットしている場合はそのモンスター参照ID
 */
tl::optional<short> Target::get_m_idx() const
{
    return std::visit(MonsterIndexGetter(), this->impl->target);
}
