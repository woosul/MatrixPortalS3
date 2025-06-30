#include "mode_pattern.h" // Renamed from mode_animation.h
#include "common.h"
#include "utils.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// #include "Aurora/Boid.hpp" // Boid.hpp는 PatternFlock에서 사용될 수 있으므로 주석 처리 유지

// Global EffectsLayer object definition
// MATRIX_WIDTH and MATRIX_HEIGHT must be defined in common.h or config.h.
EffectsLayer effects(MATRIX_WIDTH, MATRIX_HEIGHT);

// AVAILABLE_BOID_COUNT must be defined in Boid.hpp or similar
Boid boids[AVAILABLE_BOID_COUNT];

ModePattern::ModePattern() : // Renamed from ModeAnimation
    m_utils(nullptr), m_matrix(nullptr), lastUpdate(0),
    currentPatternIndex(0), lastPatternChangeTime(0), // Renamed variables
    patternChangeInterval(10 * 60 * 1000), // Initialize to default 10 minutes, was auroraPatternChangeInterval
    animationHue(0), animationBrightness(128), lastAnimationUpdate(0),
    currentAnimation(0) { // animationMode is commented out
    // Initialization of drawablePatterns array in the constructor is performed in setup
}

void ModePattern::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) { // Renamed
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    lastUpdate = 0;

    // Set EffectsLayer's virtualDisp to point to m_matrix
    effects.virtualDisp = m_matrix;

    // Create Pattern objects and assign them to the array (was Aurora pattern)
    drawablePatterns[0] = new PatternCube();
    drawablePatterns[1] = new PatternPlasma();
    drawablePatterns[2] = new PatternFlock();
    drawablePatterns[3] = new PatternSpiral();
    drawablePatterns[4] = new PatternPaletteSmear();
    drawablePatterns[5] = new PatternStarfield();
    // drawablePatterns[6] = new PatternAttract();
    // drawablePatterns[7] = new PatternElectricMandala();
    // drawablePatterns[8] = new PatternFlowField();
    drawablePatterns[6] = new PatternIncrementalDrift();
    // drawablePatterns[10] = new PatternInfinity();
    drawablePatterns[7] = new PatternMunch();
    drawablePatterns[8] = new PatternPendulumWave();
    // drawablePatterns[13] = new PatternRadar();
    // drawablePatterns[14] = new PatternSimplexNoise();
    // drawablePatterns[15] = new PatternSnake();
    drawablePatterns[9] = new PatternSpiro();
    drawablePatterns[10] = new PatternWave();
    drawablePatterns[11] = new PatternRain();
    drawablePatterns[12] = new PatternJuliaSet(); 
    // drawablePatterns[20] = new PatternFirework(); 
    // drawablePatterns[21] = new PatternStardustBurst(); 
    // GIF pattern is now a separate mode
    currentPatternIndex = 0; // Start with the first pattern
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->start(); // Renamed
    }
    lastPatternChangeTime = millis(); // Renamed

    // Initialize existing variables (if necessary)
    animationHue = 0;
    animationBrightness = 128; // Aurora patterns can have their own brightness/color control
    // lastAnimationUpdate = 0; // lastAnimationUpdate may not be directly used by Aurora patterns
    currentAnimation = 0; // Existing animation index, currently not used

    Serial.println("Pattern mode setup complete (Drawable Patterns Ready)"); // Renamed
}

// Mathematical HSV to RGB565 conversion
uint16_t ModePattern::hsv2rgb565(uint8_t h, uint8_t s, uint8_t v) { // Renamed
    uint8_t r, g, b;

    // Convert HSV to RGB using mathematical formulas
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    return m_matrix->color565(r, g, b);
}

void ModePattern::run() { // Renamed
    unsigned long currentTime = millis();

    // updateAnimation now manages its own frame delay,
    // or can use the return value of Aurora pattern's drawFrame().
    // Here, it follows the delay recommended by the Aurora pattern.
    // Instead of a fixed update interval based on ANIMATION_FPS, the pattern's recommended delay is used.
    // (Alternatively, ANIMATION_FPS can be maintained if needed, and the pattern delay can be ignored)
    // Here, timing management within updateAnimation() is simplified.
    updateAnimation();
    // lastUpdate = currentTime; // Managed inside updateAnimation
}

void ModePattern::cleanup() { // Renamed
    Serial.println("Pattern mode cleanup"); // Renamed
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->stop(); // Renamed
    }

    // Deallocate dynamically allocated Pattern objects
    for (int i = 0; i < MAX_PATTERNS; ++i) { // MAX_PATTERNS used
        if (drawablePatterns[i]) { // Renamed
            delete drawablePatterns[i]; // Renamed
            drawablePatterns[i] = nullptr; // Renamed
        }
    }
    if (m_matrix && m_utils) {
        m_matrix->fillScreen(0);
        m_utils->displayShow();
    }
}

void ModePattern::updateAnimation() { // Renamed, consider renaming to updatePattern
    unsigned long currentTime = millis();
    unsigned int frameDelay = 0;

    if (drawablePatterns[currentPatternIndex]) { // Renamed
        // Call the current Pattern's drawFrame()
        // drawFrame() typically returns the recommended frame delay time.
        frameDelay = drawablePatterns[currentPatternIndex]->drawFrame(); // Renamed
        // effects.ShowFrame() is called inside each pattern's drawFrame().
        // After that, the content drawn on m_matrix's back buffer is sent to the actual screen.
        if (m_utils) m_utils->displayShow();
    }

    // Automatic pattern switching logic (every 10 minutes)
    if (currentTime - lastPatternChangeTime > patternChangeInterval) { // Renamed
        nextPattern(); // Renamed
    }

    // Delay processing until the next update
    // If the pattern returns 0, it can follow the default FPS or apply a minimum delay.
    if (frameDelay == 0) {
        frameDelay = 1000 / ANIMATION_FPS; // Delay according to default FPS
    }

    // Use lastUpdate to adjust the next call time (instead of adjusting call frequency in run() function)
    // This approach calls updateAnimation() every time in run(),
    // and updateAnimation() internally decides whether to perform the actual work.
    // Alternatively, run() can use lastUpdate and frameDelay to adjust call frequency.
    // Here, it is assumed that run() is called frequently enough, and frameDelay is kept for reference.
    // Actual frame rate control depends on loop()'s delay(5) and pattern complexity.
    // For more precise FPS control, lastUpdate and frameDelay should be used.
    if (currentTime - lastUpdate < frameDelay) {
        // If it's not yet time for the next frame, do nothing.
        // Or, a very short delay can be given
        return;
    }
    lastUpdate = currentTime;

    // 기존 애니메이션 로직 (drawPlasma, drawRainbow, drawTetris)은 현재 사용되지 않음
    // 필요하다면 currentAnimation 변수와 switch 문을 사용하여 다시 활성화할 수 있습니다.
    /*
    bool animationWantsToContinue = true; // Assume animation wants to continue by default

    switch (currentAnimation) { // currentAnimation은 이제 Aurora 패턴 인덱스와는 다름
        case 0:
            animationWantsToContinue = drawPlasma();
            break;
        case 1:
            animationWantsToContinue = drawRainbow();
            break;
        case 2:
            animationWantsToContinue = drawTetris();
            break;
        default:
            currentAnimation = 0;
            animationWantsToContinue = drawPlasma();
            break;
    }

    static unsigned long lastForcedSwitchTime = 0;

    if (!animationWantsToContinue) {
        Serial.printf("Animation %d ended itself. Switching.\n", currentAnimation);
        currentAnimation = (currentAnimation + 1) % 3;
        lastForcedSwitchTime = millis();
        if (m_matrix) m_matrix->fillScreen(0);
    } else if (millis() - lastForcedSwitchTime > 20000) { // 20초 타임아웃은 Aurora 자동 전환으로 대체됨
        Serial.printf("Animation %d timed out (20s), switching.\n", currentAnimation);
        currentAnimation = (currentAnimation + 1) % 3;
        lastForcedSwitchTime = millis();
        if (m_matrix) m_matrix->fillScreen(0);
    }
    */
}

void ModePattern::nextPattern() { // Renamed
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->stop(); // Renamed
    }
    currentPatternIndex = (currentPatternIndex + 1) % MAX_PATTERNS; // Renamed, MAX_PATTERNS used
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->start(); // Renamed
    }
    if (m_matrix) m_matrix->fillScreen(0);
    lastPatternChangeTime = millis(); // Renamed
    Serial.printf("Switched to next Pattern: %d (%s)\n", currentPatternIndex + 1, drawablePatterns[currentPatternIndex] ? drawablePatterns[currentPatternIndex]->name : "Unknown"); // Renamed
    // if (m_utils && m_utils->isSoundFeedbackEnabled()) m_utils->playSingleTone();
}

void ModePattern::prevPattern() { // Renamed
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->stop(); // Renamed
    }
    currentPatternIndex = (currentPatternIndex - 1 + MAX_PATTERNS) % MAX_PATTERNS; // Renamed, MAX_PATTERNS used
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->start(); // Renamed
    }
    if (m_matrix) m_matrix->fillScreen(0);
    lastPatternChangeTime = millis(); // Renamed
    Serial.printf("Switched to previous Pattern: %d (%s)\n", currentPatternIndex + 1, drawablePatterns[currentPatternIndex] ? drawablePatterns[currentPatternIndex]->name : "Unknown"); // Renamed
    // if (m_utils && m_utils->isSoundFeedbackEnabled()) m_utils->playSingleTone();
}

void ModePattern::setPattern(PatternType mode) { // Renamed, parameter type changed
    int newIndex = static_cast<int>(mode);

    // Validate mode index
    if (newIndex < 0 || newIndex >= MAX_PATTERNS) { // MAX_PATTERNS used
        Serial.printf("Invalid pattern mode: %d. Valid range: 0-%d\n", newIndex, MAX_PATTERNS - 1); // Renamed
        return;
    }

    // Stop current pattern if running
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->stop(); // Renamed
    }

    // Switch to new pattern
    currentPatternIndex = newIndex; // Renamed

    // Start new pattern if available
    if (drawablePatterns[currentPatternIndex]) { // Renamed
        drawablePatterns[currentPatternIndex]->start(); // Renamed
    }

    // Clear screen and reset timer
    if (m_matrix) m_matrix->fillScreen(0);
    lastPatternChangeTime = millis(); // Renamed

    // Get pattern name for logging
    const char* patternNames[MAX_PATTERNS] = { // MAX_PATTERNS used
        "Cube",             // ANIM_CUBE
        "Plasma",           // ANIM_PLASMA
        "Flock",            // ANIM_FLOCK
        "Spiral",           // ANIM_SPIRAL
        "PaletteSmear",     // ANIM_PALETTE_SMEAR (was ANIM_NOISE)
        "Starfield",        // ANIM_STARFIELD
        // "Attract",          // ANIM_ATTRACT
        // "ElectricMandala",  // ANIM_ELECTRIC_MANDALA
        // "FlowField",        // ANIM_FLOW_FIELD
        "IncrDrift",        // ANIM_INCREMENTAL_DRIFT
        // "Infinity",         // ANIM_INFINITY
        "Munch",            // ANIM_MUNCH
        "PendulumWave",     // ANIM_PENDULUM_WAVE
        // "Radar",            // ANIM_RADAR
        // "SimplexNoise",     // ANIM_SIMPLEX_NOISE
        // "Snake",            // ANIM_SNAKE
        "Spiro",            // ANIM_SPIRO
        "Wave",             // ANIM_WAVE
        "Rain",             // ANIM_RAIN
        "JuliaSet",         // ANIM_JULIA_SET
        // "Fireworks",        // ANIM_FIREWORKS
        // "StardustBurst"    // ANIM_STARDUST_BURST (New pattern name)
            
    };
    const char* selectedPatternName = (newIndex >= 0 && newIndex < MAX_PATTERNS && patternNames[newIndex] != nullptr) ? patternNames[newIndex] : "Unknown"; // Renamed

    Serial.printf("MQTT: Set pattern to %s (index: %d)\n", selectedPatternName, newIndex); // Renamed
    // if (m_utils && m_utils->isSoundFeedbackEnabled()) m_utils->playSingleTone();
}

// Keep the existing animation functions as they are.
// bool ModePattern::drawPlasma() { ... } // Renamed
// bool ModePattern::drawRainbow() { ... } // Renamed
// bool ModePattern::drawTetris() { ... } // Renamed
// The following are existing functions. Their content will not be changed.

bool ModePattern::drawPlasma() { // Renamed
    static float time = 0;
    m_matrix->fillScreen(0);

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            float dx = (x - (float)MATRIX_WIDTH / 2.0) * 0.1;
            float dy = (y - (float)MATRIX_HEIGHT / 2.0) * 0.1;

            float plasma = sin(dx + time) +
                          sin(dy + time * 1.2) +
                          sin((dx + dy) * 0.08 + time * 0.8) +
                          sin(sqrt(dx*dx + dy*dy) * 0.1 + time * 1.5);

            uint8_t hue = (uint8_t)((plasma + 4.0) * 32.0); // Normalize plasma to hue range
            uint16_t color = hsv2rgb565(hue, 255, animationBrightness);

            m_matrix->drawPixel(setPhysicalX(x), y, color); // Apply X-shift
        }
    }

    // m_matrix->setTextColor(m_matrix->color565(255, 255, 255));
    // m_matrix->setCursor(2, 55);
    // m_matrix->print("Plasma");
    m_utils->displayShow();

    time += 0.15;
    return true; // Plasma animation continues indefinitely
}

bool ModePattern::drawRainbow() { // Renamed
    m_matrix->fillScreen(0);

    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            uint8_t pixelHue = animationHue + (x * 2) + (y * 1);
            uint16_t color = hsv2rgb565(pixelHue, 255, animationBrightness);
            m_matrix->drawPixel(setPhysicalX(x), y, color); // Apply X-shift
        }
    }

    // m_matrix->setTextColor(m_matrix->color565(255, 255, 255));
    // m_matrix->setCursor(2, 35);
    // m_matrix->print("Rainbow");
    m_utils->displayShow();

    animationHue += 1; // Cycle hue for rainbow effect
    // lastAnimationUpdate = millis(); // This variable is not currently used for timing control here
    return true; // Rainbow animation continues indefinitely
}

bool ModePattern::drawTetris() { // Renamed
    // Static variables for Tetris game state
    static uint8_t tetrisGrid[MATRIX_HEIGHT][MATRIX_WIDTH];
    static int currentBlockX, currentBlockY;
    static int currentBlockW, currentBlockH;
    static uint8_t currentBlockColor;
    static bool isBlockFalling = false;
    static bool gameIsOver = false; // Renamed to avoid conflict if 'gameOver' is used in a wider scope
    static unsigned long lastDropTime = 0;
    const unsigned long dropInterval = 200; // Milliseconds, adjust for speed

    static bool gameInitialized = false;

    // Lambda function to spawn a new block
    auto spawnNewBlock = [&]() {
        currentBlockW = random(1, 5); // Width: 1 to 4 columns
        currentBlockH = random(1, 4); // Height: 1 to 3 rows (original comment was 1-2 col, 1-3 row)
        currentBlockX = random(0, MATRIX_WIDTH - currentBlockW + 1); // Ensure block fits horizontally
        currentBlockY = 0; // Start from the top
        currentBlockColor = random(1, 8); // 7 distinct colors (1-7)

        isBlockFalling = true; // Assume it can fall initially

        // Check for immediate collision at spawn (if grid already has blocks at spawn loc)
        bool collisionAtSpawn = false;
        for (int r_offset = 0; r_offset < currentBlockH; ++r_offset) {
            for (int c_offset = 0; c_offset < currentBlockW; ++c_offset) {
                int checkY = currentBlockY + r_offset;
                int checkX = currentBlockX + c_offset;
                // Ensure check is within bounds
                if (checkY >= 0 && checkY < MATRIX_HEIGHT && checkX >= 0 && checkX < MATRIX_WIDTH) {
                    if (tetrisGrid[checkY][checkX] != 0) {
                        collisionAtSpawn = true;
                        break;
                    }
                } else { // Part of the block is out of bounds at spawn (should not happen with correct X,Y spawn)
                    collisionAtSpawn = true; // Treat as collision if spawn logic is flawed
                    break;
                }
            }
            if (collisionAtSpawn) break;
        }

        if (collisionAtSpawn) {
            gameIsOver = true;
            isBlockFalling = false; // Block cannot be placed/fall
        }
    };

    // Initialize or reset game if it's the first run or game over
    if (!gameInitialized || gameIsOver) {
        memset(tetrisGrid, 0, sizeof(tetrisGrid)); // Clear the grid
        gameIsOver = false;                        // Reset game over flag for the new game
        isBlockFalling = false;                    // No block is falling initially
        gameInitialized = true;                    // Mark as initialized

        spawnNewBlock(); // Spawn the first block of the new game
        if (gameIsOver) { // If spawnNewBlock immediately results in game over (e.g. grid too full)
             // Display a brief game over indication before switching
             m_matrix->fillScreen(m_matrix->color565(100,0,0)); // Dark red
             m_utils->displayShow();
             delay(500); // Show game over screen briefly
             return false; // Signal to switch animation
        }
    }

    // Game logic for falling block
    // This 'if' block should only run if the game is NOT over from the initialization/spawn step.
    // The 'gameIsOver' check at the beginning of the function handles the reset.
    // If spawnNewBlock sets gameIsOver, this block will be skipped.
    if (isBlockFalling && !gameIsOver) {
        if (millis() - lastDropTime > dropInterval) {
            lastDropTime = millis();

            bool canMoveDown = true;
            if (currentBlockY + currentBlockH >= MATRIX_HEIGHT) { // Block's bottom edge is at or past the matrix bottom
                canMoveDown = false;
            } else {
                // Check collision with existing blocks directly below the current block
                for (int c_offset = 0; c_offset < currentBlockW; ++c_offset) {
                    if (tetrisGrid[currentBlockY + currentBlockH][currentBlockX + c_offset] != 0) {
                        canMoveDown = false;
                        break;
                    }
                }
            }

            if (canMoveDown) {
                currentBlockY++; // Move block down
            } else {
                // Block cannot move down, so place it on the grid
                for (int r_offset = 0; r_offset < currentBlockH; ++r_offset) {
                    for (int c_offset = 0; c_offset < currentBlockW; ++c_offset) {
                        int placeY = currentBlockY + r_offset;
                        int placeX = currentBlockX + c_offset;
                        if (placeY >= 0 && placeY < MATRIX_HEIGHT && placeX >= 0 && placeX < MATRIX_WIDTH) {
                            tetrisGrid[placeY][placeX] = currentBlockColor;
                        }
                    }
                }
                isBlockFalling = false; // Block has landed

                // Check for game over (top row filled)
                for (int c = 0; c < MATRIX_WIDTH; ++c) {
                    if (tetrisGrid[0][c] != 0) {
                        gameIsOver = true;
                        break;
                    }
                }

                if (!gameIsOver) {
                    spawnNewBlock(); // Spawn the next block
                    if (gameIsOver) { // If this new block immediately causes game over
                        m_matrix->fillScreen(m_matrix->color565(100,0,0));
                        m_utils->displayShow();
                        delay(500);
                        return false; // Signal to switch
                    }
                }
            }
        }
    } else if (!isBlockFalling && !gameIsOver) {
        // This state means a block has landed, game is not over, so spawn the next one.
        // Or, it could be the very first successful spawn after initialization.
        spawnNewBlock();
        if (gameIsOver) { // If this new block immediately causes game over
            m_matrix->fillScreen(m_matrix->color565(100,0,0));
            m_utils->displayShow();
            delay(500);
            return false; // Signal to switch
        }
    }

    // Drawing phase
    m_matrix->fillScreen(0); // Clear screen for new frame


    // Draw landed blocks from tetrisGrid
    for (int r = 0; r < MATRIX_HEIGHT; ++r) {
        for (int c = 0; c < MATRIX_WIDTH; ++c) {
            if (tetrisGrid[r][c] != 0) {
                uint8_t hue = tetrisGrid[r][c] * 36; // Simple color mapping
                uint16_t color = hsv2rgb565(hue, 255, 200); // Fixed saturation and brightness
                m_matrix->drawPixel(setPhysicalX(c), r, color); // Apply X-shift
            }
        }
    }

    // Draw the current falling block if it exists and game is not (yet) over from this frame's logic
    if (isBlockFalling && !gameIsOver) {
        uint8_t hue = currentBlockColor * 36;
        uint16_t color = hsv2rgb565(hue, 255, 200);
        for (int r_offset = 0; r_offset < currentBlockH; ++r_offset) {
            for (int c_offset = 0; c_offset < currentBlockW; ++c_offset) {
                int drawY = currentBlockY + r_offset;
                int drawX = currentBlockX + c_offset;
                if (drawY >= 0 && drawY < MATRIX_HEIGHT && drawX >= 0 && drawX < MATRIX_WIDTH) {
                     m_matrix->drawPixel(setPhysicalX(drawX), drawY, color); // Apply X-shift
                }
            }
        }
    }

    m_utils->displayShow();

    if (gameIsOver) {
        // If game over was determined during this frame's logic (e.g. block placed and filled top row)
        Serial.println("Tetris: Game Over condition met. Signaling to switch.");
        m_matrix->fillScreen(m_matrix->color565(100,0,0)); // Optional: brief game over screen
        m_utils->displayShow();
        delay(500); // Show game over screen briefly
        return false; // Signal to switch animation
    }
    return true; // Continue Tetris animation
}
