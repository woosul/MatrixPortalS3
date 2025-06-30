// Drawable.h
#ifndef DRAWABLE_H
#define DRAWABLE_H

class Drawable {
public:
    char* name; 
    bool isStarted; 

    Drawable() : name((char*)"Unnamed Pattern"), isStarted(false) {}
    virtual ~Drawable() {} 

    virtual void start() {
        isStarted = true;
    }

    virtual void stop() {
        isStarted = false;
    }

    virtual unsigned int drawFrame() = 0; // 순수 가상 함수
};

#endif
