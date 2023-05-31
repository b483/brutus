cmake_minimum_required(VERSION 3.23[FATAL_ERROR])

set(SHORT_NAME brutus)
project(${SHORT_NAME} VERSION 1.0.0 LANGUAGES C)
set(IS_RELEASE_VERSION FALSE)
set(CMAKE_C_STANDARD 23)

option(DRAW_FPS "Draw FPS on the top left corner of the window." OFF)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake/")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wno-sign-compare -Wno-stringop-truncation -Wpedantic -Wformat-security -Wformat-y2k -Wnull-dereference
        -Winit-self -Wsync-nand -Wunused-const-variable=2 -Wstringop-overflow=4 -Walloc-zero -Wtrivial-auto-var-init -Warith-conversion -Wbidi-chars=any
        -Wenum-conversion -Warray-bounds=2 -Wattribute-alias=2 -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wshadow=compatible-local
        -Wunsafe-loop-optimizations -Wbad-function-cast -Wcast-align -Wwrite-strings -Wdangling-else -Wdate-time -Wlogical-op
        -Waggregate-return -Wno-aggressive-loop-optimizations -Wmissing-prototypes -Wmissing-declarations -Wopenmp-simd -Wpacked -Wredundant-decls
        -Wnested-externs -Winline -Winvalid-pch -Wvector-operation-performance -Wdisabled-optimization -Wstrict-prototypes -Werror
        -fanalyzer)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Weverything -Wno-sign-compare -Wno-sign-conversion -Wno-declaration-after-statement -Wno-c++98-compat -Wno-switch-enum -Wno-float-conversion -Wno-float-equal
        -Wno-implicit-int-float-conversion -Wno-cast-align -Wno-implicit-int-conversion -Wno-covered-switch-default -Wno-double-promotion -Wno-implicit-fallthrough -Wno-vla -Wno-reserved-identifier
    )
endif()

if(IS_RELEASE_VERSION)
    set(PROJECT_VERSION_TWEAK 0)
    set(VERSION_REVISION "")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    find_package(Git)

    if(Git_FOUND)
        execute_process(
            COMMAND git rev-list --count HEAD ^tags/v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE TWEAK_RESULT
            OUTPUT_VARIABLE PROJECT_VERSION_TWEAK
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND git rev-parse --short --verify HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE VERSION_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND git diff-index --quiet HEAD --
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE VERSION_COMMIT_DIRTY
        )

        if(VERSION_COMMIT_DIRTY)
            set(DIRTY_POSTFIX "-dirty")
        else()
            set(DIRTY_POSTFIX "")
        endif()

        if(NOT TWEAK_RESULT EQUAL 0)
            # Happens on a shallow git clone, like Travis does. Append date to version ref.
            set(PROJECT_VERSION_TWEAK 0)
            string(TIMESTAMP TODAY "%Y%m%d")
            set(VERSION_REVISION "-${TODAY}-${VERSION_COMMIT_HASH}${DIRTY_POSTFIX}")
        else()
            set(VERSION_REVISION ".${PROJECT_VERSION_TWEAK}-${VERSION_COMMIT_HASH}${DIRTY_POSTFIX}")
        endif()
    else()
        set(PROJECT_VERSION_TWEAK 0)
        set(VERSION_REVISION " unknown development version")
    endif()
else()
    set(PROJECT_VERSION_TWEAK 0)
    set(VERSION_REVISION " unknown development version")
endif()

configure_file(${PROJECT_SOURCE_DIR}/gen/version.c.in ${PROJECT_SOURCE_DIR}/src/platform/version.c)
configure_file(${PROJECT_SOURCE_DIR}/gen/version.rc.in ${PROJECT_SOURCE_DIR}/res/version.rc)
configure_file(${PROJECT_SOURCE_DIR}/gen/version.txt.in ${PROJECT_SOURCE_DIR}/res/version.txt)

set(PLATFORM_FILES
    ${PROJECT_SOURCE_DIR}/src/platform/arguments.c
    ${PROJECT_SOURCE_DIR}/src/platform/cursor.c
    ${PROJECT_SOURCE_DIR}/src/platform/brutus.c
    ${PROJECT_SOURCE_DIR}/src/platform/keyboard_input.c
    ${PROJECT_SOURCE_DIR}/src/platform/log.c
    ${PROJECT_SOURCE_DIR}/src/platform/mouse.c
    ${PROJECT_SOURCE_DIR}/src/platform/platform.c
    ${PROJECT_SOURCE_DIR}/src/platform/screen.c
    ${PROJECT_SOURCE_DIR}/src/platform/sound_device.c
    ${PROJECT_SOURCE_DIR}/src/platform/version.c
)

set(CORE_FILES
    ${PROJECT_SOURCE_DIR}/src/core/buffer.c
    ${PROJECT_SOURCE_DIR}/src/core/calc.c
    ${PROJECT_SOURCE_DIR}/src/core/config.c
    ${PROJECT_SOURCE_DIR}/src/core/dir.c
    ${PROJECT_SOURCE_DIR}/src/core/encoding.c
    ${PROJECT_SOURCE_DIR}/src/core/file.c
    ${PROJECT_SOURCE_DIR}/src/core/hotkey_config.c
    ${PROJECT_SOURCE_DIR}/src/core/image.c
    ${PROJECT_SOURCE_DIR}/src/core/io.c
    ${PROJECT_SOURCE_DIR}/src/core/lang.c
    ${PROJECT_SOURCE_DIR}/src/core/random.c
    ${PROJECT_SOURCE_DIR}/src/core/smacker.c
    ${PROJECT_SOURCE_DIR}/src/core/speed.c
    ${PROJECT_SOURCE_DIR}/src/core/string.c
    ${PROJECT_SOURCE_DIR}/src/core/time.c
    ${PROJECT_SOURCE_DIR}/src/core/zip.c
)

set(BUILDING_FILES
    ${PROJECT_SOURCE_DIR}/src/building/animation.c
    ${PROJECT_SOURCE_DIR}/src/building/barracks.c
    ${PROJECT_SOURCE_DIR}/src/building/building.c
    ${PROJECT_SOURCE_DIR}/src/building/building_state.c
    ${PROJECT_SOURCE_DIR}/src/building/clone.c
    ${PROJECT_SOURCE_DIR}/src/building/construction.c
    ${PROJECT_SOURCE_DIR}/src/building/construction_clear.c
    ${PROJECT_SOURCE_DIR}/src/building/construction_routed.c
    ${PROJECT_SOURCE_DIR}/src/building/construction_warning.c
    ${PROJECT_SOURCE_DIR}/src/building/count.c
    ${PROJECT_SOURCE_DIR}/src/building/destruction.c
    ${PROJECT_SOURCE_DIR}/src/building/dock.c
    ${PROJECT_SOURCE_DIR}/src/building/figure.c
    ${PROJECT_SOURCE_DIR}/src/building/granary.c
    ${PROJECT_SOURCE_DIR}/src/building/house.c
    ${PROJECT_SOURCE_DIR}/src/building/house_evolution.c
    ${PROJECT_SOURCE_DIR}/src/building/house_population.c
    ${PROJECT_SOURCE_DIR}/src/building/house_service.c
    ${PROJECT_SOURCE_DIR}/src/building/industry.c
    ${PROJECT_SOURCE_DIR}/src/building/list.c
    ${PROJECT_SOURCE_DIR}/src/building/maintenance.c
    ${PROJECT_SOURCE_DIR}/src/building/market.c
    ${PROJECT_SOURCE_DIR}/src/building/storage.c
    ${PROJECT_SOURCE_DIR}/src/building/warehouse.c
)
set(CITY_FILES
    ${PROJECT_SOURCE_DIR}/src/city/buildings.c
    ${PROJECT_SOURCE_DIR}/src/city/culture.c
    ${PROJECT_SOURCE_DIR}/src/city/data.c
    ${PROJECT_SOURCE_DIR}/src/city/emperor.c
    ${PROJECT_SOURCE_DIR}/src/city/entertainment.c
    ${PROJECT_SOURCE_DIR}/src/city/finance.c
    ${PROJECT_SOURCE_DIR}/src/city/gods.c
    ${PROJECT_SOURCE_DIR}/src/city/health.c
    ${PROJECT_SOURCE_DIR}/src/city/houses.c
    ${PROJECT_SOURCE_DIR}/src/city/labor.c
    ${PROJECT_SOURCE_DIR}/src/city/map.c
    ${PROJECT_SOURCE_DIR}/src/city/message.c
    ${PROJECT_SOURCE_DIR}/src/city/migration.c
    ${PROJECT_SOURCE_DIR}/src/city/military.c
    ${PROJECT_SOURCE_DIR}/src/city/population.c
    ${PROJECT_SOURCE_DIR}/src/city/ratings.c
    ${PROJECT_SOURCE_DIR}/src/city/resource.c
    ${PROJECT_SOURCE_DIR}/src/city/sentiment.c
    ${PROJECT_SOURCE_DIR}/src/city/trade.c
    ${PROJECT_SOURCE_DIR}/src/city/victory.c
    ${PROJECT_SOURCE_DIR}/src/city/view.c
    ${PROJECT_SOURCE_DIR}/src/city/warning.c
)
set(EMPIRE_FILES
    ${PROJECT_SOURCE_DIR}/src/empire/empire.c
    ${PROJECT_SOURCE_DIR}/src/empire/object.c
    ${PROJECT_SOURCE_DIR}/src/empire/trade_prices.c
)
set(FIGURE_FILES
    ${PROJECT_SOURCE_DIR}/src/figure/combat.c
    ${PROJECT_SOURCE_DIR}/src/figure/figure.c
    ${PROJECT_SOURCE_DIR}/src/figure/formation.c
    ${PROJECT_SOURCE_DIR}/src/figure/formation_enemy.c
    ${PROJECT_SOURCE_DIR}/src/figure/formation_herd.c
    ${PROJECT_SOURCE_DIR}/src/figure/formation_legion.c
    ${PROJECT_SOURCE_DIR}/src/figure/movement.c
    ${PROJECT_SOURCE_DIR}/src/figure/name.c
    ${PROJECT_SOURCE_DIR}/src/figure/phrase.c
    ${PROJECT_SOURCE_DIR}/src/figure/route.c
    ${PROJECT_SOURCE_DIR}/src/figure/service.c
    ${PROJECT_SOURCE_DIR}/src/figure/sound.c
    ${PROJECT_SOURCE_DIR}/src/figure/trader.c
)
set(FIGURETYPE_FILES
    ${PROJECT_SOURCE_DIR}/src/figuretype/animal.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/cartpusher.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/crime.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/docker.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/editor.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/enemy.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/entertainer.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/maintenance.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/market.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/migrant.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/missile.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/service.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/soldier.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/trader.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/wall.c
    ${PROJECT_SOURCE_DIR}/src/figuretype/water.c
)
set(GAME_FILES
    ${PROJECT_SOURCE_DIR}/src/game/animation.c
    ${PROJECT_SOURCE_DIR}/src/game/file.c
    ${PROJECT_SOURCE_DIR}/src/game/file_editor.c
    ${PROJECT_SOURCE_DIR}/src/game/file_io.c
    ${PROJECT_SOURCE_DIR}/src/game/game.c
    ${PROJECT_SOURCE_DIR}/src/game/orientation.c
    ${PROJECT_SOURCE_DIR}/src/game/settings.c
    ${PROJECT_SOURCE_DIR}/src/game/speed.c
    ${PROJECT_SOURCE_DIR}/src/game/state.c
    ${PROJECT_SOURCE_DIR}/src/game/tick.c
    ${PROJECT_SOURCE_DIR}/src/game/time.c
    ${PROJECT_SOURCE_DIR}/src/game/undo.c
)
set(INPUT_FILES
    ${PROJECT_SOURCE_DIR}/src/input/cursor.c
    ${PROJECT_SOURCE_DIR}/src/input/hotkey.c
    ${PROJECT_SOURCE_DIR}/src/input/keyboard.c
    ${PROJECT_SOURCE_DIR}/src/input/keys.c
    ${PROJECT_SOURCE_DIR}/src/input/mouse.c
    ${PROJECT_SOURCE_DIR}/src/input/scroll.c
)
set(MAP_FILES
    ${PROJECT_SOURCE_DIR}/src/map/aqueduct.c
    ${PROJECT_SOURCE_DIR}/src/map/bookmark.c
    ${PROJECT_SOURCE_DIR}/src/map/bridge.c
    ${PROJECT_SOURCE_DIR}/src/map/building.c
    ${PROJECT_SOURCE_DIR}/src/map/building_tiles.c
    ${PROJECT_SOURCE_DIR}/src/map/desirability.c
    ${PROJECT_SOURCE_DIR}/src/map/elevation.c
    ${PROJECT_SOURCE_DIR}/src/map/figure.c
    ${PROJECT_SOURCE_DIR}/src/map/grid.c
    ${PROJECT_SOURCE_DIR}/src/map/image.c
    ${PROJECT_SOURCE_DIR}/src/map/image_context.c
    ${PROJECT_SOURCE_DIR}/src/map/natives.c
    ${PROJECT_SOURCE_DIR}/src/map/orientation.c
    ${PROJECT_SOURCE_DIR}/src/map/point.c
    ${PROJECT_SOURCE_DIR}/src/map/property.c
    ${PROJECT_SOURCE_DIR}/src/map/random.c
    ${PROJECT_SOURCE_DIR}/src/map/ring.c
    ${PROJECT_SOURCE_DIR}/src/map/road_access.c
    ${PROJECT_SOURCE_DIR}/src/map/road_aqueduct.c
    ${PROJECT_SOURCE_DIR}/src/map/road_network.c
    ${PROJECT_SOURCE_DIR}/src/map/routing.c
    ${PROJECT_SOURCE_DIR}/src/map/routing_data.c
    ${PROJECT_SOURCE_DIR}/src/map/routing_path.c
    ${PROJECT_SOURCE_DIR}/src/map/routing_terrain.c
    ${PROJECT_SOURCE_DIR}/src/map/sprite.c
    ${PROJECT_SOURCE_DIR}/src/map/terrain.c
    ${PROJECT_SOURCE_DIR}/src/map/tiles.c
    ${PROJECT_SOURCE_DIR}/src/map/water.c
    ${PROJECT_SOURCE_DIR}/src/map/water_supply.c
)
set(SCENARIO_FILES
    ${PROJECT_SOURCE_DIR}/src/scenario/editor_events.c
    ${PROJECT_SOURCE_DIR}/src/scenario/map.c
    ${PROJECT_SOURCE_DIR}/src/scenario/scenario.c
)
set(GRAPHICS_FILES
    ${PROJECT_SOURCE_DIR}/src/graphics/arrow_button.c
    ${PROJECT_SOURCE_DIR}/src/graphics/button.c
    ${PROJECT_SOURCE_DIR}/src/graphics/font.c
    ${PROJECT_SOURCE_DIR}/src/graphics/generic_button.c
    ${PROJECT_SOURCE_DIR}/src/graphics/graphics.c
    ${PROJECT_SOURCE_DIR}/src/graphics/image.c
    ${PROJECT_SOURCE_DIR}/src/graphics/image_button.c
    ${PROJECT_SOURCE_DIR}/src/graphics/lang_text.c
    ${PROJECT_SOURCE_DIR}/src/graphics/menu.c
    ${PROJECT_SOURCE_DIR}/src/graphics/panel.c
    ${PROJECT_SOURCE_DIR}/src/graphics/rich_text.c
    ${PROJECT_SOURCE_DIR}/src/graphics/screen.c
    ${PROJECT_SOURCE_DIR}/src/graphics/screenshot.c
    ${PROJECT_SOURCE_DIR}/src/graphics/scrollbar.c
    ${PROJECT_SOURCE_DIR}/src/graphics/text.c
    ${PROJECT_SOURCE_DIR}/src/graphics/video.c
    ${PROJECT_SOURCE_DIR}/src/graphics/warning.c
    ${PROJECT_SOURCE_DIR}/src/graphics/window.c
)
set(SOUND_FILES
    ${PROJECT_SOURCE_DIR}/src/sound/city.c
    ${PROJECT_SOURCE_DIR}/src/sound/effect.c
    ${PROJECT_SOURCE_DIR}/src/sound/music.c
    ${PROJECT_SOURCE_DIR}/src/sound/speech.c
    ${PROJECT_SOURCE_DIR}/src/sound/system.c
)
set(WIDGET_FILES
    ${PROJECT_SOURCE_DIR}/src/widget/city.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_bridge.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_building_ghost.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_figure.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_overlay_education.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_overlay_entertainment.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_overlay_health.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_overlay_other.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_overlay_risks.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_with_overlay.c
    ${PROJECT_SOURCE_DIR}/src/widget/city_without_overlay.c
    ${PROJECT_SOURCE_DIR}/src/widget/input_box.c
    ${PROJECT_SOURCE_DIR}/src/widget/map_editor.c
    ${PROJECT_SOURCE_DIR}/src/widget/map_editor_tool.c
    ${PROJECT_SOURCE_DIR}/src/widget/minimap.c
    ${PROJECT_SOURCE_DIR}/src/widget/scenario_minimap.c
    ${PROJECT_SOURCE_DIR}/src/widget/top_menu.c
    ${PROJECT_SOURCE_DIR}/src/widget/top_menu_editor.c
    ${PROJECT_SOURCE_DIR}/src/widget/sidebar/city.c
    ${PROJECT_SOURCE_DIR}/src/widget/sidebar/common.c
    ${PROJECT_SOURCE_DIR}/src/widget/sidebar/editor.c
    ${PROJECT_SOURCE_DIR}/src/widget/sidebar/extra.c
    ${PROJECT_SOURCE_DIR}/src/widget/sidebar/slide.c
)
set(WINDOW_FILES
    ${PROJECT_SOURCE_DIR}/src/window/advisors.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/chief.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/education.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/entertainment.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/financial.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/health.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/imperial.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/labor.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/military.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/population.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/ratings.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/religion.c
    ${PROJECT_SOURCE_DIR}/src/window/advisor/trade.c
    ${PROJECT_SOURCE_DIR}/src/window/build_menu.c
    ${PROJECT_SOURCE_DIR}/src/window/building_info.c
    ${PROJECT_SOURCE_DIR}/src/window/building/common.c
    ${PROJECT_SOURCE_DIR}/src/window/building/culture.c
    ${PROJECT_SOURCE_DIR}/src/window/building/distribution.c
    ${PROJECT_SOURCE_DIR}/src/window/building/figures.c
    ${PROJECT_SOURCE_DIR}/src/window/building/government.c
    ${PROJECT_SOURCE_DIR}/src/window/building/house.c
    ${PROJECT_SOURCE_DIR}/src/window/building/industry.c
    ${PROJECT_SOURCE_DIR}/src/window/building/military.c
    ${PROJECT_SOURCE_DIR}/src/window/building/terrain.c
    ${PROJECT_SOURCE_DIR}/src/window/building/utility.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/allowed_buildings.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/attributes.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/briefing.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/build_menu.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/custom_messages.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/demand_changes.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/earthquakes.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/edit_earthquake.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/edit_custom_message.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/edit_demand_change.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/edit_invasion.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/edit_price_change.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/edit_request.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/empire.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/invasions.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/map.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/price_changes.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/requests.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/special_events.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/start_year.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/starting_conditions.c
    ${PROJECT_SOURCE_DIR}/src/window/editor/win_criteria.c
    ${PROJECT_SOURCE_DIR}/src/window/cck_selection.c
    ${PROJECT_SOURCE_DIR}/src/window/city.c
    ${PROJECT_SOURCE_DIR}/src/window/config.c
    ${PROJECT_SOURCE_DIR}/src/window/display_options.c
    ${PROJECT_SOURCE_DIR}/src/window/donate_to_city.c
    ${PROJECT_SOURCE_DIR}/src/window/empire.c
    ${PROJECT_SOURCE_DIR}/src/window/file_dialog.c
    ${PROJECT_SOURCE_DIR}/src/window/gift_to_emperor.c
    ${PROJECT_SOURCE_DIR}/src/window/hold_festival.c
    ${PROJECT_SOURCE_DIR}/src/window/hotkey_config.c
    ${PROJECT_SOURCE_DIR}/src/window/hotkey_editor.c
    ${PROJECT_SOURCE_DIR}/src/window/intermezzo.c
    ${PROJECT_SOURCE_DIR}/src/window/intro_video.c
    ${PROJECT_SOURCE_DIR}/src/window/labor_priority.c
    ${PROJECT_SOURCE_DIR}/src/window/logo.c
    ${PROJECT_SOURCE_DIR}/src/window/main_menu.c
    ${PROJECT_SOURCE_DIR}/src/window/message_dialog.c
    ${PROJECT_SOURCE_DIR}/src/window/message_list.c
    ${PROJECT_SOURCE_DIR}/src/window/mission_briefing.c
    ${PROJECT_SOURCE_DIR}/src/window/mission_end.c
    ${PROJECT_SOURCE_DIR}/src/window/numeric_input.c
    ${PROJECT_SOURCE_DIR}/src/window/overlay_menu.c
    ${PROJECT_SOURCE_DIR}/src/window/plain_message_dialog.c
    ${PROJECT_SOURCE_DIR}/src/window/popup_dialog.c
    ${PROJECT_SOURCE_DIR}/src/window/resource_settings.c
    ${PROJECT_SOURCE_DIR}/src/window/select_list.c
    ${PROJECT_SOURCE_DIR}/src/window/set_salary.c
    ${PROJECT_SOURCE_DIR}/src/window/sound_options.c
    ${PROJECT_SOURCE_DIR}/src/window/speed_options.c
    ${PROJECT_SOURCE_DIR}/src/window/trade_opened.c
    ${PROJECT_SOURCE_DIR}/src/window/trade_prices.c
    ${PROJECT_SOURCE_DIR}/src/window/victory_dialog.c
    ${PROJECT_SOURCE_DIR}/src/window/victory_video.c
)
set(EDITOR_FILES
    ${PROJECT_SOURCE_DIR}/src/editor/editor.c
    ${PROJECT_SOURCE_DIR}/src/editor/tool.c
    ${PROJECT_SOURCE_DIR}/src/editor/tool_restriction.c
)

set(SOURCE_FILES
    ${PLATFORM_FILES}
    ${CORE_FILES}
    ${BUILDING_FILES}
    ${CITY_FILES}
    ${EMPIRE_FILES}
    ${FIGURE_FILES}
    ${FIGURETYPE_FILES}
    ${GAME_FILES}
    ${INPUT_FILES}
    ${MAP_FILES}
    ${SCENARIO_FILES}
    ${GRAPHICS_FILES}
    ${SOUND_FILES}
    ${WIDGET_FILES}
    ${WINDOW_FILES}
    ${EDITOR_FILES}
    ${PROJECT_SOURCE_DIR}/res/brutus.rc
)

add_executable(${SHORT_NAME} WIN32 ${SOURCE_FILES})

if(DRAW_FPS)
    target_compile_definitions(${SHORT_NAME} PRIVATE -DDRAW_FPS)
endif()

if(WIN32)
    target_compile_definitions(${SHORT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

function(GET_SDL_EXT_DIR result module)
    if(NOT module STREQUAL "")
        set(module "_${module}")
    endif()

    set(SDL_LOCATION ${PROJECT_SOURCE_DIR}/ext/SDL2)
    file(GLOB children
        RELATIVE ${SDL_LOCATION}
        CONFIGURE_DEPENDS
        ${SDL_LOCATION}/SDL${module}
        ${SDL_LOCATION}/SDL2${module}
        ${SDL_LOCATION}/SDL${module}-*
        ${SDL_LOCATION}/SDL2${module}-*
    )

    foreach(child ${children})
        if(IS_DIRECTORY "${SDL_LOCATION}/${child}")
            set(${result} "${SDL_LOCATION}/${child}" PARENT_SCOPE)
            break()
        endif()
    endforeach()
endfunction()

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

if(SDL2_INCLUDE_DIR)
    target_include_directories(${SHORT_NAME} SYSTEM PRIVATE ${SDL2_INCLUDE_DIR})
endif()

if(SDL2_MIXER_INCLUDE_DIR)
    target_include_directories(${SHORT_NAME} SYSTEM PRIVATE ${SDL2_MIXER_INCLUDE_DIR})
endif()

target_link_libraries(${SHORT_NAME} ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY})

set(PNG_FILES
    ext/png/png.c
    ext/png/pngerror.c
    ext/png/pngmem.c
    ext/png/pngset.c
    ext/png/pngwio.c
    ext/png/pngwrite.c
    ext/png/pngwutil.c
)

set(ZLIB_FILES
    ext/zlib/adler32.c
    ext/zlib/crc32.c
    ext/zlib/deflate.c
    ext/zlib/trees.c
    ext/zlib/zutil.c
)

target_include_directories(${SHORT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/ext/png)
target_sources(${SHORT_NAME} PRIVATE "${PNG_FILES}" "${ZLIB_FILES}")

include_directories(ext)
target_include_directories(${SHORT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

set_source_files_properties(
    ${PNG_FILES}
    PROPERTIES
    COMPILE_FLAGS "-w"
)

set_source_files_properties(
    ${ZLIB_FILES}
    PROPERTIES
    COMPILE_FLAGS "-w"
)

if(UNIX AND CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${SHORT_NAME} m)
endif()

install(TARGETS ${SHORT_NAME} RUNTIME DESTINATION bin)
