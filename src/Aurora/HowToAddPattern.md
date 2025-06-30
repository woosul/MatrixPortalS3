# How to add a new Aurora animation pattern

새로운 Aurora 애니메이션 패턴을 프로젝트에 추가하는 방법은 다음과 같습니다.

## 1. Generate new pattern file

새로운 패턴 파일을 생성하려면, 다음 단계를 따르세요:

* src/Aurora/ 폴더 안에 새로운 .hpp 파일을 만듭니다. 예를 들어 PatternMyNewEffect.hpp라고 하겠습니다.
* 이 파일 안에는 Drawable 클래스를 상속받는 새로운 클래스를 정의합니다. 이 클래스는 최소한 생성자와 drawFrame() 메서드를 구현해야 합니다.

```cpp
#ifndef PATTERNMYNEWEFFECT_H
#define PATTERNMYNEWEFFECT_H

#include "EffectsLayer.hpp" // Drawable 클래스와 EffectsLayer 접근을 위해 필요

class PatternMyNewEffect : public Drawable {
public:
    PatternMyNewEffect() {
        name = (char *)"MyNewEffect"; // 패턴의 이름을 지정합니다.
    }

    // 필요에 따라 패턴 시작 시 호출될 초기화 코드를 여기에 작성합니다.
    // void start() override {
    //     effects.ClearFrame(); // 예: 시작 시 화면 지우기
    // }

    // 매 프레임마다 호출되어 실제 애니메이션을 그립니다.
    unsigned int drawFrame() override {
        // 여기에 픽셀 데이터를 설정하는 코드를 작성합니다.
        // 예: 화면을 특정 색으로 채우기
        // effects.ClearFrame(); // 이전 프레임 지우기
        // for (int y = 0; y < effects.height; y++) {
        //     for (int x = 0; x < effects.width; x++) {
        //         effects.setPixel(x, y, CRGB::Blue);
        //     }
        // }

        // 예: 간단한 깜빡임 효과
        static uint8_t c = 0;
        effects.ClearFrame();
        effects.setPixel(effects.getCenterX(), effects.getCenterY(), effects.ColorFromCurrentPalette(c));
        c += 5;


        effects.ShowFrame(); // 그린 내용을 실제 디스플레이로 보냅니다.

        return 50; // 다음 프레임까지의 권장 지연 시간 (밀리초 단위)
    }
};
#endif
```

---

## 2. Register the new pattern

### 2.1 새로 만든 패턴을 프로젝트에 등록하려면, 다음 단계를 따르세요 : `mode_animation.h` 파일 수정

```cpp  
// ... 기존 include 문들 ...
#include "Aurora/PatternNoiseSmearing.hpp"
#include "Aurora/PatternMyNewEffect.hpp" // 새로 추가한 패턴 헤더
// ... More patterns can be added here ...
```

### 2.2 NUM_AURORA_PATTERNS 매크로 상수를 +1증가시킵니다

```cpp
// Number of Aurora patterns to use (adjust according to the actual number of included patterns)
#define NUM_AURORA_PATTERNS 6 // 예: 기존 5개 + 새 패턴 1개 = 6
```

### 2.3 `mode_animation.cpp` 파일 수정

ModeAnimation::setup() 함수 내에서, auroraPatterns 배열의 다음 빈자리에 새로 만든 패턴 클래스의 인스턴스를 생성하여 할당합니다.

```cpp
// ... 기존 코드 ...
void ModeAnimation::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
// ... 기존 코드 ...
effects.virtualDisp = m_matrix;

// Create Aurora pattern objects and assign them to the array
auroraPatterns[0] = new PatternCube();
auroraPatterns[1] = new PatternPlasma();
auroraPatterns[2] = new PatternFlock();
auroraPatterns[3] = new PatternSpiral();
auroraPatterns[4] = new PatternPaletteSmear(); // PatternNoiseSmearing.hpp에 정의된 클래스 중 하나
auroraPatterns[5] = new PatternMyNewEffect();  // 새로 추가한 패턴 객체 생성
// ... Add more patterns here ...

// ... 나머지 setup 코드 ...
}
```

> 참고 : PatternNoiseSmearing.hpp 파일에는 PatternMultipleStream, PatternMultipleStream2, PatternPaletteSmear, PatternRainbowFlag 등 여러 패턴 클래스가 이미 정의되어 있습니다. </br>
이들을 사용하고 싶다면, 해당 클래스 이름으로 객체를 생성하여 auroraPatterns 배열에 추가하고 NUM_AURORA_PATTERNS 값을 적절히 조절하면 됩니다. </br>
현재 PatternPaletteSmear는 이미 사용 중입니다.
