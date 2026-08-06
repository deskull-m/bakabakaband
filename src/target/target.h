#pragma once

#include "util/point-2d.h"
#include <memory>
#include <tl/optional.hpp>

class CreatureEntity;
class Target {
public:
    Target(const Target &);
    Target &operator=(const Target &);
    ~Target();

    static Target none();
    static Target create_grid_target(CreatureEntity &creature, const Pos2D &pos);
    static Target create_monster_target(CreatureEntity &creature, short m_idx);

    static void set_last_target(const Target &target);
    static Target get_last_target();
    static void clear_last_target();

    bool is_okay() const;
    tl::optional<Pos2D> get_position() const;
    tl::optional<short> get_m_idx() const;

private:
    Target();

    class Impl;
    std::unique_ptr<Impl> impl;
};
