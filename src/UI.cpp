// UI.cpp - implementation of the immediate-mode UI layer.
#include "UI.h"
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cctype>

namespace {
    std::string g_assetDir;
    std::map<std::pair<int, int>, Font> g_fonts;   // (kind, size) -> font
    bool g_wantHand = false;

    const char* FontFile(FontKind k) {
        switch (k) {
            case FontKind::Sans:     return "fonts/Inter-Regular.ttf";
            case FontKind::SansBold: return "fonts/Inter-SemiBold.ttf";
            case FontKind::Display:  return "fonts/InterDisplay-Bold.ttf";
            case FontKind::Mono:     return "fonts/JetBrainsMono-Regular.ttf";
            case FontKind::MonoBold: return "fonts/JetBrainsMono-Bold.ttf";
        }
        return "";
    }

    std::vector<int> Codepoints() {
        std::vector<int> cps;
        for (int c = 32; c < 127; ++c) cps.push_back(c);
        int extra[] = {0x2022, 0x2192, 0x2190, 0x2713, 0x2717, 0x00D7, 0x2014, 0x2013, 0x2026, 0x00B7, 0x2191, 0x2193, 0x00B0};
        for (int e : extra) cps.push_back(e);
        return cps;
    }

    const std::set<std::string> kKeywords = {
        "if","else","for","while","do","switch","case","default","break","continue","return",
        "struct","class","public","private","protected","new","delete","sizeof","typedef","const",
        "static","using","namespace","virtual","override","this","nullptr","NULL","true","false",
        "enum","template","typename","goto","extern","inline","operator","friend","auto"};
    const std::set<std::string> kTypes = {
        "int","float","double","char","void","bool","long","short","unsigned","signed","size_t",
        "string","std","vector","FILE","ifstream","ofstream","fstream","cout","cin","endl","printf",
        "scanf","malloc","free","fopen","fclose","fprintf","fscanf","fgets","strlen","strcpy","strcmp",
        "main","getline","push_back","size","calloc","realloc","puts","map","fputs"};

    struct Tok { std::string s; Color c; };

    // Tokenise one line of C/C++ for highlighting. inBlock tracks /* */ across lines.
    std::vector<Tok> Highlight(const std::string& line, bool& inBlock) {
        const Color cDef = {214, 222, 235, 255};
        const Color cKw  = {199, 146, 234, 255};
        const Color cTy  = {79, 193, 255, 255};
        const Color cStr = {195, 232, 141, 255};
        const Color cNum = {247, 140, 108, 255};
        const Color cCom = {99, 119, 119, 255};
        const Color cPre = {255, 203, 107, 255};
        std::vector<Tok> out;
        size_t i = 0, n = line.size();
        auto push = [&](const std::string& s, Color c) { if (!s.empty()) out.push_back({s, c}); };

        if (!inBlock) {
            size_t first = line.find_first_not_of(" \t");
            if (first != std::string::npos && line[first] == '#') {
                // preprocessor: colour directive, and <...> or "..." as string
                size_t lt = line.find_first_of("<\"", first);
                if (lt == std::string::npos) { push(line, cPre); return out; }
                push(line.substr(0, lt), cPre);
                push(line.substr(lt), cStr);
                return out;
            }
        }
        while (i < n) {
            if (inBlock) {
                size_t end = line.find("*/", i);
                if (end == std::string::npos) { push(line.substr(i), cCom); return out; }
                push(line.substr(i, end + 2 - i), cCom); i = end + 2; inBlock = false; continue;
            }
            char ch = line[i];
            if (ch == '/' && i + 1 < n && line[i + 1] == '/') { push(line.substr(i), cCom); return out; }
            if (ch == '/' && i + 1 < n && line[i + 1] == '*') { inBlock = true; continue; }
            if (ch == '"' || ch == '\'') {
                size_t j = i + 1;
                while (j < n && line[j] != ch) { if (line[j] == '\\') ++j; ++j; }
                j = std::min(n, j + 1);
                push(line.substr(i, j - i), cStr); i = j; continue;
            }
            if (isdigit((unsigned char)ch)) {
                size_t j = i;
                while (j < n && (isalnum((unsigned char)line[j]) || line[j] == '.')) ++j;
                push(line.substr(i, j - i), cNum); i = j; continue;
            }
            if (isalpha((unsigned char)ch) || ch == '_') {
                size_t j = i;
                while (j < n && (isalnum((unsigned char)line[j]) || line[j] == '_')) ++j;
                std::string w = line.substr(i, j - i);
                Color c = kKeywords.count(w) ? cKw : (kTypes.count(w) ? cTy : cDef);
                push(w, c); i = j; continue;
            }
            size_t j = i;
            while (j < n && !isalnum((unsigned char)line[j]) && line[j] != '_' && line[j] != '"' &&
                   line[j] != '\'' && !(line[j] == '/' && j + 1 < n && (line[j+1] == '/' || line[j+1] == '*'))) ++j;
            if (j == i) j = i + 1;
            push(line.substr(i, j - i), cDef); i = j;
        }
        return out;
    }

    std::vector<std::string> SplitLines(const std::string& s) {
        std::vector<std::string> lines;
        std::string cur;
        for (char c : s) {
            if (c == '\n') { lines.push_back(cur); cur.clear(); }
            else if (c == '\t') cur += "    ";
            else if (c != '\r') cur += c;
        }
        lines.push_back(cur);
        while (!lines.empty() && lines.back().empty()) lines.pop_back();
        return lines;
    }
}

namespace UI {

void Init(const std::string& assetDir) { g_assetDir = assetDir; }

void Shutdown() {
    for (auto& kv : g_fonts) UnloadFont(kv.second);
    g_fonts.clear();
}

void BeginFrame() { g_wantHand = false; }
void EndFrame() { SetMouseCursor(g_wantHand ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT); }

Font& GetFont(FontKind kind, int size) {
    auto key = std::make_pair((int)kind, size);
    auto it = g_fonts.find(key);
    if (it != g_fonts.end()) return it->second;
    std::string path = g_assetDir + "/" + FontFile(kind);
    Font f;
    if (FileExists(path.c_str())) {
        std::vector<int> cps = Codepoints();
        f = LoadFontEx(path.c_str(), size, cps.data(), (int)cps.size());
        SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    } else {
        f = GetFontDefault();
    }
    return g_fonts.emplace(key, f).first->second;
}

Vector2 Measure(const std::string& text, int size, FontKind kind) {
    return MeasureTextEx(GetFont(kind, size), text.c_str(), (float)size, 0);
}

void Text(const std::string& text, float x, float y, int size, Color c, FontKind kind) {
    DrawTextEx(GetFont(kind, size), text.c_str(), {std::round(x), std::round(y)}, (float)size, 0, c);
}

void TextCentered(const std::string& text, Rectangle r, int size, Color c, FontKind kind) {
    Vector2 m = Measure(text, size, kind);
    Text(text, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, size, c, kind);
}

float RichText(const std::string& text, Rectangle box, int size, Color color, bool draw) {
    enum Style { Normal, Code, Bold };
    struct Word { std::string s; Style st; };
    const float lineH = size * 1.42f;
    float y = box.y;
    int codeSize = std::max(10, (int)std::round(size * 0.9f));
    float spaceW = Measure(" ", size).x;

    std::vector<std::string> paras;
    { std::string cur; for (char c : text) { if (c == '\n') { paras.push_back(cur); cur.clear(); } else if (c != '\r') cur += c; } paras.push_back(cur); }
    while (!paras.empty() && paras.back().empty()) paras.pop_back();

    for (const std::string& rawPara : paras) {
        if (rawPara.empty()) { y += lineH * 0.5f; continue; }
        std::string para = rawPara;
        float indent = 0;
        bool bullet = false;
        if (para.size() > 2 && para[0] == '-' && para[1] == ' ') { bullet = true; para = para.substr(2); indent = size * 1.1f; }

        // split into styled words
        std::vector<Word> words;
        Style st = Normal; std::string cur;
        auto flush = [&]() { if (!cur.empty()) { words.push_back({cur, st}); cur.clear(); } };
        for (size_t i = 0; i < para.size(); ++i) {
            char c = para[i];
            if (c == '`') { flush(); st = (st == Code) ? Normal : Code; continue; }
            if (c == '*' && i + 1 < para.size() && para[i + 1] == '*' && st != Code) { flush(); st = (st == Bold) ? Normal : Bold; ++i; continue; }
            if (c == ' ') { flush(); words.push_back({" ", st}); continue; }
            cur += c;
        }
        flush();

        if (bullet && draw) DrawCircle((int)(box.x + size * 0.4f), (int)(y + lineH * 0.5f), size * 0.16f, Theme::Orange);
        float x = box.x + indent;
        float maxX = box.x + box.width;
        bool lineStart = true;
        bool pendingSpace = false; Style spaceStyle = Normal;
        for (const Word& w : words) {
            if (w.s == " ") { pendingSpace = !lineStart; spaceStyle = w.st; continue; }
            FontKind fk = w.st == Code ? FontKind::Mono : (w.st == Bold ? FontKind::SansBold : FontKind::Sans);
            int fs = w.st == Code ? codeSize : size;
            float ww = Measure(w.s, fs, fk).x;
            float sw = pendingSpace ? spaceW : 0;
            bool codeJoin = w.st == Code && pendingSpace && spaceStyle == Code;
            if (w.st == Code && !codeJoin) sw += 4;   // breathing room around inline code chips
            if (codeJoin) sw -= 4;                     // ...but not inside a multi-word chip
            if (!lineStart && x + sw + ww > maxX) {
                y += lineH; x = box.x + indent; sw = 0; lineStart = true;
            }
            if (draw) {
                if (w.st == Code) {
                    float bgX = x + sw - 3;
                    if (pendingSpace && spaceStyle == Code) bgX = x - 1;
                    DrawRectangleRounded({bgX, y + lineH * 0.08f, (x + sw + ww + 3) - bgX, lineH * 0.84f}, 0.3f, 4, {43, 179, 163, 38});
                    Text(w.s, x + sw, y + (lineH - fs) / 2.0f, fs, {120, 220, 205, 255}, fk);
                } else {
                    Text(w.s, x + sw, y + (lineH - fs) / 2.0f - 1, fs, w.st == Bold ? Theme::Text : color, fk);
                }
            }
            x += sw + ww + (w.st == Code ? 4.0f : 0.0f);
            lineStart = false; pendingSpace = false;
        }
        y += lineH;
    }
    return y - box.y;
}

int CodeBlock(const std::string& code, Rectangle r, int fontSize, int highlightLine, bool selectable,
              Color highlightColor, int markLine, Color markColor) {
    std::vector<std::string> lines = SplitLines(code);
    if (lines.empty()) lines.push_back("");
    size_t maxLen = 0;
    for (auto& l : lines) maxLen = std::max(maxLen, l.size());
    const float pad = 12;
    int gutterChars = lines.size() >= 10 ? 2 : 1;

    if (fontSize <= 0) {
        // auto-fit: monospace advance is ~0.6em, line height 1.4em
        float byH = (r.height - pad * 2) / (lines.size() * 1.4f);
        float byW = (r.width - pad * 2 - 14) / ((maxLen + gutterChars + 1) * 0.6f);
        fontSize = (int)std::floor(std::min({18.0f, byH, byW}));
        fontSize = std::max(11, fontSize);
    } else {
        // fixed size requested, but never let long lines run off the right edge
        float byW = (r.width - pad * 2 - 14) / ((maxLen + gutterChars + 1) * 0.6f);
        fontSize = std::max(11, std::min(fontSize, (int)std::floor(byW)));
    }
    Font& f = GetFont(FontKind::Mono, fontSize);
    float cw = MeasureTextEx(f, "M", (float)fontSize, 0).x;
    float lh = fontSize * 1.4f;
    float gutterW = cw * (gutterChars + 1) + 10;

    DrawRectangleRounded(r, 0.03f, 6, Theme::CodeBg);
    DrawRectangleRoundedLinesEx(r, 0.03f, 6, 1, Theme::Line);
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);

    int clicked = 0;
    bool inBlock = false;
    Vector2 mouse = GetMousePosition();
    for (size_t li = 0; li < lines.size(); ++li) {
        float y = r.y + pad + li * lh;
        if (y > r.y + r.height) break;
        Rectangle lineRect = {r.x + 2, y, r.width - 4, lh};
        int ln = (int)li + 1;
        if (ln == highlightLine) {
            DrawRectangleRec(lineRect, Fade(highlightColor, 0.18f));
            DrawRectangle((int)r.x + 2, (int)y, 4, (int)lh, highlightColor);
        }
        if (ln == markLine) {
            DrawRectangleRec(lineRect, Fade(markColor, 0.22f));
            DrawRectangle((int)r.x + 2, (int)y, 4, (int)lh, markColor);
        }
        if (selectable && CheckCollisionPointRec(mouse, lineRect) && !lines[li].empty()) {
            DrawRectangleRec(lineRect, Fade(Theme::Orange, 0.12f));
            DrawRectangleLinesEx(lineRect, 1, Fade(Theme::Orange, 0.6f));
            g_wantHand = true;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) clicked = ln;
        }
        std::string num = std::to_string(ln);
        float nx = r.x + pad + cw * (gutterChars - (int)num.size());
        DrawTextEx(f, num.c_str(), {nx, y + (lh - fontSize) / 2}, (float)fontSize, 0, ln == highlightLine ? highlightColor : Theme::Dim);

        float x = r.x + pad + gutterW;
        for (const Tok& t : Highlight(lines[li], inBlock)) {
            DrawTextEx(f, t.s.c_str(), {x, y + (lh - fontSize) / 2}, (float)fontSize, 0, t.c);
            x += cw * t.s.size();
        }
    }
    EndScissorMode();
    return clicked;
}

bool Hover(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
bool Clicked(Rectangle r) { return Hover(r) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }

Color Lighten(Color c, float amt) {
    auto l = [&](unsigned char v) { return (unsigned char)std::min(255.0f, v + (255 - v) * amt); };
    return {l(c.r), l(c.g), l(c.b), c.a};
}

bool Button(Rectangle r, const std::string& label, BtnStyle style, bool enabled, int fontSize) {
    bool hover = enabled && Hover(r);
    bool down = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Color fill, text, border;
    switch (style) {
        case BtnStyle::Primary:   fill = Theme::Orange; text = Theme::Bg; border = Theme::Orange; break;
        case BtnStyle::Secondary: fill = Theme::Panel2; text = Theme::Text; border = Theme::Line; break;
        case BtnStyle::Ghost:     fill = {0, 0, 0, 0}; text = Theme::Muted; border = Theme::Line; break;
        case BtnStyle::Danger:    fill = Theme::Red; text = Theme::Text; border = Theme::Red; break;
        case BtnStyle::Success:   fill = Theme::Green; text = Theme::Bg; border = Theme::Green; break;
    }
    if (!enabled) { fill = Fade(fill, fill.a ? 0.35f : 0); text = Fade(text, 0.45f); border = Fade(border, 0.4f); }
    if (hover) {
        if (style == BtnStyle::Ghost) { fill = Fade(Theme::Panel2, 0.8f); text = Theme::Text; }
        else fill = Lighten(fill, 0.15f);
        g_wantHand = true;
    }
    Rectangle rr = r;
    if (down) { rr.y += 2; }
    else if (enabled && style != BtnStyle::Ghost) DrawRectangleRounded({r.x, r.y + 3, r.width, r.height}, 0.25f, 6, Fade(BLACK, 0.35f));
    DrawRectangleRounded(rr, 0.25f, 6, fill);
    DrawRectangleRoundedLinesEx(rr, 0.25f, 6, 1.0f, border);
    TextCentered(label, rr, fontSize, text, FontKind::SansBold);
    return hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

void Panel(Rectangle r, Color fill, Color border, float roundness) {
    DrawRectangleRounded(r, roundness, 8, fill);
    if (border.a) DrawRectangleRoundedLinesEx(r, roundness, 8, 1.0f, border);
}

void Tag(float x, float y, const std::string& text, Color bg, Color fg, int size) {
    Vector2 m = Measure(text, size, FontKind::SansBold);
    Rectangle r = {x, y, m.x + 14, size + 8.0f};
    DrawRectangleRounded(r, 0.5f, 6, bg);
    TextCentered(text, r, size, fg, FontKind::SansBold);
}

void ProgressBar(Rectangle r, float t, Color fill, Color bg) {
    t = std::clamp(t, 0.0f, 1.0f);
    DrawRectangleRounded(r, 1.0f, 6, bg);
    if (t > 0.001f) DrawRectangleRounded({r.x, r.y, std::max(r.height, r.width * t), r.height}, 1.0f, 6, fill);
}

void Block(float x, float y, float s, Color c, const std::string& label) {
    // a memory cell: rounded square with a highlight edge
    DrawRectangleRounded({x + 1, y + 3, s, s}, 0.25f, 4, Fade(BLACK, 0.35f));
    DrawRectangleRounded({x, y, s, s}, 0.25f, 4, c);
    DrawRectangleRounded({x + s * 0.12f, y + s * 0.1f, s * 0.76f, s * 0.18f}, 1.0f, 4, Fade(WHITE, 0.25f));
    if (!label.empty()) {
        int fs = std::max(9, (int)(s * 0.28f));
        Vector2 m = Measure(label, fs, FontKind::MonoBold);
        Text(label, x + (s - m.x) / 2, y + (s - fs) / 2 + 1, fs, Theme::Bg, FontKind::MonoBold);
    }
}

void Star(Vector2 c, float r, Color col) {
    Vector2 pts[10];
    for (int i = 0; i < 10; ++i) {
        float ang = -PI / 2 + i * PI / 5;
        float rr = (i % 2 == 0) ? r : r * 0.45f;
        pts[i] = {c.x + std::cos(ang) * rr, c.y + std::sin(ang) * rr};
    }
    for (int i = 0; i < 10; ++i) DrawTriangle(c, pts[(i + 1) % 10], pts[i], col);
}

void Padlock(float x, float y, float s, Color c) {
    DrawRing({x + s / 2, y + s * 0.42f}, s * 0.2f, s * 0.3f, 180, 360, 16, c);
    DrawRectangleRounded({x + s * 0.12f, y + s * 0.4f, s * 0.76f, s * 0.55f}, 0.25f, 4, c);
    DrawCircle((int)(x + s / 2), (int)(y + s * 0.62f), s * 0.07f, Theme::Bg);
}

void ChapterNode(Vector2 c, float r, Color col, bool locked, bool done, int number, float pulse) {
    if (pulse > 0) DrawCircleV(c, r + 6 + pulse * 10, Fade(col, 0.25f * (1 - pulse)));
    DrawCircleV({c.x + 2, c.y + 5}, r, Fade(BLACK, 0.4f));
    Color fill = locked ? Theme::Panel2 : col;
    DrawCircleV(c, r, fill);
    DrawRing(c, r - 5, r - 2, 0, 360, 48, locked ? Theme::Line : Fade(WHITE, 0.35f));
    if (locked) { Padlock(c.x - r * 0.35f, c.y - r * 0.42f, r * 0.7f, Theme::Dim); return; }
    TextCentered(std::to_string(number), {c.x - r, c.y - r, r * 2, r * 2}, (int)(r * 0.9f), Theme::Bg, FontKind::Display);
    if (done) {
        Vector2 b = {c.x + r * 0.72f, c.y - r * 0.72f};
        DrawCircleV(b, r * 0.34f, Theme::Green);
        DrawCircleLinesV(b, r * 0.34f, Theme::Bg);
        CheckMark(b.x - r * 0.17f, b.y - r * 0.17f, r * 0.34f, Theme::Bg);
    }
}

void CheckMark(float x, float y, float s, Color c) {
    DrawLineEx({x, y + s * 0.55f}, {x + s * 0.38f, y + s * 0.9f}, s * 0.16f, c);
    DrawLineEx({x + s * 0.38f, y + s * 0.9f}, {x + s, y + s * 0.15f}, s * 0.16f, c);
}

void CrossMark(float x, float y, float s, Color c) {
    DrawLineEx({x, y}, {x + s, y + s}, s * 0.16f, c);
    DrawLineEx({x + s, y}, {x, y + s}, s * 0.16f, c);
}

} // namespace UI
