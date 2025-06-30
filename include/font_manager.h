#ifndef FONTS_H
#define FONTS_H

#include <Arduino.h>
#include "gfxfont.h"

// Include all custom font files
#include "../fonts/Org-r-6.h"
#include "../fonts/Agency-r-8.h"
#include "../fonts/Agency-r-10.h"
#include "../fonts/Agency-r-12.h"
#include "../fonts/Agency-r-16.h"
#include "../fonts/Agency-b-16.h"
#include "../fonts/Roboto-bk-20.h"
#include "../fonts/Roboto-bk-24.h"
#include "../fonts/Roboto-bk-32.h"
#include "../fonts/Digital7-r-16.h"
#include "../fonts/DigitalDismay-r-12.h"
#include "../fonts/DigitalDismay-r-16.h"

// Font enumeration for easy switching - GFX fonts only
enum FontType {
    FONT_DEFAULT = 0,
    FONT_AGENCY_R_8,
    FONT_AGENCY_R_10, 
    FONT_AGENCY_R_12,
    FONT_AGENCY_R_16,
    FONT_AGENCY_B_16,
    FONT_ORG_R_6,
    FONT_ROBOTO_BK_20,
    FONT_ROBOTO_BK_24,
    FONT_ROBOTO_BK_32,
    FONT_DIGITAL_7_R_16,
    FONT_DIGITAL_DISMAY_R_12,
    FONT_DIGITAL_DISMAY_R_16, // Added for completeness
    FONT_COUNT  // Keep this last for counting
};

// Font structure for management - GFX fonts only
struct FontInfo {
    const GFXfont* font;
    const char* name;
    const char* description;
    uint8_t size;
};

// Font management class - GFX fonts only
class FontManager {
public:
    // Get font by type
    static const GFXfont* getFont(FontType type);
    
    // Get font info
    static FontInfo getFontInfo(FontType type);
    
    // Get font name
    static const char* getFontName(FontType type);
    
    // Get total font count
    static uint8_t getFontCount();
    
    // Validate font type
    static bool isValidFont(FontType type);
    
private:
    // Private helper methods
    static FontInfo createFontInfo(FontType type);
    static const GFXfont* getFontPointer(FontType type);
};

// Global font manager instance
extern FontManager fontManager;

#endif
