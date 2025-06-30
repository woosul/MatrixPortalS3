#ifndef MODE_PATTERN_H
#define MODE_PATTERN_H

#include "config.h" // For MATRIX_WIDTH, MATRIX_HEIGHT if defined there, or config.h
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// #include "utils.h" // Utils is forward-declared

// Aurora Demo Pattern Headers (relative to src folder)
#include "Aurora/EffectsLayer.hpp" // Include EffectsLayer header
#include "Aurora/PatternCube.hpp"
#include "Aurora/PatternPlasma.hpp"
#include "Aurora/PatternFlock.hpp"
#include "Aurora/PatternSpiral.hpp"
#include "Aurora/PatternNoiseSmearing.hpp"
#include "Aurora/PatternStarfield.hpp" 

// #include "Aurora/PatternAttract.hpp"
// #include "Aurora/PatternElectricMandala.hpp"
// #include "Aurora/PatternFlowField.hpp"
#include "Aurora/PatternIncrementalDrift.hpp"
// #include "Aurora/PatternInfinity.hpp"
#include "Aurora/PatternMunch.hpp"
#include "Aurora/PatternPendulumWave.hpp"
// #include "Aurora/PatternRadar.hpp"
// #include "Aurora/PatternSimplexNoise.hpp"
// #include "Aurora/PatternSnake.hpp"
#include "Aurora/PatternSpiro.hpp"
#include "Aurora/PatternWave.hpp"
#include "Aurora/PatternRain.hpp"
#include "Aurora/PatternJuliaSetFractal.hpp"
// #include "Aurora/PatternFireworks.hpp"
// #include "Aurora/PatternStardustBurst.hpp" // by GEMINI

// Global EffectsLayer object declaration (Aurora patterns will reference this object)
// The actual definition will be in the mode_animation.cpp file.
extern EffectsLayer effects;

// Pattern sub-mode enumeration (was AnimationModeType)
enum class PatternType {
    ANIM_CUBE = 0,              // PatternCube
    ANIM_PLASMA,                // PatternPlasma  
    ANIM_FLOCK,                 // PatternFlock
    ANIM_SPIRAL,                // PatternSpiral
    ANIM_PALETTE_SMEAR,         // PatternPaletteSmear (from PatternNoiseSmearing.hpp
    ANIM_STARFIELD,             // PatternStarfield
    // ANIM_ATTRACT,               // PatternAttract
    // ANIM_ELECTRIC_MANDALA,      // PatternElectricMandala
    // ANIM_FLOW_FIELD,            // PatternFlowField
    ANIM_INCREMENTAL_DRIFT,     // PatternIncrementalDrift
    // ANIM_INFINITY,              // PatternInfinity
    ANIM_MUNCH,                 // PatternMunch
    ANIM_PENDULUM_WAVE,         // PatternPendulumWave
    // ANIM_RADAR,                 // PatternRadar
    // ANIM_SIMPLEX_NOISE,         // PatternSimplexNoise
    // ANIM_SNAKE,                 // PatternSnake
    ANIM_SPIRO,                 // PatternSpiro
    ANIM_WAVE,                  // PatternWave
    ANIM_RAIN,                  // PatternRain
    ANIM_JULIA_SET,             // PatternJuliaSetFractal
    // ANIM_FIREWORKS,             // PatternFireworks
    // ANIM_STARDUST_BURST,        // PatternStardustBurst (새 패턴)
    PATTERN_MODE_COUNT          // Must be last - used for counting (was ANIM_MODE_COUNT)
};

// Constants for mode management
#define MAX_PATTERNS static_cast<int>(PatternType::PATTERN_MODE_COUNT) // Was MAX_ANIMATION_MODES

// Forward declarations
class Utils;
// class MatrixPanel_I2S_DMA; // Already included

class ModePattern { // Renamed from ModeAnimation
public: // Constructor added
    ModePattern(); // Renamed from ModeAnimation
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    void run();
    void cleanup();
    void nextPattern(); // Renamed from nextAuroraPattern
    void prevPattern(); // Renamed from prevAuroraPattern
    void setPattern(PatternType mode); // Renamed from setAnimationMode, parameter type changed

    // HSV to RGB565 conversion function
    uint16_t hsv2rgb565(uint8_t h, uint8_t s, uint8_t v);

private:
    
    void updateAnimation(); // Aurora 패턴 업데이트 및 전환 로직
    // void updatePattern(); // Consider renaming updateAnimation to updatePattern for consistency

    Utils* m_utils; // Pattern update and switching logic
    MatrixPanel_I2S_DMA* m_matrix; 

    // Pattern related variables (was Aurora pattern related)
    Drawable* drawablePatterns[MAX_PATTERNS]; // Renamed from auroraPatterns
    int currentPatternIndex;                  // Renamed from currentAuroraPatternIndex
    unsigned long lastPatternChangeTime;      // Renamed from lastAuroraPatternChangeTime
    unsigned long patternChangeInterval;      // Renamed from auroraPatternChangeInterval

    // Existing animation related variables (currently not directly used by Aurora)
    uint8_t animationHue;
    uint8_t animationBrightness;
    unsigned long lastAnimationUpdate;

    // uint8_t animationMode; // Can be replaced by Aurora pattern index
    int currentAnimation; // Existing animation index (0: Plasma, 1: Rainbow, 2: Tetris)
    unsigned long lastUpdate;

    // Animation drawing functions
    bool drawPlasma();
    bool drawRainbow();
    bool drawTetris();
    // int rainDrops[64]; // If needed for Tetris animation, can be kept static within the function or as a member.
                        // The currently provided mode_animation.cpp does not use rainDrops in its Tetris logic.
};

#endif