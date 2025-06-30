#include "font_manager.h"

// Static method implementations using explicit mapping - GFX fonts only
const GFXfont* FontManager::getFontPointer(FontType type) {
    switch(type) {
        case FONT_DEFAULT:
            return nullptr;
        case FONT_AGENCY_R_8:
            return &AGENCYR8pt7b;
        case FONT_AGENCY_R_10:
            return &AGENCYR10pt7b;
        case FONT_AGENCY_R_12:
            return &AGENCYR12pt7b;
        case FONT_AGENCY_R_16:
            return &AGENCYR16pt7b;
        case FONT_AGENCY_B_16:
            return &AGENCYB16pt7b;
        case FONT_ORG_R_6:
            return &Org_01;
        case FONT_ROBOTO_BK_20:
            return &Roboto_Black_20;
        case FONT_ROBOTO_BK_24:
            return &Roboto_Black_24;
        case FONT_ROBOTO_BK_32:
            return &Roboto_Black_32;
        case FONT_DIGITAL_7_R_16:
            return &digital_716pt7b; // Assuming Digital7_r16pt7b is defined in the included headers
        case FONT_DIGITAL_DISMAY_R_12:
            return &Digital_Dismay12pt7b; // Assuming DigitalDismay_r
        case FONT_DIGITAL_DISMAY_R_16:
            return &Digital_Dismay16pt7b; // Assuming DigitalDismay_r16pt7b is defined in the included headers
        default:
            return nullptr;
    }
}

FontInfo FontManager::createFontInfo(FontType type) {
    switch(type) {
        case FONT_DEFAULT:
            return {nullptr, "System", "System default font", 8};
        case FONT_AGENCY_R_8:
            return {&AGENCYR8pt7b, "Agency-R-8", "Agency FB Regular 8pt converted font", 8};
        case FONT_AGENCY_R_10:
            return {&AGENCYR10pt7b, "Agency-R-10", "Agency FB Regular 10pt converted font", 10};
        case FONT_AGENCY_R_12:
            return {&AGENCYR12pt7b, "Agency-R-12", "Agency FB Regular 12pt converted font", 12};
        case FONT_AGENCY_R_16:
            return {&AGENCYR16pt7b, "Agency-R-16", "Agency FB Regular 16pt converted font", 16};
        case FONT_AGENCY_B_16:
            return {&AGENCYB16pt7b, "Agency-B-16", "Agency FB Bold 16pt converted font", 16};
        case FONT_ORG_R_6:
            return {&Org_01, "Org-R-6", "Org v01 tiny stylized font", 6};
        case FONT_ROBOTO_BK_20:
            return {&Roboto_Black_20, "Roboto-BK-20", "Roboto Black 20pt converted font", 20};
        case FONT_ROBOTO_BK_24:
            return {&Roboto_Black_24, "Roboto-BK-24", "Roboto Black 24pt converted font", 24};
        case FONT_ROBOTO_BK_32:
            return {&Roboto_Black_32, "Roboto-BK-32", "Roboto Black 32pt converted font", 32};
        case FONT_DIGITAL_7_R_16:
            return {&digital_716pt7b, "Digital-7-R-16", "Digital 7 Regular 16pt converted font", 16};
        case FONT_DIGITAL_DISMAY_R_12:
            return {&Digital_Dismay12pt7b, "Digital-Dismay-R-12", "Digital Dismay Regular 12pt converted font", 12};
        case FONT_DIGITAL_DISMAY_R_16:
            return {&Digital_Dismay16pt7b, "Digital-Dismay-R-16", "Digital Dismay Regular 16pt converted font", 16};
        // Add more fonts as needed
        default:
            return {nullptr, "Unknown", "Unknown font", 8};
    }
}

const GFXfont* FontManager::getFont(FontType type) {
    if (!isValidFont(type)) { // Removed comment - obvious from code
        return nullptr;
    }
    return getFontPointer(type);
}

FontInfo FontManager::getFontInfo(FontType type) {
    if (!isValidFont(type)) {
        return createFontInfo(FONT_DEFAULT); // Removed comment - obvious from code
    }
    return createFontInfo(type);
}

const char* FontManager::getFontName(FontType type) {
    FontInfo info = getFontInfo(type);
    return info.name;
}

uint8_t FontManager::getFontCount() {
    return FONT_COUNT; // Removed comment - obvious from code
}

bool FontManager::isValidFont(FontType type) {
    return (type >= 0 && type < FONT_COUNT);
}
