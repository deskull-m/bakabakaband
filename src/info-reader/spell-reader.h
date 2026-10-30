#pragma once

#include "info-reader/parse-error-types.h"

#include <map>
#include <nlohmann/json_fwd.hpp>
#include <set>
#include <string>

enum class RealmType;
class SpellInfoList;

class SpellReader {
public:
    //! 1 回の呪文定義の読み込み (init_json の呼び出し) ごとに所有し、登録済みの ID・タグを領域ごとに保持する
    class Session {
        friend class SpellReader;

        struct RealmSpells {
            std::set<short> ids;
            std::set<std::string> tags;
        };
        std::map<RealmType, RealmSpells> realms;
        int record_index = -1; //!< 呪文領域は数値IDを持たないため、エラー表示用に要素の順序を用いる (読み込み全体で共有)
    };

    SpellReader(const nlohmann::json &realm_data, SpellInfoList &spell_info_list, Session &session);
    SpellReader(nlohmann::json &&, SpellInfoList &, Session &) = delete;
    SpellReader(const SpellReader &) = delete;
    SpellReader(SpellReader &&) = delete;
    SpellReader &operator=(const SpellReader &) = delete;
    SpellReader &operator=(SpellReader &&) = delete;

    ParseErrorType read() const;

private:
    ParseErrorType set_realm(RealmType &realm) const;
    ParseErrorType set_spell_data(const nlohmann::json &spell_data, RealmType realm) const;
    ParseErrorType set_book_data(RealmType realm) const;

    const nlohmann::json &realm_data;
    SpellInfoList &spell_info_list;
    Session &session;
};
