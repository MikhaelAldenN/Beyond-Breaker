#pragma once

#include <string>
#include <map>
#include <memory>
#include "System/Sprite.h" // Menggunakan Sprite milikmu

// Struktur data untuk menyimpan info huruf dari file .fnt
struct CharData {
    int id;
    int x, y;
    int width, height;
    int xoffset, yoffset;
    int xadvance;
};

class BitmapFont
{
public:
    // Constructor: Load gambar & parse data .fnt
    BitmapFont(const std::string& texturePath, const std::string& fontDataPath);
    ~BitmapFont() = default;

    // Fungsi untuk menggambar teks
    void Draw(const std::string& text, float x, float y, float scale, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

private:
    std::unique_ptr<Sprite> sprite; // Pointer ke Sprite sistem kamu
    std::map<int, CharData> chars;  // Database koordinat huruf

    // Helper parsing sederhana
    int ParseValue(const std::string& line, const std::string& key);
};