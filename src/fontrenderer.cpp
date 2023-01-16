#include "fontrenderer.h"

// 3rdparty lib includes
#include <tftinterface.h>

// local includes
#ifdef CONFIG_TFT_IS_AUTOBAHN
#include "din1451alt10pt8b.h"
#endif

#define pgm_read_byte(addr)   (*(const unsigned char *)(addr))
#define pgm_read_word(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(const unsigned short *)(_addr); \
})
#define pgm_read_dword(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(const unsigned long *)(_addr); \
})
#define pgm_read_float(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(const float *)(_addr); \
})
#define pgm_read_ptr(addr) ({ \
  typeof(addr) _addr = (addr); \
  *(void * const *)(_addr); \
})

#define pgm_get_far_address(x) ((uint32_t)(&(x)))

#define pgm_read_byte_near(addr)  pgm_read_byte(addr)
#define pgm_read_word_near(addr)  pgm_read_word(addr)
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_float_near(addr) pgm_read_float(addr)
#define pgm_read_ptr_near(addr)   pgm_read_ptr(addr)
#define pgm_read_byte_far(addr)   pgm_read_byte(addr)
#define pgm_read_word_far(addr)   pgm_read_word(addr)
#define pgm_read_dword_far(addr)  pgm_read_dword(addr)
#define pgm_read_float_far(addr)  pgm_read_float(addr)
#define pgm_read_ptr_far(addr)    pgm_read_ptr(addr)

namespace espgui {

namespace {
constexpr const auto _xDatum = 0;
constexpr const auto _yDatum = 0;
constexpr const auto _vpX = 0;
constexpr const auto _vpY = 0;
constexpr const auto _vpW = 640;
constexpr const auto _vpH = 480;
}

/***************************************************************************************
** Function name:           drawChar
** Description:             draw a single character in the GLCD or GFXFF font
***************************************************************************************/
void FontRenderer::drawChar(int32_t x, int32_t y, uint16_t c, uint16_t color, uint16_t bg, uint8_t size)
{
  if (c < 32) return;
#ifdef LOAD_GLCD
//>>>>>>>>>>>>>>>>>>
  #ifdef LOAD_GFXFF
  if(!gfxFont) { // 'Classic' built-in font
  #endif
//>>>>>>>>>>>>>>>>>>

  int32_t xd = x + _xDatum;
  int32_t yd = y + _yDatum;

  if ((xd >= _vpW)                 || // Clip right
     ( yd >= _vpH)                 || // Clip bottom
     ((xd + 6 * size - 1) < _vpX)  || // Clip left
     ((yd + 8 * size - 1) < _vpY))    // Clip top
    return;

  bool fillbg = (bg != color);
  bool clip = xd < _vpX || xd + 6  * textsize >= _vpW || yd < _vpY || yd + 8 * textsize >= _vpH;

  if ((size==1) && fillbg && !clip) {
    uint8_t column[6];
    uint8_t mask = 0x1;
    m_tft.begin_tft_write();

    m_tft.setWindow(xd, yd, xd+5, yd+7);

    for (int8_t i = 0; i < 5; i++)
      column[i] = pgm_read_byte(font + (c * 5) + i);
    column[5] = 0;

    for (int8_t j = 0; j < 8; j++) {
      for (int8_t k = 0; k < 5; k++ ) {
        if (column[k] & mask)
          m_tft.Write_16(color);
        else
          m_tft.Write_16(bg);
      }
      mask <<= 1;
      m_tft.Write_16(bg);
    }

    m_tft.end_tft_write();
  }
  else {
    //begin_tft_write();          // Sprite class can use this function, avoiding begin_tft_write()
//    inTransaction = true;

    for (int8_t i = 0; i < 6; i++ ) {
      uint8_t line;
      if (i == 5)
        line = 0x0;
      else
        line = pgm_read_byte(font + (c * 5) + i);

      if (size == 1 && !fillbg) { // default size
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) m_tft.drawPixel(x + i, y + j, color);
          line >>= 1;
        }
      }
      else {  // big size or clipped
        for (int8_t j = 0; j < 8; j++) {
          if (line & 0x1) m_tft.fillRect(x + (i * size), y + (j * size), size, size, color);
          else if (fillbg) m_tft.fillRect(x + i * size, y + j * size, size, size, bg);
          line >>= 1;
        }
      }
    }
//    inTransaction = lockTransaction;
    m_tft.end_tft_write();              // Does nothing if Sprite class uses this function
  }

//>>>>>>>>>>>>>>>>>>>>>>>>>>>
  #ifdef LOAD_GFXFF
  } else { // Custom font
  #endif
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
#endif // LOAD_GLCD

#ifdef LOAD_GFXFF
    // Filter out bad characters not present in font
    if ((c >= pgm_read_word(&gfxFont->first)) && (c <= pgm_read_word(&gfxFont->last ))) {
      //begin_tft_write();          // Sprite class can use this function, avoiding begin_tft_write()
//      inTransaction = true;
//>>>>>>>>>>>>>>>>>>>>>>>>>>>

      c -= pgm_read_word(&gfxFont->first);
      GFXglyph *glyph  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c]);
      uint8_t  *bitmap = (uint8_t *)pgm_read_dword(&gfxFont->bitmap);

      uint32_t bo = pgm_read_word(&glyph->bitmapOffset);
      uint8_t  w  = pgm_read_byte(&glyph->width),
               h  = pgm_read_byte(&glyph->height);
               //xa = pgm_read_byte(&glyph->xAdvance);
      int8_t   xo = pgm_read_byte(&glyph->xOffset),
               yo = pgm_read_byte(&glyph->yOffset);
      uint8_t  xx, yy, bits=0, bit=0;
      int16_t  xo16 = 0, yo16 = 0;

      if(size > 1) {
        xo16 = xo;
        yo16 = yo;
      }

      // GFXFF rendering speed up
      uint16_t hpc = 0; // Horizontal foreground pixel count
      for(yy=0; yy<h; yy++) {
        for(xx=0; xx<w; xx++) {
          if(bit == 0) {
            bits = pgm_read_byte(&bitmap[bo++]);
            bit  = 0x80;
          }
          if(bits & bit) hpc++;
          else {
           if (hpc) {
              if (size == 1) m_tft.drawFastHLine(x+xo+xx-hpc, y+yo+yy, hpc, color);
              else m_tft.fillRect(x+(xo16+xx-hpc)*size, y+(yo16+yy)*size, size*hpc, size, color);
              hpc=0;
            }
          }
          bit >>= 1;
        }
        // Draw pixels for this line as we are about to increment yy
        if (hpc) {
          if (size == 1) m_tft.drawFastHLine(x+xo+xx-hpc, y+yo+yy, hpc, color);
          else m_tft.fillRect(x+(xo16+xx-hpc)*size, y+(yo16+yy)*size, size*hpc, size, color);
          hpc=0;
        }
      }

//      inTransaction = lockTransaction;
      m_tft.end_tft_write();              // Does nothing if Sprite class uses this function
    }
#endif

#ifdef LOAD_GLCD
  #ifdef LOAD_GFXFF
  } // End classic vs custom font
  #endif
#else
  #ifndef LOAD_GFXFF
    // Avoid warnings if fonts are disabled
    x = x;
    y = y;
    color = color;
    bg = bg;
    size = size;
  #endif
#endif

}

/***************************************************************************************
** Function name:           drawChar
** Description:             draw a Unicode glyph onto the screen
***************************************************************************************/

// Any UTF-8 decoding must be done before calling drawChar()
int16_t FontRenderer::drawChar(uint16_t uniCode, int32_t x, int32_t y, uint16_t color, uint16_t bgcolor, uint8_t font)
{
    if (!uniCode) return 0;

  #ifdef CONFIG_TFT_IS_AUTOBAHN
      if (font == 4)
      {
          if (gfxFont != &din1451alt10pt8b)
              setFreeFont(&din1451alt10pt8b);
          font = 1;
      }
  #endif

    if (font==1) {
  #ifdef LOAD_GLCD
    #ifndef LOAD_GFXFF
      drawChar(x, y, uniCode, textcolor, textbgcolor, textsize);
      return 6 * textsize;
    #endif
  #else
    #ifndef LOAD_GFXFF
      return 0;
    #endif
  #endif

  #ifdef LOAD_GFXFF
      drawChar(x, y, uniCode, color, bgcolor, textsize);
      if(!gfxFont) { // 'Classic' built-in font
      #ifdef LOAD_GLCD
        return 6 * textsize;
      #else
        return 0;
      #endif
      }
      else {
        if((uniCode >= pgm_read_word(&gfxFont->first)) && (uniCode <= pgm_read_word(&gfxFont->last) )) {
          uint16_t   c2    = uniCode - pgm_read_word(&gfxFont->first);
          GFXglyph *glyph = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c2]);
          return pgm_read_byte(&glyph->xAdvance) * textsize;
        }
        else {
          return 0;
        }
      }
  #endif
    }

    if ((font>1) && (font<9) && ((uniCode < 32) || (uniCode > 127))) return 0;

    int32_t width  = 0;
    int32_t height = 0;
    uint32_t flash_address = 0;
    uniCode -= 32;

  #ifdef LOAD_FONT2
    if (font == 2) {
      flash_address = pgm_read_dword(&chrtbl_f16[uniCode]);
      width = pgm_read_byte(widtbl_f16 + uniCode);
      height = chr_hgt_f16;
    }
    #ifdef LOAD_RLE
    else
    #endif
  #endif

  #ifdef LOAD_RLE
    {
      if ((font>2) && (font<9)) {
        flash_address = pgm_read_dword( (const void*)(pgm_read_dword( &(fontdata[font].chartbl ) ) + uniCode*sizeof(void *)) );
        width = pgm_read_byte( (uint8_t *)pgm_read_dword( &(fontdata[font].widthtbl ) ) + uniCode );
        height= pgm_read_byte( &fontdata[font].height );
      }
    }
  #endif

    int32_t xd = x + _xDatum;
    int32_t yd = y + _yDatum;

    if ((xd + width * textsize < _vpX || xd >= _vpW) && (yd + height * textsize < _vpY || yd >= _vpH)) return width * textsize ;

    int32_t w = width;
    int32_t pX      = 0;
    int32_t pY      = y;
    uint8_t line = 0;
    bool clip = xd < _vpX || xd + width  * textsize >= _vpW || yd < _vpY || yd + height * textsize >= _vpH;

  #ifdef LOAD_FONT2 // chop out code if we do not need it
    if (font == 2) {
      w = w + 6; // Should be + 7 but we need to compensate for width increment
      w = w / 8;

      if (color == bgcolor || textsize != 1 || clip) {
        //begin_tft_write();          // Sprite class can use this function, avoiding begin_tft_write()
//        inTransaction = true;

        for (int32_t i = 0; i < height; i++) {
          if (color != bgcolor)
            m_tft.fillRect(x, pY, width * textsize, textsize, bgcolor);

          for (int32_t k = 0; k < w; k++) {
            line = pgm_read_byte((uint8_t *)flash_address + w * i + k);
            if (line) {
              if (textsize == 1) {
                pX = x + k * 8;
                if (line & 0x80) m_tft.drawPixel(pX,     pY, color);
                if (line & 0x40) m_tft.drawPixel(pX + 1, pY, color);
                if (line & 0x20) m_tft.drawPixel(pX + 2, pY, color);
                if (line & 0x10) m_tft.drawPixel(pX + 3, pY, color);
                if (line & 0x08) m_tft.drawPixel(pX + 4, pY, color);
                if (line & 0x04) m_tft.drawPixel(pX + 5, pY, color);
                if (line & 0x02) m_tft.drawPixel(pX + 6, pY, color);
                if (line & 0x01) m_tft.drawPixel(pX + 7, pY, color);
              }
              else {
                pX = x + k * 8 * textsize;
                if (line & 0x80) m_tft.fillRect(pX, pY, textsize, textsize, color);
                if (line & 0x40) m_tft.fillRect(pX + textsize, pY, textsize, textsize, color);
                if (line & 0x20) m_tft.fillRect(pX + 2 * textsize, pY, textsize, textsize, color);
                if (line & 0x10) m_tft.fillRect(pX + 3 * textsize, pY, textsize, textsize, color);
                if (line & 0x08) m_tft.fillRect(pX + 4 * textsize, pY, textsize, textsize, color);
                if (line & 0x04) m_tft.fillRect(pX + 5 * textsize, pY, textsize, textsize, color);
                if (line & 0x02) m_tft.fillRect(pX + 6 * textsize, pY, textsize, textsize, color);
                if (line & 0x01) m_tft.fillRect(pX + 7 * textsize, pY, textsize, textsize, color);
              }
            }
          }
          pY += textsize;
        }

//        inTransaction = lockTransaction;
        m_tft.end_tft_write();
      }
      else { // Faster drawing of characters and background using block write

        m_tft.begin_tft_write();

        m_tft.setWindow(xd, yd, xd + width - 1, yd + height - 1);

        uint8_t mask;
        for (int32_t i = 0; i < height; i++) {
          pX = width;
          for (int32_t k = 0; k < w; k++) {
            line = pgm_read_byte((uint8_t *) (flash_address + w * i + k) );
            mask = 0x80;
            while (mask && pX) {
              if (line & mask)
                m_tft.Write_16(color);
              else
                m_tft.Write_16(bgcolor);
              pX--;
              mask = mask >> 1;
            }
          }
          if (pX)
            m_tft.Write_16(bgcolor);
        }

        m_tft.end_tft_write();
      }
    }

    #ifdef LOAD_RLE
    else
    #endif
  #endif  //FONT2

  #ifdef LOAD_RLE  //674 bytes of code
    // Font is not 2 and hence is RLE encoded
    {
      m_tft.begin_tft_write();
//      inTransaction = true;

      w *= height; // Now w is total number of pixels in the character
      if (color == bgcolor && !clip) {

        int32_t px = 0, py = pY; // To hold character block start and end column and row values
        int32_t pc = 0; // Pixel count
        uint8_t np = textsize * textsize; // Number of pixels in a drawn pixel

        uint8_t tnp = 0; // Temporary copy of np for while loop
        uint8_t ts = textsize - 1; // Temporary copy of textsize
        // 16 bit pixel count so maximum font size is equivalent to 180x180 pixels in area
        // w is total number of pixels to plot to fill character block
        while (pc < w) {
          line = pgm_read_byte((uint8_t *)flash_address);
          flash_address++;
          if (line & 0x80) {
            line &= 0x7F;
            line++;
            if (ts) {
              px = xd + textsize * (pc % width); // Keep these px and py calculations outside the loop as they are slow
              py = yd + textsize * (pc / width);
            }
            else {
              px = xd + pc % width; // Keep these px and py calculations outside the loop as they are slow
              py = yd + pc / width;
            }
            while (line--) { // In this case the while(line--) is faster
              pc++; // This is faster than putting pc+=line before while()?
              m_tft.setWindow(px, py, px + ts, py + ts);

              if (ts) {
                tnp = np;
                while (tnp--)
                  m_tft.Write_16(color);
              }
              else
                m_tft.Write_16(color);
              px += textsize;

              if (px >= (xd + width * textsize)) {
                px = xd;
                py += textsize;
              }
            }
          }
          else {
            line++;
            pc += line;
          }
        }
      }
      else {
        // Text colour != background and textsize = 1 and character is within viewport area
        // so use faster drawing of characters and background using block write
        if (color != bgcolor && textsize == 1 && !clip)
        {
          m_tft.setWindow(xd, yd, xd + width - 1, yd + height - 1);

          // Maximum font size is equivalent to 180x180 pixels in area
          while (w > 0) {
            line = pgm_read_byte((uint8_t *)flash_address++); // 8 bytes smaller when incrementing here
            if (line & 0x80) {
              line &= 0x7F;
              line++; w -= line;
              m_tft.pushBlock(color,line);
            }
            else {
              line++; w -= line;
              m_tft.pushBlock(bgcolor,line);
            }
          }
        }
        else
        {
          int32_t px = 0, py = 0;  // To hold character pixel coords
          int32_t tx = 0, ty = 0;  // To hold character TFT pixel coords
          int32_t pc = 0;          // Pixel count
          int32_t pl = 0;          // Pixel line length
          uint16_t pcol = 0;       // Pixel color
          bool     pf = true;      // Flag for plotting
          while (pc < w) {
            line = pgm_read_byte((uint8_t *)flash_address);
            flash_address++;
            if (line & 0x80) { pcol = color; line &= 0x7F; pf = true;}
            else { pcol = bgcolor; if (color == bgcolor) pf = false;}
            line++;
            px = pc % width;
            tx = x + textsize * px;
            py = pc / width;
            ty = y + textsize * py;

            pl = 0;
            pc += line;
            while (line--) {
              pl++;
              if ((px+pl) >= width) {
                if (pf) m_tft.fillRect(tx, ty, pl * textsize, textsize, pcol);
                pl = 0;
                px = 0;
                tx = x;
                py ++;
                ty += textsize;
              }
            }
            if (pl && pf) m_tft.fillRect(tx, ty, pl * textsize, textsize, pcol);
          }
        }
      }
//      inTransaction = lockTransaction;
      m_tft.end_tft_write();
    }
    // End of RLE font rendering
  #endif

  #if !defined (LOAD_FONT2) && !defined (LOAD_RLE)
    // Stop warnings
    flash_address = flash_address;
    w = w;
    pX = pX;
    pY = pY;
    line = line;
    clip = clip;
  #endif

    return width * textsize;    // x +
}

/***************************************************************************************
** Function name:           drawString (with or without user defined font)
** Description :            draw string with padding if it is defined
***************************************************************************************/

// With font number. Note: font number is over-ridden if a smooth font is loaded
int16_t FontRenderer::drawString(std::string_view string, int32_t poX, int32_t poY, uint16_t color, uint16_t bgcolor, uint8_t font, uint8_t datum)
{
    textdatum = datum;
#ifdef CONFIG_TFT_IS_AUTOBAHN
    if (font == 4)
    {
        if (gfxFont != &din1451alt10pt8b)
            setFreeFont(&din1451alt10pt8b);
        font = 1;
    }
#endif

  int16_t sumX = 0;
  uint8_t padding = 1, baseline = 0;
  uint16_t cwidth = textWidth(string, font); // Find the pixel width of the string in the font
  uint16_t cheight = 8 * textsize;

#ifdef LOAD_GFXFF
  #ifdef CONFIG_TFT_SMOOTH_FONT
    bool freeFont = (font == 1 && gfxFont && !fontLoaded);
  #else
    bool freeFont = (font == 1 && gfxFont);
  #endif

  if (freeFont) {
    cheight = glyph_ab * textsize;
    poY += cheight; // Adjust for baseline datum of free fonts
    baseline = cheight;
    padding =101; // Different padding method used for Free Fonts

    // We need to make an adjustment for the bottom of the string (eg 'y' character)
    if ((textdatum == BL_DATUM) || (textdatum == BC_DATUM) || (textdatum == BR_DATUM)) {
      cheight += glyph_bb * textsize;
    }
  }
#endif


  // If it is not font 1 (GLCD or free font) get the baseline and pixel height of the font
#ifdef CONFIG_TFT_SMOOTH_FONT
  if(fontLoaded) {
    baseline = gFont.maxAscent;
    cheight  = fontHeight();
  }
  else
#endif
  if (font!=1) {
    baseline = pgm_read_byte( &fontdata[font].baseline ) * textsize;
    cheight = fontHeight(font);
  }

  if (textdatum) {

    switch(textdatum) {
      case TC_DATUM:
        poX -= cwidth/2;
        padding += 1;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding += 2;
        break;
      case ML_DATUM:
        poY -= cheight/2;
        //padding += 0;
        break;
      case MC_DATUM:
        poX -= cwidth/2;
        poY -= cheight/2;
        padding += 1;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight/2;
        padding += 2;
        break;
      case BL_DATUM:
        poY -= cheight;
        //padding += 0;
        break;
      case BC_DATUM:
        poX -= cwidth/2;
        poY -= cheight;
        padding += 1;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding += 2;
        break;
      case L_BASELINE:
        poY -= baseline;
        //padding += 0;
        break;
      case C_BASELINE:
        poX -= cwidth/2;
        poY -= baseline;
        padding += 1;
        break;
      case R_BASELINE:
        poX -= cwidth;
        poY -= baseline;
        padding += 2;
        break;
    }
  }


  int8_t xo = 0;
#ifdef LOAD_GFXFF
  if (freeFont && (color!=bgcolor)) {
      cheight = (glyph_ab + glyph_bb) * textsize;
      // Get the offset for the first character only to allow for negative offsets
      uint16_t c2 = 0;
      uint16_t len = string.size();
      uint16_t n = 0;

      while (n < len && c2 == 0) c2 = decodeUTF8((const uint8_t*)std::begin(string), &n, len - n);

      if((c2 >= pgm_read_word(&gfxFont->first)) && (c2 <= pgm_read_word(&gfxFont->last) )) {
        c2 -= pgm_read_word(&gfxFont->first);
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c2]);
        xo = pgm_read_byte(&glyph->xOffset) * textsize;
        // Adjust for negative xOffset
        if (xo > 0) xo = 0;
        else cwidth -= xo;
        // Add 1 pixel of padding all round
        //cheight +=2;
        //m_tft.fillRect(poX+xo-1, poY - 1 - glyph_ab * textsize, cwidth+2, cheight, textbgcolor);
        m_tft.fillRect(poX+xo, poY - glyph_ab * textsize, cwidth, cheight, bgcolor);
      }
      padding -=100;
    }
#endif

  uint16_t len = string.size();
  uint16_t n = 0;

#ifdef CONFIG_TFT_SMOOTH_FONT
  if(fontLoaded) {
    m_tft.setCursor(poX, poY);

    bool fillbg = _fillbg;
    // If padding is requested then fill the text background
    if (padX && !_fillbg) _fillbg = true;

    while (n < len) {
      uint16_t uniCode = decodeUTF8((const uint8_t*)std::begin(string), &n, len - n);
      drawGlyph(uniCode);
    }
    _fillbg = fillbg; // restore state
    sumX += cwidth;
    //fontFile.close();
  }
  else
#endif
  {
    while (n < len) {
      uint16_t uniCode = decodeUTF8((const uint8_t*)std::begin(string), &n, len - n);
      sumX += drawChar(uniCode, poX+sumX, poY, color, bgcolor, font);
      if (poX+sumX > m_tft.width())
        break;
    }
  }

  auto padX = 0;

//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv DEBUG vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Switch on debugging for the padding areas
//#define PADDING_DEBUG

#ifndef PADDING_DEBUG
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ DEBUG ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  if((0>cwidth) && (color!=bgcolor)) {
    int16_t padXc = poX+cwidth+xo;
#ifdef LOAD_GFXFF
    if (freeFont) {
      poX +=xo; // Adjust for negative offset start character
      poY -= glyph_ab * textsize;
      sumX += poX;
    }
#endif
    switch(padding) {
      case 1:
        m_tft.fillRect(padXc,poY,padX-cwidth,cheight, bgcolor);
        break;
      case 2:
        m_tft.fillRect(padXc,poY,(padX-cwidth)>>1,cheight, bgcolor);
        padXc = poX - ((padX-cwidth)>>1);
        m_tft.fillRect(padXc,poY,(padX-cwidth)>>1,cheight, bgcolor);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        m_tft.fillRect(poX + cwidth - padXc,poY,padXc-cwidth,cheight, bgcolor);
        break;
    }
  }


#else

//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv DEBUG vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// This is debug code to show text (green box) and blanked (white box) areas
// It shows that the padding areas are being correctly sized and positioned

  if((padX>sumX) && (textcolor!=textbgcolor)) {
    int16_t padXc = poX+sumX; // Maximum left side padding
#ifdef LOAD_GFXFF
    if ((font == 1) && (gfxFont)) poY -= glyph_ab;
#endif
    drawRect(poX,poY,sumX,cheight, TFT_GREEN);
    switch(padding) {
      case 1:
        drawRect(padXc,poY,padX-sumX,cheight, TFT_WHITE);
        break;
      case 2:
        drawRect(padXc,poY,(padX-sumX)>>1, cheight, TFT_WHITE);
        padXc = (padX-sumX)>>1;
        drawRect(poX - padXc,poY,(padX-sumX)>>1,cheight, TFT_WHITE);
        break;
      case 3:
        if (padXc>padX) padXc = padX;
        drawRect(poX + sumX - padXc,poY,padXc-sumX,cheight, TFT_WHITE);
        break;
    }
  }
#endif
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ DEBUG ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

textdatum = 0;
return sumX;
}

/***************************************************************************************
** Function name:           drawCentreString (deprecated, use setTextDatum())
** Descriptions:            draw string centred on dX
***************************************************************************************/
int16_t FontRenderer::drawCentreString(std::string_view string, int32_t dX, int32_t poY, uint16_t color, uint16_t bgcolor, uint8_t font)
{
  uint8_t tempdatum = textdatum;
  int32_t sumX = 0;
  sumX = drawString(string, dX, poY, color, bgcolor, font, TC_DATUM);
  textdatum = tempdatum;
  return sumX;
}


/***************************************************************************************
** Function name:           drawRightString (deprecated, use setTextDatum())
** Descriptions:            draw string right justified to dX
***************************************************************************************/
int16_t FontRenderer::drawRightString(std::string_view string, int32_t dX, int32_t poY, uint16_t color, uint16_t bgcolor, uint8_t font)
{
  uint8_t tempdatum = textdatum;
  int16_t sumX = 0;
  sumX = drawString(string, dX, poY, color, bgcolor, font, TR_DATUM);
  textdatum = tempdatum;
  return sumX;
}

/***************************************************************************************
** Function name:           setTextSize
** Description:             Set the text size multiplier
***************************************************************************************/
void FontRenderer::setTextSize(uint8_t s)
{
  if (s>7) s = 7; // Limit the maximum size multiplier so byte variables can be used for rendering
  textsize = (s > 0) ? s : 1; // Don't allow font size 0
}

/***************************************************************************************
** Function name:           setTextWrap
** Description:             Define if text should wrap at end of line
***************************************************************************************/
void FontRenderer::setTextWrap(bool wrapX, bool wrapY)
{
  textwrapX = wrapX;
  textwrapY = wrapY;
}

/***************************************************************************************
** Function name:           setTextDatum
** Description:             Set the text position reference datum
***************************************************************************************/
void FontRenderer::setTextDatum(uint8_t d)
{
  textdatum = d;
}

/***************************************************************************************
** Function name:           getTextDatum
** Description:             Return the text datum value (as used by setTextDatum())
***************************************************************************************/
uint8_t FontRenderer::getTextDatum(void)
{
  return textdatum;
}

/***************************************************************************************
** Function name:           setFreeFont
** Descriptions:            Sets the GFX free font to use
***************************************************************************************/

#ifdef LOAD_GFXFF

void FontRenderer::setFreeFont(const GFXfont *f)
{
  if (f == nullptr) { // Fix issue #400 (ESP32 crash)
    // TODO FIX THIS AGAIN
    //setTextFont(1); // Use GLCD font
    return;
  }

//  textfont = 1;
  gfxFont = (GFXfont *)f;

  glyph_ab = 0;
  glyph_bb = 0;
  uint16_t numChars = pgm_read_word(&gfxFont->last) - pgm_read_word(&gfxFont->first);

  // Find the biggest above and below baseline offsets
  for (uint16_t c = 0; c < numChars; c++) {
    GFXglyph *glyph1  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[c]);
    int8_t ab = -pgm_read_byte(&glyph1->yOffset);
    if (ab > glyph_ab) glyph_ab = ab;
    int8_t bb = pgm_read_byte(&glyph1->height) - ab;
    if (bb > glyph_bb) glyph_bb = bb;
  }
}

#endif

/***************************************************************************************
** Function name:           textWidth
** Description:             Return the width in pixels of a string in a given font
***************************************************************************************/

int16_t FontRenderer::textWidth(std::string_view string, uint8_t font)
{
  int32_t str_width = 0;
  uint16_t uniCode  = 0;

#ifdef CONFIG_TFT_IS_AUTOBAHN
  if (font == 1 || gfxFont != &din1451alt10pt8b || font == 4)
  {
      if (font == 4)
      {
          if (gfxFont != &din1451alt10pt8b)
          {
              setFreeFont(&din1451alt10pt8b);
          }
          font = 1;
      }
      str_width = 1;
  }
#endif

  auto iter = std::begin(string);

#ifdef CONFIG_TFT_SMOOTH_FONT
  if(fontLoaded) {
    while (iter != std::end(string)) {
      uniCode = decodeUTF8(*iter++);
      if (uniCode) {
        if (uniCode == 0x20) str_width += gFont.spaceWidth;
        else {
          uint16_t gNum = 0;
          bool found = getUnicodeIndex(uniCode, &gNum);
          if (found) {
            if(str_width == 0 && gdX[gNum] < 0) str_width -= gdX[gNum];
            if (iter != std::end(string) || isDigits) str_width += gxAdvance[gNum];
            else str_width += (gdX[gNum] + gWidth[gNum]);
          }
          else str_width += gFont.spaceWidth + 1;
        }
      }
    }
    isDigits = false;
    return str_width;
  }
#endif

  if (font>1 && font<9) {
    char *widthtable = (char *)pgm_read_dword( &(fontdata[font].widthtbl ) ) - 32; //subtract the 32 outside the loop

    while (iter != std::end(string)) {
      uniCode = *iter++;
      if (uniCode > 31 && uniCode < 128)
      str_width += pgm_read_byte( widthtable + uniCode); // Normally we need to subtract 32 from uniCode
      else str_width += pgm_read_byte( widthtable + 32); // Set illegal character = space width
    }

  }
  else {

#ifdef LOAD_GFXFF
    if(gfxFont)
    {
      // New font
      while (*iter)
      {
        uniCode = decodeUTF8(*iter++);
        if ((uniCode >= pgm_read_word(&gfxFont->first)) && (uniCode <= pgm_read_word(&gfxFont->last)))
        {
          uniCode -= pgm_read_word(&gfxFont->first);
          GFXglyph *glyph  = &(((GFXglyph *)pgm_read_dword(&gfxFont->glyph))[uniCode]);
          // If this is not the  last character or is a digit then use xAdvance
          if (iter != std::end(string) || isDigits)
            str_width += pgm_read_byte(&glyph->xAdvance);
          // Else use the offset plus width since this can be bigger than xAdvance
          else
            str_width += ((int8_t)pgm_read_byte(&glyph->xOffset) + pgm_read_byte(&glyph->width));
        }
      }
    }
    else
#endif
    {
#ifdef LOAD_GLCD
      while (*iter++) str_width += 6;
#endif
    }
  }
  isDigits = false;
  return str_width * textsize;
}

/***************************************************************************************
** Function name:           fontHeight
** Description:             return the height of a font (yAdvance for free fonts)
***************************************************************************************/
int16_t FontRenderer::fontHeight(int16_t font)
{
#ifdef CONFIG_TFT_IS_AUTOBAHN
    if (font == 4)
    {
        if (gfxFont != &din1451alt10pt8b)
            setFreeFont(&din1451alt10pt8b);
        font = 1;
    }
#endif

#ifdef CONFIG_TFT_SMOOTH_FONT
  if(fontLoaded) return gFont.yAdvance;
#endif

#ifdef LOAD_GFXFF
  if (font == 1)
    if (gfxFont) // New font
      return pgm_read_byte(&gfxFont->yAdvance) * textsize;

#endif
  return pgm_read_byte( &fontdata[font].height ) * textsize;
}

/***************************************************************************************
** Function name:           decodeUTF8
** Description:             Serial UTF-8 decoder with fall-back to extended ASCII
*************************************************************************************x*/
uint16_t FontRenderer::decodeUTF8(uint8_t c)
{
  if (!_utf8) return c;

  // 7 bit Unicode Code Point
  if ((c & 0x80) == 0x00) {
    decoderState = 0;
    return c;
  }

  if (decoderState == 0) {
    // 11 bit Unicode Code Point
    if ((c & 0xE0) == 0xC0) {
      decoderBuffer = ((c & 0x1F)<<6);
      decoderState = 1;
      return 0;
    }
    // 16 bit Unicode Code Point
    if ((c & 0xF0) == 0xE0) {
      decoderBuffer = ((c & 0x0F)<<12);
      decoderState = 2;
      return 0;
    }
    // 21 bit Unicode  Code Point not supported so fall-back to extended ASCII
    // if ((c & 0xF8) == 0xF0) return c;
  }
  else {
    if (decoderState == 2) {
      decoderBuffer |= ((c & 0x3F)<<6);
      decoderState--;
      return 0;
    }
    else {
      decoderBuffer |= (c & 0x3F);
      decoderState = 0;
      return decoderBuffer;
    }
  }

  decoderState = 0;

  return c; // fall-back to extended ASCII
}

/***************************************************************************************
** Function name:           decodeUTF8
** Description:             Line buffer UTF-8 decoder with fall-back to extended ASCII
*************************************************************************************x*/
uint16_t FontRenderer::decodeUTF8(const uint8_t *buf, uint16_t *index, uint16_t remaining)
{
  uint16_t c = buf[(*index)++];
  //Serial.print("Byte from string = 0x"); Serial.println(c, HEX);

  if (!_utf8) return c;

  // 7 bit Unicode
  if ((c & 0x80) == 0x00) return c;

  // 11 bit Unicode
  if (((c & 0xE0) == 0xC0) && (remaining > 1))
    return ((c & 0x1F)<<6) | (buf[(*index)++]&0x3F);

  // 16 bit Unicode
  if (((c & 0xF0) == 0xE0) && (remaining > 2)) {
    c = ((c & 0x0F)<<12) | ((buf[(*index)++]&0x3F)<<6);
    return  c | ((buf[(*index)++]&0x3F));
  }

  // 21 bit Unicode not supported so fall-back to extended ASCII
  // if ((c & 0xF8) == 0xF0) return c;

  return c; // fall-back to extended ASCII
}

} // namespace espgui
