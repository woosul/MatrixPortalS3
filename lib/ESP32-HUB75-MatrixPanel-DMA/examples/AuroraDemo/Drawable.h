#ifndef DRAWABLE_H
#define DRAWABLE_H

// Drawable 클래스는 모든 Aurora 패턴의 기본 클래스입니다.
// 각 패턴은 이 클래스를 상속받아 drawFrame() 메서드 등을 구현합니다.

class Drawable {
public:
    char* name; // 패턴의 이름을 저장하는 포인터
    bool isStarted; // 패턴이 시작되었는지 여부를 나타내는 플래그

    Drawable() : name((char*)"Unnamed Pattern"), isStarted(false) {}
    virtual ~Drawable() {} // 가상 소멸자 (다형성 사용 시 중요)

    // 패턴 시작 시 호출되는 메서드
    virtual void start() {
        isStarted = true;
    }

    // 패턴 정지 시 호출되는 메서드
    virtual void stop() {
        isStarted = false;
    }

    // 매 프레임마다 호출되어 패턴을 그리는 순수 가상 메서드 (반드시 파생 클래스에서 구현해야 함)
    // 반환값: 다음 프레임까지의 권장 지연 시간 (밀리초)
    virtual unsigned int drawFrame() = 0;
};

#endif