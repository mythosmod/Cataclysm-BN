#pragma once

#include <string>
#include <vector>

#include "translations.h"
#include "type_id.h"

class JsonObject;

struct construction_group {
        void load( const JsonObject &jo, const std::string &src );

        construction_group_str_id id;
        bool was_loaded = false;

        std::string name() const;

        static size_t count();

        // Singleton compare-by-id, used by Lua bindings.
        bool operator==( const construction_group &rhs ) const {
            return id == rhs.id;
        }
        bool operator<( const construction_group &rhs ) const {
            return id < rhs.id;
        }

    private:
        translation _name;
};

namespace construction_groups
{

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<construction_group> &get_all();

} // namespace construction_groups


