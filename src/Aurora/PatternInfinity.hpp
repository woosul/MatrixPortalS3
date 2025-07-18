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

#ifndef PatternInfinity_H
#define PatternInfinity_H
#include "EffectsLayer.hpp"


class PatternInfinity : public Drawable {
public:
    PatternInfinity() {
        name = (char *)"Infinity";
    }

    void start() {
        effects.ClearFrame();
    }

    unsigned int drawFrame() {
        // dim all pixels on the display slightly 
        // to 250/255 (98%) of their current brightness
      //blur2d(effects.leds, VPANEL_W > 255 ? 255 : VPANEL_W, VPANEL_H > 255 ? 255 : VPANEL_H, 250);
        //        effects.DimAll(250); effects.ShowFrame();


        // the EffectsLayer class has some sample oscillators
        // that move from 0 to 255 at different speeds
        effects.MoveOscillators();

        // the horizontal position of the head of the infinity sign
        // oscillates from 0 to the maximum horizontal and back
        int x = (VPANEL_W - 4) - effects.p[1];

        // the vertical position of the head oscillates
        // from 8 to 23 and back (hard-coded for a 32x32 matrix)
        int y = map8(sin8(effects.osci[3]), 8, VPANEL_H - 8);

        // the hue oscillates from 0 to 255, overflowing back to 0
        byte hue = sin8(effects.osci[5]);

        // draw a pixel at x,y using a color from the current palette
        CRGB color = effects.ColorFromCurrentPalette(hue);
        // Use effects.drawTriangle to draw on the EffectsLayer's buffer (effects.leds)
        effects.drawTriangle(x, y, x+1, y+1, x+2, y+2, effects.virtualDisp->color565(color.r, color.g, color.b));
        ////effects.setPixelFromPaletteIndex(x, y, hue);

        effects.ShowFrame();
        return 30;
    }
};

#endif
