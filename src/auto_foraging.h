#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include "enums.h"

class JsonIn;
class JsonOut;
class item;
struct itype;

namespace auto_foraging
{

/**
 * The currently-active set of auto-foraging rules, in a form that allows quick
 * lookup. When this is filled (by @ref auto_foraging::rule_list::create_rule()),
 * every item existing in the game that matches a rule (either white- or blacklist)
 * is added as the key, with RULE_WHITELISTED or RULE_BLACKLISTED as the values.
 */
class cache : public std::unordered_map<std::string, rule_state>
{
    public:
        bool ready = false;

        std::unordered_map<std::string, const itype *> temp_items;
};

class rule
{
    public:
        std::string sRule;
        bool bActive = false;
        bool bExclude = false;

        rule() = default;

        rule( const std::string &r, const bool a, const bool e ) : sRule( r ), bActive( a ), bExclude( e ) {
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        void test_pattern() const;
};

class rule_list : public std::vector<rule>
{
    public:
        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        void refresh_map_items( cache &map_items ) const;

        void create_rule( cache &map_items, const std::string &to_match );
        void create_rule( cache &map_items, const item &it );
};

class user_interface
{
    public:
        class tab
        {
            public:
                std::string title;
                rule_list new_rules;
                std::reference_wrapper<rule_list> rules;

                tab( const std::string &t, rule_list &r ) : title( t ), new_rules( r ), rules( r ) { }
        };

        std::string title;
        std::vector<tab> tabs;
        bool is_autoforaging = false;

        void show();

        bool bStuffChanged = false;
};

class base_settings
{
    protected:
        mutable cache map_items;

        void invalidate();

    private:
        virtual void refresh_map_items( cache &map_items ) const = 0;

        void recreate() const;

    public:
        virtual ~base_settings() = default;
        rule_state check_item( const std::string &sItemName ) const;
};

class player_settings : public base_settings
{
    private:
        void load( bool bCharacter );
        bool save( bool bCharacter );

        rule_list global_rules;
        rule_list character_rules;

        void refresh_map_items( cache &map_items ) const override;

    public:
        ~player_settings() override = default;

        void clear_character_rules();

        /**
         * Late-bind: populate the cache for this specific item name by checking
         * all rules against it. Needed when only exclude rules are present —
         * refresh_map_items' exclude path only modifies already-mapped items, so
         * a blacklist-only rule never reaches the cache via the normal path.
         */
        void create_rule( const std::string &to_match );

        void show();
        bool save_character();
        bool save_global();
        void load_character();
        void load_global();

        bool empty() const;
};

} // namespace auto_foraging

auto_foraging::player_settings &get_auto_foraging();
