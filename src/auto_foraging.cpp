#include "auto_foraging.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

#include "avatar.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"

#define afg_log() DebugLog( DL::Info, DC::Main ) << "[auto_foraging] "
#include "filesystem.h"
#include "fstream_utils.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "material.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "string_utils.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "world.h"

using namespace auto_foraging;

static bool check_special_rule( const std::vector<material_id> &materials,
                                const std::string &rule );

auto_foraging::player_settings &get_auto_foraging()
{
    static auto_foraging::player_settings single_instance;
    return single_instance;
}

void user_interface::show()
{
    if( tabs.empty() ) {
        return;
    }

    const int iHeaderHeight = 4;
    int iContentHeight = 0;
    const int iTotalCols = 2;

    catacurses::window w_border;
    catacurses::window w_header;
    catacurses::window w;

    ui_adaptor ui;

    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;
        const point iOffset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                             TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );

        w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       iOffset );
        w_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2,
                                       iOffset + point_south_east );
        w = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
                                iOffset + point( 1, iHeaderHeight + 1 ) );

        ui.position_from_window( w_border );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    size_t iTab = 0;
    int iLine = 0;
    int iColumn = 1;
    int iStartPos = 0;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w_border, BORDER_COLOR, title );
        mvwputch( w_border, point( 0, 3 ), c_light_gray, LINE_XXXO );
        mvwputch( w_border, point( 79, 3 ), c_light_gray, LINE_XOXX );
        mvwputch( w_border, point( 5, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        mvwputch( w_border, point( 51, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        mvwputch( w_border, point( 61, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        wnoutrefresh( w_border );

        int tmpx = 0;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<A>dd" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<R>emove" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<C>opy" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<M>ove" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<E>nable" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<D>isable" ) ) + 2;
        if( !g->u.name.empty() ) {
            shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<T>est" ) );
        }
        tmpx = 0;
        tmpx += shortcut_print( w_header, point( tmpx, 1 ), c_white, c_light_green,
                                _( "<+-> Move up/down" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 1 ), c_white, c_light_green,
                                _( "<Enter>-Edit" ) ) + 2;
        shortcut_print( w_header, point( tmpx, 1 ), c_white, c_light_green, _( "<Tab>-Switch Page" ) );

        for( int i = 0; i < 78; i++ ) {
            if( i == 4 || i == 50 || i == 60 ) {
                mvwputch( w_header, point( i, 2 ), c_light_gray, LINE_OXXX );
                mvwputch( w_header, point( i, 3 ), c_light_gray, LINE_XOXO );
            } else {
                mvwputch( w_header, point( i, 2 ), c_light_gray, LINE_OXOX );
            }
        }
        mvwprintz( w_header, point( 1, 3 ), c_white, "#" );
        mvwprintz( w_header, point( 8, 3 ), c_white, _( "Rules" ) );
        mvwprintz( w_header, point( 52, 3 ), c_white, _( "I/E" ) );

        rule_list &cur_rules = tabs[iTab].new_rules;
        int locx = 17;
        for( size_t i = 0; i < tabs.size(); i++ ) {
            const auto color = iTab == i ? hilite( c_white ) : c_white;
            locx += shortcut_print( w_header, point( locx, 2 ), c_white, color, tabs[i].title ) + 1;
        }

        if( is_autoforaging ) {
            locx = 55;
            mvwprintz( w_header, point( locx, 0 ), c_white, _( "Auto foraging:" ) );
            const std::string cur = get_option<std::string>( "AUTO_FORAGING" );
            const nc_color val_color = ( cur == "off" ) ? c_light_red : c_light_green;
            locx += shortcut_print( w_header, point( locx, 1 ), val_color, c_white,
                                    get_options().get_option( "AUTO_FORAGING" ).getValueName() );
            locx += shortcut_print( w_header, point( locx, 1 ), c_white, c_light_green, "  " );
            locx += shortcut_print( w_header, point( locx, 1 ), c_white, c_light_green, _( "<S>witch" ) );
            shortcut_print( w_header, point( locx, 1 ), c_white, c_light_green, "  " );
        }

        wnoutrefresh( w_header );

        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < 79; j++ ) {
                if( j == 4 || j == 50 || j == 60 ) {
                    mvwputch( w, point( j, i ), c_light_gray, LINE_XOXO );
                } else {
                    mvwputch( w, point( j, i ), c_black, ' ' );
                }
            }
        }

        draw_scrollbar( w_border, iLine, iContentHeight, cur_rules.size(), point( 0, 5 ) );
        wnoutrefresh( w_border );

        calcStartPos( iStartPos, iLine, iContentHeight, cur_rules.size() );

        for( int i = iStartPos; i < static_cast<int>( cur_rules.size() ); i++ ) {
            if( i >= iStartPos &&
                i < iStartPos + ( iContentHeight > static_cast<int>( cur_rules.size() ) ?
                                  static_cast<int>( cur_rules.size() ) : iContentHeight ) ) {
                nc_color cLineColor = cur_rules[i].bActive ? c_white : c_light_gray;

                mvwprintz( w, point( 1, i - iStartPos ), cLineColor, "%d", i + 1 );
                mvwprintz( w, point( 5, i - iStartPos ), cLineColor, "" );

                if( iLine == i ) {
                    wprintz( w, c_yellow, ">> " );
                } else {
                    wprintz( w, c_yellow, "   " );
                }

                wprintz( w, iLine == i && iColumn == 1 ? hilite( cLineColor ) : cLineColor, "%s",
                         cur_rules[i].sRule.empty() ? _( "<empty rule>" ) : cur_rules[i].sRule );

                mvwprintz( w, point( 52, i - iStartPos ), iLine == i && iColumn == 2 ?
                           hilite( cLineColor ) : cLineColor, "%s",
                           cur_rules[i].bExclude ? _( "Exclude" ) :  _( "Include" ) );
            }
        }

        wnoutrefresh( w );
    } );

    bStuffChanged = false;
    input_context ctxt( "AUTO_FORAGING" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Page up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Page down" ) );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    if( tabs.size() > 1 ) {
        ctxt.register_action( "NEXT_TAB" );
        ctxt.register_action( "PREV_TAB" );
    }
    ctxt.register_action( "ADD_RULE" );
    ctxt.register_action( "REMOVE_RULE" );
    ctxt.register_action( "COPY_RULE" );
    ctxt.register_action( "ENABLE_RULE" );
    ctxt.register_action( "DISABLE_RULE" );
    ctxt.register_action( "MOVE_RULE_UP" );
    ctxt.register_action( "MOVE_RULE_DOWN" );
    ctxt.register_action( "TEST_RULE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    const bool allow_swapping = tabs.size() == 2;
    if( allow_swapping ) {
        ctxt.register_action( "SWAP_RULE_GLOBAL_CHAR" );
    }

    if( is_autoforaging ) {
        ctxt.register_action( "SWITCH_AUTO_FORAGING_OPTION" );
    }

    while( true ) {
        rule_list &cur_rules = tabs[iTab].new_rules;

        const bool currentPageNonEmpty = !cur_rules.empty();

        ui_manager::redraw();

        const std::string action = ctxt.handle_input();

        if( action == "NEXT_TAB" ) {
            iTab++;
            if( iTab >= tabs.size() ) {
                iTab = 0;
            }
            iLine = 0;
        } else if( action == "PREV_TAB" ) {
            if( iTab > 0 ) {
                iTab--;
            } else {
                iTab = tabs.size() - 1;
            }
            iLine = 0;
        } else if( action == "QUIT" ) {
            break;
        } else if( action == "DOWN" ) {
            iLine++;
            iColumn = 1;
            if( iLine >= static_cast<int>( cur_rules.size() ) ) {
                iLine = 0;
            }
        } else if( action == "UP" ) {
            iLine--;
            iColumn = 1;
            if( iLine < 0 ) {
                iLine = cur_rules.size() - 1;
            }
        } else if( action == "PAGE_DOWN" || action == "PAGE_UP" ) {
            // Advance the view by one full visible-list page. Because
            // calcStartPos re-centres the cursor on each redraw when
            // MENU_SCROLL is on, anchor the target cursor at the centre
            // of the next page so the view truly shifts by iContentHeight.
            if( !cur_rules.empty() ) {
                const int last = static_cast<int>( cur_rules.size() ) - 1;
                const int half = std::max( 0, ( iContentHeight - 1 ) / 2 );
                iColumn = 1;
                if( action == "PAGE_DOWN" ) {
                    iLine = iLine == last
                            ? 0
                            : std::min( last, iStartPos + iContentHeight + half );
                } else {
                    iLine = iLine == 0
                            ? last
                            : std::max( 0, iStartPos - iContentHeight + half );
                }
            }
        } else if( action == "REMOVE_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            cur_rules.erase( cur_rules.begin() + iLine );
            if( iLine > static_cast<int>( cur_rules.size() ) - 1 ) {
                iLine--;
            }
            if( iLine < 0 ) {
                iLine = 0;
            }
        } else if( action == "COPY_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            cur_rules.push_back( cur_rules[iLine] );
            iLine = cur_rules.size() - 1;
        } else if( allow_swapping && action == "SWAP_RULE_GLOBAL_CHAR" && currentPageNonEmpty ) {
            const size_t other_iTab = ( iTab + 1 ) % 2;
            rule_list &other_rules = tabs[other_iTab].new_rules;
            bStuffChanged = true;
            other_rules.push_back( cur_rules[iLine] );
            cur_rules.erase( cur_rules.begin() + iLine );
            iTab = other_iTab;
            iLine = other_rules.size() - 1;
        } else if( action == "ADD_RULE" || ( action == "CONFIRM" && currentPageNonEmpty ) ) {
            const int old_iLine = iLine;
            if( action == "ADD_RULE" ) {
                cur_rules.push_back( rule( "", true, false ) );
                iLine = cur_rules.size() - 1;
            }
            ui_manager::redraw();

            if( iColumn == 1 || action == "ADD_RULE" ) {
                ui_adaptor help_ui;
                catacurses::window w_help;
                const auto init_help_window = [&]( ui_adaptor & help_ui ) {
                    const point iOffset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                         TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );
                    w_help = catacurses::newwin( FULL_SCREEN_HEIGHT / 2 + 2,
                                                 FULL_SCREEN_WIDTH * 3 / 4,
                                                 iOffset + point( 19 / 2, 7 + FULL_SCREEN_HEIGHT / 2 / 2 ) );
                    help_ui.position_from_window( w_help );
                };
                init_help_window( help_ui );
                help_ui.on_screen_resize( init_help_window );

                help_ui.on_redraw( [&]( const ui_adaptor & ) {
                    // NOLINTNEXTLINE(cata-use-named-point-constants)
                    fold_and_print( w_help, point( 1, 1 ), 999, c_white,
                                    _(
                                        "* is used as a Wildcard.  A few Examples:\n"
                                        "\n"
                                        "cattail stalk   matches the itemname exactly\n"
                                        "cattail*        matches items beginning with cattail\n"
                                        "*berries        matches items ending with berries\n"
                                        "*wild*veg*      multiple * are allowed\n"
                                        "DanDeLion*      case insensitive search\n"
                                        "\n"
                                        "Foraging based on item materials:\n"
                                        "m:wheat         matches items made of wheat\n"
                                        "M:fruit         matches items made purely of fruit\n"
                                        "M:veggy,fruit   multiple materials allowed (OR search)" )
                                  );

                    draw_border( w_help );
                    wnoutrefresh( w_help );
                } );
                const std::string r = string_input_popup()
                                      .title( _( "Foraging Rule:" ) )
                                      .width( 30 )
                                      .text( cur_rules[iLine].sRule )
                                      .query_string();
                if( !r.empty() ) {
                    cur_rules[iLine].sRule = wildcard_trim_rule( r );
                    afg_log() << "rule pattern set: '" << cur_rules[iLine].sRule << "'";
                    bStuffChanged = true;
                } else if( action == "ADD_RULE" ) {
                    cur_rules.pop_back();
                    iLine = old_iLine;
                }
            } else if( iColumn == 2 ) {
                bStuffChanged = true;
                cur_rules[iLine].bExclude = !cur_rules[iLine].bExclude;
            }
        } else if( action == "ENABLE_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            cur_rules[iLine].bActive = true;
        } else if( action == "DISABLE_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            cur_rules[iLine].bActive = false;
        } else if( action == "LEFT" ) {
            iColumn--;
            if( iColumn < 1 ) {
                iColumn = iTotalCols;
            }
        } else if( action == "RIGHT" ) {
            iColumn++;
            if( iColumn > iTotalCols ) {
                iColumn = 1;
            }
        } else if( action == "MOVE_RULE_UP" && currentPageNonEmpty ) {
            bStuffChanged = true;
            if( iLine < static_cast<int>( cur_rules.size() ) - 1 ) {
                std::swap( cur_rules[iLine], cur_rules[iLine + 1] );
                iLine++;
                iColumn = 1;
            }
        } else if( action == "MOVE_RULE_DOWN" && currentPageNonEmpty ) {
            bStuffChanged = true;
            if( iLine > 0 ) {
                std::swap( cur_rules[iLine], cur_rules[iLine - 1] );
                iLine--;
                iColumn = 1;
            }
        } else if( action == "TEST_RULE" && currentPageNonEmpty && !g->u.name.empty() ) {
            cur_rules[iLine].test_pattern();
        } else if( action == "SWITCH_AUTO_FORAGING_OPTION" ) {
            get_options().get_option( "AUTO_FORAGING" ).setNext();
            get_options().save();
        }
    }

    if( !bStuffChanged ) {
        return;
    }

    if( !query_yn( _( "Save changes?" ) ) ) {
        return;
    }

    for( tab &t : tabs ) {
        t.rules.get() = t.new_rules;
    }
}

void player_settings::show()
{
    afg_log() << "show() opening; global=" << global_rules.size()
              << " char=" << character_rules.size();

    user_interface ui;

    ui.title = _( " AUTO FORAGING MANAGER " );
    ui.tabs.emplace_back( _( "[<Global>]" ), global_rules );
    if( !g->u.name.empty() ) {
        ui.tabs.emplace_back( _( "[<Character>]" ), character_rules );
    }
    ui.is_autoforaging = true;

    ui.show();

    if( !ui.bStuffChanged ) {
        afg_log() << "show() closed without changes";
        return;
    }

    afg_log() << "show() saving changes; global=" << global_rules.size()
              << " char=" << character_rules.size();
    save_global();
    if( !g->u.name.empty() ) {
        save_character();
    }
    invalidate();
}

void rule::test_pattern() const
{
    std::vector<std::string> vMatchingItems;

    if( sRule.empty() ) {
        return;
    }

    for( const itype *e : item_controller->all() ) {
        const std::string sItemName = e->nname( 1 );
        if( !check_special_rule( e->materials, sRule ) && !wildcard_match( sItemName, sRule ) ) {
            continue;
        }

        vMatchingItems.push_back( sItemName );
    }

    int iStartPos = 0;
    int iContentHeight = 0;
    int iContentWidth = 0;

    catacurses::window w_test_rule_border;
    catacurses::window w_test_rule_content;

    ui_adaptor ui;

    const auto init_windows = [&]( ui_adaptor & ui ) {
        const point iOffset( 15 + ( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ),
                             5 + ( TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 :
                                   0 ) );
        iContentHeight = FULL_SCREEN_HEIGHT - 8;
        iContentWidth = FULL_SCREEN_WIDTH - 30;

        w_test_rule_border = catacurses::newwin( iContentHeight + 2, iContentWidth,
                             iOffset );
        w_test_rule_content = catacurses::newwin( iContentHeight,
                              iContentWidth - 2,
                              iOffset + point_south_east );

        ui.position_from_window( w_test_rule_border );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    int nmatch = vMatchingItems.size();
    const std::string buf = string_format( vgettext( "%1$d item matches: %2$s",
                                           "%1$d items match: %2$s",
                                           nmatch ), nmatch, sRule );

    int iLine = 0;

    input_context ctxt( "AUTO_FORAGING_TEST" );
    ctxt.register_updown();
    ctxt.register_action( "PAGE_UP", to_translation( "Page up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Page down" ) );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w_test_rule_border, BORDER_COLOR, buf, hilite( c_white ) );
        center_print( w_test_rule_border, iContentHeight + 1, red_background( c_white ),
                      _( "Won't display content or suffix matches" ) );
        wnoutrefresh( w_test_rule_border );

        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < 79; j++ ) {
                mvwputch( w_test_rule_content, point( j, i ), c_black, ' ' );
            }
        }

        calcStartPos( iStartPos, iLine, iContentHeight, vMatchingItems.size() );

        for( int i = iStartPos; i < static_cast<int>( vMatchingItems.size() ); i++ ) {
            if( i >= iStartPos &&
                i < iStartPos + ( iContentHeight > static_cast<int>( vMatchingItems.size() ) ?
                                  static_cast<int>( vMatchingItems.size() ) : iContentHeight ) ) {
                nc_color cLineColor = c_white;

                mvwprintz( w_test_rule_content, point( 0, i - iStartPos ), cLineColor, "%d", i + 1 );
                mvwprintz( w_test_rule_content, point( 4, i - iStartPos ), cLineColor, "" );

                if( iLine == i ) {
                    wprintz( w_test_rule_content, c_yellow, ">> " );
                } else {
                    wprintz( w_test_rule_content, c_yellow, "   " );
                }

                wprintz( w_test_rule_content, iLine == i ? hilite( cLineColor ) : cLineColor, vMatchingItems[i] );
            }
        }

        wnoutrefresh( w_test_rule_content );
    } );

    while( true ) {
        ui_manager::redraw();

        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            iLine++;
            if( iLine >= static_cast<int>( vMatchingItems.size() ) ) {
                iLine = 0;
            }
        } else if( action == "UP" ) {
            iLine--;
            if( iLine < 0 ) {
                iLine = vMatchingItems.size() - 1;
            }
        } else if( action == "PAGE_DOWN" || action == "PAGE_UP" ) {
            // Advance the view by one full visible-list page. Because
            // calcStartPos re-centres the cursor on each redraw when
            // MENU_SCROLL is on, anchor the target cursor at the centre
            // of the next page so the view truly shifts by iContentHeight.
            if( !vMatchingItems.empty() ) {
                const int last = static_cast<int>( vMatchingItems.size() ) - 1;
                const int half = std::max( 0, ( iContentHeight - 1 ) / 2 );
                if( action == "PAGE_DOWN" ) {
                    iLine = iLine == last
                            ? 0
                            : std::min( last, iStartPos + iContentHeight + half );
                } else {
                    iLine = iLine == 0
                            ? last
                            : std::max( 0, iStartPos - iContentHeight + half );
                }
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }
}

bool player_settings::empty() const
{
    return global_rules.empty() && character_rules.empty();
}

bool check_special_rule( const std::vector<material_id> &materials, const std::string &rule )
{
    char type = ' ';
    std::vector<std::string> filter;
    if( rule[1] == ':' ) {
        type = rule[0];
        filter = string_split( rule.substr( 2 ), ',' );
    }

    if( filter.empty() || materials.empty() ) {
        return false;
    }

    if( type == 'm' ) {
        return std::ranges::any_of( materials, [&filter]( const material_id & mat ) {
            return std::ranges::any_of( filter, [&mat]( const std::string & search ) {
                return lcmatch( mat->name(), search );
            } );
        } );

    } else if( type == 'M' ) {
        return std::ranges::all_of( materials, [&filter]( const material_id & mat ) {
            return std::ranges::any_of( filter, [&mat]( const std::string & search ) {
                return lcmatch( mat->name(), search );
            } );
        } );
    }

    return false;
}

void player_settings::create_rule( const std::string &to_match )
{
    global_rules.create_rule( map_items, to_match );
    character_rules.create_rule( map_items, to_match );
}

void rule_list::create_rule( cache &map_items, const std::string &to_match )
{
    for( const rule &elem : *this ) {
        if( !elem.bActive || !wildcard_match( to_match, elem.sRule ) ) {
            continue;
        }

        map_items[ to_match ] = elem.bExclude ? RULE_BLACKLISTED : RULE_WHITELISTED;
    }
}

void rule_list::create_rule( cache &map_items, const item &it )
{
    const std::string to_match = it.tname( 1, false );

    for( const rule &elem : *this ) {
        if( !elem.bActive ) {
            continue;
        } else if( !check_special_rule( it.made_of(), elem.sRule ) &&
                   !wildcard_match( to_match, elem.sRule ) ) {
            continue;
        }

        map_items[ to_match ] = elem.bExclude ? RULE_BLACKLISTED : RULE_WHITELISTED;
    }
}

void player_settings::refresh_map_items( cache &map_items ) const
{
    global_rules.refresh_map_items( map_items );
    character_rules.refresh_map_items( map_items );
}

void rule_list::refresh_map_items( cache &map_items ) const
{
    for( const rule &elem : *this ) {
        if( elem.sRule.empty() || !elem.bActive ) {
            continue;
        }

        if( !elem.bExclude ) {
            for( const itype *e : item_controller->all() ) {
                const std::string &cur_item = e->nname( 1 );

                if( !check_special_rule( e->materials, elem.sRule ) && !wildcard_match( cur_item, elem.sRule ) ) {
                    continue;
                }

                map_items[ cur_item ] = RULE_WHITELISTED;
                map_items.temp_items[ cur_item ] = e;
            }
        } else {
            for( auto &map_item : map_items ) {
                if( !check_special_rule( map_items.temp_items[ map_item.first ]->materials, elem.sRule ) &&
                    !wildcard_match( map_item.first, elem.sRule ) ) {
                    continue;
                }

                map_items[ map_item.first ] = RULE_BLACKLISTED;
            }
        }
    }
}

rule_state base_settings::check_item( const std::string &sItemName ) const
{
    if( !map_items.ready ) {
        recreate();
    }

    const auto iter = map_items.find( sItemName );
    if( iter != map_items.end() ) {
        return iter->second;
    }
    return RULE_NONE;
}

void player_settings::clear_character_rules()
{
    character_rules.clear();
    invalidate();
}

bool player_settings::save_character()
{
    return save( true );
}

bool player_settings::save_global()
{
    return save( false );
}

bool player_settings::save( const bool bCharacter )
{
    if( bCharacter ) {
        if( !g->get_active_world()->player_file_exist( ".sav" ) ) {
            return true;
        }

        return g->get_active_world()->write_to_player_file( ".afg.json", [&]( std::ostream & fout ) {
            JsonOut jout( fout, true );
            ( bCharacter ? character_rules : global_rules ).serialize( jout );
        }, _( "autoforaging configuration" ) );
    } else {
        return write_to_file( PATH_INFO::autoforaging(), [&]( std::ostream & fout ) {
            JsonOut jout( fout, true );
            ( bCharacter ? character_rules : global_rules ).serialize( jout );
        }, _( "autoforaging configuration" ) );
    }
}

void player_settings::load_character()
{
    load( true );
}

void player_settings::load_global()
{
    load( false );
}

void player_settings::load( const bool bCharacter )
{
    // Clear the target rule list before reading. The deserialize callback only
    // fires if the file exists; without this, missing-file cases leak stale
    // rules from a previous character (singleton state persists across
    // new-game starts since start_game() doesn't load_character).
    ( bCharacter ? character_rules : global_rules ).clear();
    if( bCharacter ) {
        g->get_active_world()->read_from_player_file_json( ".afg.json", [&]( JsonIn & jsin ) {
            ( bCharacter ? character_rules : global_rules ).deserialize( jsin );
        }, true );
        afg_log() << "load_character() done; character_rules=" << character_rules.size();
    } else {
        read_from_file_json( PATH_INFO::autoforaging(), [&]( JsonIn & jsin ) {
            ( bCharacter ? character_rules : global_rules ).deserialize( jsin );
        }, true );
        afg_log() << "load_global() done from " << PATH_INFO::autoforaging()
                  << "; global_rules=" << global_rules.size();
    }
    invalidate();
}

void rule::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "rule", sRule );
    jsout.member( "active", bActive );
    jsout.member( "exclude", bExclude );
    jsout.end_object();
}

void rule_list::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    for( const rule &elem : *this ) {
        elem.serialize( jsout );
    }
    jsout.end_array();
}

void rule::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    sRule = jo.get_string( "rule" );
    bActive = jo.get_bool( "active" );
    bExclude = jo.get_bool( "exclude" );
}

void rule_list::deserialize( JsonIn &jsin )
{
    clear();

    jsin.start_array();
    while( !jsin.end_array() ) {
        rule tmp;
        tmp.deserialize( jsin );
        push_back( tmp );
    }
}

void base_settings::recreate() const
{
    map_items.clear();
    map_items.temp_items.clear();
    refresh_map_items( map_items );
    map_items.ready = true;
    map_items.temp_items.clear();
}

void base_settings::invalidate()
{
    map_items.ready = false;
}
