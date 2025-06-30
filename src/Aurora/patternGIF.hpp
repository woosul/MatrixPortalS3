#ifndef PatternGIF_H
#define PatternGIF_H

#include "EffectsLayer.hpp"
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <vector> // For std::vector
#include <map> // PROGMEM GIF 이름과 데이터 매핑을 위해 추가

// 콜백 함수 선언 (정의는 mode_animation.cpp 또는 유사 파일에 위치)
void GIFDrawCallback(GIFDRAW *pDraw);
void * GIFOpenFile(const char *fname, int32_t *pFileSize);
void GIFCloseFile(void *pFile);
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition);

enum class GIFPlaybackMode {
    SEQUENTIAL,
    RANDOM
};

// 각 GIF 소스(파일 시스템 또는 PROGMEM) 정보를 담는 구조체
struct GifSource {
    String name; // GIF 이름 (예: "x_wing" 또는 "animation.gif")
    String path; // LittleFS 파일 경로 (PROGMEM GIF의 경우 비어 있음)
    const uint8_t* progmemData; // PROGMEM 데이터 포인터
    size_t progmemSize;         // PROGMEM 데이터 크기
    bool isProgmem;             // PROGMEM GIF 여부 플래그

    // 파일 시스템 GIF용 생성자
    GifSource(String n, String p) : 
        name(n.substring(0, n.lastIndexOf('.'))), // 확장자 제외한 파일명을 이름으로 사용
        path(p), 
        progmemData(nullptr), 
        progmemSize(0), 
        isProgmem(false) {}

    // PROGMEM GIF용 생성자
    GifSource(String n, const uint8_t* data, size_t size) : 
        name(n), // 제공된 이름을 사용
        path(""), 
        progmemData(data), 
        progmemSize(size), 
        isProgmem(true) {}
};

// 미리 등록된 PROGMEM GIF 정보를 위한 구조체
struct ProgmemGifInfo {
    const uint8_t* data;
    size_t size;
};

class PatternGIF : public Drawable {
private:
    AnimatedGIF gif;
    std::vector<GifSource> gifPlaylist; // GIF 소스 목록
    int currentGifIndex;
    bool isGifLoaded;
    
    unsigned long lastFrameTime;
    int frameDelay;

    // .h 파일 이름(확장자 제외)을 키로 PROGMEM GIF 정보를 저장하는 맵
    std::map<String, ProgmemGifInfo> progmemGifMap;

    void cleanupCurrentGif() {
        Serial.println(F("PatternGIF::cleanupCurrentGif - Entered."));
        if (isGifLoaded) {
            Serial.println(F("PatternGIF::cleanupCurrentGif - Closing GIF..."));
            gif.close(); // AnimatedGIF 라이브러리의 close 함수 호출
            Serial.println(F("PatternGIF::cleanupCurrentGif - gif.close() completed."));
        } else {
            Serial.println(F("PatternGIF::cleanupCurrentGif - No GIF to close."));
        }
        isGifLoaded = false;
    }

    // 지정된 디렉토리에서 .gif 파일과 등록된 .h 파일 기반 GIF를 스캔하여 재생 목록 생성
    void scanGifSources(const char *directory) {
        gifPlaylist.clear();
        currentGifIndex = -1; // 인덱스 초기화

        // 1. 등록된 모든 PROGMEM GIF를 재생 목록에 추가
        Serial.printf("PatternGIF::scanGifSources - Adding %d registered PROGMEM GIFs to playlist.\n", progmemGifMap.size());
        if (progmemGifMap.empty()) {
            Serial.println(F("PatternGIF::scanGifSources - No PROGMEM GIFs registered in progmemGifMap."));
        }
        for (const auto& pair : progmemGifMap) {
            const String& name = pair.first;
            const ProgmemGifInfo& info = pair.second;
            gifPlaylist.push_back(GifSource(name, info.data, info.size)); // PROGMEM GIF용 생성자 사용
            Serial.printf("PatternGIF::scanGifSources - Added PROGMEM GIF from map: %s\n", name.c_str());
        }

        // 2. LittleFS의 지정된 디렉토리에서 .gif 파일 스캔
        Serial.printf("PatternGIF::scanGifSources - Scanning LittleFS directory: %s for .gif files.\n", directory);
        File root = LittleFS.open(directory);

        if (!root) {
            Serial.printf("PatternGIF::scanGifSources - Failed to open LittleFS directory: %s. Skipping FS scan for .gif files.\n", directory);
        } else if (!root.isDirectory()) {
            Serial.printf("PatternGIF::scanGifSources - Path %s is not a directory. Skipping FS scan for .gif files.\n", directory);
            root.close();
        } else {
            Serial.printf("PatternGIF::scanGifSources - Successfully opened LittleFS directory: %s\n", directory);
            File file = root.openNextFile();
            while(file){
                if(!file.isDirectory()){
                    String fullPath = String(directory);
                    if (!fullPath.endsWith("/")) {
                        fullPath += "/";
                    }
                    fullPath += file.name();
                    String fileName = String(file.name());

                    if (fileName.endsWith(".gif") || fileName.endsWith(".GIF")) {
                        gifPlaylist.push_back(GifSource(fileName, fullPath));
                        Serial.printf("PatternGIF::scanGifSources - Found and added FS GIF: %s (Name for playlist: %s)\n",
                                      fullPath.c_str(),
                                      fileName.substring(0, fileName.lastIndexOf('.')).c_str());
                    }
                }
                if (file) file.close(); // 현재 file 객체 닫기
                file = root.openNextFile(); // 다음 파일 열기
            }
            if (root) root.close(); // 디렉토리 객체 닫기
        }
        Serial.printf("PatternGIF::scanGifSources - Total GIFs in playlist after scan: %d\n", gifPlaylist.size());
    }

    bool openNextGif() {
        if (gifPlaylist.empty()) {
            Serial.println(F("PatternGIF::openNextGif - Playlist is empty."));
            return false;
        }

        Serial.printf("PatternGIF::openNextGif - Free heap before GIF open: %d bytes\n", ESP.getFreeHeap());
        
        if (isGifLoaded) {
            cleanupCurrentGif();
        }

        if (playbackMode == GIFPlaybackMode::SEQUENTIAL) {
            currentGifIndex = (currentGifIndex + 1) % gifPlaylist.size();
        } else { // RANDOM
            if (gifPlaylist.size() > 1) {
                int nextIndex = currentGifIndex;
                while (nextIndex == currentGifIndex) { // Ensure next is different
                    nextIndex = random(gifPlaylist.size());
                }
                currentGifIndex = nextIndex;
            } else {
                currentGifIndex = 0;
            }
        }

        if (currentGifIndex < 0 || currentGifIndex >= gifPlaylist.size()) {
             Serial.println(F("PatternGIF::openNextGif - Invalid GIF index after selection."));
             return false;
        }

        const GifSource& source = gifPlaylist[currentGifIndex];
        Serial.printf("PatternGIF::openNextGif - Attempting to open GIF: %s (Type: %s)\n", 
                      source.name.c_str(), 
                      source.isProgmem ? "PROGMEM" : "LittleFS");

        bool openResult = false;
        // AnimatedGIF 라이브러리는 콜백 함수에 사용자 정의 포인터를 전달하는 기능을 지원합니다.
        // GIFDrawCallback이 'effects' 객체에 접근해야 하므로, 해당 포인터를 설정합니다.
        // PatternGIF의 멤버 변수 접근이 필요하면 'this'를 전달하고 콜백을 static 멤버 함수로 만들거나 래퍼 사용.
        // 현재 GIFDrawCallback은 전역 'effects'를 사용하므로 별도 pUser 설정은 필수 아님.
        // gif.setUserPointer(&effects); // 필요시 활성화

        if (source.isProgmem) {
            if (source.progmemData != nullptr && source.progmemSize > 0) {
                if (gif.open((uint8_t*)source.progmemData, source.progmemSize, GIFDrawCallback)) {
                    openResult = true;
                }
            } else {
                 Serial.printf("PatternGIF::openNextGif - PROGMEM source '%s' has null data or zero size.\n", source.name.c_str());
            }
        } else { // LittleFS GIF
            if (!LittleFS.exists(source.path)) {
                Serial.printf("PatternGIF::openNextGif - File not found: %s\n", source.path.c_str());
                return false;
            }
            if (gif.open(source.path.c_str(), GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDrawCallback)) {
                openResult = true;
            }
        }

        if (openResult) {
            isGifLoaded = true;
            Serial.printf("PatternGIF::openNextGif - Successfully opened GIF: %s (%d x %d)\n", 
                         source.name.c_str(), gif.getCanvasWidth(), gif.getCanvasHeight());
            gif.reset(); // 첫 프레임부터 시작
            effects.ClearFrame(); 
        } else {
            isGifLoaded = false;
            Serial.printf("PatternGIF::openNextGif - Failed to open GIF: %s. Error: %d\n", source.name.c_str(), gif.getLastError());
            return false;
        }

        lastFrameTime = millis();
        frameDelay = 33; // 기본값, playFrame에서 업데이트됨
        return true;
    }

public:
    GIFPlaybackMode playbackMode;

    PatternGIF() : gifPlaylist() { 
        name = (char *)"GIF Player";
        lastFrameTime = 0;
        frameDelay = 33; // 약 30FPS
        playbackMode = GIFPlaybackMode::SEQUENTIAL;
        isGifLoaded = false;
        currentGifIndex = -1;
        
        gif.begin(GIF_PALETTE_RGB565_LE); 
    } // Removed effects_ptr from constructor, will be set in start()

    // PROGMEM에 저장된 GIF 데이터를 이름과 함께 등록하는 메소드
    // 예: addProgmemGif("x_wing", x_wing_data, sizeof(x_wing_data));
    void addProgmemGif(const char* gifName, const uint8_t* data, size_t size) {
        progmemGifMap[String(gifName)] = {data, size};
        Serial.printf("PatternGIF::addProgmemGif - Registered PROGMEM GIF: %s, Size: %d\n", gifName, size);
    }

    // start() now accepts EffectsLayer pointer if it needs to use it directly,
    // or assumes GIFDrawCallback can access a global 'effects' object.
    // For current GIFDrawCallback, global 'effects' is used.
    void start() { // Removed override as it's not inheriting from Drawable in this context if used by ModeGIF directly
        if (!LittleFS.begin(true)) {
            Serial.println(F("PatternGIF::start - LittleFS mount failed."));
            return;
        }
        // LittleFS 마운트 후 /gifs 디렉토리 존재 확인 및 생성 (선택 사항)
        // if (!LittleFS.exists("/gifs")) {
        //     if (LittleFS.mkdir("/gifs")) {
        //         Serial.println("PatternGIF::start - Created /gifs directory.");
        //     } else {
        //         Serial.println("PatternGIF::start - Failed to create /gifs directory.");
        //     }
        // }
        // PROGMEM GIF들은 주 애플리케이션의 setup() 등에서 addProgmemGif를 통해 미리 등록되어야 합니다.
        // 예: patternGIF.addProgmemGif("x_wing", x_wing_data_array, sizeof(x_wing_data_array));
        // 그런 다음 scanGifSources를 호출하여 파일 시스템 GIF와 함께 목록을 만듭니다.
        scanGifSources("/gifs"); 
        effects.ClearFrame();
        if (!openNextGif()) {
            Serial.println(F("PatternGIF::start - No GIFs to play or failed to load initial GIF."));
        }
    }

    // drawFrame() no longer an override if not used as a Drawable pattern directly
    // It will be called by ModeGIF::run()
    unsigned int drawFrame() {
        unsigned long currentTime = millis();

        if (!isGifLoaded) {
            if (!openNextGif()) {
                return 1000; // 1초 후 재시도
            }
        }

        // 프레임 시간 간격 확인
        if (currentTime - lastFrameTime >= (unsigned long)frameDelay) {
            lastFrameTime = currentTime;

            if (gif.playFrame(true, &frameDelay)) { // true는 디더링 사용 의미
                // GIFDrawCallback에 의해 프레임이 그려짐
            } else {
                // GIF 끝 또는 오류 발생
                int lastError = gif.getLastError();
                const char* currentName = (currentGifIndex >= 0 && currentGifIndex < gifPlaylist.size()) ? 
                                          gifPlaylist[currentGifIndex].name.c_str() : "Unknown";
                Serial.printf("PatternGIF::drawFrame - GIF '%s' ended or error. Code: %d. Loading next.\n", currentName, lastError);
                
                if (!openNextGif()) {
                    Serial.println(F("PatternGIF::drawFrame - Failed to open next GIF."));
                    isGifLoaded = false; 
                    return 1000; 
                }
                // 새 GIF가 로드되면, frameDelay는 해당 GIF의 첫 프레임 지연 시간으로 설정됨
            }
        }
        
        effects.ShowFrame(); 
        
        // 다음 프레임까지 남은 시간 또는 최소 딜레이 반환
        // frameDelay가 0이면 (일부 GIF의 경우), 최소 33ms (약 30FPS)로 처리
        unsigned long intendedNextFrameTime = lastFrameTime + frameDelay;
        unsigned long timeToNextFrame = (intendedNextFrameTime > currentTime) ? (intendedNextFrameTime - currentTime) : 0;
        
        return (frameDelay == 0) ? 33 : max(10UL, timeToNextFrame); // 최소 10ms 딜레이 보장
    }

    void setPlaybackMode(GIFPlaybackMode mode) {
        playbackMode = mode;
    }

    // stop() no longer an override
    void stop() {
        cleanupCurrentGif();
        Serial.println(F("PatternGIF::stop"));
    }
};

#endif
