#include "gif_player.h"
#include "common.h"

// Global instance
GifPlayer gifPlayer;
// Static instance pointer for C-style callbacks
GifPlayer* GifPlayer::s_instance = nullptr;

GifPlayer::GifPlayer() {
    m_utils = nullptr;
    m_matrix = nullptr;
    pFrameBuffer = nullptr;
    currentSource = GIF_BUILTIN;
    gifBackgroundIndex = 0;
    currentFileIndex = 0;
    playMode = GIF_LOOP;
    isPlaying = false;
    isPaused = false;
    hasEnded = false;
    isStaticImageDisplayed = false; // Flag for indicating if a static image is currently displayed
    totalFiles = 0;
    s_instance = this;
}

GifPlayer::~GifPlayer() {
    end();
}

bool GifPlayer::begin(Utils* utils, MatrixPanel_I2S_DMA* matrix) {
    // Serial.println("Initializing GIF Player...");  // DEBUG
    
    m_utils = utils;
    m_matrix = matrix;
    
    if (!m_utils || !m_matrix) {
        Serial.println("ERROR: GIF Player: Invalid parameters");  // Keep error messages
        return false;
    }

    // Clean up existing frame buffer if it exists
    if (pFrameBuffer) {
        free(pFrameBuffer);
        pFrameBuffer = nullptr;
    }

    // Allocate frame buffer - attempt to use PSRAM first
    size_t bufferSize = m_matrix->width() * m_matrix->height();
    
    // ESP32 with PSRAM support for larger frame buffers
    #ifdef ESP32
    if (psramFound()) {
        pFrameBuffer = (uint8_t *)ps_malloc(bufferSize);
        // Serial.println("DEBUG: Attempting to allocate frame buffer in PSRAM");
    }
    #endif
    
    // Fallback to regular RAM if PSRAM allocation fails
    if (!pFrameBuffer) {
        pFrameBuffer = (uint8_t *)malloc(bufferSize);
        // Serial.println("DEBUG: Allocating frame buffer in regular RAM");
    }
    
    if (!pFrameBuffer) {
        Serial.println("ERROR: GIF Player: Failed to allocate frame buffer");
        return false;
    }

    // Initialize frame buffer with background color
    memset(pFrameBuffer, 0, bufferSize);
    memset(currentPalette, 0, sizeof(currentPalette));
    
    // Serial.printf("INFO: Frame buffer allocated at address: %p, size: %d bytes\n", 
    //               pFrameBuffer, bufferSize);

    // Initialize AnimatedGIF library - order is important!
    gif.begin(LITTLE_ENDIAN_PIXELS);
    
    // Set frame buffer - call after begin()
    gif.setFrameBuf(pFrameBuffer);

    // Initialize LittleFS for file-based GIFs
    if (!LittleFS.begin()) {
        Serial.println("WARNING: LittleFS Mount Failed - only builtin GIFs will be available");  // DEBUG
    } else {
        // Serial.println("INFO: LittleFS mounted successfully");  // DEBUG
        scanGifFiles();
    }

    // Serial.println("INFO: GIF Player initialized successfully");  // DEBUG
    return true;
}

void GifPlayer::end() {
    safeMemoryCleanup();
    if (gifFile) {
        gifFile.close();
    }
    m_utils = nullptr;
    m_matrix = nullptr;
}

void GifPlayer::scanGifFiles() {
    totalFiles = 0;
    
    if (!LittleFS.exists(GIF_DIR)) {
        Serial.println(GIF_DIR " directory not found");  // DEBUG
        return;
    }
    
    File root = LittleFS.open(GIF_DIR);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open " GIF_DIR " directory");  // DEBUG
        return;
    }
    
    File file = root.openNextFile();
    while (file && totalFiles < MAX_GIF_FILES) {
        String fileName = file.name();
        if (fileName.endsWith(".gif") || fileName.endsWith(".GIF")) {
            gifFiles[totalFiles] = fileName;
            totalFiles++;
            Serial.println("Found GIF: " + fileName);  // DEBUG
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    
    // Serial.printf("Total GIF files found: %d\n", totalFiles);  // DEBUG
}


bool GifPlayer::loadBuiltinGif() {
    currentSource = GIF_BUILTIN;
    currentFileName = "homer_tiny";
    
    if (gif.open((uint8_t*)homer_tiny, sizeof(homer_tiny), GIFDraw)) {
        // Serial.println("Loaded builtin GIF: homer_tiny");  // DEBUG
        return true;
    } else {
        Serial.println("Failed to load builtin GIF");  // Keep error messages
        return false;
    }
}

bool GifPlayer::loadFileGif(const String& filename) {
    if (gifFile) {
        gifFile.close();
    }
    
    String fullPath = String(GIF_DIR) + "/" + filename;

    if (!LittleFS.exists(fullPath)) {
        Serial.println("File does not exist: " + fullPath);  // Keep error messages
        return false;
    }
    
    // Serial.printf("Loading GIF file: %s\n", fullPath.c_str());  // DEBUG

    if (gif.open(fullPath.c_str(), GIFOpen, GIFClose, GIFRead, GIFSeek, GIFDraw)) {
        currentFileName = filename; // Update current file name
        int canvasWidth = gif.getCanvasWidth();
        int canvasHeight = gif.getCanvasHeight();
        
        // Debug output (can be enabled for troubleshooting)
        // Serial.printf("DEBUG: FS GIF - Successfully loaded: %s\n", filename.c_str());
        // Serial.printf("DEBUG: FS GIF - Canvas size: %dx%d\n", canvasWidth, canvasHeight);
        // Serial.printf("DEBUG: FS GIF - Matrix size: %dx%d\n", m_matrix->width(), m_matrix->height());
        
        // File metadata check (can be enabled for debugging)
        // Serial.printf("DEBUG: FS GIF - File size: %d bytes\n", LittleFS.open(fullPath).size());
        
        return true;
    }
    
    int lastError = gif.getLastError();
    // Serial.printf("Failed to decode GIF: %s (error: %d)\n", filename.c_str(), lastError);  // DEBUG
    if (gifFile) gifFile.close();
    return false;
}

void GifPlayer::renderFrameToMatrix() {
    if (!m_matrix || !pFrameBuffer) {
        Serial.println("ERROR: renderFrameToMatrix - Invalid matrix or buffer");
        return;
    }

    // Ensure the matrix is cleared before rendering a new frame, especially for static images
    // This function is for animated GIFs. The clearing of the screen (if needed)
    // is handled by the GIFDraw callback based on the GIF's disposal method.
    
    int width = m_matrix->width();
    int height = m_matrix->height();
    int renderedPixels = 0;
    
    // Debug counter for periodic output (disabled in production)
    // static int debugCounter = 0;
    // bool shouldDebug = (debugCounter++ % 30 == 0);
    
    // Render only non-background pixels to matrix
    for (int y = 0; y < height; y++) {
        int rowOffset = y * width;
        for (int x = 0; x < width; x++) {
            int bufferIndex = rowOffset + x;
            
            if (bufferIndex >= width * height) {
                Serial.printf("ERROR: Buffer overflow detected at (%d,%d)\n", x, y);
                return;
            }
            
            uint8_t pixelIndex = pFrameBuffer[bufferIndex];
            
            // Render only non-background pixels (core rendering logic)
            if (pixelIndex != gifBackgroundIndex) {
                uint16_t color = currentPalette[pixelIndex];
                m_matrix->drawPixel(setPhysicalX(x), y, color);
                renderedPixels++;
                
                // Debug output for first few pixels (disabled in production)
                // if (shouldDebug && renderedPixels <= 3) {
                //     Serial.printf("DEBUG: Pixel (%d,%d) index:%d color:0x%04X\n", (x + 1) % MATRIX_WIDTH,
                //                   y, pixelIndex, color);
                // }
            }
            // Background pixels maintain previous frame content
        }
    }
    
    // Debug summary (disabled in production)
    // if (shouldDebug) {
    //     Serial.printf("DEBUG: renderFrameToMatrix - Rendered %d non-background pixels\n", renderedPixels);
    // }
    
    // Frame counter (disabled in production)
    // static int renderCount = 0;
    // Serial.printf("DEBUG: renderFrameToMatrix call #%d\n", ++renderCount);
}

/**
 * @brief Renders a static GIF frame to the matrix.
 * 
 * This function is specifically for static images. It clears the screen
 * and then draws every pixel from the frame buffer using the current palette,
 * ensuring no background residue and correct color representation for all pixels.
 */
void GifPlayer::renderStaticImageFrameToMatrix() {
    if (!m_matrix || !pFrameBuffer) {
        Serial.println("ERROR: renderStaticImageFrameToMatrix - Invalid matrix or buffer");
        return;
    }

    // For static images, always clear the screen completely before drawing.
    // This ensures no previous animation frames or artifacts remain.
    m_matrix->fillScreen(0);

    int width = m_matrix->width();
    int height = m_matrix->height();
    
    // DEBUG: Print first few palette entries for static images
    // Serial.println("DEBUG: Static Image Palette (first 16 entries):");
    // for (int i = 0; i < 16; i++) {
    //     Serial.printf("  Palette[%d] = 0x%04X\n", i, currentPalette[i]);
    // }
    // Draw every pixel from the frame buffer, ignoring gifBackgroundIndex.
    // This ensures all colors are rendered as defined in the palette,
    // preventing unwanted transparency or color inversions for static images.
    for (int y = 0; y < height; y++) {
        int rowOffset = y * width;
        for (int x = 0; x < width; x++) {
            int bufferIndex = rowOffset + x;
            if (bufferIndex >= width * height) { // Bounds check
                Serial.printf("ERROR: Buffer overflow detected at (%d,%d)\n", x, y);
                return;
            }
            uint8_t pixelIndex = pFrameBuffer[bufferIndex];
            // DEBUG: Print pixel index and color for first few pixels
            // if (y < 2 && x < 10) { // Only for first 2 rows, 10 columns
            //     Serial.printf("  Pixel(%d,%d) Index: %d, Color: 0x%04X\n", x, y, pixelIndex, currentPalette[pixelIndex]);
            // }
            uint16_t color = currentPalette[pixelIndex];
            m_matrix->drawPixel(x, y, color);
        }
    }
}

/**
 * @brief Start GIF playback with current content selection
 * 
 * Initializes playback state, loads GIF content, and renders first frame.
 * Supports both built-in and file-system based GIF content.
 * 
 * @return true if playback started successfully, false otherwise
 */
bool GifPlayer::play() {
    // Serial.println("DEBUG: play() called - starting GIF playback");
    stop(); // This will reset isStaticImageDisplayed = false;
    
    if (!m_utils || !m_matrix || !pFrameBuffer) {
        Serial.println("ERROR: GIF Player not properly initialized or frame buffer missing.");
        return false;
    }
    
    // Clear display buffers before starting new content
    // Serial.println("DEBUG: Clearing matrix buffers...");
    m_matrix->fillScreen(0);
    m_utils->displayShow(); 
    delay(50);
    
    // Initialize frame buffer and palette safely
    memset(pFrameBuffer, 0, m_matrix->width() * m_matrix->height());
    memset(currentPalette, 0, sizeof(currentPalette));
    gifBackgroundIndex = 0;

    bool loaded = false;
    
    // Determine content type and load accordingly
    // All gif files are opened through loadFileGif using the AnimatedGIF library.
    // And then gif file is static or animated based on playFrame result.
    if (currentSource == GIF_BUILTIN) {
        // Serial.println("DEBUG: Loading builtin GIF");
        loaded = loadBuiltinGif();
    } else {
        // Serial.printf("DEBUG: Loading file GIF index %d\n", currentFileIndex);
        if (currentFileIndex >= 0 && currentFileIndex < totalFiles) {
            // Serial.printf("DEBUG: Switching from builtin to file GIF: %s\n", gifFiles[currentFileIndex].c_str());
            loaded = loadFileGif(gifFiles[currentFileIndex]);
        } else {
            Serial.printf("ERROR: Invalid file index %d\n", currentFileIndex);
            loaded = false;
        }
    }

    // If file loading is successful, decode the first frame to determine if it's animated or static
    if (loaded) {
        // Serial.printf("DEBUG: Current file name for play(): %s\n", currentFileName.c_str()); // Debug log
        gif.reset(); // Reset GIF to start from the beginning (mandatory)

        int result = gif.playFrame(false, NULL); // Decode first frame
        Serial.printf("DEBUG: play() - First frame decode result: %d, error: %d\n", result, gif.getLastError());

        if (result == 1) { // Animated GIF: First frame decoded, more frames exist
            isPlaying = true;
            isPaused = false;
            hasEnded = false;
            isStaticImageDisplayed = false; // Flag for indicating if a static image is currently displayed
            Serial.println("INFO: Animated GIF playback started successfully. First frame displayed.");
        } else if (result == 0) { // Static GIF: First frame decoded, no more frames
            isStaticImageDisplayed = true;
            isPlaying = false;
            isPaused = false;
            hasEnded = true;
            Serial.println("INFO: Static GIF displayed successfully. No further frames.");

            // --- Generalized palette forcing logic ---
            // If determined to be a static image and the loaded palette is all black
            // (Workaround for AnimatedGIF library not reading palette correctly)
            bool allPaletteEntriesAreBlack = true;
            for (int i = 0; i < 16; i++) { // Checking only the first 16 entries is sufficient
                if (currentPalette[i] != 0x0000) {
                    allPaletteEntriesAreBlack = false;
                    break;
                }
            }
            if (allPaletteEntriesAreBlack) {
                Serial.println("WARNING: Detected all-black palette for static GIF. Forcing palette to B/W/DeepGray.");
                currentPalette[0] = 0x0000; // Black
                currentPalette[1] = 0xFFFF; // White
                currentPalette[2] = m_utils->hexToRgb565(0x333333); // DeepGray #333333
                currentPalette[3] = m_utils->hexToRgb565(0x565656); // DeepGray #565656
                currentPalette[4] = m_utils->hexToRgb565(0x808080); // DeepGray #808080
                currentPalette[5] = m_utils->hexToRgb565(0xAAAAAA); // DeepGray #AAAAAA
                currentPalette[6] = m_utils->hexToRgb565(0xCCCCCC); // DeepGray #CCCCCC
                // More indices can be filled as needed.
            }
            // In the case of a static GIF, if playFrame(false, NULL) returns 0, it means there are no more frames,
            // so there's no need to run the update() loop. The isStaticImageDisplayed flag controls this.
            // Therefore, no additional gif.close() is needed here.
        } else { // result < 0: Actual error occurred
            if (result <= 0) {
                Serial.printf("ERROR: Failed to decode first frame (error: %d).\n", gif.getLastError());
                stop();
                return false;
            }
        }
        
        // Render the decoded frame to the matrix based on whether it's static or animated
        if (isStaticImageDisplayed) {
            renderStaticImageFrameToMatrix(); // Use specific renderer for static images
        } else {
            renderFrameToMatrix(); // Use standard renderer for animated GIFs
        }
        if (!verifyFirstFrameRender()) {
            Serial.println("WARNING: First frame rendering verification failed");
        }
        m_utils->displayShow(); // Display the rendered frame
        delay(50); // Short delay after displaying the frame

        return true; // Playback initiated successfully (either animated or static)
    }
    
    Serial.println("ERROR: Failed to start GIF playback"); // File loading failed
    return false;
}

void GifPlayer::pause() {
    if (isPlaying) {
        isPaused = true;
        Serial.println("GIF paused");
    }
}

void GifPlayer::resume() {
    if (isPlaying && isPaused) {
        isPaused = false;
        Serial.println("GIF resumed");
    }
}

void GifPlayer::stop() {
    if (isPlaying || isStaticImageDisplayed) { // Stop even if a static image is displayed
        isPlaying = false;
        isPaused = false;
        hasEnded = false;
        isStaticImageDisplayed = false; // Reset static image flag
        gif.close();
        if (gifFile) {
            gifFile.close();
        }
        Serial.println("GIF stopped");
    }
}

void GifPlayer::nextContent() {
    if (currentSource == GIF_BUILTIN) {
        if (totalFiles > 0) {
            currentSource = GIF_FILE;
            currentFileIndex = 0;
        }
    } else {
        currentFileIndex++;
        if (currentFileIndex >= totalFiles) {
            currentSource = GIF_BUILTIN;
            currentFileIndex = 0;
        }
    }
    
    Serial.println("Switched to: " + getCurrentContentName());
    
    if (isPlaying || isStaticImageDisplayed) { // If currently playing or displaying static, start new content
        play();
    }
}

void GifPlayer::prevContent() {
    if (currentSource == GIF_BUILTIN) {
        if (totalFiles > 0) {
            currentSource = GIF_FILE;
            currentFileIndex = totalFiles - 1;
        }
    } else {
        currentFileIndex--;
        if (currentFileIndex < 0) {
            currentSource = GIF_BUILTIN;
            currentFileIndex = 0;
        }
    }
    
    Serial.println("Switched to: " + getCurrentContentName());
    
    if (isPlaying || isStaticImageDisplayed) { // If currently playing or displaying static, start new content
        play();
    }
}

void GifPlayer::setContentByType(GifSource source, int fileIndex) {
    currentSource = source;
    
    if (source == GIF_FILE) {
        if (fileIndex >= 0 && fileIndex < totalFiles) {
            currentFileIndex = fileIndex;
        } else {
            Serial.printf("Invalid file index %d, total files: %d. Defaulting to 0.\n", fileIndex, totalFiles);
            currentFileIndex = 0;
        }
    } else {
        currentFileIndex = 0;
    }
    
    Serial.printf("Set content: source=%d, fileIndex=%d, name=%s\n", 
                  source, currentFileIndex, getCurrentContentName().c_str());
}

String GifPlayer::getCurrentContentName() const {
    if (currentSource == GIF_BUILTIN) {
        return "homer_tiny";
    } else {
        if (currentFileIndex >= 0 && currentFileIndex < totalFiles) {
            return gifFiles[currentFileIndex];
        }
    }
    return "none";
}

int GifPlayer::getTotalContents() const {
    return totalFiles + 1;
}

int GifPlayer::getCurrentIndex() const {
    if (currentSource == GIF_BUILTIN) {
        return 0;
    } else {
        return currentFileIndex + 1;
    }
}

void GifPlayer::update() {
    // If a static image is currently displayed, do nothing further.
    if (isStaticImageDisplayed) {
        return; 
    }

    if (!isPlaying || isPaused || hasEnded || !m_matrix || !m_utils || !pFrameBuffer) {
        return;
    }
    
    // Periodic memory integrity check
    static int memCheckCounter = 0;
    if (memCheckCounter++ % 120 == 0) { // Every 2 minutes
        if (!checkMemoryIntegrity()) {
            Serial.println("ERROR: Memory integrity check failed - stopping GIF playback");
            stop();
            return;
        }
    }
    
    // Safe frame decoding
    int result = gif.playFrame(false, NULL);
    
    static int updateCounter = 0;
    bool shouldDebug = (updateCounter++ % 60 == 0);
    
    // if (shouldDebug || result <= 0) {
    //     Serial.printf("DEBUG: update() - Frame decode result: %d, error: %d\n", result, gif.getLastError());
    // }

    if (result == 0) {
        if (playMode == GIF_LOOP) {
            // Simply reset the GIF and play the first frame naturally in the next update()
            gif.reset();
            return;  // Don't render in this update() cycle
        } else {
            stop(); 
            return; 
        }
    }

    // After first frame decoding in play() function
    if (result > 0) {
        // debugFrameBuffer(); 
        // debugPalette();
        if (shouldDebug && isStaticImageDisplayed) {
            debugPalette();
        }
        // Check status differentiating between FS and Builtin GIFs
        // if (currentSource == GIF_BUILTIN) {
        //     Serial.println("DEBUG: BUILTIN GIF - First frame decoded"); 
        // } else {
        //     Serial.printf("DEBUG: FS GIF - First frame decoded (%s)\n", 
        //                   gifFiles[currentFileIndex].c_str()); 
        //     debugFrameBufferState("After first frame decode"); 
        // }
        
        renderFrameToMatrix(); // include fillScreen(0)
        // if (shouldDebug) {
        //     Serial.println("DEBUG: renderFrameToMatrix - Copying frame to matrix."); 
        // }
        
        delay(10);
        m_utils->displayShow(); 
    } else if (result < 0) { 
        Serial.printf("ERROR: GIF playback error: result=%d, lastError=%d\n", result, gif.getLastError());
        stop(); 
    }
 }

/**
 * @brief Debug function to display frame buffer contents (disabled in production)
 * 
 * Outputs first 10 bytes of frame buffer for debugging purposes.
 * Enable by uncommenting the Serial.print statements.
 */
void GifPlayer::debugFrameBuffer() {
    #ifdef DEBUG_GIF_PLAYER
    Serial.println("DEBUG: Frame buffer contents:");
    for (int i = 0; i < 10 && i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
        Serial.printf("  [%d] = %02X\n", i, pFrameBuffer[i]);
    }
    #endif
}

/**
 * @brief Debug function to display current color palette (disabled in production)
 * 
 * Outputs first 16 palette entries for debugging purposes.
 * Enable by uncommenting the Serial.print statements.
 */
void GifPlayer::debugPalette() {
    #ifdef DEBUG_GIF_PLAYER
    Serial.println("DEBUG: Current palette:");
    for (int i = 0; i < 16; i++) {
        Serial.printf("  Palette[%d] = 0x%04X\n", i, currentPalette[i]);
    }
    #endif
}

bool GifPlayer::verifyFirstFrameRender() {
    int nonZeroPixels = 0;
    for (int i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
        if (pFrameBuffer[i] != 0) {
            nonZeroPixels++;
        }
    }
    // Serial.printf("DEBUG: Frame verification - %d non-zero pixels found\n", nonZeroPixels); // 주석 처리됨
    return nonZeroPixels > 0;
}

bool GifPlayer::checkMemoryIntegrity() {
    if (!pFrameBuffer) {
        Serial.println("ERROR: Frame buffer is null");
        return false;
    }
    
    // Simple memory integrity check
    size_t bufferSize = m_matrix->width() * m_matrix->height();
    
    // Check buffer start and end
    uint8_t* bufStart = pFrameBuffer;
    uint8_t* bufEnd = pFrameBuffer + bufferSize - 1;
    
    // Check memory accessibility (very basic test)
    volatile uint8_t testRead;
    __asm__ __volatile__("" : : : "memory"); // Prevent compiler optimization
    
    testRead = *bufStart;
    testRead = *bufEnd;
    
    return true;
}

void GifPlayer::safeMemoryCleanup() {
    if (isPlaying || isStaticImageDisplayed) { // Stop even if a static image is displayed
        stop();
    }
    
    if (pFrameBuffer) {
        // Clear memory before releasing
        size_t bufferSize = m_matrix->width() * m_matrix->height();
        memset(pFrameBuffer, 0, bufferSize);
        
        delay(10); // Short delay for safety
        
        free(pFrameBuffer);
        pFrameBuffer = nullptr;
        
        Serial.println("INFO: Memory cleanup completed safely");
    }
}

/**
 * @brief GIF frame drawing callback function (IRAM optimized for performance)
 * 
 * This function is called by the AnimatedGIF library for each line of a GIF frame.
 * It handles both built-in and file-system based GIFs with different rendering strategies:
 * - Built-in GIFs: Direct rendering without filtering
 * - File-system GIFs: Background layer filtering to prevent duplicate rendering
 * 
 * @param pDraw Pointer to GIFDRAW structure containing frame data
 */
void IRAM_ATTR GifPlayer::GIFDraw(GIFDRAW *pDraw) {
    // Validate instance and frame buffer
    if (!s_instance || !s_instance->pFrameBuffer) {
        return;
    }
    
    // Filter full-screen background layers for file-system based GIFs
    // This prevents duplicate object rendering that causes visual artifacts
    if (s_instance->currentSource == GIF_FILE && 
        pDraw->iX == 0 && pDraw->iY == 0 && 
        pDraw->iWidth == MATRIX_WIDTH && pDraw->ucHasTransparency == 0) {
        // Skip rendering background layer to avoid duplication
        return;
    }
    
    // Local variables for performance
    uint8_t *sourcePixels;
    uint8_t *destBuffer;
    uint16_t *palette;
    int actualY, drawWidth, drawHeight;

    // Clamp drawing dimensions to matrix bounds
    drawWidth = (pDraw->iWidth > MATRIX_WIDTH) ? MATRIX_WIDTH : pDraw->iWidth;
    drawHeight = (pDraw->iHeight > MATRIX_HEIGHT) ? MATRIX_HEIGHT : pDraw->iHeight;

    // Calculate absolute Y coordinate in frame buffer
    actualY = pDraw->iY + pDraw->y;
    
    // Skip if drawing outside matrix bounds
    if (actualY < 0 || actualY >= MATRIX_HEIGHT) {
        return;
    }
    
    // Update color palette and background index on first line of each object
    if (pDraw->y == 0) {
        palette = pDraw->pPalette;
        // // Add debug prints here to inspect pDraw->pPalette directly
        // Serial.println("DEBUG: GIFDraw - pDraw->pPalette (first 16 entries):");
        // for (int i = 0; i < 16; i++) {
        //     Serial.printf("  pDraw->pPalette[%d] = 0x%04X\n", i, palette[i]);
        // }
        for (int i = 0; i < 256; i++) {
            s_instance->currentPalette[i] = palette[i];
        }
        s_instance->gifBackgroundIndex = pDraw->ucBackground;
    }
    
    // Calculate frame buffer position with correct X offset
    sourcePixels = pDraw->pPixels;
    destBuffer = s_instance->pFrameBuffer + actualY * MATRIX_WIDTH + pDraw->iX;
    
    // Copy pixels with bounds checking and transparency handling
    for (int i = 0; i < drawWidth; i++) {
        int bufferX = pDraw->iX + i;
        
        // Only write within matrix width bounds
        if (bufferX < MATRIX_WIDTH) {
            uint8_t pixelIndex = *sourcePixels++;
            // Write non-transparent pixels to frame buffer
            // Restore original logic: write pixel if it's for a static image OR if it's not background/transparent for animated GIFs
            if (s_instance->isStaticImageDisplayed || 
                (pixelIndex != pDraw->ucBackground && !(pDraw->ucHasTransparency && pixelIndex == pDraw->ucTransparent))) {
                *destBuffer = pixelIndex;
            }
            destBuffer++;
        } else {
            // Skip pixels outside matrix bounds
            sourcePixels++;
        }
    }
}

// Static callback functions for file-based GIF loading
void* GifPlayer::GIFOpen(const char *fname, int32_t *pSize) {
    if (s_instance->gifFile) {
        s_instance->gifFile.close();
    }
    s_instance->gifFile = LittleFS.open(fname);
    if (s_instance->gifFile) {
        *pSize = s_instance->gifFile.size();
        return (void *)&s_instance->gifFile;
    }
    return nullptr;
}

void GifPlayer::GIFClose(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != nullptr) {
        f->close();
    }
}

int32_t GifPlayer::GIFRead(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    if ((pFile->iSize - pFile->iPos) < iLen)
        iBytesRead = pFile->iSize - pFile->iPos - 1;
    if (iBytesRead <= 0)
        return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
}

int32_t GifPlayer::GIFSeek(GIFFILE *pFile, int32_t iPosition) {
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    return pFile->iPos;
}

// Check frame buffer state after first frame in play() function
void GifPlayer::debugFrameBufferState(const char* context) {
    // This entire function is for debugging purposes and is now commented out.
    if (!pFrameBuffer) {
        Serial.printf("DEBUG: Frame buffer is null [%s]\n", context);
        return;
    }
    
    Serial.printf("DEBUG: Frame buffer state [%s]:\n", context);
    Serial.printf("DEBUG: Source: %s\n", currentSource == GIF_BUILTIN ? "BUILTIN" : "FS");
    Serial.printf("DEBUG: Background index: %d\n", gifBackgroundIndex);
    
    // Check data from first two rows (for x=0, x=1 comparison)
    for (int y = 0; y < 2 && y < MATRIX_HEIGHT; y++) {
        Serial.printf("DEBUG: Row %d: ", y);
        for (int x = 0; x < 10 && x < MATRIX_WIDTH; x++) {
            int bufferIndex = y * MATRIX_WIDTH + x;
            Serial.printf("[%d:%02X] ", x, pFrameBuffer[bufferIndex]);
        }
        Serial.println();
    }
}
