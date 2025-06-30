#ifndef GIF_PLAYER_H
#define GIF_PLAYER_H

#include <AnimatedGIF.h>
#include <LittleFS.h>
#include "config.h"
#include "utils.h"
#include "gifs/01_homer.h"

enum GifSource {
    GIF_BUILTIN,    // 01_homer.h
    GIF_FILE        // LittleFS files
};

enum GifPlayMode {
    GIF_LOOP,       // Loop playing
    GIF_ONCE        // Play once and stop
};

#define MAX_GIF_FILES 10 // Maximum number of GIF files to handle
#define GIF_DIR "/gifs"  // Directory path

class GifPlayer {
private:
    AnimatedGIF gif;
    File gifFile;
    
    // Display and utility pointers
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;

    // Frame buffer for robust GIF rendering
    uint8_t *pFrameBuffer;
    uint16_t currentPalette[256];
    uint8_t gifBackgroundIndex; // Store the GIF's background color index
    
    // Current GIF information
    GifSource currentSource;
    int currentFileIndex;
    String currentFileName;
    bool isStaticImageDisplayed; // Flag for indicating if a static image is currently displayed

    // Playback settings
    GifPlayMode playMode;
    bool isPlaying;
    bool isPaused;
    bool hasEnded;  // State of end on Once playback mode

    // GIF file list
    String gifFiles[MAX_GIF_FILES];
    int totalFiles;
    
    // Static instance pointer for callbacks
    static GifPlayer* s_instance;
    
    // Callback functions
    static void GIFDraw(GIFDRAW *pDraw);
    static void* GIFOpen(const char *fname, int32_t *pSize);
    static void GIFClose(void *pHandle);
    static int32_t GIFRead(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
    static int32_t GIFSeek(GIFFILE *pFile, int32_t iPosition);
    
    // Internal functions
    void scanGifFiles();
    bool loadBuiltinGif();
    bool loadFileGif(const String& filename);
    void renderStaticImageFrameToMatrix();  // Rendering function for static image frames specifically
    void renderFrameToMatrix();             // Render the current frame to the matrix display for normal GIF playback (not static images)
    void initializeBuffersWithFirstFrame();

    void debugFrameBuffer();
    void debugPalette();
    void debugMatrixState();
    void clearFrameBufferSafely();
    bool verifyFirstFrameRender();
    bool checkMemoryIntegrity();
    void safeMemoryCleanup();
    
public:
    GifPlayer();
    ~GifPlayer();
    
    bool begin(Utils* utils, MatrixPanel_I2S_DMA* matrix);
    void end();
    
    // Playback control
    bool play();
    void pause();
    void resume();
    void stop();
    
    // Content switching
    void nextContent();
    void prevContent();
    
    // MQTT direct access support
    void setContentByType(GifSource source, int fileIndex);
    GifSource getCurrentSource() const { return currentSource; }
    int getCurrentFileIndex() const { return currentFileIndex; }
    
    // Setting, getting play mode but not use.
    void setPlayMode(GifPlayMode mode) { playMode = mode; }
    GifPlayMode getPlayMode() const { return playMode; }

    // Status checking
    bool isGifPlaying() const { return isPlaying && !hasEnded; }
    bool isGifPaused() const { return isPaused; }
    String getCurrentContentName() const;
    int getTotalContents() const;
    int getCurrentIndex() const;

    // Update (called in main loop)
    void update();
    void debugGifFileInfo();
    
    // Debugging functions for FS GIF issues
    void debugFrameBufferState(const char* context);
};

extern GifPlayer gifPlayer;

#endif
