#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "catalua_bindings.h"

#include "catalua.h"
#include "catalua_bindings_utils.h"
#include "catalua_impl.h"
#include "catalua_luna.h"
#include "catalua_luna_doc.h"

#include "construction.h"
#include "construction_category.h"
#include "construction_group.h"
#include "requirements.h"

void cata::detail::reg_construction( sol::state &lua )
{
#define UT_CLASS construction
    {
        sol::usertype<UT_CLASS> ut =
        luna::new_usertype<UT_CLASS>(
            lua,
            luna::no_bases,
            luna::no_constructor
        );

        SET_MEMB_RO( id );
        SET_MEMB_RO( category );
        SET_MEMB_RO( group );
        SET_MEMB_RO( pre_flags );
        SET_MEMB_RO( deny_flags );
        SET_MEMB_RO( post_flags );
        SET_MEMB_RO( time );
        SET_MEMB_RO( needs_diggable );
        SET_MEMB_RO( vehicle_start );
        SET_MEMB_RO( on_display );
        SET_MEMB_RO( dark_craftable );

        SET_FX( adjusted_time );
        SET_FX( time_scale );
        SET_FX( is_blacklisted );

        // The remaining members are exposed via lambda getters that return
        // strings or value objects. SET_MEMB_RO on a string_id<T> field
        // requires T to have operator== and operator< for sol2's automatic
        // comparison wrap; rather than add those to a long chain of
        // game-state types just for the Lua surface, we expose the id as a
        // string here and let modders re-wrap via Ter.new(...), Furn.new(...),
        // SkillId.new(...) etc. if they need a typed handle.

        DOC( "Terrain id that must be present before construction starts." );
        luna::set_fx( ut, "pre_terrain_id", []( const construction & c ) -> std::string {
            return c.pre_terrain.str();
        } );
        DOC( "Furniture id that must be present before construction starts." );
        luna::set_fx( ut, "pre_furniture_id", []( const construction & c ) -> std::string {
            return c.pre_furniture.str();
        } );
        DOC( "Terrain id the tile is converted to on completion." );
        luna::set_fx( ut, "post_terrain_id", []( const construction & c ) -> std::string {
            return c.post_terrain.str();
        } );
        DOC( "Furniture id the tile is converted to on completion." );
        luna::set_fx( ut, "post_furniture_id", []( const construction & c ) -> std::string {
            return c.post_furniture.str();
        } );

        DOC( "Returns a map of required skill-id strings to required levels." );
        luna::set_fx( ut, "get_required_skills_str_map",
        []( const construction & c ) -> std::map<std::string, int> {
            std::map<std::string, int> out;
            for( const auto &kv : c.required_skills )
            {
                out.emplace( kv.first.str(), kv.second );
            }
            return out;
        } );

        DOC( "Returns the fully resolved tool / quality / component "
             "requirements for this construction, or nil if it has none." );
        luna::set_fx( ut, "get_requirements",
        []( const construction & c ) -> std::optional<requirement_data> {
            if( c.requirements.is_valid() )
            {
                return *c.requirements;
            }
            return std::nullopt;
        } );

        DOC( "Returns the id of the item_group that this construction's "
             "byproducts come from, or empty string if it has none." );
        luna::set_fx( ut, "byproduct_item_group_id",
        []( const construction & c ) -> std::string {
            return c.byproduct_item_group.str();
        } );

        DOC( "Returns every loaded construction recipe." );
        luna::set_fx( ut, "get_all", []() -> std::vector<construction> {
            std::vector<construction> out;
            const std::vector<construction_id> &ids = constructions::get_all_sorted();
            out.reserve( ids.size() );
            for( const construction_id &cid : ids )
            {
                out.push_back( cid.obj() );
            }
            return out;
        } );
    }
#undef UT_CLASS // #define UT_CLASS construction

#define UT_CLASS construction_category
    {
        sol::usertype<UT_CLASS> ut =
        luna::new_usertype<UT_CLASS>(
            lua,
            luna::no_bases,
            luna::no_constructor
        );

        SET_MEMB_RO( id );
        SET_FX( name );

        DOC( "Returns every loaded construction category." );
        luna::set_fx( ut, "get_all", []() -> std::vector<construction_category> {
            return construction_categories::get_all();
        } );
    }
#undef UT_CLASS // #define UT_CLASS construction_category

#define UT_CLASS construction_group
    {
        sol::usertype<UT_CLASS> ut =
        luna::new_usertype<UT_CLASS>(
            lua,
            luna::no_bases,
            luna::no_constructor
        );

        SET_MEMB_RO( id );
        SET_FX( name );

        DOC( "Returns every loaded construction group." );
        luna::set_fx( ut, "get_all", []() -> std::vector<construction_group> {
            return construction_groups::get_all();
        } );
    }
#undef UT_CLASS // #define UT_CLASS construction_group
}
