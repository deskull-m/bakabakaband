#pragma once

#include <memory>
#include <string>
#include <string_view>

class CreatureEntity;
class ItemEntity;
struct text_body_type;
int find_autopick_list(CreatureEntity &creature, const ItemEntity *o_ptr);
void search_for_object(CreatureEntity &creature, text_body_type *tb, const ItemEntity *o_ptr, bool forward);
void search_for_string(text_body_type *tb, std::string_view search_str, bool forward);

enum class AutopickSearchResult {
    BACK = -1,
    CANCEL = 0,
    FORWARD = 1,
};

class AutopickSearch {
public:
    AutopickSearch(std::shared_ptr<ItemEntity> item, std::string_view search_str)
        : item(item)
        , search_str(search_str)
    {
    }

    std::shared_ptr<ItemEntity> item;
    std::string search_str;
    AutopickSearchResult result = AutopickSearchResult::CANCEL;
};

AutopickSearch get_string_for_search(CreatureEntity &creature, const AutopickSearch &as_initial);
bool get_object_for_search(CreatureEntity &creature, AutopickSearch &as);
bool get_destroyed_object_for_search(CreatureEntity &creature, AutopickSearch &as);
