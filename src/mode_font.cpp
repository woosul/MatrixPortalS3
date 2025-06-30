#include "mode_font.h"
#include "font_manager.h"
#include "common.h"
#include "utils.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

static const char* const SAMPLE_TEXT_CONTENT = "ABCXYZ123890!@#$";
const int SAMPLE_TEXT_Y_POS = 22; // Y position for the sample text
const unsigned long SCROLL_INTERVAL_MS = 50; // Scroll speed in milliseconds

void ModeFont::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    
    currentFontIndex = 0; // Start with the first font (FONT_DEFAULT)
    // activate() will call updateFontMetricsAndScrollState()
    Serial.println("Font mode setup complete.");
}

void ModeFont::activate() {
    // updateFontMetricsAndScrollState will use currentFontIndex (0) to initialize
    // scrolling parameters for the first font.
    updateFontMetricsAndScrollState(); 
    Serial.println("Font mode activated and metrics updated");
}

void ModeFont::run() {
    unsigned long currentTime = millis();

    // Handle initial display delay and scrolling for Sample Text
    if (!sampleTextHasInitialDisplayPassed && currentTime >= sampleTextInitialDisplayUntil) {
        sampleTextHasInitialDisplayPassed = true;
        lastSampleTextScrollTime = currentTime; // Start scroll timing now
    }

    if (sampleTextHasInitialDisplayPassed && FONT_SAMPLE_TEXT_SCROLL_ENABLED && isSampleTextScrolling) {
        if (currentTime - lastSampleTextScrollTime >= SCROLL_INTERVAL_MS) {
            sampleTextScrollX--;
            // When text fully scrolled left (its right edge is at or past screen left edge)
            if (sampleTextScrollX + sampleTextWidth < 0) {
                sampleTextScrollX = MATRIX_WIDTH - 3; // Reset to scroll in from physical X=62 (logical X=61)
            }
            lastSampleTextScrollTime = currentTime;
        }
    }

    // Handle initial display delay and scrolling for Font Name
    if (!fontNameHasInitialDisplayPassed && currentTime >= fontNameInitialDisplayUntil) {
        fontNameHasInitialDisplayPassed = true;
        lastFontNameScrollTime = currentTime; // Start scroll timing now
    }

    if (fontNameHasInitialDisplayPassed && FONT_NAME_SCROLL_ENABLED && isFontNameScrolling) {
        if (currentTime - lastFontNameScrollTime >= SCROLL_INTERVAL_MS) {
            fontNameScrollX--;
            if (fontNameScrollX + fontNameWidth < 0) {
                fontNameScrollX = MATRIX_WIDTH - 3; // Reset to scroll in from physical X=62 (logical X=61)
            }
            lastFontNameScrollTime = currentTime;
        }
    }
    displayFont();
}

void ModeFont::cleanup() {
    Serial.println("Font mode cleanup");
    m_matrix->fillScreen(0);
    if (m_utils) {
        m_utils->displayShow();
    }
}

void ModeFont::displayFont() {
    m_matrix->fillScreen(0);
    
    // Get current font info
    FontType currentFontType = static_cast<FontType>(currentFontIndex);
    FontInfo fontInfo = FontManager::getFontInfo(currentFontType);
    
    // --- Display Font Name ---
    m_matrix->setFont(); // Reset to default system font for labels
    m_matrix->setTextColor(m_matrix->color565(128, 128, 128)); // Gray
    m_matrix->setTextSize(1);

    int currentFontNameX = 2; // Default left-aligned X with 2px margin
    if (fontNameHasInitialDisplayPassed && FONT_NAME_SCROLL_ENABLED && isFontNameScrolling) {
        currentFontNameX = fontNameScrollX;
    }
    if (m_utils) {
        m_utils->setCursorTopBased(currentFontNameX, 4, false);
    } else {
        m_matrix->setCursor(setPhysicalX(currentFontNameX), 4); // Fallback
    }
    m_matrix->print(fontInfo.name);
    
    // --- Display Sample Text ---
    uint16_t textColor = m_matrix->color565(255, 191, 0); // Amber for sample text

    // Simplified font handling - GFX fonts only
    const GFXfont* font = FontManager::getFont(currentFontType);
    if (font) {
        m_matrix->setFont(font);
    } else {
        m_matrix->setFont(); // Fallback to system font if current font is null
    }
    m_matrix->setTextColor(textColor);
    
    int currentSampleTextX = 2; // Default left-aligned X with 2px margin
    if (sampleTextHasInitialDisplayPassed && FONT_SAMPLE_TEXT_SCROLL_ENABLED && isSampleTextScrolling) {
        currentSampleTextX = sampleTextScrollX;
    }
    if (m_utils) {
        m_utils->setCursorTopBased(currentSampleTextX, SAMPLE_TEXT_Y_POS, true);
    } else {
        m_matrix->setCursor(setPhysicalX(currentSampleTextX), SAMPLE_TEXT_Y_POS); // Fallback
    }
    m_matrix->print(SAMPLE_TEXT_CONTENT);
    
    // --- Display Font Index (Navigation Info) --- Right Aligned
    m_matrix->setFont(); // Reset to default font
    m_matrix->setTextColor(m_matrix->color565(80, 80, 80)); // Dark Gray
    m_matrix->setTextSize(1);

    char indexStr[16]; // Buffer for "X/Y" string
    snprintf(indexStr, sizeof(indexStr), "%d/%d", currentFontIndex + 1, FontManager::getFontCount());

    int16_t x1_idx, y1_idx; // Renamed to avoid conflict
    uint16_t w_idx, h_idx;  // Renamed to avoid conflict
    m_matrix->getTextBounds(indexStr, 0, 0, &x1_idx, &y1_idx, &w_idx, &h_idx);
    int indexX = MATRIX_WIDTH - w_idx - 2; // 2px margin from right
    if (indexX < 0) indexX = 0; // Prevent negative X, ensure it's at least 0

    if (m_utils) {
        m_utils->setCursorTopBased(indexX, 54, false);
    } else {
        m_matrix->setCursor(setPhysicalX(indexX), 54); // Fallback
    }
    m_matrix->print(indexStr);

    // Apply masks to ensure scrolling text is only visible in logical X=2 to X=61.
    // MATRIX_X_OFFSET is assumed to be 1 as per config.h.
    // Physical columns 0, 1, 2 should be black.
    // Physical column 63 should be black.
    // Text visible physical columns: 3 to 62.

    m_matrix->fillRect(0, 0, 2 + MATRIX_X_OFFSET, MATRIX_HEIGHT, 0); // Left border (blacks out physical 0, 1, 2 if MATRIX_X_OFFSET=1)
    m_matrix->fillRect(MATRIX_WIDTH - 1, 0, 1, MATRIX_HEIGHT, 0);    // Right border (blacks out physical 63 if MATRIX_WIDTH=64)

    if (m_utils) {
        m_utils->displayShow();
    }
}

void ModeFont::nextFont() {
    currentFontIndex = (currentFontIndex + 1) % MAX_FONT_MODES; // FontManager::getFontCount() -> MAX_FONT_MODES
    updateFontMetricsAndScrollState();
    FontInfo fontInfo = FontManager::getFontInfo(static_cast<FontType>(currentFontIndex));
    Serial.printf("Font changed to: %s (%d)\n", fontInfo.name, currentFontIndex);
}

void ModeFont::prevFont() {
    currentFontIndex = (currentFontIndex - 1 + MAX_FONT_MODES) % MAX_FONT_MODES; // FontManager::getFontCount() -> MAX_FONT_MODES
    updateFontMetricsAndScrollState();
    FontInfo fontInfo = FontManager::getFontInfo(static_cast<FontType>(currentFontIndex));
    Serial.printf("Font changed to: %s (%d)\n", fontInfo.name, currentFontIndex);
}

void ModeFont::updateFontMetricsAndScrollState() {
    // --- Update Sample Text Metrics ---
    FontType newFontType = static_cast<FontType>(currentFontIndex);
    const GFXfont* sampleFont = FontManager::getFont(newFontType);
    if (sampleFont) {
        m_matrix->setFont(sampleFont); // Set font to measure sample text
    } else {
        m_matrix->setFont(); // Use default system font
    }

    int16_t st_x1, st_y1;
    uint16_t st_w, st_h;
    m_matrix->getTextBounds(SAMPLE_TEXT_CONTENT, 0, 0, &st_x1, &st_y1, &st_w, &st_h);
    sampleTextWidth = st_w;

    if (FONT_SAMPLE_TEXT_SCROLL_ENABLED && sampleTextWidth > MATRIX_WIDTH) {
        isSampleTextScrolling = true;
        // sampleTextScrollX will be set to 2 for initial display, then scroll.
    } else {
        isSampleTextScrolling = false;
    }
    sampleTextScrollX = 2; // Initial X position (left-aligned 2px margin)
    sampleTextInitialDisplayUntil = millis() + INITIAL_DISPLAY_DELAY_MS;
    sampleTextHasInitialDisplayPassed = false;
    // lastSampleTextScrollTime will be set in run() when initial display period passes

    // --- Update Font Name Metrics ---
    m_matrix->setFont(); // Use default system font for font name
    FontInfo fontInfo = FontManager::getFontInfo(newFontType);
    
    int16_t fn_x1, fn_y1;
    uint16_t fn_w, fn_h;
    m_matrix->getTextBounds(fontInfo.name, 0, 0, &fn_x1, &fn_y1, &fn_w, &fn_h);
    fontNameWidth = fn_w;

    if (FONT_NAME_SCROLL_ENABLED && fontNameWidth > MATRIX_WIDTH) {
        isFontNameScrolling = true;
        // fontNameScrollX will be set to 2 for initial display, then scroll.
    } else {
        isFontNameScrolling = false;
    }
    fontNameScrollX = 2; // Initial X position (left-aligned 2px margin)
    fontNameInitialDisplayUntil = millis() + INITIAL_DISPLAY_DELAY_MS;
    fontNameHasInitialDisplayPassed = false;
    // lastFontNameScrollTime will be set in run() when initial display period passes
}

void ModeFont::setFontMode(FontModeType mode) {
    int newIndex = static_cast<int>(mode);
    
    // Validate mode index
    if (newIndex < 0 || newIndex >= MAX_FONT_MODES) {
        Serial.printf("Invalid font mode: %d. Valid range: 0-%d\n", newIndex, MAX_FONT_MODES - 1);
        return;
    }
    
    // Set new font index
    currentFontIndex = newIndex;
    
    // Update metrics and scroll state for new font
    updateFontMetricsAndScrollState();
    
    // Get font name for logging
    FontInfo fontInfo = FontManager::getFontInfo(static_cast<FontType>(currentFontIndex));
    Serial.printf("MQTT: Set font mode to %s (index: %d)\n", fontInfo.name, newIndex);
    
    if (m_utils && m_utils->isSoundFeedbackEnabled()) {
        m_utils->playSingleTone();
    }
}