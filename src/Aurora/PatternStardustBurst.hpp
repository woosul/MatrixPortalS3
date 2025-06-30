#ifndef PatternStardustBurst_H
#define PatternStardustBurst_H

#include "EffectsLayer.hpp"
#include <vector> // For std::vector if preferred, or C-style array

#define MAX_STARDUST_PARTICLES 80
#define DEFAULT_PARTICLES_PER_BURST 6
#define BURST_INTERVAL_MIN_MS 150
#define BURST_INTERVAL_MAX_MS 700
#define PARTICLE_MAX_AGE_MIN 25 // frames
#define PARTICLE_MAX_AGE_MAX 60 // frames
#define PARTICLE_SPEED_MAX 0.9f

struct StardustParticle {
    float x, y;
    float vx, vy;
    CRGB color;
    int age;
    int maxAge;
    bool active;

    StardustParticle() : x(0), y(0), vx(0), vy(0), color(CRGB::Black), age(0), maxAge(0), active(false) {}
};

class PatternStardustBurst : public Drawable {
private:
    StardustParticle particles[MAX_STARDUST_PARTICLES];
    unsigned long lastBurstTime;
    unsigned long nextBurstInterval;

    void spawnBurst() {
        float burstOriginX = random(0, effects.width);
        float burstOriginY = random(0, effects.height);
        CRGB burstBaseColor = effects.ColorFromCurrentPalette(random8(), 255);

        int spawnedCount = 0;
        for (int i = 0; i < MAX_STARDUST_PARTICLES && spawnedCount < DEFAULT_PARTICLES_PER_BURST; ++i) {
            if (!particles[i].active) {
                particles[i].active = true;
                particles[i].x = burstOriginX;
                particles[i].y = burstOriginY;
                
                float angle = random(0, 360) * (PI / 180.0f); // Random angle in radians
                float speed = random(10, (int)(PARTICLE_SPEED_MAX * 100)) / 100.0f; // Random speed
                
                particles[i].vx = cos(angle) * speed;
                particles[i].vy = sin(angle) * speed;
                
                // Particles in the same burst can have slightly varied shades or be the same
                particles[i].color = burstBaseColor; 
                // Or, for variation:
                // uint8_t hueVariation = random8(20) - 10; // +/- 10 hue variation
                // particles[i].color = effects.ColorFromCurrentPalette(random8() + hueVariation, 255);


                particles[i].age = 0;
                particles[i].maxAge = random(PARTICLE_MAX_AGE_MIN, PARTICLE_MAX_AGE_MAX);
                spawnedCount++;
            }
        }
    }

public:
    PatternStardustBurst() {
        name = (char *)"Stardust Burst";
        lastBurstTime = 0;
        nextBurstInterval = random(BURST_INTERVAL_MIN_MS, BURST_INTERVAL_MAX_MS);
    }

    void start() {
        for (int i = 0; i < MAX_STARDUST_PARTICLES; ++i) {
            particles[i].active = false;
        }
        effects.ClearFrame();
        lastBurstTime = millis(); // Initialize to current time to avoid immediate burst
    }

    unsigned int drawFrame() {
        effects.DimAll(235); // Slightly stronger dim for more pronounced trails

        unsigned long currentTime = millis();
        if (currentTime - lastBurstTime > nextBurstInterval) {
            spawnBurst();
            lastBurstTime = currentTime;
            nextBurstInterval = random(BURST_INTERVAL_MIN_MS, BURST_INTERVAL_MAX_MS);
        }

        for (int i = 0; i < MAX_STARDUST_PARTICLES; ++i) {
            if (particles[i].active) {
                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;
                particles[i].age++;

                if (particles[i].age > particles[i].maxAge ||
                    particles[i].x < 0 || particles[i].x >= effects.width ||
                    particles[i].y < 0 || particles[i].y >= effects.height) {
                    particles[i].active = false;
                    continue;
                }

                // Fade out brightness based on age
                uint8_t brightness = 255;
                if (particles[i].maxAge > 0) {
                     brightness = ease8InOutQuad( (float)(particles[i].maxAge - particles[i].age) * 255.0f / particles[i].maxAge );
                }
                
                CRGB displayColor = particles[i].color;
                displayColor.nscale8(brightness);
                
                // Draw the particle if it's within bounds (already checked, but good practice)
                int drawX = round(particles[i].x);
                int drawY = round(particles[i].y);
                if (drawX >= 0 && drawX < effects.width && drawY >= 0 && drawY < effects.height) {
                     effects.leds[effects.XY16(drawX, drawY)] += displayColor; // Use += for additive blending if desired, or =
                }
            }
        }

        effects.ShowFrame();
        return 30; // Aim for roughly 33 FPS
    }
};

#endif
