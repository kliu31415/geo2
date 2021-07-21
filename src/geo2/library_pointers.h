#pragma once

namespace kx { namespace gfx {
class GfxLibrary;
class FontLibrary;
}}

struct LibraryPointers
{
    kx::gfx::GfxLibrary *gfx_library;
    kx::gfx::FontLibrary *font_library;
};
