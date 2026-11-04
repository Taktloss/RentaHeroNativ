// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/present/gl.hpp"

#include <SDL3/SDL_video.h>

namespace rah::gl {

bool Api::load(const char** missing) {
#define RAH_GL_LOAD(type, name)                                                        \
    name = reinterpret_cast<type>(SDL_GL_GetProcAddress("gl" #name));                  \
    if (!name) {                                                                       \
        if (missing) *missing = "gl" #name;                                            \
        return false;                                                                  \
    }
    RAH_GL_FUNCS(RAH_GL_LOAD)
#undef RAH_GL_LOAD
    return true;
}

}  // namespace rah::gl
