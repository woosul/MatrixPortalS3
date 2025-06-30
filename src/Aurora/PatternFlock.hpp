/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from "Flocking" in "The Nature of Code" by Daniel Shiffman: http://natureofcode.com/
 * Copyright (c) 2014 Daniel Shiffman
 * http://www.shiffman.net
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

// Flocking
// Daniel Shiffman <http://www.shiffman.net>
// The Nature of Code, Spring 2009

// Demonstration of Craig Reynolds' "Flocking" behavior
// See: http://www.red3d.com/cwr/
// Rules: Cohesion, Separation, Alignment

#ifndef PatternFlock_H
#define PatternFlock_H

#include "EffectsLayer.hpp"
#include "Vector2.hpp" // PVector 대신 사용
#include "Boid.hpp"    // Boid 클래스

class PatternFlock : public Drawable {

  private:
    unsigned long last_update_hue_ms = 0;
    unsigned long last_update_predator_ms = 0;

  public:
    PatternFlock() {
      name = (char *)"Flock";
    }

    static const int boidCount = MATRIX_WIDTH > 1 ? MATRIX_WIDTH - 1 : 1; // 최소 1개 보장
    // Boid predator; // Predator 클래스가 없으므로 주석 처리 또는 다른 방식으로 대체

    PVector wind; // PVector는 Vector2<float>의 typedef일 수 있음, Vector2.hpp 내용에 따라 PVector 또는 Vector2<float> 사용
    byte hue = 0;
    bool predatorPresent = false; // Predator 기능을 사용하지 않으므로 false로 고정
    CRGB boidColor; // 멤버 변수로 선언

    void start() {
      for (int i = 0; i < boidCount; i++) {
        // boids 배열은 Drawable.h 또는 Boid.hpp에서 전역/정적으로 선언되어 있어야 함
        // 여기서는 boids 배열이 PatternFlock 클래스의 멤버 또는 접근 가능한 전역 배열이라고 가정
        boids[i] = Boid(effects.getCenterX(), effects.getCenterY()); // 중앙에서 시작하도록 수정
        boids[i].maxspeed = 0.380;
        boids[i].maxforce = 0.015;
      }

      // Predator 관련 초기화 주석 처리
      // predatorPresent = random(0, 2) >= 1;
      // if (predatorPresent) {
      //   predator = Boid(effects.width -1, effects.height -1);
      //   predator.maxspeed = 0.385;
      //   predator.maxforce = 0.020;
      //   predator.neighbordist = 16.0;
      //   predator.desiredseparation = 0.0;
      // }
      wind = PVector(0,0); // wind 초기화
    }

    unsigned int drawFrame() {
      effects.DimAll(230); effects.ShowFrame();

      bool applyWind = random(0, 255) > 250;
      if (applyWind) {
        wind.x = Boid::randomf() * .015;
        wind.y = Boid::randomf() * .015;
      }

      boidColor = effects.ColorFromCurrentPalette(hue); // 멤버 변수 사용

      for (int i = 0; i < boidCount; i++) {
        Boid * boid = &boids[i];

        // Predator 로직 주석 처리
        // if (predatorPresent) {
        //   // flee from predator
        //   boid->repelForce(predator.location, 10);
        // }

        boid->run(boids, boidCount);
        boid->wrapAroundBorders();
        PVector location = boid->location;
        effects.setPixel(location.x, location.y, boidColor);

        if (applyWind) {
          boid->applyForce(wind);
          applyWind = false;
        }
      }

      // Predator 로직 주석 처리
      // if (predatorPresent) {
      //   predator.run(boids, boidCount);
      //   predator.wrapAroundBorders();
      //   CRGB predatorColor = effects.ColorFromCurrentPalette(hue + 128);
      //   PVector pred_location = predator.location;
      //   effects.setPixel(pred_location.x, pred_location.y, predatorColor);
      // }

      if (millis() - last_update_hue_ms > 200) {
        last_update_hue_ms = millis();
        hue++;
      }
      
      // Predator 존재 여부 변경 로직 주석 처리
      // if (millis() - last_update_predator_ms > 30000) {
      //   last_update_predator_ms = millis();
      //   // predatorPresent = !predatorPresent; // Predator 기능 비활성화
      // }

      return 0;
    }
};

#endif
