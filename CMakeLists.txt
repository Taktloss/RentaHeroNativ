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
)
target_include_directories(rah_core PUBLIC src)

if(RAH_BUILD_APP)
  find_package(SDL3 CONFIG)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG IMPORTED_TARGET libavformat libavcodec libswscale libswresample libavutil)
  endif()
  if(SDL3_FOUND AND FFMPEG_FOUND)
    add_executable(rentahero src/app/main.cpp)
    target_link_libraries(rentahero PRIVATE rah_core SDL3::SDL3 PkgConfig::FFMPEG)
  else()
    message(WARNING "SDL3 oder FFmpeg nicht gefunden – nur Kern und Tests werden gebaut")
  endif()
endif()

if(RAH_BUILD_TESTS)
  enable_testing()
  add_executable(rah_tests
    tests/test_main.cpp
    tests/test_vfs.cpp
    tests/test_tcc.cpp
    tests/test_ini.cpp
  )
  target_link_libraries(rah_tests PRIVATE rah_core)
  add_test(NAME rah_tests COMMAND rah_tests)
endif()
