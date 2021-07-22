#pragma once

namespace kx { namespace gfx {
class GfxLibrary;
class FontLibrary;
}}

namespace kx { namespace sfx {
class SfxLibrary;
}}

struct LibraryPointers
{
    kx::gfx::GfxLibrary *gfx_library;
    kx::gfx::FontLibrary *font_library;
    kx::sfx::SfxLibrary *sfx_library;
};
