// UI.h - immediate-mode UI helpers, theme, fonts and supply-chain drawing primitives.
#pragma once
#include "raylib.h"
#include <string>

namespace Theme {
    // Industrial / logistics palette: night-shift slate + safety orange + container colours.
    inline const Color Bg        = {15, 23, 32, 255};
    inline const Color Bg2       = {20, 30, 41, 255};
    inline const Color Panel     = {24, 35, 47, 255};
    inline const Color Panel2    = {33, 48, 64, 255};
    inline const Color Line      = {48, 66, 86, 255};
    inline const Color Text      = {230, 237, 243, 255};
    inline const Color Muted     = {143, 163, 181, 255};
    inline const Color Dim       = {92, 110, 128, 255};
    inline const Color Orange    = {255, 138, 31, 255};
    inline const Color Yellow    = {255, 200, 61, 255};
    inline const Color Teal      = {43, 179, 163, 255};
    inline const Color Red       = {229, 72, 77, 255};
    inline const Color Green     = {63, 185, 80, 255};
    inline const Color Blue      = {76, 154, 255, 255};
    inline const Color CodeBg    = {11, 17, 24, 255};
    inline const Color Asphalt   = {38, 46, 56, 255};
}

enum class FontKind { Sans, SansBold, Display, Mono, MonoBold };
enum class BtnStyle { Primary, Secondary, Ghost, Danger, Success };

namespace UI {
    void Init(const std::string& assetDir);
    void Shutdown();
    void BeginFrame();   // resets cursor etc.
    void EndFrame();

    Font& GetFont(FontKind kind, int size);
    Vector2 Measure(const std::string& text, int size, FontKind kind = FontKind::Sans);
    void Text(const std::string& text, float x, float y, int size, Color c, FontKind kind = FontKind::Sans);
    void TextCentered(const std::string& text, Rectangle r, int size, Color c, FontKind kind = FontKind::Sans);

    // Rich text with word wrap. Supports `inline code`, **bold**, "- " bullets and blank lines.
    // Returns the height used. If draw == false only measures.
    float RichText(const std::string& text, Rectangle box, int size, Color c, bool draw = true);

    // Syntax-highlighted C/C++ code block. highlightLine is 1-based (-1 = none).
    // If selectable, hovering highlights lines and the clicked line (1-based) is returned, else 0.
    // fontSize <= 0 means auto-fit the block.
    int CodeBlock(const std::string& code, Rectangle r, int fontSize, int highlightLine = -1,
                  bool selectable = false, Color highlightColor = Theme::Yellow, int markLine = -1,
                  Color markColor = Theme::Red);

    bool Button(Rectangle r, const std::string& label, BtnStyle style = BtnStyle::Primary,
                bool enabled = true, int fontSize = 20);
    bool Hover(Rectangle r);
    bool Clicked(Rectangle r);
    void Panel(Rectangle r, Color fill = Theme::Panel, Color border = Theme::Line, float roundness = 0.06f);
    void Tag(float x, float y, const std::string& text, Color bg, Color fg = Theme::Bg, int size = 14);
    void ProgressBar(Rectangle r, float t, Color fill, Color bg = Theme::Panel2);

    // Learning-app art
    void Block(float x, float y, float s, Color c, const std::string& label = "");   // one memory cell
    void ChapterNode(Vector2 c, float r, Color col, bool locked, bool done, int number, float pulse = 0);
    void Star(Vector2 c, float r, Color col);
    void CheckMark(float x, float y, float s, Color c);
    void CrossMark(float x, float y, float s, Color c);
    void Padlock(float x, float y, float s, Color c);

    Color Lighten(Color c, float amt);
}
