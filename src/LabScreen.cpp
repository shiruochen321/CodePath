// LabScreen.cpp - step-through "debugger" for a small program, with memory drawn as stacks of blocks.
#include "Screens.h"
#include "UI.h"
#include <algorithm>
#include <cmath>

LabScreen::LabScreen(int moduleId) : id_(moduleId) {}

std::string LabScreen::ConsoleUpTo(int step) const {
    const Module* m = TheApp().content.Get(id_);
    std::string out;
    if (!m) return out;
    for (int i = 0; i <= step && i < (int)m->lab.steps.size(); ++i) out += m->lab.steps[i].output;
    return out;
}

void LabScreen::Update(float dt) {
    const Module* m = TheApp().content.Get(id_);
    if (!m) return;
    int n = (int)m->lab.steps.size();
    step_ = std::clamp(step_, -1, n - 1);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_SPACE)) { step_ = std::min(step_ + 1, n - 1); playing_ = false; }
    if (IsKeyPressed(KEY_LEFT)) { step_ = std::max(step_ - 1, -1); playing_ = false; }
    if (IsKeyPressed(KEY_R)) { step_ = -1; playing_ = false; }
    if (IsKeyPressed(KEY_P)) playing_ = !playing_;
    if (playing_) {
        timer_ += dt;
        if (timer_ > 1.15f) { timer_ = 0; if (step_ < n - 1) step_++; else playing_ = false; }
    }
    if (n > 0 && step_ == n - 1) TheApp().progress.MarkLab(id_);

    // animate block counts
    std::vector<std::pair<std::string, int>> bins;
    if (step_ >= 0) bins = m->lab.steps[step_].bins;
    else if (n > 0) { bins = m->lab.steps[0].bins; for (auto& b : bins) b.second = 0; }
    shown_.resize(bins.size(), 0.0f);
    for (size_t i = 0; i < bins.size(); ++i) {
        float target = (float)bins[i].second;
        shown_[i] += (target - shown_[i]) * std::min(1.0f, dt * 7.0f);
        if (std::fabs(target - shown_[i]) < 0.02f) shown_[i] = target;
    }
}

void LabScreen::Draw() {
    App& app = TheApp();
    const Module* m = app.content.Get(id_);
    if (!m) { app.Goto(std::make_unique<MapScreen>()); return; }
    const TraceLab& lab = m->lab;
    int n = (int)lab.steps.size();
    const LabStep* cur = step_ >= 0 && step_ < n ? &lab.steps[step_] : nullptr;
    const LabStep* prev = step_ > 0 ? &lab.steps[step_ - 1] : nullptr;

    // ------------ code
    UI::Text("PROGRAM", 24, 80, 12, Theme::Muted, FontKind::MonoBold);
    UI::Text(lab.title, 96, 78, 15, Theme::Text, FontKind::SansBold);
    UI::CodeBlock(lab.code, {24, 102, 636, 380}, 0, cur ? cur->line : -1, false, Theme::Yellow);

    // ------------ explanation
    Rectangle note = {24, 494, 636, 118};
    UI::Panel(note, Theme::Panel, Theme::Line, 0.08f);
    std::string head = cur ? "STEP " + std::to_string(step_ + 1) + " / " + std::to_string(n) + "   ·   LINE " + std::to_string(cur->line) : "BRIEFING";
    UI::Text(head, note.x + 16, note.y + 12, 12, cur ? Theme::Yellow : Theme::Teal, FontKind::MonoBold);
    UI::RichText(cur ? cur->note : lab.intro, {note.x + 16, note.y + 32, note.width - 32, note.height - 36}, 16, Theme::Text);
    UI::ProgressBar({note.x, note.y + note.height + 6, note.width, 4}, n ? (step_ + 1) / (float)n : 0, m->color);

    // ------------ controls
    float cy = kScreenH - 70;
    if (UI::Button({24, cy, 96, 44}, "Reset", BtnStyle::Ghost, step_ >= 0, 15)) { step_ = -1; playing_ = false; }
    if (UI::Button({128, cy, 110, 44}, "< Back", BtnStyle::Secondary, step_ >= 0, 16)) { step_--; playing_ = false; }
    if (UI::Button({246, cy, 130, 44}, playing_ ? "Pause" : "Auto-play", BtnStyle::Secondary, step_ < n - 1, 16)) { playing_ = !playing_; timer_ = 0.8f; }
    if (step_ < n - 1) {
        if (UI::Button({384, cy, 276, 44}, step_ < 0 ? "Start executing  >" : "Next line  >", BtnStyle::Primary, true, 17)) { step_++; playing_ = false; }
    } else {
        if (UI::Button({384, cy, 276, 44}, "Chapter Quiz  >", BtnStyle::Success, true, 17)) app.Goto(std::make_unique<QuizScreen>(QuizScreen::ForModule(id_)));
    }

    // ------------ memory view
    Rectangle floor = {680, 80, 576, 252};
    UI::Panel(floor, Theme::Bg2, Theme::Line, 0.05f);
    UI::Text("MEMORY", floor.x + 16, floor.y + 12, 12, Theme::Muted, FontKind::MonoBold);
    // pick a block size so the biggest value in this lab fits (1, 2, 5, 10, 20 ...)
    int maxVal = 1;
    for (const LabStep& st : lab.steps) for (auto& b : st.bins) maxVal = std::max(maxVal, b.second);
    int unit = 1;
    for (int u : {1, 2, 5, 10, 20, 50, 100, 1000}) { unit = u; if (maxVal / u <= 16) break; }
    std::string unitLabel = "each block = " + std::to_string(unit);
    UI::Text(unitLabel, floor.x + floor.width - 16 - UI::Measure(unitLabel, 12).x, floor.y + 12, 12, Theme::Dim);
    std::vector<std::pair<std::string, int>> bins;
    if (cur) bins = cur->bins; else if (n > 0) { bins = lab.steps[0].bins; for (auto& b : bins) b.second = 0; }
    int nb = (int)bins.size();
    if (nb > 0) {
        float slotW = (floor.width - 32) / nb;
        float binW = std::min(112.0f, slotW - 10);
        float s = std::clamp(binW * 0.36f, 12.0f, 26.0f);
        float baseY = floor.y + floor.height - 44;
        for (int i = 0; i < nb; ++i) {
            float cx = floor.x + 16 + slotW * i + slotW / 2;
            // base line for the stack
            DrawRectangleRounded({cx - binW / 2, baseY, binW, 4}, 1.0f, 4, Theme::Line);
            float shown = i < (int)shown_.size() ? shown_[i] : (float)bins[i].second;
            int count = (int)std::ceil(shown / unit - 0.01f);
            int maxRows = (int)((baseY - floor.y - 70) / (s + 2));
            int maxCols = std::max(2, (int)((binW + 8) / (s + 2)));
            int cols = std::clamp((count + maxRows - 1) / std::max(1, maxRows), 2, maxCols);
            int drawN = std::min(count, cols * maxRows);
            for (int k = 0; k < drawN; ++k) {
                int col = k % cols, row = k / cols;
                float x = cx - cols * (s + 2) / 2 + 1 + col * (s + 2);
                float y = baseY - (row + 1) * (s + 2);
                UI::Block(x, y, s, (k % 2 == 0) ? m->color : UI::Lighten(m->color, 0.25f));
            }
            bool changed = prev && i < (int)prev->bins.size() && prev->bins[i].second != bins[i].second;
            std::string val = std::to_string(bins[i].second);
            int rowsUsed = (drawN + cols - 1) / cols;
            float topY = baseY - rowsUsed * (s + 2) - 26;
            UI::TextCentered(val, {cx - 40, topY, 80, 22}, 18, changed ? Theme::Yellow : Theme::Text, FontKind::MonoBold);
            if (count > cols * maxRows) UI::TextCentered("+" + std::to_string(count - cols * maxRows), {cx - 40, topY - 18, 80, 16}, 12, Theme::Muted);
            std::string name = bins[i].first;
            UI::TextCentered(name, {cx - slotW / 2, baseY + 14, slotW, 18}, 13, Theme::Muted, FontKind::Mono);
        }
    }

    // ------------ variables
    Rectangle vars = {680, 344, 576, 156};
    UI::Panel(vars, Theme::Panel, Theme::Line, 0.06f);
    UI::Text("VARIABLE WATCH", vars.x + 16, vars.y + 12, 12, Theme::Muted, FontKind::MonoBold);
    if (cur) {
        int i = 0;
        for (auto& [name, value] : cur->vars) {
            int col = i % 2, row = i / 2;
            if (row > 4) break;
            float x = vars.x + 16 + col * 280, y = vars.y + 36 + row * 23;
            bool changed = true;
            if (prev) for (auto& pv : prev->vars) if (pv.first == name) changed = pv.second != value;
            if (changed) DrawRectangleRounded({x - 6, y - 2, 270, 21}, 0.3f, 4, Fade(Theme::Yellow, 0.12f));
            UI::Text(name, x, y, 15, Theme::Blue, FontKind::Mono);
            std::string v = value;
            if (v.size() > 20) v = v.substr(0, 19) + "~";
            UI::Text("= " + v, x + 110, y, 15, changed ? Theme::Yellow : Theme::Text, FontKind::Mono);
            ++i;
        }
        if (cur->vars.empty()) UI::Text("(no local variables yet)", vars.x + 16, vars.y + 40, 15, Theme::Dim);
    } else {
        UI::Text("Press  Start executing  to run line by line.", vars.x + 16, vars.y + 40, 15, Theme::Dim);
        UI::Text("Keys: Space / -> next, <- back, P auto-play, R reset", vars.x + 16, vars.y + 64, 14, Theme::Dim, FontKind::Mono);
    }

    // ------------ console
    Rectangle con = {680, 512, 576, 192};
    DrawRectangleRounded(con, 0.05f, 6, {5, 8, 12, 255});
    DrawRectangleRoundedLinesEx(con, 0.05f, 6, 1, Theme::Line);
    DrawCircle((int)con.x + 18, (int)con.y + 16, 5, Theme::Red);
    DrawCircle((int)con.x + 34, (int)con.y + 16, 5, Theme::Yellow);
    DrawCircle((int)con.x + 50, (int)con.y + 16, 5, Theme::Green);
    UI::Text("console", con.x + 66, con.y + 8, 13, Theme::Dim, FontKind::Mono);
    std::string out = ConsoleUpTo(step_);
    // show last lines that fit
    std::vector<std::string> lines; { std::string c; for (char ch : out) { if (ch == '\n') { lines.push_back(c); c.clear(); } else c += ch; } lines.push_back(c); }
    int maxLines = 7;
    int start = std::max(0, (int)lines.size() - maxLines);
    float ly = con.y + 34;
    for (int i = start; i < (int)lines.size(); ++i, ly += 21) UI::Text(lines[i], con.x + 16, ly, 15, {170, 230, 170, 255}, FontKind::Mono);
    if (std::fmod(app.time, 1.0f) < 0.5f) {
        float cx = con.x + 16 + UI::Measure(lines.back(), 15, FontKind::Mono).x;
        DrawRectangle((int)cx + 1, (int)(ly - 21 + 2), 8, 16, {170, 230, 170, 255});
    }

    if (TopBar("Code Lab  ·  " + m->title, "Watch the program run line by line. Blocks show the values held in memory.", m->color, "Lessons"))
        app.Goto(std::make_unique<ModuleScreen>(id_, (int)m->lessons.size() - 1));
}
