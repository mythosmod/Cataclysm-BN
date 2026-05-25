#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "type_id.h"
#include "translations.h"

class JsonObject;

struct construction_category {
        void load( const JsonObject &jo, const std::string &src );

        construction_category_id id;
        bool was_loaded = false;

        std::string name() const {
            return _name.translated();
        }
        static size_t count();

        // Singleton compare-by-id, used by Lua bindings.
        bool operator==( const construction_category &rhs ) const {
            return id == rhs.id;
        }
        bool operator<( const construction_category &rhs ) const {
            return id < rhs.id;
        }

    private:
        translation _name;
};

namespace construction_categories
{

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<construction_category> &get_all();

} // namespace construction_categories


