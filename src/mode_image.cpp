/**
 * @file mode_image.cpp
 * @brief Implementation of image display mode for LED matrix panels
 * 
 * Provides PNG image loading, scaling, and display functionality with automatic
 * slideshow capabilities and manual navigation controls.
 */

#include "mode_image.h"
#include "common.h"
#include <algorithm>

// Static pointer for PNG decoder callbacks
static ModeImage* s_currentInstance = nullptr;

/**
 * @brief Default constructor - initializes all member variables
 */
ModeImage::ModeImage() {
    m_utils = nullptr;
    m_matrix = nullptr;
    currentImage = ImageContentType::IMAGE_01_WOOSUL;
    autoMode = false;
    lastAutoSwitch = 0;
    autoSwitchInterval = 5000; // Default 5 second interval
    isInitialized = false;
    imageLoaded = false;
    imageBuffer = nullptr;
    imageWidth = 0;
    imageHeight = 0;
    displayWidth = 0;
    displayHeight = 0;
    offsetX = 0;
    offsetY = 0;
    scaleX = 1.0f;
    scaleY = 1.0f;
    useMatrixXOffset = true;  // Enable hardware offset correction by default
}

/**
 * @brief Destructor - ensures proper cleanup of resources
 */
ModeImage::~ModeImage() {
    cleanup();
}

/**
 * @brief Initialize image mode with system components
 * @param utils_ptr Pointer to utility functions and display management
 * @param matrix_ptr Pointer to LED matrix panel interface
 */
void ModeImage::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    Serial.println("Setting up Image Mode");

    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    
    // Validate essential system pointers
    if (!m_utils || !m_matrix) {
        Serial.println("Image Mode: Invalid pointers provided");
        return;
    }

    // Initialize file system for image storage access
    if (!LittleFS.begin()) {
        Serial.println("Image Mode: LittleFS initialization failed");
        return;
    }
    
    // Allocate RGB565 frame buffer for image processing
    imageBuffer = (uint16_t*)malloc(MATRIX_WIDTH * MATRIX_HEIGHT * sizeof(uint16_t));
    if (!imageBuffer) {
        Serial.println("Image Mode: Failed to allocate image buffer");
        return;
    }
    
    // Set static instance for PNG decoder callbacks
    s_currentInstance = this;
    isInitialized = true;
    
    Serial.println("Image Mode: Setup complete");
}

/**
 * @brief Activate image mode and load initial content
 */
void ModeImage::activate() {
    if (!isInitialized) {
        Serial.println("Image Mode: Not initialized, cannot activate");
        return;
    }
    
    Serial.println("Activating Image Mode");
    
    // Reset auto-switch timer
    lastAutoSwitch = millis();
    
    // Load the currently selected image
    if (!loadImage(currentImage)) {
        Serial.println("Image Mode: Failed to load initial image");
        displayInfo();
    }
}

/**
 * @brief Main execution loop for image mode
 */
void ModeImage::run() {
    if (!isInitialized || !m_utils || !m_matrix) {
        return;
    }

    // Handle automatic image switching
    if (autoMode && (millis() - lastAutoSwitch > autoSwitchInterval)) {
        nextImage();
        lastAutoSwitch = millis();
    }
    
    // Render current content to display
    if (imageLoaded) {
        renderImage();
    } else {
        displayInfo();
    }
    
    // Brief delay to reduce CPU usage
    delay(50);
}

/**
 * @brief Clean up resources and reset module state
 */
void ModeImage::cleanup() {
    Serial.println("Cleaning up Image Mode");
    
    // Free allocated image buffer
    if (imageBuffer) {
        free(imageBuffer);
        imageBuffer = nullptr;
    }
    
    // Reset state flags
    imageLoaded = false;
    isInitialized = false;
    s_currentInstance = nullptr;
}

/**
 * @brief Navigate to next image in sequence
 */
void ModeImage::nextImage() {
    if (!isInitialized) return;
    
    // Calculate next image index with wraparound
    int nextIndex = (static_cast<int>(currentImage) + 1) % MAX_IMAGE_CONTENTS;
    currentImage = static_cast<ImageContentType>(nextIndex);
    
    Serial.printf("Image Mode: Switching to next image: %s\n", 
                  getImageDisplayName(currentImage).c_str());
    
    // Load the new image
    loadImage(currentImage);
}

/**
 * @brief Navigate to previous image in sequence
 */
void ModeImage::prevImage() {
    if (!isInitialized) return;
    
    // Calculate previous image index with wraparound
    int prevIndex = (static_cast<int>(currentImage) - 1 + MAX_IMAGE_CONTENTS) % MAX_IMAGE_CONTENTS;
    currentImage = static_cast<ImageContentType>(prevIndex);
    
    Serial.printf("Image Mode: Switching to previous image: %s\n", 
                  getImageDisplayName(currentImage).c_str());
    
    // Load the new image
    loadImage(currentImage);
}

/**
 * @brief Set specific image for display
 * @param imageType Target image type to display
 */
void ModeImage::setImage(ImageContentType imageType) {
    if (!isInitialized) return;
    
    // Validate image type range
    if (imageType >= ImageContentType::IMAGE_CONTENT_COUNT) {
        Serial.println("Image Mode: Invalid image type");
        return;
    }
    
    currentImage = imageType;
    Serial.printf("Image Mode: Switching to image: %s\n", 
                  getImageDisplayName(currentImage).c_str());
    
    loadImage(currentImage);
}

/**
 * @brief Load and decode PNG image from file system
 * @param imageType Type of image to load
 * @return true if image loaded successfully, false otherwise
 */
bool ModeImage::loadImage(ImageContentType imageType) {
    if (!isInitialized || !imageBuffer) {
        Serial.println("LoadImage: Not initialized or buffer is null");
        return false;
    }
    
    // Get filename for the requested image type
    const char* filename = getImageFileName(imageType);
    if (!filename) {
        Serial.println("Image Mode: Invalid filename");
        imageLoaded = false;
        return false;
    }
    
    // Construct full file path
    currentFilePath = String(IMAGE_DIRECTORY) + "/" + filename;
    Serial.printf("LoadImage: Attempting to load %s\n", currentFilePath.c_str());
    
    // Verify file exists in file system
    if (!LittleFS.exists(currentFilePath)) {
        Serial.printf("Image Mode: File not found: %s\n", currentFilePath.c_str());
        imageLoaded = false;
        return false;
    }
    
    // Initialize PNG decoder with file callbacks
    int result = png.open(currentFilePath.c_str(), pngOpen, pngClose, pngRead, pngSeek, ModeImage::pngDraw);
    if (result != PNG_SUCCESS) {
        Serial.printf("Image Mode: PNG open failed with code: %d\n", result);
        imageLoaded = false;
        return false;
    }
    
    // Extract image properties
    imageWidth = png.getWidth();
    imageHeight = png.getHeight();
    
    // Log detailed PNG file information
    Serial.printf("Image Mode: Loaded %s (%dx%d)\n", filename, imageWidth, imageHeight);
    Serial.printf("PNG Info: Color Type=%d, Bit Depth=%d, Has Alpha=%s\n", 
                 png.getPixelType(), png.getBpp(), 
                 png.hasAlpha() ? "Yes" : "No");

    int pixelType = png.getPixelType();
    int bpp = png.getBpp();
    
    // Calculate display scaling and positioning parameters
    calculateDisplayParameters();
    Serial.printf("LoadImage: Display params - size(%dx%d), offset(%d,%d), scale(%.2f,%.2f)\n",
                 displayWidth, displayHeight, offsetX, offsetY, scaleX, scaleY);
    
    // Initialize image buffer with transparent/black pixels
    memset(imageBuffer, 0, MATRIX_WIDTH * MATRIX_HEIGHT * sizeof(uint16_t));
    Serial.println("LoadImage: Image buffer cleared.");

    uint8_t* tempRawBuffer = nullptr;
    
    // Handle downscaling - allocate temporary buffer for full image decode
    if (scaleX < 1.0f) {
        Serial.println("LoadImage: Downscaling required. Allocating temporary raw buffer.");
        
        // Calculate bytes per pixel based on PNG color type
        size_t bytesPerPixel = 0;
        if (pixelType == PNG_PIXEL_GRAYSCALE || pixelType == PNG_PIXEL_INDEXED) bytesPerPixel = 1;
        else if (pixelType == PNG_PIXEL_TRUECOLOR) bytesPerPixel = 3;
        else if (pixelType == PNG_PIXEL_TRUECOLOR_ALPHA) bytesPerPixel = 4;
        
        if (bytesPerPixel == 0) {
            Serial.printf("Image Mode: Unsupported pixel type for raw buffer: %d\n", pixelType);
            png.close();
            imageLoaded = false;
            return false;
        }

        // Allocate temporary buffer (prefer PSRAM if available)
        size_t rawBufferSize = (size_t)imageWidth * imageHeight * bytesPerPixel;
        tempRawBuffer = (uint8_t*)ps_malloc(rawBufferSize);
        if (!tempRawBuffer) {
            tempRawBuffer = (uint8_t*)malloc(rawBufferSize);
        }
        
        if (!tempRawBuffer) {
            Serial.println("Image Mode: Failed to allocate temporary raw buffer for scaling.");
            png.close();
            imageLoaded = false;
            return false;
        }
        
        // Configure PNG decoder to use temporary buffer
        png.setBuffer(tempRawBuffer);
        Serial.println("LoadImage: Decoding into temporary raw buffer...");
    } else {
        // No scaling needed - use direct callback rendering
        png.setBuffer(nullptr);
        Serial.println("LoadImage: Decoding using pngDraw callback...");
    }
    
    // Decode the PNG image
    result = png.decode(nullptr, 0);
    png.close();
    
    if (result == PNG_SUCCESS) {
        Serial.println("LoadImage: PNG decode successful.");
        
        // Post-process for downscaled images
        if (tempRawBuffer) {
            Serial.println("LoadImage: Resampling from temporary raw buffer to final image buffer.");
            
            // Resample pixels using nearest neighbor algorithm
            for (int y = 0; y < displayHeight; y++) {
                for (int x = 0; x < displayWidth; x++) {
                    // Map display coordinates to source image coordinates
                    int sourceX = (int)(x / scaleX);
                    int sourceY = (int)(y / scaleY);
                    
                    // Clamp to source image bounds
                    sourceX = std::min(sourceX, (int)imageWidth - 1);
                    sourceY = std::min(sourceY, (int)imageHeight - 1);

                    // Extract and convert pixel color
                    uint16_t rgb565 = getPixelColor(tempRawBuffer, sourceX, sourceY, imageWidth, pixelType, bpp);
                    imageBuffer[(offsetY + y) * MATRIX_WIDTH + (offsetX + x)] = rgb565;
                }
            }
            
            // Release temporary buffer
            free(tempRawBuffer);
            tempRawBuffer = nullptr;
        }
        
        imageLoaded = true;
        return true;
    } else {
        Serial.printf("Image Mode: PNG decode failed with code: %d\n", result);
        
        // Clean up on failure
        if (tempRawBuffer) {
            free(tempRawBuffer);
            tempRawBuffer = nullptr;
        }
        
        imageLoaded = false;
        return false;
    }
}

/**
 * @brief Render loaded image to LED matrix display
 */
void ModeImage::renderImage() {
    if (!imageLoaded || !m_matrix || !imageBuffer) {
        return;
    }
    
    // Clear display before rendering new content
    m_matrix->fillScreen(0);
    
    // Copy image buffer to matrix with hardware offset correction
    for (int y = 0; y < displayHeight; y++) {
        for (int x = 0; x < displayWidth; x++) {
            // Calculate logical buffer coordinates
            int logicalY = offsetY + y;
            int logicalX = offsetX + x;
            int bufferIndex = logicalY * MATRIX_WIDTH + logicalX;
            
            // Bounds check for buffer access
            if (bufferIndex < 0 || bufferIndex >= (MATRIX_WIDTH * MATRIX_HEIGHT)) continue;
            
            uint16_t pixel = imageBuffer[bufferIndex];
            
            // Only render non-transparent pixels
            if (pixel != 0x0000) {
                // Validate physical display coordinates
                if (logicalX >= 0 && logicalX < MATRIX_WIDTH && 
                    logicalY >= 0 && logicalY < MATRIX_HEIGHT) {
                    m_matrix->drawPixel(setPhysicalX(logicalX), logicalY, pixel);
                }
            }
        }
    }
    
    // Update display buffer
    m_matrix->flipDMABuffer();
}

/**
 * @brief Display mode information when no image is loaded
 */
void ModeImage::displayInfo() {
    if (!m_matrix || !m_utils) return;
    
    // Clear display
    m_matrix->fillScreen(0);
    
    // Configure text display properties
    uint16_t color = m_utils->hexToRgb565(0x00FF00); // Green text
    m_matrix->setTextColor(color);
    m_matrix->setFont();
    m_matrix->setTextSize(1);
    
    // Display mode name (centered)
    String modeName = "IMAGE";
    int x = m_utils->calculateTextCenterX(modeName.c_str(), MATRIX_WIDTH);
    m_matrix->setCursor(x, 20);
    m_matrix->print(modeName);
    
    // Display current image name (centered)
    String imageName = getImageDisplayName(currentImage);
    x = m_utils->calculateTextCenterX(imageName.c_str(), MATRIX_WIDTH);
    m_matrix->setCursor(x, 35);
    m_matrix->print(imageName);
    
    // Update display buffer
    m_matrix->flipDMABuffer();
}

/**
 * @brief Calculate display parameters for image scaling and positioning
 */
void ModeImage::calculateDisplayParameters() {
    // Calculate initial scaling factors
    float calculatedScaleX = (float)MATRIX_WIDTH / imageWidth;
    float calculatedScaleY = (float)MATRIX_HEIGHT / imageHeight;
    
    if (imageWidth <= MATRIX_WIDTH && imageHeight <= MATRIX_HEIGHT) {
        // Image fits within matrix dimensions - no scaling needed
        this->scaleX = 1.0f;
        this->scaleY = 1.0f;
    } else {
        // Image exceeds matrix size - scale down uniformly to maintain aspect ratio
        this->scaleX = std::min(calculatedScaleX, calculatedScaleY) - 0.1f; // Slight reduction to prevent clipping
        this->scaleY = this->scaleX;
    }
    
    // Calculate final display dimensions
    displayWidth = (int)(imageWidth * this->scaleX);
    displayHeight = (int)(imageHeight * this->scaleY);
    
    // Calculate centering offsets
    offsetX = (MATRIX_WIDTH - displayWidth) / 2;
    offsetY = (MATRIX_HEIGHT - displayHeight) / 2;
    
    // Hardware offset correction is applied during rendering, not here
    useMatrixXOffset = true;
}

/**
 * @brief Process decoded PNG line data into image buffer
 * @param y Line number being processed
 * @param width Number of pixels in the line
 * @param pixels Raw pixel data for the line
 * @param pixelType PNG color format type
 * @param bpp Bits per pixel
 */
void ModeImage::drawPixels(int y, int width, uint8_t *pixels, int pixelType, int bpp) {
    if (!imageBuffer || !pixels) {
        Serial.println("DrawPixels: Buffer or pixels is null");
        return;
    }
    
    // Calculate target display row with vertical offset
    int displayY = offsetY + y;
    if (displayY < 0 || displayY >= MATRIX_HEIGHT) return;

    // Determine processing bounds for current line
    int pixelsToProcess = std::min((int)imageWidth, MATRIX_WIDTH);
    pixelsToProcess = std::min(pixelsToProcess, width);

    // Calculate source pixel format byte size
    size_t bytesPerSourcePixel = 0;
    if (pixelType == PNG_PIXEL_GRAYSCALE || pixelType == PNG_PIXEL_INDEXED) bytesPerSourcePixel = 1;
    else if (pixelType == PNG_PIXEL_TRUECOLOR) bytesPerSourcePixel = 3;
    else if (pixelType == PNG_PIXEL_TRUECOLOR_ALPHA) bytesPerSourcePixel = 4;
    else return; // Unsupported pixel format

    // Process each pixel in the line
    for (int x = 0; x < pixelsToProcess; x++) {
        // Calculate target display column with horizontal offset
        int displayX = offsetX + x;
        
        // Skip pixels outside matrix bounds
        if (displayX < 0 || displayX >= MATRIX_WIDTH) continue;
        
        // Extract raw pixel data for current position
        uint8_t* currentPixelData = pixels + (x * bytesPerSourcePixel);
        
        // Convert raw pixel data to RGB565 format
        // RGB565 format compared to Hex color:
        // - Bit allocation: RGB565 uses 16 bits (5-red, 6-green, 5-blue) vs Hex uses 24 bits (8-8-8)
        // - Color depth: RGB565 supports 65,536 colors vs Hex supports 16.7 million colors
        // - Memory efficiency: RGB565 uses half the memory (2 bytes) compared to Hex RGB (3 bytes)
        // - Green emphasis: RGB565 allocates extra bit to green channel (human eye sensitivity)
        // - Common usage: RGB565 for embedded displays/MCUs, Hex for web/desktop applications

        uint16_t rgb565 = 0x0000;
        switch (pixelType) {
            case PNG_PIXEL_GRAYSCALE:
                rgb565 = m_utils->rgb888to565(currentPixelData[0], currentPixelData[0], currentPixelData[0]);
                break;
            case PNG_PIXEL_TRUECOLOR:
                rgb565 = m_utils->rgb888to565(currentPixelData[0], currentPixelData[1], currentPixelData[2]);
                break;
            case PNG_PIXEL_TRUECOLOR_ALPHA:
                if (currentPixelData[3] == 0) rgb565 = 0x0000; // Transparent pixel
                else rgb565 = m_utils->rgb888to565(currentPixelData[0], currentPixelData[1], currentPixelData[2]);
                break;
            case PNG_PIXEL_INDEXED:
                {
                    uint8_t index = currentPixelData[0];
                    uint8_t* palette = png.getPalette();
                    if (palette) {
                        rgb565 = m_utils->rgb888to565(palette[index * 3], palette[index * 3 + 1], palette[index * 3 + 2]);
                    }
                }
                break;
            default:
                break;
        }
        
        // Store converted pixel in image buffer
        size_t bufferIndex = (size_t)displayY * MATRIX_WIDTH + displayX;
        if (bufferIndex < (MATRIX_WIDTH * MATRIX_HEIGHT)) {
            imageBuffer[bufferIndex] = rgb565;
        }
    }
}

/**
 * @brief Extract RGB565 color from raw pixel buffer
 * @param rawBuffer Raw pixel data buffer
 * @param x Pixel X coordinate
 * @param y Pixel Y coordinate
 * @param imageWidth Source image width
 * @param pixelType PNG color format type
 * @param bpp Bits per pixel (unused)
 * @return RGB565 color value
 */
uint16_t ModeImage::getPixelColor(uint8_t* rawBuffer, int x, int y, int imageWidth, int pixelType, int bpp) {
    // Calculate bytes per pixel based on color format
    size_t bytesPerPixel = 0;
    if (pixelType == PNG_PIXEL_GRAYSCALE || pixelType == PNG_PIXEL_INDEXED) bytesPerPixel = 1;
    else if (pixelType == PNG_PIXEL_TRUECOLOR) bytesPerPixel = 3;
    else if (pixelType == PNG_PIXEL_TRUECOLOR_ALPHA) bytesPerPixel = 4;
    else return 0x0000; // Unsupported format

    // Calculate pixel offset in buffer
    size_t pixelOffset = (size_t)(y * imageWidth + x) * bytesPerPixel;
    uint8_t* pixelData = rawBuffer + pixelOffset;

    uint8_t r, g, b, a;

    // Convert pixel data based on format type
    switch (pixelType) {
        case PNG_PIXEL_GRAYSCALE:
            r = g = b = pixelData[0];
            return m_utils->rgb888to565(r, g, b);
            
        case PNG_PIXEL_TRUECOLOR:
            r = pixelData[0];
            g = pixelData[1];
            b = pixelData[2];
            return m_utils->rgb888to565(r, g, b);
            
        case PNG_PIXEL_TRUECOLOR_ALPHA:
            r = pixelData[0];
            g = pixelData[1];
            b = pixelData[2];
            a = pixelData[3];
            if (a == 0) return 0x0000; // Transparent pixel
            return m_utils->rgb888to565(r, g, b);
            
        case PNG_PIXEL_INDEXED:
            {
                uint8_t index = pixelData[0];
                uint8_t* palette = png.getPalette();
                if (palette) {
                    r = palette[index * 3];
                    g = palette[index * 3 + 1];
                    b = palette[index * 3 + 2];
                    return m_utils->rgb888to565(r, g, b);
                }
                return 0x0000; // Fallback if palette unavailable
            }
            
        default:
            return 0x0000;
    }
}

// Static callback functions for PNG decoder file operations

/**
 * @brief Open PNG file for reading (static callback)
 * @param szFilename Path to PNG file
 * @param pFileSize Pointer to store file size
 * @return File handle or nullptr on failure
 */
void* ModeImage::pngOpen(const char *szFilename, int32_t *pFileSize) {
    File* pFile = new File();
    *pFile = LittleFS.open(szFilename, "r");
    if (*pFile) {
        *pFileSize = pFile->size();
        return pFile;
    } else {
        delete pFile;
        return nullptr;
    }
}

/**
 * @brief Close PNG file handle (static callback)
 * @param pHandle File handle to close
 */
void ModeImage::pngClose(void *pHandle) {
    if (pHandle) {
        File* pFile = (File*)pHandle;
        pFile->close();
        delete pFile;
    }
}

/**
 * @brief Read data from PNG file (static callback)
 * @param pFile PNG file structure
 * @param pBuf Buffer to read into
 * @param iLen Number of bytes to read
 * @return Number of bytes actually read
 */
int32_t ModeImage::pngRead(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    File* file = (File*)pFile->fHandle;
    if (file && file->available()) {
        return file->read(pBuf, iLen);
    }
    return 0;
}

/**
 * @brief Seek to position in PNG file (static callback)
 * @param pFile PNG file structure
 * @param iPosition Target file position
 * @return New file position
 */
int32_t ModeImage::pngSeek(PNGFILE *pFile, int32_t iPosition) {
    File* file = (File*)pFile->fHandle;
    if (file) {
        return file->seek(iPosition);
    }
    return 0;
}

/**
 * @brief PNG decode callback for processing image lines (static callback)
 * @param pDraw PNG draw structure with line data
 */
void ModeImage::pngDraw(PNGDRAW *pDraw) {
    if (s_currentInstance) {
        s_currentInstance->drawPixels(pDraw->y, pDraw->iWidth, pDraw->pPixels, 
                                      pDraw->iPixelType, pDraw->iBpp);
    }
}

/**
 * @brief Handle infrared remote control commands
 * @param command IR command code received
 */
void ModeImage::handleIRCommand(uint32_t command) {
    // IR command handling is implemented in main.cpp's handleIRCommand function
    // This method provides interface for future direct IR handling if needed
}

/**
 * @brief Get filename for specified image type
 * @param imageType Image content type enum
 * @return Filename string or nullptr if invalid type
 */
const char* ModeImage::getImageFileName(ImageContentType imageType) {
    switch (imageType) {
        case ImageContentType::IMAGE_01_WOOSUL:   return "01_woosulQr.png";
        case ImageContentType::IMAGE_02_PEAKNINE: return "02_peaknineQr.png";
        case ImageContentType::IMAGE_03_MOOSE:     return "03_mooseLogo.png";
        case ImageContentType::IMAGE_04_MOOSE_BIG: return "04_mooseLogoBig.png";
        default: return nullptr;
    }
}

/**
 * @brief Get display name for specified image type
 * @param imageType Image content type enum
 * @return Human-readable display name
 */
String ModeImage::getImageDisplayName(ImageContentType imageType) {
    switch (imageType) {
        case ImageContentType::IMAGE_01_WOOSUL:   return "WOOSUL";
        case ImageContentType::IMAGE_02_PEAKNINE: return "PEAKNINE";
        case ImageContentType::IMAGE_03_MOOSE:     return "MOOSE";
        case ImageContentType::IMAGE_04_MOOSE_BIG: return "MOOSE_BIG";
        default: return "UNKNOWN";
    }
}
