#pragma once

#include "system/monrace/monrace-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include <map>
#include <memory>
#include <vector>

namespace test {
/*!
 * @brief テストの間だけ MonraceList の中身を空にし、スコープを抜けるときに元へ戻す
 * @details 種族の一覧を差し替えるため monraces を退避し、添字キャッシュ (monrace_flat_cache)
 *          は退避後の shared_ptr を指す宙ぶらりんのポインタになるので空にする。
 */
class MonraceListTestAccess {
public:
    MonraceListTestAccess()
        : monraces(MonraceList::get_instance())
        , reader_state()
    {
        this->monraces.monraces.swap(this->saved_monraces);
        this->monraces.monrace_flat_cache.clear();
    }

    MonraceListTestAccess(const MonraceListTestAccess &) = delete;
    MonraceListTestAccess &operator=(const MonraceListTestAccess &) = delete;

    ~MonraceListTestAccess()
    {
        this->monraces.monraces.swap(this->saved_monraces);
        this->monraces.monrace_flat_cache.clear();
    }

private:
    MonraceList &monraces;
    std::map<MonraceId, std::shared_ptr<MonraceDefinition>> saved_monraces;
    ScopedReaderState reader_state;
};
}
