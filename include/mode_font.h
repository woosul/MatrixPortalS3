#ifndef MODE_FONT_H
#define MODE_FONT_H

#include "config.h"
#include "font_manager.h"

// Forward declarations
class Utils;
class MatrixPanel_I2S_DMA;

// Font sub-mode enumeration (기존 FontType을 활용)
enum class FontModeType {
    FONT_MODE_DEFAULT = 0,     // FONT_DEFAULT
    FONT_MODE_AGENCY_R_8,      // FONT_AGENCY_R_8
    FONT_MODE_AGENCY_R_10,     // FONT_AGENCY_R_10
    FONT_MODE_AGENCY_R_12,     // FONT_AGENCY_R_12
    FONT_MODE_AGENCY_R_16,     // FONT_AGENCY_R_16
    FONT_MODE_AGENCY_B_16,     // FONT_AGENCY_B_16
    FONT_MODE_ORG_R_6,         // FONT_ORG_R_6
    FONT_MODE_ROBOTO_BK_20,    // FONT_ROBOTO_BK_20
    FONT_MODE_ROBOTO_BK_24,    // FONT_ROBOTO_BK_24
    FONT_MODE_ROBOTO_BK_32,    // FONT_ROBOTO_BK_32
    FONT_MODE_DIGITAL_7_R_16,  // FONT_DIGITAL_7_R_16
    FONT_MODE_DIGITAL_DISMAY_R_12, // FONT_DIGITAL_DISMAY_R_12
    FONT_MODE_DIGITAL_DISMAY_R_16, // FONT_DIGITAL_DISMAY_R_16
    FONT_MODE_COUNT            // Must be last - used for counting
};

// Constants for mode management
#define MAX_FONT_MODES static_cast<int>(FontModeType::FONT_MODE_COUNT)

class ModeFont {
public:
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    void run();
    void cleanup();
    void activate(); // Called when the mode becomes active
    void nextFont();
    void prevFont();
    void setFontMode(FontModeType mode); // Direct sub-mode access for MQTT 추가
    
private:
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;

    const unsigned long INITIAL_DISPLAY_DELAY_MS = 3000; // 3 seconds

    // Member variables
    int currentFontIndex;
    bool isSampleTextScrolling;
    int sampleTextScrollX;
    unsigned long lastSampleTextScrollTime;
    int sampleTextWidth;
    unsigned long sampleTextInitialDisplayUntil;
    bool sampleTextHasInitialDisplayPassed;

    bool isFontNameScrolling;
    int fontNameScrollX;
    unsigned long lastFontNameScrollTime;
    int fontNameWidth;
    unsigned long fontNameInitialDisplayUntil;
    bool fontNameHasInitialDisplayPassed;

    void displayFont();
    void updateFontMetricsAndScrollState();
};

#endif