#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_TFT_LOAD_GFXFF

// system includes
#include <cstdint>

// local includes
#include "gfxfont.h"

namespace espgui {

extern const uint8_t din1451alt10pt8bBitmaps[];

extern const GFXglyph din1451alt10pt8bGlyphs[];

extern const GFXfont din1451alt10pt8b;

} // namespace espgui
#endif
