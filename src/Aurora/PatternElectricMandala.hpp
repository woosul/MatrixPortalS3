/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from "Funky Noise" by Stefan Petrick: https://github.com/StefanPetrick/FunkyNoise
 * Copyright (c) 2014 Stefan Petrick
 * http://www.stefan-petrick.de/wordpress_beta
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

#ifndef PatternElectricMandala_H
#define PatternElectricMandala_H // Ensure header guard is present and correct
#include "EffectsLayer.hpp" // Ensure EffectsLayer is included for Drawable and effects object

class PatternElectricMandala : public Drawable {
  private:

    // The coordinates for 16-bit noise spaces.
#define NUM_LAYERS 1

    // used for the random based animations
    int16_t dx;
    int16_t dy;
    int16_t dz;
    int16_t dsx;
    int16_t dsy;


    unsigned int last_parameter_change_ms = 0;

  public:
    PatternElectricMandala() {
      name = (char *)"ElectricMandala";
    }

    void start() {
      // set to reasonable values to avoid a black out
      effects.noisesmoothing = 200;

      // just any free input pin
      //random16_add_entropy(analogRead(18));

      // fill coordinates with random values
      // set zoom levels
      effects.noise_x = random16();
      effects.noise_y = random16();
      effects.noise_z = random16();
      effects.noise_scale_x = 6000;
      effects.noise_scale_y = 6000;

      // for the random movement
      dx = random8();
      dy = random8();
      dz = random8();
      dsx = random8();
      dsy = random8();
    }

    unsigned int drawFrame() {
#if FASTLED_VERSION >= 3001000
      // a new parameter set every 15 seconds
      if(millis() - last_parameter_change_ms > 15000) {
        last_parameter_change_ms = millis();
        //SetupRandomPalette3();
        dy = random16(500) - 250; // random16(2000) - 1000 is pretty fast but works fine, too
        dx = random16(500) - 250;
        dz = random16(500) - 250;
        effects.noise_scale_x = random16(10000) + 2000;
        effects.noise_scale_y = random16(10000) + 2000;
      }
#endif

      effects.noise_y += dy;
      effects.noise_x += dx;
      effects.noise_z += dz;

      effects.FillNoise();
      ShowNoiseLayer(0, 1, 0);

      effects.Caleidoscope3();
      effects.Caleidoscope1();

      effects.ShowFrame();

      return 30;
    }

    // show just one layer
    void ShowNoiseLayer(byte layer, byte colorrepeat, byte colorshift) {
      for (uint16_t i = 0; i < VPANEL_W; i++) {
        for (uint16_t j = 0; j < VPANEL_H; j++) {

          uint8_t color = effects.noise[i][j];

          uint8_t bri = color;

          // assign a color depending on the actual palette using effects object
          CRGB pixel = ColorFromPalette(effects.currentPalette, colorrepeat * (color + colorshift), bri);

          effects.leds[effects.XY16(i, j)] = pixel;
        }
      }
    }
};

#endif
