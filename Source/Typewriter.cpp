#include "Typewriter.h"
#include <string> // butuh to_string

Typewriter::Typewriter() {}

void Typewriter::AddLine(const std::string& text, float x, float y, float size, const float color[4])
{
    TypewriterLine line;
    line.text = text;
    line.x = x; line.y = y; line.fontSize = size;
    for (int i = 0; i < 4; i++) line.color[i] = color[i];
    line.isMemoryTest = false; // Baris biasa
    lines.push_back(line);
}

// Perhatikan ada parameter baru: float duration
void Typewriter::AddMemoryTestLine(const std::string& prefix, int startVal, int endVal, float duration, const std::string& suffix, float x, float y, float size, const float color[4])
{
    TypewriterLine line;
    line.text = prefix;
    line.x = x; line.y = y; line.fontSize = size;
    for (int i = 0; i < 4; i++) line.color[i] = color[i];

    line.isMemoryTest = true;
    line.memStart = startVal;
    line.memEnd = endVal;
    line.memCurrent = startVal;
    line.memSuffix = suffix;

    // --- LOGIC BARU ---
    line.memCurrentFloat = (float)startVal; // Set float awal

    // Rumus: Kecepatan = Jarak / Waktu
    if (duration > 0.0f) {
        line.memIncrementPerSecond = (float)(endVal - startVal) / duration;
    }
    else {
        line.memIncrementPerSecond = (float)(endVal - startVal); // Langsung selesai kalau 0
    }

    lines.push_back(line);
}

bool Typewriter::Update(float dt)
{
    if (IsFinished()) return false;
    bool charTyped = false;
    TypewriterLine& currentLine = lines[currentLineIndex];

    // FASE 1: Ngetik Teks Biasa (Prefix)
    if (currentCharIndex < currentLine.text.size())
    {
        timer += dt;
        while (timer >= typeSpeed)
        {
            if (typeSpeed <= 0.0f) { /* Safety */ break; }
            timer -= typeSpeed;
            currentCharIndex++;
            charTyped = true;
            if (currentCharIndex >= currentLine.text.size()) {
                timer = 0.0f;
                break;
            }
        }
    }

    // FASE 2: Memory Test Counting
    else if (currentLine.isMemoryTest && currentLine.memCurrent < currentLine.memEnd)
    {
        // Tambahkan angka berdasarkan waktu (dt)
        // Ini menjamin durasinya pas, mau 60 FPS atau 144 FPS
        currentLine.memCurrentFloat += currentLine.memIncrementPerSecond * dt;

        // Update tampilan angka (cast ke int)
        currentLine.memCurrent = (int)currentLine.memCurrentFloat;

        // Cap biar nggak kelebihan
        if (currentLine.memCurrent >= currentLine.memEnd) {
            currentLine.memCurrent = currentLine.memEnd;
        }

        charTyped = true; // Tetap return true biar ada suara SFX 'trrrrt'
    }

    // FASE 3: Selesai / Delay pindah baris
    else
    {
        lineDelayTimer += dt;
        if (lineDelayTimer >= LINE_DELAY_DURATION)
        {
            lineDelayTimer = 0.0f;
            currentLineIndex++;
            currentCharIndex = 0;
        }
    }

    return charTyped;
}

void Typewriter::Render(BitmapFont* font)
{
    if (!font) return;

    for (int i = 0; i < lines.size(); ++i)
    {
        if (i > currentLineIndex) break;

        TypewriterLine& line = lines[i];
        std::string textToDraw;

        // --- RENDER LOGIC ---

        // 1. Ambil teks dasar (Prefix)
        if (i < currentLineIndex) {
            // Baris masa lalu: Full text
            textToDraw = line.text;
        }
        else {
            // Baris aktif: Substring ngetik
            size_t len = (currentCharIndex < line.text.size()) ? currentCharIndex : line.text.size();
            textToDraw = line.text.substr(0, len);
        }

        // 2. Jika ini Memory Test DAN Prefix sudah selesai diketik -> Tampilkan Angka
        if (line.isMemoryTest)
        {
            // Cek apakah prefix sudah selesai diketik?
            // (Jika baris masa lalu, pasti sudah selesai. Jika baris aktif, cek currentCharIndex)
            bool prefixDone = (i < currentLineIndex) || (currentCharIndex >= line.text.size());

            if (prefixDone)
            {
                // Format: "Memory Test : " + "262144" + "KB OK"
                textToDraw += std::to_string(line.memCurrent) + line.memSuffix;
            }
        }

        font->Draw(textToDraw.c_str(), line.x, line.y, line.fontSize,
            line.color[0], line.color[1], line.color[2], line.color[3]);
    }
}

void Typewriter::SkipCurrentLine()
{
    if (!IsFinished()) {
        TypewriterLine& line = lines[currentLineIndex];
        // Skip ketik
        currentCharIndex = line.text.size();
        // Skip counting juga
        if (line.isMemoryTest) {
            line.memCurrent = line.memEnd;
        }
    }
}
// IsFinished tetep sama...
bool Typewriter::IsFinished() const { return currentLineIndex >= lines.size(); }