/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from "Funky Clouds" by Stefan Petrick: https://gist.github.com/anonymous/876f908333cd95315c35
 * Portions of this code are adapted from "NoiseSmearing" by Stefan Petrick: https://gist.github.com/StefanPetrick/9ee2f677dbff64e3ba7a
 * Copyright (c) 2014 Stefan Petrick
 * http://www.stefan-petrick.de/wordpress_beta
 * 
 * Modified by Codetastic 2024
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

#ifndef Effects_H
#define Effects_H

// Adafruit GFX 라이브러리 (GFX 기본 클래스)
#include <Adafruit_GFX.h>
// MatrixPanel 라이브러리 (실제 매트릭스 드라이버)
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// FastLED는 GFX보다 나중에 포함될 수 있음 (의존성 순서 고려)
#include <FastLED.h>
// Aurora 패턴의 기본 클래스
#include "Drawable.h" // Drawable.h가 EffectsLayer.hpp와 같은 src/Aurora 폴더에 있다고 가정

class EffectsLayer; // extern 선언을 위한 전방 선언
extern EffectsLayer effects; // 전역 effects 객체 (mode_animation.cpp에 정의됨)

class EffectsLayer : public Adafruit_GFX {

public:

  uint32_t noise_x;
  uint32_t noise_y;
  uint32_t noise_z;
  uint32_t noise_scale_x;
  uint32_t noise_scale_y;

  uint8_t **noise = nullptr;  // we will allocate mem later
  uint8_t noisesmoothing;


  CRGB *leds;
  int width;
  int height;
  MatrixPanel_I2S_DMA *virtualDisp = nullptr; // m_matrix를 가리킬 포인터 멤버 추가
  int num_leds = 0;

  // 멤버 함수로 변경, 인스턴스의 width, height 사용
  uint16_t XY16(uint16_t x_coord, uint16_t y_coord) const {
      if (x_coord >= width) return 0; // 범위 초과 시 0 반환 (또는 다른 오류 처리)
      if (y_coord >= height) return 0;
      return (y_coord * width) + x_coord;
  }
  uint16_t XY(uint16_t x_coord, uint16_t y_coord) const {
    return XY16(x_coord, y_coord);
  }

  EffectsLayer(int w, int h) : Adafruit_GFX(w, h), width(w), height(h) {

    // we do dynamic allocation for leds buffer, otherwise esp32 toolchain can't link static arrays of such a big size for 256+ matrices
    leds = (CRGB *)malloc((width * height + 1) * sizeof(CRGB));
    num_leds = width * height;

    // allocate mem for noise effect
    // (there should be some guards for malloc errors eventually)
    noise = (uint8_t **)malloc(width * sizeof(uint8_t *));
    for (int i = 0; i < width; ++i) {
      noise[i] = (uint8_t *)malloc(height * sizeof(uint8_t));
    }

    // Set starting palette
    currentPalette = RainbowColors_p;
    loadPalette(0);
    NoiseVariablesSetup();    

    ClearFrame();
  }

  ~EffectsLayer(){
    free(leds);
    for (int i = 0; i < width; ++i) {
      free(noise[i]);
    }
    free(noise);
  }

  /* The only 'framebuffer' we have is what is contained in the leds and leds2 variables.
   * We don't store what the color a particular pixel might be, other than when it's turned
   * into raw electrical signal output gobbly-gook (i.e. the DMA matrix buffer), but this * is not reversible.
   * 
   * As such, any time these effects want to write a pixel color, we first have to update
   * the leds or leds2 array, and THEN write it to the RGB panel. This enables us to 'look up' the array to see what a pixel color was previously, each drawFrame().
   */
  void setPixel(int16_t x, int16_t y, CRGB color)
  {
	  if (x >= 0 && x < width && y >= 0 && y < height) leds[XY16(x, y)] = color;
  }

  // write one pixel with the specified color from the current palette to coordinates
  void setPixelFromPaletteIndex(int x, int y, uint8_t colorIndex) {
    if (x >= 0 && x < width && y >= 0 && y < height) leds[XY16(x, y)] = ColorFromCurrentPalette(colorIndex);
  }
  
 void PrepareFrame() { }

  void ShowFrame() { // send to display
    currentPalette = targetPalette;
  
    if (!virtualDisp) return; // virtualDisp 포인터 유효성 검사

    for (int y=0; y<height; ++y){
          for (int x=0; x<width; ++x) { // Iterate through logical coordinates
          uint16_t _pixel = XY16(x,y);
          // Apply MATRIX_X_OFFSET when drawing to the physical display, and wrap around
          virtualDisp->drawPixelRGB888( (x + 1) % width, y, leds[_pixel].r, leds[_pixel].g, leds[_pixel].b);
        } // end loop to copy fast led to the dma matrix
    }
  }

  uint16_t getCenterX() {
    return width / 2;
  }

  uint16_t getCenterY() {
    return height / 2;
  }


  // scale the brightness of the screenbuffer down
  void DimAll(byte value)  {
      for (int i = 0; i < num_leds; i++)
        leds[i].nscale8(value);
  }  

  void ClearFrame() {
      for (int i = 0; i < num_leds; i++)
        leds[i]= CRGB(0,0,0);

  }
  

  uint8_t beatcos8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0)
  {
    uint8_t beat = beat8(beats_per_minute, timebase);
    uint8_t beatcos = cos8(beat + phase_offset);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8(beatcos, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
  }

  uint8_t mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
    uint8_t beatsin = sin8(theta);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8(beatsin, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
  }

  uint8_t mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
    uint8_t beatcos = cos8(theta);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8(beatcos, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
  }

	 

  // palettes
  static const int paletteCount = 10;
  int paletteIndex = -1;
  TBlendType currentBlendType = LINEARBLEND;
  CRGBPalette16 currentPalette;
  CRGBPalette16 targetPalette;
  char* currentPaletteName;

  static const int HeatColorsPaletteIndex = 6;
  static const int RandomPaletteIndex = 9;


  void CyclePalette(int offset = 1) {
    loadPalette(paletteIndex + offset);
  }

  void RandomPalette() {
    loadPalette(RandomPaletteIndex);
  }

  void loadPalette(int index) {
    paletteIndex = index;

    if (paletteIndex >= paletteCount)
      paletteIndex = 0;
    else if (paletteIndex < 0)
      paletteIndex = paletteCount - 1;

    switch (paletteIndex) {
      case 0:
        targetPalette = RainbowColors_p;
        currentPaletteName = (char *)"Rainbow";
        break;
        //case 1:
        //  targetPalette = RainbowStripeColors_p;
        //  currentPaletteName = (char *)"RainbowStripe";
        //  break;
      case 1:
        targetPalette = OceanColors_p;
        currentPaletteName = (char *)"Ocean";
        break;
      case 2:
        targetPalette = CloudColors_p;
        currentPaletteName = (char *)"Cloud";
        break;
      case 3:
        targetPalette = ForestColors_p;
        currentPaletteName = (char *)"Forest";
        break;
      case 4:
        targetPalette = PartyColors_p;
        currentPaletteName = (char *)"Party";
        break;
      case 5:
        setupGrayscalePalette();
        currentPaletteName = (char *)"Grey";
        break;
      case HeatColorsPaletteIndex:
        targetPalette = HeatColors_p;
        currentPaletteName = (char *)"Heat";
        break;
      case 7:
        targetPalette = LavaColors_p;
        currentPaletteName = (char *)"Lava";
        break;
      case 8:
        setupIcePalette();
        currentPaletteName = (char *)"Ice";
        break;
      case RandomPaletteIndex:
        loadPalette(random(0, paletteCount - 1));
        paletteIndex = RandomPaletteIndex;
        currentPaletteName = (char *)"Random";
        break;
    }
  }

  void setPalette(String paletteName) {
    if (paletteName == "Rainbow")
      loadPalette(0);
    //else if (paletteName == "RainbowStripe")
    //  loadPalette(1);
    else if (paletteName == "Ocean")
      loadPalette(1);
    else if (paletteName == "Cloud")
      loadPalette(2);
    else if (paletteName == "Forest")
      loadPalette(3);
    else if (paletteName == "Party")
      loadPalette(4);
    else if (paletteName == "Grayscale")
      loadPalette(5);
    else if (paletteName == "Heat")
      loadPalette(6);
    else if (paletteName == "Lava")
      loadPalette(7);
    else if (paletteName == "Ice")
      loadPalette(8);
    else if (paletteName == "Random")
      RandomPalette();
  }

  void listPalettes() {
    Serial.println(F("{"));
    Serial.print(F("  \"count\": "));
    Serial.print(paletteCount);
    Serial.println(",");
    Serial.println(F("  \"results\": ["));

    String paletteNames [] = {
      "Rainbow",
      // "RainbowStripe",
      "Ocean",
      "Cloud",
      "Forest",
      "Party",
      "Grayscale",
      "Heat",
      "Lava",
      "Ice",
      "Random"
    };

    for (int i = 0; i < paletteCount; i++) {
      Serial.print(F("    \""));
      Serial.print(paletteNames[i]);
      if (i == paletteCount - 1)
        Serial.println(F("\""));
      else
        Serial.println(F("\","));
    }

    Serial.println("  ]");
    Serial.println("}");
  }

  void setupGrayscalePalette() {
    targetPalette = CRGBPalette16(CRGB::Black, CRGB::White);
  }

  void setupIcePalette() {
    targetPalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);
  }

  // Oscillators and Emitters

  // the oscillators: linear ramps 0-255
  byte osci[6];

  // sin8(osci) swinging between 0 to VPANEL_W - 1
  byte p[6];

  // set the speeds (and by that ratios) of the oscillators here
  void MoveOscillators() {
    osci[0] = osci[0] + 5;
    osci[1] = osci[1] + 2;
    osci[2] = osci[2] + 3;
    osci[3] = osci[3] + 4;
    osci[4] = osci[4] + 1;
    if (osci[4] % 2 == 0)
      osci[5] = osci[5] + 1; // .5
    for (int i = 0; i < 4; i++) {
      p[i] = map8(sin8(osci[i]), 0, width - 1); //why? to keep the result in the range of 0-VPANEL_W (matrix size)
    }
  }

 
  // All the caleidoscope functions work directly within the screenbuffer (leds array).
  // Draw whatever you like in the area x(0-15) and y (0-15) and then copy it arround.

  // rotates the first 16x16 quadrant 3 times onto a 32x32 (+90 degrees rotation for each one)
  void Caleidoscope1() {
    for (int x = 0; x < width / 2; x++) {
      for (int y = 0; y < height / 2; y++) {
        leds[XY16(width - 1 - x, y)] = leds[XY16(x, y)];
        leds[XY16(width - 1 - x, height - 1 - y)] = leds[XY16(x, y)];
        leds[XY16(x, height - 1 - y)] = leds[XY16(x, y)];
      }
    }
  }


  // mirror the first 16x16 quadrant 3 times onto a 32x32
  void Caleidoscope2() {
    for (int x = 0; x < width / 2; x++) {
      for (int y = 0; y < height / 2; y++) {
        leds[XY16(width - 1 - x, y)] = leds[XY16(y, x)];
        leds[XY16(x, height - 1 - y)] = leds[XY16(y, x)];
        leds[XY16(width - 1 - x, height - 1 - y)] = leds[XY16(x, y)];
      }
    }
  }

  // copy one diagonal triangle into the other one within a 16x16
  void Caleidoscope3() {
    for (int x = 0; x <= width / 2 - 1 && x < height; x++) {
      for (int y = 0; y <= x && y<height; y++) {
        leds[XY16(x, y)] = leds[XY16(y, x)];
      }
    }
  }

  // copy one diagonal triangle into the other one within a 16x16 (90 degrees rotated compared to Caleidoscope3)
  void Caleidoscope4() {
    for (int x = 0; x <= width / 2 - 1; x++) {
      for (int y = 0; y <= height / 2 - 1 - x; y++) {
        leds[XY16(height / 2 - 1 - y, width / 2 - 1 - x)] = leds[XY16(x, y)];
      }
    }
  }

  // copy one diagonal triangle into the other one within a 8x8
  void Caleidoscope5() {
    for (int x = 0; x < width / 4; x++) {
      for (int y = 0; y <= x && y<=height; y++) {
        leds[XY16(x, y)] = leds[XY16(y, x)];
      }
    }

    for (int x = width / 4; x < width / 2; x++) {
      for (int y = height / 4; y >= 0; y--) {
        leds[XY16(x, y)] = leds[XY16(y, x)];
      }
    }
  }

  void Caleidoscope6() {
    for (int x = 1; x < width / 2; x++) {
      leds[XY16(7 - x, 7)] = leds[XY16(x, 0)];
    } //a
    for (int x = 2; x < width / 2; x++) {
      leds[XY16(7 - x, 6)] = leds[XY16(x, 1)];
    } //b
    for (int x = 3; x < width / 2; x++) {
      leds[XY16(7 - x, 5)] = leds[XY16(x, 2)];
    } //c
    for (int x = 4; x < width / 2; x++) {
      leds[XY16(7 - x, 4)] = leds[XY16(x, 3)];
    } //d
    for (int x = 5; x < width / 2; x++) {
      leds[XY16(7 - x, 3)] = leds[XY16(x, 4)];
    } //e
    for (int x = 6; x < width / 2; x++) {
      leds[XY16(7 - x, 2)] = leds[XY16(x, 5)];
    } //f
    for (int x = 7; x < width / 2; x++) {
      leds[XY16(7 - x, 1)] = leds[XY16(x, 6)];
    } //g
  }

  // create a square twister to the left or counter-clockwise
  // x and y for center, r for radius
  void SpiralStream(int x, int y, int r, byte dimm) {
    for (int d = r; d >= 0; d--) { // from the outside to the inside
      for (int i = x - d; i <= x + d; i++) {
        if (i + 1 < width && i >=0 && (y-d) >=0 && (y-d) < height) // Bounds check
          leds[XY16(i, y - d)] += leds[XY16(i + 1, y - d)]; // lowest row to the right
        if (i >=0 && i < width && (y-d) >=0 && (y-d) < height) // Bounds check
          leds[XY16(i, y - d)].nscale8(dimm);
      }
      for (int i = y - d; i <= y + d; i++) {
        if ((x+d) < width && (x+d) >=0 && i+1 < height && i >=0) // Bounds check
          leds[XY16(x + d, i)] += leds[XY16(x + d, i + 1)]; // right column up
        if ((x+d) < width && (x+d) >=0 && i < height && i >=0) // Bounds check
          leds[XY16(x + d, i)].nscale8(dimm);
      }
      for (int i = x + d; i >= x - d; i--) {
         if (i-1 >=0 && i < width && (y+d) < height && (y+d) >=0 ) // Bounds check
          leds[XY16(i, y + d)] += leds[XY16(i - 1, y + d)]; // upper row to the left
        if (i >=0 && i < width && (y+d) < height && (y+d) >=0 ) // Bounds check
          leds[XY16(i, y + d)].nscale8(dimm);
      }
      for (int i = y + d; i >= y - d; i--) {
        if ((x-d) >=0 && (x-d) < width && i-1 >=0 && i < height) // Bounds check
          leds[XY16(x - d, i)] += leds[XY16(x - d, i - 1)]; // left column down
        if ((x-d) >=0 && (x-d) < width && i >=0 && i < height) // Bounds check
          leds[XY16(x - d, i)].nscale8(dimm);
      }
    }
  }

  // expand everything within a circle
  void Expand(int centerX, int centerY, int radius, byte dimm) {
    if (radius == 0)
      return;

    int currentRadius = radius;

    while (currentRadius > 0) {
      int a = radius, b = 0;
      int radiusError = 1 - a;

      int nextRadius = currentRadius - 1;
      int nextA = nextRadius - 1, nextB = 0;
      int nextRadiusError = 1 - nextA;

      while (a >= b)
      {
        // move them out one pixel on the radius
        if (a + centerX < width && b + centerY < height && nextA + centerX < width && nextB + centerY < height &&
            a + centerX >=0 && b + centerY >=0 && nextA + centerX >=0 && nextB + centerY >=0)
             leds[XY16(a + centerX, b + centerY)] = leds[XY16(nextA + centerX, nextB + centerY)];
        if (b + centerX < width && a + centerY < height && nextB + centerX < width && nextA + centerY < height &&
            b + centerX >=0 && a + centerY >=0 && nextB + centerX >=0 && nextA + centerY >=0)
            leds[XY16(b + centerX, a + centerY)] = leds[XY16(nextB + centerX, nextA + centerY)];
        if (-a + centerX >=0 && b + centerY < height && -nextA + centerX >=0 && nextB + centerY < height &&
            -a + centerX < width && b + centerY >=0 && -nextA + centerX < width && nextB + centerY >=0)
            leds[XY16(-a + centerX, b + centerY)] = leds[XY16(-nextA + centerX, nextB + centerY)];
        if (-b + centerX >=0 && a + centerY < height && -nextB + centerX >=0 && nextA + centerY < height &&
            -b + centerX < width && a + centerY >=0 && -nextB + centerX < width && nextA + centerY >=0)
            leds[XY16(-b + centerX, a + centerY)] = leds[XY16(-nextB + centerX, nextA + centerY)];
        if (-a + centerX >=0 && -b + centerY >=0 && -nextA + centerX >=0 && -nextB + centerY >=0 &&
            -a + centerX < width && -b + centerY < height && -nextA + centerX < width && -nextB + centerY < height)
            leds[XY16(-a + centerX, -b + centerY)] = leds[XY16(-nextA + centerX, -nextB + centerY)];
        if (-b + centerX >=0 && -a + centerY >=0 && -nextB + centerX >=0 && -nextA + centerY >=0 &&
            -b + centerX < width && -a + centerY < height && -nextB + centerX < width && -nextA + centerY < height)
            leds[XY16(-b + centerX, -a + centerY)] = leds[XY16(-nextB + centerX, -nextA + centerY)];
        if (a + centerX < width && -b + centerY >=0 && nextA + centerX < width && -nextB + centerY >=0 &&
            a + centerX >=0 && -b + centerY < height && nextA + centerX >=0 && -nextB + centerY < height)
            leds[XY16(a + centerX, -b + centerY)] = leds[XY16(nextA + centerX, -nextB + centerY)];
        if (b + centerX < width && -a + centerY >=0 && nextB + centerX < width && -nextA + centerY >=0 &&
            b + centerX >=0 && -a + centerY < height && nextB + centerX >=0 && -nextA + centerY < height)
            leds[XY16(b + centerX, -a + centerY)] = leds[XY16(nextB + centerX, -nextA + centerY)];


        // dim them
        if (a + centerX < width && b + centerY < height && a + centerX >=0 && b + centerY >=0) leds[XY16(a + centerX, b + centerY)].nscale8(dimm);
        if (b + centerX < width && a + centerY < height && b + centerX >=0 && a + centerY >=0) leds[XY16(b + centerX, a + centerY)].nscale8(dimm);
        if (-a + centerX >=0 && b + centerY < height && -a + centerX < width && b + centerY >=0) leds[XY16(-a + centerX, b + centerY)].nscale8(dimm);
        if (-b + centerX >=0 && a + centerY < height && -b + centerX < width && a + centerY >=0) leds[XY16(-b + centerX, a + centerY)].nscale8(dimm);
        if (-a + centerX >=0 && -b + centerY >=0 && -a + centerX < width && -b + centerY < height) leds[XY16(-a + centerX, -b + centerY)].nscale8(dimm);
        if (-b + centerX >=0 && -a + centerY >=0 && -b + centerX < width && -a + centerY < height) leds[XY16(-b + centerX, -a + centerY)].nscale8(dimm);
        if (a + centerX < width && -b + centerY >=0 && a + centerX >=0 && -b + centerY < height) leds[XY16(a + centerX, -b + centerY)].nscale8(dimm);
        if (b + centerX < width && -a + centerY >=0 && b + centerX >=0 && -a + centerY < height) leds[XY16(b + centerX, -a + centerY)].nscale8(dimm);

        b++;
        if (radiusError < 0)
          radiusError += 2 * b + 1;
        else
        {
          a--;
          radiusError += 2 * (b - a + 1);
        }

        nextB++;
        if (nextRadiusError < 0)
          nextRadiusError += 2 * nextB + 1;
        else
        {
          nextA--;
          nextRadiusError += 2 * (nextB - nextA + 1);
        }
      }

      currentRadius--;
    }
  }

  // give it a linear tail to the right
  void StreamRight(byte scale, int fromX = 0, int toX = 0, int fromY = 0, int toY = 0)
  {
    if (toX == 0) toX = width;
    if (toY == 0) toY = height;
    for (int x = fromX + 1; x < toX; x++) {
      for (int y = fromY; y < toY; y++) {
        if (x-1 >=0) // Bounds check
          leds[XY16(x, y)] += leds[XY16(x - 1, y)];
        leds[XY16(x, y)].nscale8(scale);
      }
    }
    for (int y = fromY; y < toY; y++)
      if (fromX >=0 && fromX < width) // Bounds check
        leds[XY16(fromX, y)].nscale8(scale); // Use fromX instead of 0
  }

  // give it a linear tail to the left
  void StreamLeft(byte scale, int fromX = 0, int toX = 0, int fromY = 0, int toY = 0)
  {
    if (toX == 0) toX = width;
    if (toY == 0) toY = height;
    // Corrected loop: fromX should be greater than toX for left stream
    for (int x = fromX - 1; x >= toX; x--) {
      for (int y = fromY; y < toY; y++) {
        if (x+1 < width) // Bounds check
          leds[XY16(x, y)] += leds[XY16(x + 1, y)];
        leds[XY16(x, y)].nscale8(scale);
      }
    }
    for (int y = fromY; y < toY; y++)
      if (fromX >=0 && fromX < width) // Bounds check
        leds[XY16(fromX, y)].nscale8(scale); // Use fromX
  }

  // give it a linear tail downwards
  void StreamDown(byte scale)
  {
    for (int x = 0; x < width; x++) {
      for (int y = 1; y < height; y++) {
        if (y-1 >=0) // Bounds check
          leds[XY16(x, y)] += leds[XY16(x, y - 1)];
        leds[XY16(x, y)].nscale8(scale);
      }
    }
    for (int x = 0; x < width; x++)
      leds[XY16(x, 0)].nscale8(scale);
  }

  // give it a linear tail upwards
  void StreamUp(byte scale)
  {
    for (int x = 0; x < width; x++) {
      for (int y = height - 2; y >= 0; y--) {
        if (y+1 < height) // Bounds check
          leds[XY16(x, y)] += leds[XY16(x, y + 1)];
        leds[XY16(x, y)].nscale8(scale);
      }
    }
    for (int x = 0; x < width; x++)
      leds[XY16(x, height - 1)].nscale8(scale);
  }

  // give it a linear tail up and to the left
  void StreamUpAndLeft(byte scale)
  {
    for (int x = 0; x < width - 1; x++) {
      for (int y = height - 2; y >= 0; y--) {
        if (x+1 < width && y+1 < height) // Bounds check
          leds[XY16(x, y)] += leds[XY16(x + 1, y + 1)];
        leds[XY16(x, y)].nscale8(scale);
      }
    }
    for (int x = 0; x < width; x++)
      leds[XY16(x, height - 1)].nscale8(scale);
    for (int y = 0; y < height; y++)
      leds[XY16(width - 1, y)].nscale8(scale);
  }

  // give it a linear tail up and to the right
  void StreamUpAndRight(byte scale)
  {
    for (int x = 0; x < width - 1; x++) {
      for (int y = height - 2; y >= 0; y--) {
        if (y+1 < height) // Bounds check
          leds[XY16(x + 1, y)] += leds[XY16(x, y + 1)];
        leds[XY16(x, y)].nscale8(scale);
      }
    }
    // fade the bottom row
    for (int x = 0; x < width; x++)
      leds[XY16(x, height - 1)].nscale8(scale);

    // fade the right column
    for (int y = 0; y < height; y++)
      leds[XY16(width - 1, y)].nscale8(scale);
  }

  // just move everything one line down
  void MoveDown() {
    for (int y = height - 1; y > 0; y--) {
      for (int x = 0; x < width; x++) {
        leds[XY16(x, y)] = leds[XY16(x, y - 1)];
      }
    }
  }

  // just move everything one line down
  void VerticalMoveFrom(int start, int end) {
    for (int y = end; y > start; y--) {
      for (int x = 0; x < width; x++) {
        if (y-1 >=0) // Bounds check
          leds[XY16(x, y)] = leds[XY16(x, y - 1)];
      }
    }
  }

  // copy the rectangle defined with 2 points x0, y0, x1, y1
  // to the rectangle beginning at x2, x3
  void Copy(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2) {
    for (int y = y0; y < y1 + 1; y++) {
      for (int x = x0; x < x1 + 1; x++) {
        if (x + x2 - x0 < width && y + y2 - y0 < height && x + x2 - x0 >=0 && y + y2 - y0 >=0) // Bounds check
          leds[XY16(x + x2 - x0, y + y2 - y0)] = leds[XY16(x, y)];
      }
    }
  }

  // rotate + copy triangle (MATRIX_CENTER_X*MATRIX_CENTER_X)
  void RotateTriangle() {
    for (int x = 1; x < width / 2; x++) {
      for (int y = 0; y < x; y++) {
        if (height / 2 - 1 - y >=0 && width / 2 - 1 - x >=0) // Bounds check
          leds[XY16(x, height / 2 - 1 - y)] = leds[XY16(width / 2 - 1 - x, y)];
      }
    }
  }

  // mirror + copy triangle (MATRIX_CENTER_X*MATRIX_CENTER_X)
  void MirrorTriangle() {
    for (int x = 1; x < width / 2; x++) {
      for (int y = 0; y < x; y++) {
        if (height / 2 - 1 - y >=0 && width / 2 - 1 - x >=0) // Bounds check
          leds[XY16(height / 2 - 1 - y, x)] = leds[XY16(width / 2 - 1 - x, y)];
      }
    }
  }

  // draw static rainbow triangle pattern (MATRIX_CENTER_XxWIDTH / 2)
  // (just for debugging)
  void RainbowTriangle() {
    for (int i = 0; i < width / 2; i++) {
      for (int j = 0; j <= i; j++) {
        if (height / 2 - 1 - i >=0) // Bounds check
          setPixelFromPaletteIndex(height / 2 - 1 - i, j, i * j * 4);
      }
    }
  }

  void BresenhamLine(int x0, int y0, int x1, int y1, byte colorIndex)
  {
    BresenhamLine(x0, y0, x1, y1, ColorFromCurrentPalette(colorIndex));
  }

  void BresenhamLine(int x0, int y0, int x1, int y1, CRGB color)
  {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
      if (x0 >=0 && x0 < width && y0 >=0 && y0 < height) // Bounds check
        leds[XY16(x0, y0)] += color;
      if (x0 == x1 && y0 == y1) break;
      e2 = 2 * err;
      if (e2 > dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 < dx) {
        err += dx;
        y0 += sy;
      }
    }
  }


  CRGB ColorFromCurrentPalette(uint8_t index = 0, uint8_t brightness = 255, TBlendType blendType = LINEARBLEND) {
    return ColorFromPalette(currentPalette, index, brightness, currentBlendType);
  }

  CRGB HsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
    CHSV hsv = CHSV(h, s, v);
    CRGB rgb;
    hsv2rgb_spectrum(hsv, rgb);
    return rgb;
  }

  void NoiseVariablesSetup() {
    noisesmoothing = 200;

    noise_x = random16();
    noise_y = random16();
    noise_z = random16();
    noise_scale_x = 6000;
    noise_scale_y = 6000;
  }

  void FillNoise() {
    for (uint16_t i = 0; i < width; i++) {
      uint32_t ioffset = noise_scale_x * (i - width / 2);

      for (uint16_t j = 0; j < height; j++) {
        uint32_t joffset = noise_scale_y * (j - height / 2);

        byte data = inoise16(noise_x + ioffset, noise_y + joffset, noise_z) >> 8;

        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8(olddata, noisesmoothing) + scale8(data, 256 - noisesmoothing);
        data = newdata;

        noise[i][j] = data;
      }
    }
  }

  // non leds2 memory version.
  void MoveX(byte delta) 
  {
    if (delta == 0) return; // No movement needed
    CRGB tmp_row[width]; // Temporary buffer for one row

    for (int y = 0; y < height; y++) 
    {
        // Copy current row to temporary buffer
        for(int x = 0; x < width; x++) {
            tmp_row[x] = leds[XY16(x,y)];
        }

        // Shift elements in the row
        for(int x = 0; x < width; x++) {
            leds[XY16(x,y)] = tmp_row[(x + delta) % width]; // Wrap around
        }
    }
  }

  void MoveY(byte delta)
  {
    if (delta == 0) return;
    CRGB tmp_col[height]; // Temporary buffer for one column

    for (int x = 0; x < width; x++) 
    {
        // Copy current column to temporary buffer
        for(int y = 0; y < height; y++) {
            tmp_col[y] = leds[XY16(x,y)];
        }

        // Shift elements in the column
        for(int y = 0; y < height; y++) {
            leds[XY16(x,y)] = tmp_col[(y + delta) % height]; // Wrap around
        }
    }
  }

  // Override GFX methods
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    // Convert GFX's RGB565 to CRGB for internal storage/processing
    uint8_t r = (color & 0xF800) >> 8; // Extract red component
    uint8_t g = (color & 0x07E0) >> 3; // Extract green component
    uint8_t b = (color & 0x001F) << 3; // Extract blue component
    setPixel(x, y, CRGB(r, g, b));     // Call our CRGB setPixel
  }

  // This drawPixel is not an override of GFX, so remove 'override'
  void drawPixel(int16_t x, int16_t y, CRGB color) {
    setPixel(x, y, color);
  }  

  void fillScreen(uint16_t color) override {
    // ClearFrame(); // ClearFrame sets to black, fillScreen should use the provided color
    // Add any other GFX methods you want to override
    uint8_t r = (color & 0xF800) >> 8;
    uint8_t g = (color & 0x07E0) >> 3;
    uint8_t b = (color & 0x001F) << 3;
    CRGB crgb_fill_color = CRGB(r,g,b);
    for (int i = 0; i < num_leds; i++) {
       leds[i] = crgb_fill_color;
    }
  }

};

#endif
