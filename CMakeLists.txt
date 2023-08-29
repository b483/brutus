cmake_minimum_required(VERSION 3.23[FATAL_ERROR])

set(SHORT_NAME brutus)
project(${SHORT_NAME} VERSION 1.0.0 LANGUAGES C)
set(IS_RELEASE_VERSION FALSE)
set(CMAKE_C_STANDARD 23)

option(DRAW_FPS "Draw FPS on the top left corner of the window." OFF)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake/")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wno-sign-compare -Wno-discarded-qualifiers -Wno-stringop-truncation -Wpedantic -Wformat-security -Wformat-y2k -Wnull-dereference
        -Winit-self -Wsync-nand -Wunused-const-variable=2 -Wstringop-overflow=4 -Walloc-zero -Wtrivial-auto-var-init -Warith-conversion -Wbidi-chars=any
        -Wenum-conversion -Warray-bounds=2 -Wattribute-alias=2 -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wshadow=compatible-local
        -Wunsafe-loop-optimizations -Wbad-function-cast -Wcast-align -Wwrite-strings -Wdangling-else -Wdate-time -Wlogical-op
        -Waggregate-return -Wno-aggressive-loop-optimizations -Wmissing-prototypes -Wmissing-declarations -Wopenmp-simd -Wpacked -Wredundant-decls
        -Wnested-externs -Winline -Winvalid-pch -Wvector-operation-performance -Wdisabled-optimization -Wstrict-prototypes -Werror
        -fanalyzer)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wno-sign-compare)
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
    ${PROJECT_SOURCE_DIR}/src/platform/brutus.c
    ${PROJECT_SOURCE_DIR}/src/platform/version.c
)

set(CORE_FILES
    ${PROJECT_SOURCE_DIR}/src/core/buffer.c
    ${PROJECT_SOURCE_DIR}/src/core/calc.c
    ${PROJECT_SOURCE_DIR}/src/core/config.c
    ${PROJECT_SOURCE_DIR}/src/core/file.c
    ${PROJECT_SOURCE_DIR}/src/core/hotkey_config.c
    ${PROJECT_SOURCE_DIR}/src/core/image.c
    ${PROJECT_SOURCE_DIR}/src/core/lang.c
    ${PROJECT_SOURCE_DIR}/src/core/random.c
    ${PROJECT_SOURCE_DIR}/src/core/smacker.c
    ${PROJECT_SOURCE_DIR}/src/core/speed.c
    ${PROJECT_SOURCE_DIR}/src/core/string.c
    ${PROJECT_SOURCE_DIR}/src/core/time.c
)

set(BUILDING_FILES
    ${PROJECT_SOURCE_DIR}/src/building/building.c
)
set(CITY_FILES
    ${PROJECT_SOURCE_DIR}/src/city/city_new.c
    ${PROJECT_SOURCE_DIR}/src/city/resource.c
)
set(EMPIRE_FILES
    ${PROJECT_SOURCE_DIR}/src/empire/empire.c
    ${PROJECT_SOURCE_DIR}/src/empire/object.c
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
    ${PROJECT_SOURCE_DIR}/src/game/game.c
)
set(INPUT_FILES
    ${PROJECT_SOURCE_DIR}/src/input/input.c
)
set(MAP_FILES
    ${PROJECT_SOURCE_DIR}/src/map/map.c
)
set(SCENARIO_FILES
    ${PROJECT_SOURCE_DIR}/src/scenario/scenario.c
)
set(GRAPHICS_FILES
    ${PROJECT_SOURCE_DIR}/src/graphics/graphics.c
)
set(SOUND_FILES
    ${PROJECT_SOURCE_DIR}/src/sound/sound.c
)
set(WINDOW_FILES
    ${PROJECT_SOURCE_DIR}/src/window/window.c
)
set(EDITOR_FILES
    ${PROJECT_SOURCE_DIR}/src/editor/editor.c
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

include_directories(ext)
target_include_directories(${SHORT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

if(UNIX AND CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${SHORT_NAME} m)
endif()

install(TARGETS ${SHORT_NAME} RUNTIME DESTINATION bin)
