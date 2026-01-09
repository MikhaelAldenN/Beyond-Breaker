#pragma once
#include <vector>
#include <string>
#include <memory>
#include "BitmapFont.h"

struct TypewriterLine
{
    std::string text;       // Jika mode biasa: Teks penuh. Jika mode Memory: Prefix ("Memory Test : ")
    float x, y;
    float fontSize;
    float color[4];

    // --- FITUR BARU: MEMORY TEST ---
    bool isMemoryTest = false;      // Apakah baris ini spesial?
    int memStart = 0;               // Angka awal (misal 0)
    int memEnd = 0;                 // Angka target (misal 262144)
    int memCurrent = 0;             // Angka saat ini
    std::string memSuffix;          // Teks buntut ("KB OK")

    // TAMBAHAN BARU:
    float memCurrentFloat = 0.0f;       // Akumulator desimal (biar halus)
    float memIncrementPerSecond = 0.0f; // Kecepatan nambah per detik
};

class Typewriter
{
public:
    Typewriter();
    ~Typewriter() = default;

    void AddLine(const std::string& text, float x, float y, float size, const float color[4]);

    // Function baru khusus buat baris Memory Test
    void AddMemoryTestLine(const std::string& prefix, int startVal, int endVal, float duration, const std::string& suffix, float x, float y, float size, const float color[4]);
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
    float lineDelayTimer = 0.0f;
    const float LINE_DELAY_DURATION = 0.2f;
};