/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef PatternRadar_H
#define PatternRadar_H // Ensure header guard is present and correct
#include "EffectsLayer.hpp" // Ensure EffectsLayer is included for Drawable and effects object

class PatternRadar : public Drawable {
  private:
    byte theta = 0;
    byte hueoffset = 0;
    unsigned long last_update_hue_ms = 0;

  public:
    PatternRadar() {
      name = (char *)"Radar";
    }

    unsigned int drawFrame() {
      effects.DimAll(254); effects.ShowFrame();

      for (int offset = 0; offset < effects.getCenterX(); offset++) {
        byte hue = 255 - (offset * 16 + hueoffset);
        CRGB color = effects.ColorFromCurrentPalette(hue);
        uint8_t x = effects.mapcos8(theta, offset, (VPANEL_W - 1) - offset);
        uint8_t y = effects.mapsin8(theta, offset, (VPANEL_H - 1) - offset);
        uint16_t xy = effects.XY16(x, y);
        effects.leds[xy] = color;

        if (millis() - last_update_hue_ms > 25) {
          last_update_hue_ms = millis();
          theta += 2;
          hueoffset += 1;
        }
      }

      return 0;
    }
};

#endif
