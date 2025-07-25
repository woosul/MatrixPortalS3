/*
*
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

#ifndef PatternIncrementalDrift_H
#define PatternIncrementalDrift_H

#include "EffectsLayer.hpp" // Ensure EffectsLayer is included for Drawable, Boid, PVector, and effects object

class PatternIncrementalDrift : public Drawable {
  public:
    PatternIncrementalDrift() {
      name = (char *)"Incremental Drift";
    }

    unsigned int drawFrame() {
      //uint8_t dim = beatsin8(2, 230, 250);
      effects.DimAll(250); 

      for (int i = 2; i <= VPANEL_W / 2; i++)
      {
        CRGB color = effects.ColorFromCurrentPalette((i - 2) * (240 / (VPANEL_W / 2)));

        uint8_t x = effects.beatcos8((17 - i) * 2, effects.getCenterX() - i, effects.getCenterX() + i);
        uint8_t y = beatsin8((17 - i) * 2, effects.getCenterY() - i, effects.getCenterY() + i);

        effects.leds[effects.XY16(x,y)] = color; // Use effects.leds for direct CRGB assignment
      }

       effects.ShowFrame();

      return 0;
    }
};

#endif
