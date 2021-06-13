#pragma once

#ifdef KX_RENDERER_SDL2
#include "kx/gfx/sdl2_renderer.h"
#elif defined(KX_RENDERER_GL)
#include "kx/gfx/gl_renderer.h"
#elif defined(KX_RENDERER_VK)
#include "kx/gfx/vk_renderer.h"
#else
#error "No renderer defined!"
#endif
