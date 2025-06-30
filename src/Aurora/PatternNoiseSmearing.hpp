/*
* Aurora: https://github.com/pixelmatix/aurora
* Copyright (c) 2014 Jason Coon
*
* Portions of this code are adapted from "Noise Smearing" by Stefan Petrick: https://gist.githubusercontent.com/embedded-creations/5cd47d83cb0e04f4574d/raw/ebf6a82b4755d55cfba3bf6598f7b19047f89daf/NoiseSmearing.ino
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

#ifndef PatternNoiseSmearing_H
#define PatternNoiseSmearing_H

#include "EffectsLayer.hpp" // Drawable.h는 EffectsLayer.hpp를 통해 포함됨

// byte patternNoiseSmearingHue = 0; // 전역 변수 대신 각 클래스 내부에서 hue 관리

class PatternMultipleStream : public Drawable {
public:
  PatternMultipleStream() {
    name = (char *)"MultipleStream";
  }

  unsigned int drawFrame() {
    static unsigned long counter = 0;
    uint8_t currentHue = effects.osci[0]; // effects.osci 또는 다른 방법으로 hue 값 가져오기

#if 0
    counter++;
#else
    counter = millis() / 10;
#endif

    byte x1 = 4 + effects.mapsin8(counter * 2, 0, MATRIX_WIDTH -1); // sin8 대신 effects.mapsin8 사용 및 범위 수정
    byte x2 = 8 + effects.mapsin8(counter * 2, 0, MATRIX_HEIGHT -1); // sin8 대신 effects.mapsin8 사용 및 범위 수정
    byte y2 = 8 + effects.mapcos8((counter * 2) / 3, 0, MATRIX_HEIGHT -1); // cos8 대신 effects.mapcos8 사용 및 범위 수정

    effects.leds[effects.XY16(x1, x2)] = effects.ColorFromCurrentPalette(currentHue);
    effects.leds[effects.XY16(x2, y2)] = effects.ColorFromCurrentPalette(currentHue + 128);

    // Noise
    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(8);
    // effects.MoveFractionalNoiseX(); // EffectsLayer에 해당 함수가 있는지 확인 필요, 없다면 주석 처리 또는 구현

    effects.MoveY(8);
    // effects.MoveFractionalNoiseY(); // EffectsLayer에 해당 함수가 있는지 확인 필요, 없다면 주석 처리 또는 구현

    // patternNoiseSmearingHue++; // currentHue 사용
    effects.ShowFrame();
    return 0;
  }
};

class PatternMultipleStream2 : public Drawable {
public:
  PatternMultipleStream2() {
    name = (char *)"MultipleStream2";
  }

  unsigned int drawFrame() {
    effects.DimAll(230); // ShowFrame은 drawFrame의 마지막에 한번만 호출하는 것이 좋음

    uint8_t currentHue = effects.osci[1];

    byte xx = 4 + effects.mapsin8(millis() / 9, 0, MATRIX_WIDTH -1);
    byte yy = 4 + effects.mapcos8(millis() / 10, 0, MATRIX_HEIGHT -1);
    effects.leds[effects.XY16(xx, yy)] += effects.ColorFromCurrentPalette(currentHue);

    xx = 8 + effects.mapsin8(millis() / 10, 0, MATRIX_WIDTH -1);
    yy = 8 + effects.mapcos8(millis() / 7, 0, MATRIX_HEIGHT -1);
    effects.leds[effects.XY16(xx, yy)] += effects.ColorFromCurrentPalette(currentHue + 80);

    effects.leds[effects.XY16(MATRIX_WIDTH -1, MATRIX_HEIGHT -1)] += effects.ColorFromCurrentPalette(currentHue + 160); // 중앙 대신 우하단으로 변경 (15,15는 32x32 기준)

    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_z += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(3);
    // effects.MoveFractionalNoiseY(4);

    effects.MoveY(3);
    // effects.MoveFractionalNoiseX(4);

    // patternNoiseSmearingHue++;
    effects.ShowFrame();
    return 0;
  }
};

class PatternMultipleStream3 : public Drawable {
public:
  PatternMultipleStream3() {
    name = (char *)"MultipleStream3";
  }

  unsigned int drawFrame() {
    effects.DimAll(235);

    for (uint8_t i = 3; i < MATRIX_WIDTH; i = i + 4) { // 32 대신 MATRIX_WIDTH 사용
      effects.leds[effects.XY16(i, MATRIX_HEIGHT -1)] += effects.ColorFromCurrentPalette(i * 8); // 15 대신 MATRIX_HEIGHT -1 사용
    }

    // Noise
    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_z += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(3);
    // effects.MoveFractionalNoiseY(4);

    effects.MoveY(3);
    // effects.MoveFractionalNoiseX(4);

    effects.ShowFrame();
    return 1;
  }
};

class PatternMultipleStream4 : public Drawable {
public:
  PatternMultipleStream4() {
    name = (char *)"MultipleStream4";
  }

  unsigned int drawFrame() {
    effects.DimAll(235);
    uint8_t currentHue = effects.osci[2];

    effects.leds[effects.XY16(MATRIX_WIDTH / 2, MATRIX_HEIGHT / 2)] += effects.ColorFromCurrentPalette(currentHue); // 중앙점 (15,15 대신)

    // Noise
    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(8);
    // effects.MoveFractionalNoiseX();

    effects.MoveY(8);
    // effects.MoveFractionalNoiseY();

    // patternNoiseSmearingHue++;
    effects.ShowFrame();
    return 0;
  }
};

class PatternMultipleStream5 : public Drawable {
public:
  PatternMultipleStream5() {
    name = (char *)"MultipleStream5";
  }

  unsigned int drawFrame() {
    effects.DimAll(235);

    for (uint8_t i = 3; i < MATRIX_WIDTH; i = i + 4) { // 32 대신 MATRIX_WIDTH
      effects.leds[effects.XY16(i, MATRIX_HEIGHT -1)] += effects.ColorFromCurrentPalette(i * 8); // 31 대신 MATRIX_HEIGHT -1
    }

    // Noise
    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_z += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(3);
    // effects.MoveFractionalNoiseY(4);

    effects.MoveY(4);
    // effects.MoveFractionalNoiseX(4);
    effects.ShowFrame();
    return 0;
  }
};

class PatternMultipleStream8 : public Drawable {
public:
  PatternMultipleStream8() {
    name = (char *)"MultipleStream8";
  }

  unsigned int drawFrame() {
    effects.DimAll(230);

    // draw grid of rainbow dots on top of the dimmed image
    for (uint8_t y = 1; y < MATRIX_HEIGHT; y = y + 6) { // 32 대신 MATRIX_HEIGHT
      for (uint8_t x = 1; x < MATRIX_WIDTH; x = x + 6) { // 32 대신 MATRIX_WIDTH
        effects.leds[effects.XY16(x, y)] += effects.ColorFromCurrentPalette((x * y) / 4);
      }
    }

    // Noise
    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_z += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(3);
    // effects.MoveFractionalNoiseX(4);

    effects.MoveY(3);
    // effects.MoveFractionalNoiseY(4);
    effects.ShowFrame();
    return 0;
  }
};

class PatternPaletteSmear : public Drawable {
public:
  PatternPaletteSmear() {
    name = (char *)"PaletteSmear";
  }

  // PatternPaletteSmear 클래스 내에 start() 메서드 추가 (선택적이지만 좋은 습관)
  void start() override {
    Drawable::start();
    // effects.ClearFrame(); // 필요하다면 effects.leds 버퍼를 여기서 초기화
  }

  unsigned int drawFrame() {
    static uint8_t time_offset = 0; // For base hue animation

    // 1. Update noise field - this should make the pattern irregular
    effects.noise_x += 700; // Adjust speed of noise evolution
    effects.noise_y += 700;
    // effects.noise_z += 700; // if using 3D noise for more variation
    effects.noise_scale_x = 3000; // Adjust scale for noise granularity
    effects.noise_scale_y = 3000;
    effects.FillNoise(); // Populates effects.noise[x][y]

    // 2. Dim the existing buffer to create trails (smearing effect).
    // With direct assignment below, DimAll controls the fade/trail length.
    effects.DimAll(180); // Try a more noticeable dimming for smearing
   
    // 3. Draw the new "paint" layer, modulated by noise, overwriting pixels.
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) { // VPANEL_H -> MATRIX_HEIGHT
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) { // VPANEL_W -> MATRIX_WIDTH
        // Base hue from x-coordinate and time_offset
        uint8_t base_hue = ( (uint16_t)x * 128 / (MATRIX_WIDTH - 1) + time_offset) % 255; // Max 128 from x to leave room for noise
        
        // Modulate hue with noise from effects.noise[x][y] (0-255)
        uint8_t hue_perturbation = effects.noise[x][y] / 2; // Noise contributes up to 127 to hue
        uint8_t final_hue = (base_hue + hue_perturbation) % 255;
        
        // Y-axis brightness variation: dimmer at top (y=0), brighter at bottom
        uint8_t paint_brightness = map(y, 0, MATRIX_HEIGHT - 1, 40, 240); // Ensure a visible gradient

        CRGB new_paint = effects.ColorFromCurrentPalette(final_hue, paint_brightness);
        effects.leds[effects.XY16(x, y)] = new_paint; // Use assignment '='
      }
    }
    time_offset += 1; // Slower base hue shift to let noise be more visible
  
    // 4. Shift the entire buffer for the "smearing" movement.
    // Adjust internal X movement to be consistent with the global MATRIX_X_OFFSET.
    // If MATRIX_X_OFFSET is 1, MoveX(0) will prevent the internal left shift,
    // allowing the global offset in ShowFrame() to apply consistently.
    effects.MoveX(1 - MATRIX_X_OFFSET); // If MATRIX_X_OFFSET = 1, this becomes MoveX(0)
    effects.MoveY(1); // Assuming Y offset consistency is not an issue or MATRIX_Y_OFFSET is 0

    // 5. Show the frame.
    effects.ShowFrame();

    return 0; // Rely on ANIMATION_FPS for frame rate control
  }
};

class PatternRainbowFlag : public Drawable {
public:
  PatternRainbowFlag() {
    name = (char *)"RainbowFlag";
  }

  unsigned int drawFrame() {
    effects.DimAll(10);

    CRGB rainbow[7] = { // CRGB는 FastLED.h에 정의되어 있음
      CRGB::Red,
      CRGB::Orange,
      CRGB::Yellow,
      CRGB::Green,
      CRGB::Blue,
      CRGB::Violet,
      CRGB::Indigo // 7번째 색 추가 (선택적)
    };

    uint8_t band_height = MATRIX_HEIGHT / 6; // 각 색상 밴드의 높이
    if (band_height == 0) band_height = 1; // 최소 높이 1

    uint8_t current_y = 0;

    for (uint8_t c = 0; c < 6; c++) { // 6가지 기본 무지개 색상
      for (uint8_t j = 0; j < band_height; j++) {
        if (current_y >= MATRIX_HEIGHT) break;
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) { // VPANEL_W -> MATRIX_WIDTH
          effects.leds[effects.XY16(x, current_y)] += rainbow[c];
        }
        current_y++;
      }
      if (current_y >= MATRIX_HEIGHT) break;
    }

    // Noise
    effects.noise_x += 1000;
    effects.noise_y += 1000;
    effects.noise_scale_x = 4000;
    effects.noise_scale_y = 4000;
    effects.FillNoise();

    effects.MoveX(3);
    // effects.MoveFractionalNoiseY(4);

    effects.MoveY(3);
    // effects.MoveFractionalNoiseX(4);
    effects.ShowFrame();
    return 0;
  }
};
#endif
