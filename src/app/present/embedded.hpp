// SPDX-License-Identifier: GPL-3.0-or-later
// Ins Programm eingebettete Dateien (Shader aus third_party/libretro-shaders; cmake/embed.cmake)
#pragma once

#include <optional>
#include <string_view>

namespace rah {

std::optional<std::string_view> embeddedFile(std::string_view path);

}  // namespace rah
