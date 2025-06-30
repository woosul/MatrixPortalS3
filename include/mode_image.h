/**
 * @file mode_image.h
 * @brief Image display mode for LED matrix panel
 * 
 * Provides PNG image loading, scaling, and display functionality for LED matrix panels (64x64).
 * Supports both automatic slideshow and manual navigation through images by mqtt messages and IR remote control.
 */

#ifndef MODE_IMAGE_H
#define MODE_IMAGE_H

#include "config.h"
#include "utils.h"

// Prevent macro conflicts between PNGdec and AnimatedGIF libraries
#ifdef INTELSHORT
    #undef INTELSHORT
#endif
#ifdef INTELLONG
    #undef INTELLONG
#endif
#ifdef MOTOSHORT
    #undef MOTOSHORT
#endif
#ifdef MOTOLONG
    #undef MOTOLONG
#endif

#include <PNGdec.h>
#include <FS.h>
#include <LittleFS.h>

/**
 * @brief Enumeration of available image content types
 * 
 * Each enum value corresponds to a specific PNG file in the /images directory
 */
enum class ImageContentType {
    IMAGE_01_WOOSUL = 0,    // QR code for Woosul service
    IMAGE_02_PEAKNINE,      // QR code for Peaknine service
    IMAGE_03_MOOSE,         // Moose logo standard size
    IMAGE_04_MOOSE_BIG,     // Moose logo large size
    IMAGE_CONTENT_COUNT     // Total count of available images
};

// Image management constants
#define MAX_IMAGE_CONTENTS static_cast<int>(ImageContentType::IMAGE_CONTENT_COUNT)
#define IMAGE_DIRECTORY "/images"

// PNG pixel type constants for color format identification
#define PNG_PIXEL_GRAYSCALE       0
#define PNG_PIXEL_TRUECOLOR       2
#define PNG_PIXEL_INDEXED         3
#define PNG_PIXEL_GRAYSCALE_ALPHA 4
#define PNG_PIXEL_TRUECOLOR_ALPHA 6

/**
 * @brief Image display mode class for LED matrix panels
 * 
 * Handles PNG image loading, scaling, positioning, and rendering on LED matrix displays.
 * Provides automatic slideshow functionality and manual navigation controls.
 */
class ModeImage {
private:
    // Core system references
    Utils* m_utils;                    // Utility functions and display management
    MatrixPanel_I2S_DMA* m_matrix;     // LED matrix panel interface
    PNG png;                           // PNG decoder instance
    
    // Image state management
    ImageContentType currentImage;     // Currently selected image
    bool autoMode;                     // Automatic slideshow enabled flag
    unsigned long lastAutoSwitch;      // Timestamp of last auto switch
    unsigned long autoSwitchInterval;  // Interval between auto switches (ms)
    bool isInitialized;                // Module initialization state
    bool imageLoaded;                  // Current image load status

    // Image processing buffers and dimensions
    uint16_t* imageBuffer;            // RGB565 frame buffer for display
    uint16_t imageWidth;              // Original image width in pixels
    uint16_t imageHeight;             // Original image height in pixels
    uint16_t displayWidth;            // Scaled display width
    uint16_t displayHeight;           // Scaled display height
    int16_t offsetX;                  // Horizontal centering offset
    int16_t offsetY;                  // Vertical centering offset
    float scaleX;                     // Horizontal scaling factor
    float scaleY;                     // Vertical scaling factor
    
    // File system interface
    File currentFile;                 // Active PNG file handle
    String currentFilePath;           // Path to current image file
    bool useMatrixXOffset;            // Hardware offset correction flag
    
    // Internal processing methods
    void displayInfo();
    bool loadImage(ImageContentType imageType);
    void calculateDisplayParameters();
    void renderImage();
    const char* getImageFileName(ImageContentType imageType);
    String getImageDisplayName(ImageContentType imageType);
    
    // PNG decoder callback functions (static for C-style callbacks)
    static void pngDraw(PNGDRAW *pDraw);
    static int32_t pngRead(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen);
    static int32_t pngSeek(PNGFILE *pFile, int32_t iPosition);
    static void* pngOpen(const char *szFilename, int32_t *pFileSize);
    static void pngClose(void *pHandle);
    
    // Image processing utilities
    void drawPixels(int y, int width, uint8_t *pixels, int pixelType, int bpp);
    uint16_t getPixelColor(uint8_t* rawBuffer, int x, int y, int imageWidth, int pixelType, int bpp);
    
public:
    /**
     * @brief Default constructor
     */
    ModeImage();
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~ModeImage();

    /**
     * @brief Initialize image mode with system components
     * @param utils_ptr Pointer to utility functions
     * @param matrix_ptr Pointer to LED matrix interface
     */
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    
    /**
     * @brief Activate image mode and load initial content
     */
    void activate();
    
    /**
     * @brief Main execution loop for image mode
     */
    void run();
    
    /**
     * @brief Clean up resources and reset state
     */
    void cleanup();
    
    /**
     * @brief Handle infrared remote control commands
     * @param command IR command code
     */
    void handleIRCommand(uint32_t command);
    
    /**
     * @brief Get human-readable mode name
     * @return Mode display name
     */
    String getModeName() const { return "Image Viewer"; }

    /**
     * @brief Navigate to next image in sequence
     */
    void nextImage();
    
    /**
     * @brief Navigate to previous image in sequence
     */
    void prevImage();
    
    /**
     * @brief Set specific image for display
     * @param imageType Target image type to display
     */
    void setImage(ImageContentType imageType);
    
    // Getter and setter methods
    /**
     * @brief Get currently displayed image type
     * @return Current image type
     */
    ImageContentType getCurrentImage() const { return currentImage; }
    
    /**
     * @brief Check if automatic slideshow is enabled
     * @return Auto mode status
     */
    bool isAutoMode() const { return autoMode; }
    
    /**
     * @brief Enable or disable automatic slideshow
     * @param enabled Auto mode state
     */
    void setAutoMode(bool enabled) { autoMode = enabled; }
    
    /**
     * @brief Set interval for automatic image switching
     * @param interval Time between switches in milliseconds
     */
    void setAutoSwitchInterval(unsigned long interval) { autoSwitchInterval = interval; }
};

#endif // MODE_IMAGE_H