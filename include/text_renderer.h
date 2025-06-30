#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <Arduino.h>
#include <Adafruit_GFX.h> // For GFXfont
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "font_manager.h"
#include "config.h" // For FontType, MATRIX_WIDTH, MATRIX_HEIGHT, SCROLL_CYCLE_END_WAIT_MS etc.
#include <string>   // For std::string

// Enums previously in utils.h or specific to text rendering
enum TextAlignment { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };
enum TextBaseLine { BASELINE_TOP, BASELINE_MIDDLE, BASELINE_BOTTOM, BASELINE_GFX_DEFAULT };
enum ScrollDirection { SCROLL_HORIZONTAL_LEFT, SCROLL_HORIZONTAL_RIGHT, SCROLL_VERTICAL_UP, SCROLL_VERTICAL_DOWN };


class TextRenderer {
public:
    TextRenderer();
    void setup(MatrixPanel_I2S_DMA* matrixPtr, FontManager* fontManagerPtr);

    FontType getCurrentAppliedFontType() const { return currentAppliedFontType; } // Ensure this is public

    // --- Basic Text Drawing Methods ---
    void drawText(const String& text, int x, int y, uint16_t color,
                  FontType fontType = FONT_PRIMARY,
                  TextAlignment alignment = ALIGN_LEFT,
                  TextBaseLine baseLine = BASELINE_GFX_DEFAULT,
                  int16_t wrapWidth = 0, // 0 for no wrap (basic GFX print)
                  int16_t xOffset = 0, int16_t yOffset = 0);

    void drawText(const char* text, int x, int y, uint16_t color,
                  FontType fontType = FONT_PRIMARY,
                  TextAlignment alignment = ALIGN_LEFT,
                  TextBaseLine baseLine = BASELINE_GFX_DEFAULT,
                  int16_t wrapWidth = 0, // 0 for no wrap (basic GFX print)
                  int16_t xOffset = 0, int16_t yOffset = 0);

    // --- Wrapped Text Display & Vertical Scrolling Methods (ported from Utils) ---
    void drawWrappedText(const char* text, int x, int y, int maxWidth, uint16_t color,
                         TextAlignment align = ALIGN_LEFT, int charSpacing = 1, int lineSpacing = 0,
                         bool isCustomFont = false, int displayAreaHeight = -1);
    void updateVerticalScroll();
    void stopVerticalScroll();
    bool isVerticalScrollingActive() const;

    // --- Horizontal Scrolling Text Methods ---
    void startHorizontalScroll(const String& text, int16_t yPos, uint16_t color, int speedMillis,
                               FontType fontType = FONT_PRIMARY,
                               ScrollDirection direction = SCROLL_HORIZONTAL_LEFT,
                               bool loop = true, int16_t scrollWidth = MATRIX_WIDTH);
    void updateHorizontalScroll();
    void stopHorizontalScroll();
    bool isHorizontalScrollingActive() const;
    void setHorizontalScrollLoop(bool loop);
    void setHorizontalScrollSpeed(int speedMillis);
    void setHorizontalScrollFont(FontType fontType);
    void setHorizontalScrollColor(uint16_t color);
    void setHorizontalScrollText(const String& text);
    String getHorizontalScrollText() const;

    // --- Text Metric Methods ---
    void getTextBounds(const char* str, FontType fontType, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    int16_t getTextWidth(const char* text, FontType fontType);
    int16_t getFontHeight(FontType fontType); // Typically yAdvance
    int16_t getFontAscent(FontType fontType);
    int16_t getFontDescent(FontType fontType);

    // --- Getters for scroll parameters (ported from Utils) ---
    int getScrollTextX() const;
    int getScrollTextY() const;
    int getDisplayableScrollHeight() const;
    int getScrollTextMaxWidth() const;


private:
    MatrixPanel_I2S_DMA* matrix; // Renamed from m_matrix
    FontManager* fontManager;    // Renamed from m_fontManager

    // --- Helpers for Basic Text Drawing & Metrics ---
    const GFXfont* getCurrentFont(FontType fontType);
    FontType currentAppliedFontType; // Track the font type currently applied to the matrix
    void applyFontToMatrix(FontType fontType);

    // --- State and Helpers for Horizontal Scrolling ---
    String hScrollTextContent;
    int16_t hScrollCurrentX;
    int16_t hScrollTargetY;
    uint16_t hScrollTextColor;
    FontType hScrollFontType;
    const GFXfont* hActiveScrollFont;
    int hScrollSpeedMillis;
    ScrollDirection hScrollDirection;
    unsigned long hLastScrollUpdateTime;
    int16_t hScrollTextPixelWidth;
    int16_t hScrollFontPixelHeight;
    int16_t hScrollFontAscent;
    bool hScrollingActiveFlag;
    bool hScrollLoopFlag;
    int16_t hScrollWindowWidth;
    void resetHorizontalScrollState();
    void calculateHorizontalScrollTextDimensions();

    // --- State and Helpers for Wrapped Text & Vertical Scrolling (ported from Utils) ---
    bool verticalScrollingActive;
    std::string scrollTextBuffer; // Use std::string for potentially long messages
    int scrollTextX;
    int scrollTextY;
    int scrollTextMaxWidth;
    uint16_t scrollTextColor;
    FontType scrollFontType; // Added for vertical scroll font type
    TextAlignment scrollTextAlign;
    int scrollCharSpacing;
    int scrollLineSpacing;
    bool scrollIsCustomFont;
    int currentScrollYOffset;
    int totalScrollTextHeight;
    int displayableScrollHeight;
    unsigned long lastScrollActionTime;
    bool isWaitingBeforeScroll;
    bool isWaitingAfterCycleEnd;

    // Constants for vertical scrolling behavior
    static const unsigned long SCROLL_WAIT_DURATION_MS = 3000;
    static const unsigned long SCROLL_STEP_INTERVAL_MS = 100;
    // SCROLL_CYCLE_END_WAIT_MS is used from config.h

    // Internal helper methods for wrapped text and vertical scrolling
    void drawWrappedTextInternal(const char* text, int x, int y, int maxWidth, uint16_t color, TextAlignment align, int charSpacing, int lineSpacing, bool isCustomFont);
    int calculateTextHeight(const char* text, int maxWidth, int charSpacing, int lineSpacing, bool isCustomFont);
    uint16_t calculateCurrentLineWidth(const std::string& line, int spacing);
    const char* findNextWordOrNewline(const char* text, std::string& word); // Helper for word wrapping
};

#endif // TEXTRENDERER_H
