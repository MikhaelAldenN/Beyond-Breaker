#pragma once
#include <vector>
#include <string>
#include <memory>
#include "BitmapFont.h"

struct TypewriterLine
{
    std::string text;
    float x, y;
    float fontSize;
    float color[4];

    // --- FITUR BARU: MEMORY TEST ---
    bool isMemoryTest = false;
    int memStart = 0;
    int memEnd = 0;
    int memCurrent = 0;
    std::string memSuffix;
    float memCurrentFloat = 0.0f;
    float memIncrementPerSecond = 0.0f;

    // --- TAMBAHAN BARU: CUSTOM DELAY ---
    // Default 0.2 detik biar standard
    float postLineDelay = 0.2f;
};

class Typewriter
{
public:
    Typewriter();
    ~Typewriter() = default;

    // Tambahkan parameter default 'delay' di akhir = 0.2f
    void AddLine(const std::string& text, float x, float y, float size, const float color[4], float delay = 0.2f);

    // Tambahkan parameter default 'delay' di akhir = 0.2f
    void AddMemoryTestLine(const std::string& prefix, int startVal, int endVal, float duration, const std::string& suffix, float x, float y, float size, const float color[4], float delay = 0.2f);

    bool Update(float dt);
    void Render(BitmapFont* font);
    void SkipCurrentLine();
    bool IsFinished() const;
    void SetTypingSpeed(float speed) { typeSpeed = speed; }

private:
    std::vector<TypewriterLine> lines;
    int currentLineIndex = 0;
    size_t currentCharIndex = 0;
    float timer = 0.0f;
    float typeSpeed = 0.001f;

    // Timer penghitung delay saat ini
    float lineDelayTimer = 0.0f;

    // HAPUS konstanta ini karena sekarang delay diambil dari struct per baris
    // const float LINE_DELAY_DURATION = 0.2f; 
};