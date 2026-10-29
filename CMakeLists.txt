# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)
project(RentaHeroNativ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RAH_BUILD_APP "Spiel bauen (benötigt SDL3 und FFmpeg)" ON)
option(RAH_BUILD_TESTS "Tests bauen" ON)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion)
endif()

# Kern ohne Plattformabhängigkeiten: Datenquellen, Dateiformate, Spiellogik
add_library(rah_core STATIC
  src/core/bytes.cpp
  src/core/strings.cpp
  src/vfs/cue.cpp
  src/vfs/iso9660.cpp
  src/vfs/dirfs.cpp
  src/vfs/vfs.cpp
  src/formats/tcc.cpp
  src/formats/ini.cpp
  src/formats/zbf.cpp
  src/formats/szn.cpp
  src/engine/paths.cpp
  src/engine/layer.cpp
  src/engine/event.cpp
  src/engine/area.cpp
  src/engine/animlayer.cpp
  src/engine/actor.cpp
  src/engine/pathfinder.cpp
  src/engine/aviimage.cpp
  src/engine/scene.cpp
  src/engine/inventory.cpp
  src/engine/help.cpp
  src/engine/speech.cpp
  src/engine/audio.cpp
  src/audio/mixer.cpp
  src/formats/wav.cpp
  src/game/game.cpp
  src/game/loc01.cpp
  src/game/loc02.cpp
  src/game/loc03.cpp
  src/game/loc04.cpp
  src/game/loc05.cpp
  src/game/loc06.cpp
  src/game/loc10.cpp
  src/game/loc12.cpp
  src/game/loc31.cpp
  src/game/loc35.cpp
  src/game/loc37.cpp
  src/game/gamestate.cpp
  src/game/loc40.cpp
  src/video/iv5mask.cpp
  src/formats/bmp.cpp
  src/formats/pe.cpp
  src/render/tccsprite.cpp
  src/render/blit.cpp
)
target_include_directories(rah_core PUBLIC src)

# Schrift (GDI-Ersatz) über FreeType
find_package(Freetype)
if(FREETYPE_FOUND)
  add_library(rah_font STATIC src/render/font.cpp)
  target_link_libraries(rah_font PUBLIC rah_core Freetype::Freetype)
endif()

# Videoebenen über FFmpeg (docs/VIDEO.md)
find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(FFMPEG IMPORTED_TARGET libavformat libavcodec libswscale libswresample libavutil)
endif()
if(FFMPEG_FOUND)
  add_library(rah_video STATIC src/video/ffvideo.cpp)
  target_link_libraries(rah_video PUBLIC rah_core PkgConfig::FFMPEG)
endif()

if(RAH_BUILD_APP)
  find_package(SDL3 CONFIG)
  if(SDL3_FOUND AND FFMPEG_FOUND AND FREETYPE_FOUND)
    add_executable(rentahero src/app/main.cpp)
    target_link_libraries(rentahero PRIVATE rah_core rah_font rah_video SDL3::SDL3)
  else()
    message(WARNING "SDL3, FFmpeg oder FreeType nicht gefunden – nur Kern und Tests werden gebaut")
  endif()
endif()

if(RAH_BUILD_TESTS)
  enable_testing()
  add_executable(rah_tests
    tests/test_main.cpp
    tests/test_vfs.cpp
    tests/test_tcc.cpp
    tests/test_inventory.cpp
    tests/test_help.cpp
    tests/test_ini.cpp
    tests/test_blit.cpp
    tests/test_szn.cpp
    tests/test_scene.cpp
    tests/test_anim.cpp
    tests/test_pe.cpp
    tests/test_avi.cpp
    tests/test_game.cpp
    tests/test_audio.cpp
    tests/test_actor.cpp
    tests/test_iv5mask.cpp
  )
  target_link_libraries(rah_tests PRIVATE rah_core)
  if(FFMPEG_FOUND)
    target_sources(rah_tests PRIVATE tests/test_ffvideo.cpp)
    target_link_libraries(rah_tests PRIVATE rah_video)
  endif()
  if(FREETYPE_FOUND)
    target_sources(rah_tests PRIVATE tests/test_font.cpp)
    target_link_libraries(rah_tests PRIVATE rah_font)
  endif()
  add_test(NAME rah_tests COMMAND rah_tests)
endif()
