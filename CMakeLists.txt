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
  src/game/menu.cpp
  src/game/savegame.cpp
  src/game/loc01.cpp
  src/game/loc02.cpp
  src/game/loc03.cpp
  src/game/loc04.cpp
  src/game/loc05.cpp
  src/game/loc06.cpp
  src/game/loc07.cpp
  src/game/loc08.cpp
  src/game/loc10.cpp
  src/game/loc12.cpp
  src/game/loc14.cpp
  src/game/loc15.cpp
  src/game/loc16.cpp
  src/game/loc17.cpp
  src/game/loc18.cpp
  src/game/loc09.cpp
  src/game/loc19.cpp
  src/game/loc20.cpp
  src/game/loc21.cpp
  src/game/loc22.cpp
  src/game/loc23.cpp
  src/game/loc24.cpp
  src/game/loc25.cpp
  src/game/loc26.cpp
  src/game/loc27.cpp
  src/game/loc28.cpp
  src/game/loc29.cpp
  src/game/loc30.cpp
  src/game/loc31.cpp
  src/game/loc35.cpp
  src/game/loc37.cpp
  src/game/gamestate.cpp
  src/game/loc40.cpp
  src/game/loc41.cpp
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
  # Pixelvergleich mit der GDI-Messung unter Wine (docs/WINEPROBE.md)
  add_executable(rah_fontcmp tools/fontcmp.cpp)
  target_link_libraries(rah_fontcmp PRIVATE rah_font)
endif()

# Videoebenen über FFmpeg (docs/VIDEO.md)
find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(FFMPEG IMPORTED_TARGET libavformat libavcodec libswscale libavutil)
endif()
# Common-Dateien aus SETUP/DATA1.CAB entpacken (libunshield, MIT; docs/DATAPATHS.md)
if(PkgConfig_FOUND)
  pkg_check_modules(UNSHIELD IMPORTED_TARGET libunshield)
endif()
if(UNSHIELD_FOUND)
  add_library(rah_install STATIC src/vfs/install.cpp)
  target_link_libraries(rah_install PUBLIC rah_core PkgConfig::UNSHIELD)
  if(WIN32)
    # statische libunshield (tools/windows/build.sh): ihr .pc nennt zlib nicht
    find_package(ZLIB REQUIRED)
    target_link_libraries(rah_install PUBLIC ZLIB::ZLIB)
  endif()
endif()

if(FFMPEG_FOUND)
  add_library(rah_video STATIC src/video/ffvideo.cpp)
  target_link_libraries(rah_video PUBLIC rah_core PkgConfig::FFMPEG)
endif()

# Indeo-5-Masken über alle Bilder (docs/VIDEO.md)
add_executable(rah_iv5scan tools/iv5scan.cpp)
target_link_libraries(rah_iv5scan PRIVATE rah_core)

# Messwerkzeug Wegsucher (docs/PATHFINDING.md „Messung“)
add_executable(rah_pathstat tools/pathstat.cpp)
target_link_libraries(rah_pathstat PRIVATE rah_core)

# Shader-Presets lesen, Filterliste, Einstellungen (ohne GL; auch für die Tests)
include(cmake/embed.cmake)
rah_embed_dir(${CMAKE_CURRENT_BINARY_DIR}/shaders_embedded.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libretro-shaders "*.glsl" "*.glslp" "*.png")
add_library(rah_filters STATIC src/app/present/preset.cpp src/app/present/filters.cpp
  src/app/launchersettings.cpp ${CMAKE_CURRENT_BINARY_DIR}/shaders_embedded.cpp)
target_link_libraries(rah_filters PUBLIC rah_core)

if(RAH_BUILD_APP)
  find_package(SDL3 CONFIG)
  if(SDL3_FOUND AND FFMPEG_FOUND AND FREETYPE_FOUND)
    # Dear ImGui v1.92.9b (MIT, unverändert eingebunden) für das Startmenü (docs/LAUNCHER.md)
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
    add_library(rah_imgui STATIC
      ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp
      ${IMGUI_DIR}/imgui_widgets.cpp ${IMGUI_DIR}/imgui_stdlib.cpp
      ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
      ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(rah_imgui SYSTEM PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
    target_compile_definitions(rah_imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
    target_link_libraries(rah_imgui PUBLIC SDL3::SDL3)
    set_target_properties(rah_imgui PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(rah_imgui PRIVATE -w)
    endif()

    # Bildfilter-Darstellung (OpenGL, eingebettete libretro-Shader; docs/FILTERS.md)
    add_library(rah_present STATIC src/app/present/gl.cpp src/app/present/presenter.cpp src/app/ui.cpp)
    target_include_directories(rah_present SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
    target_link_libraries(rah_present PUBLIC rah_filters rah_imgui rah_font SDL3::SDL3)

    add_executable(rentahero src/app/main.cpp src/app/platform.cpp src/app/launcher.cpp)
    if(WIN32)
      # GUI-Programm (kein Konsolenfenster), Manifest: UTF-8-Codepage, DPI (docs/WINDOWS.md)
      enable_language(RC)
      target_sources(rentahero PRIVATE src/app/rentahero.rc)
      set_target_properties(rentahero PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    target_link_libraries(rentahero PRIVATE rah_core rah_font rah_video rah_present rah_imgui SDL3::SDL3)
    if(UNSHIELD_FOUND)
      target_link_libraries(rentahero PRIVATE rah_install)
      target_compile_definitions(rentahero PRIVATE RAH_HAVE_UNSHIELD=1)
    else()
      message(WARNING "libunshield nicht gefunden – Common-Ordner muss mit --common angegeben werden")
    endif()
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
    tests/test_launcher.cpp
    tests/test_filters.cpp
  )
  target_link_libraries(rah_tests PRIVATE rah_core rah_filters)
  if(FFMPEG_FOUND)
    target_sources(rah_tests PRIVATE tests/test_ffvideo.cpp)
    target_link_libraries(rah_tests PRIVATE rah_video)
  endif()
  if(UNSHIELD_FOUND)
    target_sources(rah_tests PRIVATE tests/test_install.cpp)
    target_link_libraries(rah_tests PRIVATE rah_install)
  endif()
  if(FREETYPE_FOUND)
    target_sources(rah_tests PRIVATE tests/test_font.cpp)
    target_link_libraries(rah_tests PRIVATE rah_font)
  endif()
  add_test(NAME rah_tests COMMAND rah_tests)
endif()
