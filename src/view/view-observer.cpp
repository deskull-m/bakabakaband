#include "view/view-observer.h"

namespace {
ViewObserver *view_observer = nullptr;
}

void set_view_observer(ViewObserver *observer)
{
    view_observer = observer;
}

void notify_view_grid_changed(const Pos2D &pos)
{
    if (view_observer != nullptr) {
        view_observer->on_grid_changed(pos);
    }
}

void notify_view_all_grids_changed()
{
    if (view_observer != nullptr) {
        view_observer->on_all_grids_changed();
    }
}
