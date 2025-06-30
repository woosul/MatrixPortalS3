#include "text_renderer.h"
// #include "common.h" // For LOG_D, LOG_E if used
#include "font_manager.h"  // For FontManager, though TextRenderer gets it via setup

TextRenderer::TextRenderer()
    : matrix(nullptr), fontManager(nullptr),
      // Horizontal scroll init
      hScrollCurrentX(0), hScrollTargetY(0), hScrollTextColor(0xFFFF),
      hScrollFontType(FONT_PRIMARY), hActiveScrollFont(nullptr), hScrollSpeedMillis(100),
      hScrollDirection(SCROLL_HORIZONTAL_LEFT), hLastScrollUpdateTime(0),
      hScrollTextPixelWidth(0), hScrollFontPixelHeight(0), hScrollFontAscent(0),
      hScrollingActiveFlag(false), hScrollLoopFlag(true), hScrollWindowWidth(MATRIX_WIDTH),
      // Vertical scroll init (ported from Utils)
      verticalScrollingActive(false), scrollTextBuffer(""),
      scrollTextX(0), scrollTextY(0), scrollTextMaxWidth(0), scrollTextColor(0), currentAppliedFontType(FONT_DEFAULT),
      scrollFontType(FONT_PRIMARY), scrollTextAlign(ALIGN_LEFT), scrollCharSpacing(0), scrollLineSpacing(0),
      scrollIsCustomFont(false), currentScrollYOffset(0), totalScrollTextHeight(0),
      displayableScrollHeight(0), lastScrollActionTime(0),
      isWaitingBeforeScroll(false), isWaitingAfterCycleEnd(false) {
}

void TextRenderer::setup(MatrixPanel_I2S_DMA* matrixPtr, FontManager* fontManagerPtr) {
    matrix = matrixPtr;
    fontManager = fontManagerPtr;
    if (!matrix) {
        Serial.println("TextRenderer Error: Matrix pointer is null!");
    }
    if (!fontManager) {
        Serial.println("TextRenderer Error: FontManager pointer is null!");
    // } else if (!fontManager->isInitialized()) { // Assuming FontManager has an isInitialized method
        // Serial.println("TextRenderer Warning: FontManager not initialized during setup.");
    }
}

const GFXfont* TextRenderer::getCurrentFont(FontType type) {
    if (!fontManager) return nullptr;
    return fontManager->getFont(type); 
}

void TextRenderer::applyFontToMatrix(FontType type) {
    if (!matrix || !fontManager) return;
    const GFXfont* fontPtr = fontManager->getFont(type); 
    matrix->setFont(fontPtr);
    currentAppliedFontType = type; // Track the applied font type
}

void TextRenderer::drawText(const String& textVal, int x, int y, uint16_t color,
                            FontType type, TextAlignment alignment, TextBaseLine baseLine,
                            int16_t wrapWidth, int16_t xOffset, int16_t yOffset) {
    drawText(textVal.c_str(), x, y, color, type, alignment, baseLine, wrapWidth, xOffset, yOffset);
}

void TextRenderer::drawText(const char* textVal, int x, int y, uint16_t color,
                            FontType type, TextAlignment alignment, TextBaseLine baseLine,
                            int16_t wrapWidth, int16_t xOffset, int16_t yOffset) {
    if (!matrix || !fontManager || !textVal || strlen(textVal) == 0) return;

    // Save the font type that was on the matrix before this draw operation
    FontType previousFontType = currentAppliedFontType;
    applyFontToMatrix(type);
    // currentAppliedFontType is now 'type'
    matrix->setTextColor(color);

    int16_t text_x = x + xOffset;
    int16_t text_y = y + yOffset;

    int16_t x1_bounds, y1_bounds;
    uint16_t w_bounds, h_bounds;
    matrix->getTextBounds(textVal, 0, 0, &x1_bounds, &y1_bounds, &w_bounds, &h_bounds);

    if (alignment == ALIGN_CENTER) {
        text_x = x - w_bounds / 2 + xOffset;
    } else if (alignment == ALIGN_RIGHT) {
        text_x = x - w_bounds + xOffset;
    }

    switch (baseLine) {
        case BASELINE_TOP:    text_y = y - y1_bounds + yOffset; break;
        case BASELINE_MIDDLE: text_y = y - h_bounds / 2 - y1_bounds + yOffset; break;
        case BASELINE_BOTTOM: text_y = y - h_bounds - y1_bounds + yOffset; break;
        case BASELINE_GFX_DEFAULT:
        default:              text_y = y + yOffset; break;
    }

    matrix->setCursor(text_x, text_y);

    if (wrapWidth > 0 && w_bounds > wrapWidth) {
        char* mutableText = strdup(textVal);
        if (!mutableText) { matrix->print(textVal); applyFontToMatrix(previousFontType); return; }

        char* word = strtok(mutableText, " \n");
        String lineBuffer = "";
        int16_t currentLineWidth = 0;
        int16_t spaceWidthVal = getTextWidth(" ", type);
        int16_t fontYAdvance = getFontHeight(type);

        while (word != nullptr) {
            int16_t wordWidth = getTextWidth(word, type);
            if (currentLineWidth > 0 && (currentLineWidth + spaceWidthVal + wordWidth > wrapWidth)) {
                matrix->setCursor(text_x, text_y); // Use original text_x for each new line
                matrix->print(lineBuffer);
                lineBuffer = "";
                currentLineWidth = 0;
                text_y += fontYAdvance;
            }
            if (currentLineWidth > 0) {
                lineBuffer += " ";
                currentLineWidth += spaceWidthVal;
            }
            lineBuffer += word;
            currentLineWidth += wordWidth;
            word = strtok(nullptr, " \n");
        }
        if (lineBuffer.length() > 0) {
            matrix->setCursor(text_x, text_y); // Use original text_x
            matrix->print(lineBuffer);
        }
        free(mutableText);
    } else {
        matrix->print(textVal);
    }
    applyFontToMatrix(previousFontType); // Restore the previous font type
}

// --- Horizontal Scrolling Text ---
void TextRenderer::resetHorizontalScrollState() {
    hScrollingActiveFlag = false;
    hScrollTextContent = "";
    hActiveScrollFont = nullptr;
    hScrollTextPixelWidth = 0;
    hScrollFontPixelHeight = 0;
    hScrollFontAscent = 0;
}

void TextRenderer::calculateHorizontalScrollTextDimensions() {
    if (!fontManager || hScrollTextContent.isEmpty()) {
        resetHorizontalScrollState();
        return;
    }
    hActiveScrollFont = getCurrentFont(hScrollFontType);
    if (!hActiveScrollFont) {
        hActiveScrollFont = fontManager->getFont(FontType::FONT_DEFAULT); // Assuming FONT_SYSTEM_SMALL is in FontType
    }

    if (!matrix) { resetHorizontalScrollState(); return; }

    // Save the font type that was on the matrix
    FontType previousFontTypeOnMatrix = currentAppliedFontType;
    matrix->setFont(hActiveScrollFont ? hActiveScrollFont : nullptr); // Use GFX default if hActiveScrollFont is null

    int16_t x1, y1; uint16_t w, h;
    matrix->getTextBounds(hScrollTextContent.c_str(), 0, 0, &x1, &y1, &w, &h);
    hScrollTextPixelWidth = w;

    if (hActiveScrollFont) {
        hScrollFontPixelHeight = hActiveScrollFont->yAdvance;
        hScrollFontAscent = getFontAscent(hScrollFontType);
    } else { // System font
        hScrollFontPixelHeight = 8;
        hScrollFontAscent = 6;
    }
    matrix->setFont(fontManager->getFont(previousFontTypeOnMatrix)); // Restore previous font
    currentAppliedFontType = previousFontTypeOnMatrix; // Ensure tracking is correct
}

void TextRenderer::startHorizontalScroll(const String& textVal, int16_t yPos, uint16_t color, int speedMillis,
                               FontType type, ScrollDirection directionVal, bool loop, int16_t scrollW) {
    if (!matrix || !fontManager) return;
    stopHorizontalScroll();

    hScrollTextContent = textVal;
    hScrollTargetY = yPos;
    hScrollTextColor = color;
    hScrollSpeedMillis = speedMillis > 0 ? speedMillis : 10;
    hScrollFontType = type;
    hScrollDirection = directionVal;
    hScrollLoopFlag = loop;
    hScrollWindowWidth = scrollW > 0 ? scrollW : MATRIX_WIDTH;

    calculateHorizontalScrollTextDimensions();

    if (!hActiveScrollFont && hScrollFontType != FONT_DEFAULT) { // If a specific font was requested but not found
         // Serial.printf("Warning: Horizontal scroll font type %d not found, using system font.\n", type);
    }
    if (hScrollTextPixelWidth == 0 && !hScrollTextContent.isEmpty()) {
        // Serial.println("Warning: Horizontal scroll text width is 0, might not display.");
    }


    if (hScrollTargetY == -1) {
        hScrollTargetY = (MATRIX_HEIGHT / 2) + (hScrollFontPixelHeight / 2) - getFontDescent(hScrollFontType);
    }

    switch (hScrollDirection) {
        case SCROLL_HORIZONTAL_LEFT:  hScrollCurrentX = hScrollWindowWidth; break;
        case SCROLL_HORIZONTAL_RIGHT: hScrollCurrentX = -hScrollTextPixelWidth; break;
        default: hScrollCurrentX = hScrollWindowWidth; break; // Default to left
    }

    hScrollingActiveFlag = true;
    hLastScrollUpdateTime = millis();
}

void TextRenderer::updateHorizontalScroll() {
    if (!hScrollingActiveFlag || !matrix ) return;
    // No need to check hActiveScrollFont here, as system font is a fallback

    unsigned long currentTime = millis();
    if (currentTime - hLastScrollUpdateTime >= (unsigned long)hScrollSpeedMillis) {
        hLastScrollUpdateTime = currentTime;

        switch (hScrollDirection) {
            case SCROLL_HORIZONTAL_LEFT:
                hScrollCurrentX--;
                if (hScrollCurrentX < -hScrollTextPixelWidth) {
                    if (hScrollLoopFlag) hScrollCurrentX = hScrollWindowWidth;
                    else { stopHorizontalScroll(); return; }
                }
                break;
            case SCROLL_HORIZONTAL_RIGHT:
                hScrollCurrentX++;
                if (hScrollCurrentX > hScrollWindowWidth) {
                    if (hScrollLoopFlag) hScrollCurrentX = -hScrollTextPixelWidth;
                    else { stopHorizontalScroll(); return; }
                }
                break;
            default: break;
        }

        FontType previousFontTypeOnMatrix = currentAppliedFontType;
        matrix->setFont(hActiveScrollFont); // This is okay if hActiveScrollFont is nullptr (uses GFX default)
        // Note: hActiveScrollFont might be nullptr (system font). applyFontToMatrix handles this.
        matrix->setTextColor(hScrollTextColor);
        matrix->setCursor(hScrollCurrentX, hScrollTargetY);
        matrix->print(hScrollTextContent);
        applyFontToMatrix(previousFontTypeOnMatrix); // Restore
    }
}

void TextRenderer::stopHorizontalScroll() { resetHorizontalScrollState(); }
bool TextRenderer::isHorizontalScrollingActive() const { return hScrollingActiveFlag; }
void TextRenderer::setHorizontalScrollLoop(bool loop) { hScrollLoopFlag = loop; }
void TextRenderer::setHorizontalScrollSpeed(int speedMillis) { hScrollSpeedMillis = max(10, speedMillis); }
void TextRenderer::setHorizontalScrollFont(FontType type) {
    if (hScrollFontType != type) {
        hScrollFontType = type;
        if(hScrollingActiveFlag) calculateHorizontalScrollTextDimensions();
    }
}
void TextRenderer::setHorizontalScrollColor(uint16_t color) { hScrollTextColor = color; }
void TextRenderer::setHorizontalScrollText(const String& textVal) {
    if (hScrollTextContent != textVal || !hScrollingActiveFlag) {
        startHorizontalScroll(textVal, hScrollTargetY, hScrollTextColor, hScrollSpeedMillis, hScrollFontType, hScrollDirection, hScrollLoopFlag, hScrollWindowWidth);
    }
}
String TextRenderer::getHorizontalScrollText() const { return hScrollTextContent; }


// --- Wrapped Text & Vertical Scrolling Methods (ported from Utils) ---

// Helper to find the next word or newline (ported from Utils)
const char* TextRenderer::findNextWordOrNewline(const char* text, std::string& word) {
    word.clear();
    if (!text || *text == '\0') return nullptr;

    while (*text == ' ') text++; // Skip leading spaces
    if (*text == '\0') return nullptr;

    if (*text == '\n') {
        word = "\n";
        return text + 1;
    }

    const char* wordStart = text;
    while (*text != ' ' && *text != '\n' && *text != '\0') text++;
    word.assign(wordStart, text - wordStart);
    return text;
}

uint16_t TextRenderer::calculateCurrentLineWidth(const std::string& line, int spacing) {
    if (!matrix || line.empty()) return 0;
    uint16_t totalWidth = 0;
    // Font is assumed to be set by the caller context (e.g., calculateTextHeight or drawWrappedTextInternal)

    for (size_t i = 0; i < line.length(); ++i) {
        char charToMeasure = line[i];
        char singleCharMeasureStr[2] = {charToMeasure, '\0'};
        int16_t x1_m, y1_m; uint16_t w_m, h_m_char;
        matrix->getTextBounds(singleCharMeasureStr, 0, 0, &x1_m, &y1_m, &w_m, &h_m_char);
        totalWidth += w_m;
        if (i < line.length() - 1) {
            totalWidth += spacing;
        }
    }
    // matrix->setFont(originalFont); // Restore if changed, but it's not changed here
    return totalWidth;
}

int TextRenderer::calculateTextHeight(const char* text, int maxWidth, int charSpacingVal, int lineSpacingVal, bool isCustomFontVal) {
    if (!matrix || !text || maxWidth <= 0) return 0;

    FontType previousFontTypeOnMatrix = currentAppliedFontType;
    if (isCustomFontVal) {
        applyFontToMatrix(this->scrollFontType); // Use the font type stored for vertical scrolling
    } else {
        applyFontToMatrix(FONT_DEFAULT); // System font
    }

    matrix->setTextWrap(false);
    int totalHeightVal = 0;
    uint16_t fontLineHeightVal = 0;
    int16_t x1_fh, y1_fh; uint16_t w_fh, h_fh;

    matrix->getTextBounds("Aj", 0, 0, &x1_fh, &y1_fh, &w_fh, &h_fh); // Get height of current font
    fontLineHeightVal = h_fh;
    if (fontLineHeightVal == 0) fontLineHeightVal = 8; // Fallback for system font if bounds are weird

    const char* textPtr = text;
    std::string currentLineBuffer;
    bool firstLine = true;
    // int currentLinePixelWidth = 0; // Not needed for height calculation

    while (*textPtr) {
        std::string word;
        const char* nextTextPtr = findNextWordOrNewline(textPtr, word);

        if (word.empty() && nextTextPtr == textPtr) { textPtr = nextTextPtr; continue; }
        if (word.empty() && nextTextPtr != textPtr) { textPtr = nextTextPtr; continue; }

        if (word == "\n") {
            if (!firstLine) totalHeightVal += lineSpacingVal;
            totalHeightVal += fontLineHeightVal;
            firstLine = false;
            currentLineBuffer.clear();
            // currentLinePixelWidth = 0;
            textPtr = nextTextPtr;
            continue;
        }

        int16_t wordX1_calc, wordY1_calc; uint16_t wordPixelWidth_calc, wordPixelHeight_calc;
        matrix->getTextBounds(word.c_str(), 0, 0, &wordX1_calc, &wordY1_calc, &wordPixelWidth_calc, &wordPixelHeight_calc);

        uint16_t spaceWidthWithSpacing = 0;
        if (!currentLineBuffer.empty()) {
            int16_t space_x1, space_y1; uint16_t space_w, space_h_s;
            matrix->getTextBounds(" ", 0, 0, &space_x1, &space_y1, &space_w, &space_h_s);
            spaceWidthWithSpacing = space_w + charSpacingVal;
        }
        
        // Recalculate current line width with the font set for this operation
        uint16_t currentLinePixelWidthVal = calculateCurrentLineWidth(currentLineBuffer, charSpacingVal);
        int widthNeeded = currentLinePixelWidthVal + spaceWidthWithSpacing + wordPixelWidth_calc;


        if (widthNeeded <= maxWidth) {
            if (!currentLineBuffer.empty()) currentLineBuffer += " ";
            currentLineBuffer += word;
        } else {
            if (!currentLineBuffer.empty()) {
                if (!firstLine) totalHeightVal += lineSpacingVal;
                totalHeightVal += fontLineHeightVal;
                firstLine = false;
            }
            currentLineBuffer = word;
        }
        textPtr = nextTextPtr;
    }

    if (!currentLineBuffer.empty()) {
        if (!firstLine) totalHeightVal += lineSpacingVal;
        totalHeightVal += fontLineHeightVal;
    }
    applyFontToMatrix(previousFontTypeOnMatrix); // Restore original font
    return totalHeightVal;
}


void TextRenderer::drawWrappedTextInternal(const char* text, int x, int y_top_of_block, int maxWidth, uint16_t color, TextAlignment align, int charSpacingVal, int lineSpacingVal, bool isCustomFontVal) {
    if (!matrix || !text || maxWidth <= 0) return;
    // Font is assumed to be already set by the calling function (drawWrappedText)
    matrix->setTextColor(color);
    matrix->setTextWrap(false);

    int currentLineDrawingTopY = y_top_of_block;
    uint16_t fontLineHeightVal = 0;
    int16_t x1_fh, y1_fh; uint16_t w_fh, h_fh;

    matrix->getTextBounds("Aj", 0, 0, &x1_fh, &y1_fh, &w_fh, &h_fh);
    fontLineHeightVal = h_fh;
    if (fontLineHeightVal == 0) fontLineHeightVal = 8;

    const char* textPtr = text;
    std::string currentLineBuffer;
    // int currentLinePixelWidth = 0; // Not needed here, calculated on the fly

    auto drawCurrentLineLambda = [&]() { // Renamed to avoid conflict
        if (currentLineBuffer.empty()) return;

        uint16_t finalDisplayWidth = calculateCurrentLineWidth(currentLineBuffer, charSpacingVal);
        int lineDrawStartX = x;
        if (align == ALIGN_CENTER) {
            lineDrawStartX = x + (maxWidth - finalDisplayWidth) / 2;
        } else if (align == ALIGN_RIGHT) {
            lineDrawStartX = x + maxWidth - finalDisplayWidth;
        }
        if (lineDrawStartX < x) lineDrawStartX = x;

        int charPrintX = lineDrawStartX;
        
        // Calculate baseline for GFX drawing
        // y1_fh is the offset from baseline to top of char 'A' (usually negative)
        // So, baseline = top_y - y1_fh
        int lineBaselineY = currentLineDrawingTopY - y1_fh; 
        if (!isCustomFontVal) { // For system font, cursor Y is top.
            lineBaselineY = currentLineDrawingTopY;
        }


        if (currentLineDrawingTopY >= scrollTextY &&
            currentLineDrawingTopY < (scrollTextY + displayableScrollHeight)) {
            for (size_t i = 0; i < currentLineBuffer.length(); ++i) {
                char charToPrint = currentLineBuffer[i];
                int physicalPrintX = setPhysicalX(charPrintX);
                int viewportPhysXStart = setPhysicalX(scrollTextX);
                int viewportPhysXEnd = viewportPhysXStart + scrollTextMaxWidth;

                if (physicalPrintX >= viewportPhysXStart && physicalPrintX < viewportPhysXEnd) {
                    matrix->setCursor(physicalPrintX, lineBaselineY);
                    matrix->print(charToPrint);
                }
                int16_t x1_p, y1_p; uint16_t w_p, h_p_char;
                char tempStr[2] = {charToPrint, '\0'};
                matrix->getTextBounds(tempStr, 0, 0, &x1_p, &y1_p, &w_p, &h_p_char);
                charPrintX += w_p;
                if (i < currentLineBuffer.length() - 1) {
                    charPrintX += charSpacingVal;
                }
            }
        }
        currentLineDrawingTopY += fontLineHeightVal + lineSpacingVal;
        currentLineBuffer.clear();
    };

    while (*textPtr) {
        std::string word;
        const char* nextTextPtr = findNextWordOrNewline(textPtr, word);

        if (word.empty() && nextTextPtr == textPtr) { textPtr = nextTextPtr; continue; }
        if (word.empty() && nextTextPtr != textPtr) { textPtr = nextTextPtr; continue; }

        if (word == "\n") {
            drawCurrentLineLambda();
            // If the line was empty before the newline, it still consumes vertical space
            if (currentLineBuffer.empty() && calculateCurrentLineWidth(currentLineBuffer, charSpacingVal) == 0) {
                 currentLineDrawingTopY += fontLineHeightVal + lineSpacingVal;
            }
            textPtr = nextTextPtr;
            continue;
        }

        int16_t wordX1, wordY1; uint16_t wordPixelWidth, wordPixelHeight;
        matrix->getTextBounds(word.c_str(), 0, 0, &wordX1, &wordY1, &wordPixelWidth, &wordPixelHeight);

        uint16_t spaceWidthWithSpacing = 0;
        if (!currentLineBuffer.empty()) {
            int16_t space_x1, space_y1; uint16_t space_w, space_h_space;
            matrix->getTextBounds(" ", 0, 0, &space_x1, &space_y1, &space_w, &space_h_space);
            spaceWidthWithSpacing = space_w + charSpacingVal;
        }
        
        uint16_t currentLinePixelWidthVal = calculateCurrentLineWidth(currentLineBuffer, charSpacingVal);
        int widthNeededWithCurrentLine = currentLinePixelWidthVal + spaceWidthWithSpacing + wordPixelWidth;

        if (widthNeededWithCurrentLine <= maxWidth) {
            if (!currentLineBuffer.empty()) currentLineBuffer += " ";
            currentLineBuffer += word;
        } else {
            if (!currentLineBuffer.empty()) drawCurrentLineLambda();
            currentLineBuffer = word;
        }
        textPtr = nextTextPtr;
    }
    drawCurrentLineLambda();
    // matrix->setFont(originalFont); // Restore font if it was changed within this function
}


void TextRenderer::drawWrappedText(const char* text, int x, int y, int maxWidthVal, uint16_t color,
                                   TextAlignment align, int charSpacingVal, int lineSpacingVal,
                                   bool isCustomFontVal, int displayAreaHeightVal) {
    if (!matrix || !text || maxWidthVal <= 0) return;

    FontType previousFontTypeOnMatrix = currentAppliedFontType; // Save current font type
    if (isCustomFontVal) {
        // applyFontToMatrix(FONT_PRIMARY); // Or a specific font type if passed
                                         // This needs careful handling. Assume font is set by caller or use a default.
                                         // Or, TextRenderer needs to know which custom font to use for vertical scroll.
                                         // Or, TextRenderer needs to know which custom font to use.
                                         // scrollFontType is a member of TextRenderer, used for vertical scrolling text.
        applyFontToMatrix(this->scrollFontType); // Use the font type stored for vertical scrolling
    } else {
        matrix->setFont(nullptr); // System font
    }
    matrix->setTextSize(1);

    int totalHeightVal = calculateTextHeight(text, maxWidthVal, charSpacingVal, lineSpacingVal, isCustomFontVal);

    if (displayAreaHeightVal > 0 && totalHeightVal > displayAreaHeightVal) {
        verticalScrollingActive = true;
        scrollTextBuffer = text; // std::string assignment
        scrollTextX = x;
        scrollTextY = y;
        scrollTextMaxWidth = maxWidthVal;
        scrollTextColor = color;
        scrollTextAlign = align;
        scrollCharSpacing = charSpacingVal;
        scrollLineSpacing = lineSpacingVal;
        this->scrollIsCustomFont = isCustomFontVal; 

        totalScrollTextHeight = totalHeightVal;
        displayableScrollHeight = displayAreaHeightVal;
        currentScrollYOffset = 0;
        isWaitingBeforeScroll = true;
        isWaitingAfterCycleEnd = false;
        lastScrollActionTime = millis();

        matrix->fillRect(setPhysicalX(scrollTextX), scrollTextY, scrollTextMaxWidth, displayableScrollHeight, 0);
        drawWrappedTextInternal(scrollTextBuffer.c_str(), scrollTextX, scrollTextY - currentScrollYOffset, scrollTextMaxWidth, scrollTextColor, scrollTextAlign, scrollCharSpacing, scrollLineSpacing, scrollIsCustomFont);
        // displayShow() is called by Utils after this
    } else {
        verticalScrollingActive = false;
        displayableScrollHeight = (displayAreaHeightVal > 0) ? displayAreaHeightVal : MATRIX_HEIGHT;
        // Clear the area where text will be drawn if not scrolling to avoid artifacts
        matrix->fillRect(setPhysicalX(x), y, maxWidthVal, displayableScrollHeight, 0);
        drawWrappedTextInternal(text, x, y, maxWidthVal, color, align, charSpacingVal, lineSpacingVal, isCustomFontVal);
        // displayShow() is called by Utils
    }
    applyFontToMatrix(previousFontTypeOnMatrix); // Restore original font type
}

void TextRenderer::updateVerticalScroll() {
    if (!verticalScrollingActive || !matrix) return;

    unsigned long currentTime = millis();
    FontType previousFontTypeOnMatrix = currentAppliedFontType; // Save current font type

    // Ensure correct font is set for height calculations and drawing
    if (scrollIsCustomFont) {
        applyFontToMatrix(this->scrollFontType); // Assuming scrollFontType holds the correct font
    } else {
        matrix->setFont(nullptr); // System font
    }
    matrix->setTextSize(1);


    uint16_t fontLineHeightVal = 0;
    int16_t x1_fh_scroll, y1_fh_scroll; uint16_t w_fh_scroll, h_fh_scroll;
    matrix->getTextBounds("Aj", 0, 0, &x1_fh_scroll, &y1_fh_scroll, &w_fh_scroll, &h_fh_scroll);
    fontLineHeightVal = h_fh_scroll;
    if (fontLineHeightVal == 0) fontLineHeightVal = 8;

    if (isWaitingAfterCycleEnd) {
        if (currentTime - lastScrollActionTime >= SCROLL_CYCLE_END_WAIT_MS) {
            currentScrollYOffset = 0;
            isWaitingAfterCycleEnd = false;
            isWaitingBeforeScroll = true;
            lastScrollActionTime = currentTime;
        }
    } else if (isWaitingBeforeScroll) {
        if (currentTime - lastScrollActionTime >= SCROLL_WAIT_DURATION_MS) {
            isWaitingBeforeScroll = false;
            lastScrollActionTime = currentTime;
        }
    } else {
        if (currentTime - lastScrollActionTime >= SCROLL_STEP_INTERVAL_MS) {
            currentScrollYOffset++;
            lastScrollActionTime = currentTime;

            if (totalScrollTextHeight <= displayableScrollHeight) {
                currentScrollYOffset = 0;
                isWaitingBeforeScroll = true; // It fits, so just wait before "scrolling" (which means re-displaying)
                // isWaitingAfterCycleEnd = false; // Ensure this is false
            } else if (currentScrollYOffset >= (totalScrollTextHeight - displayableScrollHeight + fontLineHeightVal)) {
                isWaitingAfterCycleEnd = true;
                // currentScrollYOffset remains at its max value during this wait
            }
        }
    }

    matrix->fillRect(setPhysicalX(scrollTextX), scrollTextY, scrollTextMaxWidth, displayableScrollHeight, 0);
    // Font is already set above
    drawWrappedTextInternal(scrollTextBuffer.c_str(), scrollTextX, scrollTextY - currentScrollYOffset, scrollTextMaxWidth, scrollTextColor, scrollTextAlign, scrollCharSpacing, scrollLineSpacing, scrollIsCustomFont);
    
    applyFontToMatrix(previousFontTypeOnMatrix); // Restore original font type
    // displayShow() is called by Utils
}

void TextRenderer::stopVerticalScroll() {
    verticalScrollingActive = false;
    currentScrollYOffset = 0;
    scrollTextBuffer.clear();
    isWaitingBeforeScroll = false;
    isWaitingAfterCycleEnd = false;
}

bool TextRenderer::isVerticalScrollingActive() const {
    return verticalScrollingActive;
}

// --- Text Metrics ---
void TextRenderer::getTextBounds(const char* str, FontType type, int16_t x, int16_t y,
                                 int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    if (!matrix || !fontManager) {
        if (x1) *x1 = x; if (y1) *y1 = y; if (w) *w = 0; if (h) *h = 0;
        return;
    }
    FontType previousFontTypeOnMatrix = currentAppliedFontType;
    applyFontToMatrix(type);
    matrix->getTextBounds(str, x, y, x1, y1, w, h);
    applyFontToMatrix(previousFontTypeOnMatrix);
}

int16_t TextRenderer::getTextWidth(const char* textVal, FontType type) {
    int16_t x1, y1; uint16_t w, h;
    getTextBounds(textVal, type, 0, 0, &x1, &y1, &w, &h);
    return w;
}

int16_t TextRenderer::getFontHeight(FontType type) {
    const GFXfont* fontPtr = getCurrentFont(type);
    return fontPtr ? fontPtr->yAdvance : 8;
}

int16_t TextRenderer::getFontAscent(FontType type) {
    const GFXfont* fontPtr = getCurrentFont(type);
    if (!fontPtr) return 6;

    int16_t max_ascent = 0;
    bool found_char = false;
    for (uint8_t c = fontPtr->first; c <= fontPtr->last; c++) {
        GFXglyph *glyph = &fontPtr->glyph[c - fontPtr->first];
        if (glyph->height > 0) {
            if (-glyph->yOffset > max_ascent) {
                max_ascent = -glyph->yOffset;
            }
            found_char = true;
        }
    }
    return found_char ? max_ascent : (fontPtr->yAdvance * 0.75);
}

int16_t TextRenderer::getFontDescent(FontType type) {
    const GFXfont* fontPtr = getCurrentFont(type);
    if (!fontPtr) return 2;

    int16_t ascentVal = getFontAscent(type);
    if (fontPtr->yAdvance > ascentVal) {
        return fontPtr->yAdvance - ascentVal;
    }
    return fontPtr->yAdvance * 0.25; // Fallback
}

// --- Getters for scroll parameters (ported from Utils) ---
int TextRenderer::getScrollTextX() const {
    return scrollTextX;
}
int TextRenderer::getScrollTextY() const {
    return scrollTextY;
}
int TextRenderer::getDisplayableScrollHeight() const {
    return displayableScrollHeight;
}
int TextRenderer::getScrollTextMaxWidth() const {
    return scrollTextMaxWidth;
}
