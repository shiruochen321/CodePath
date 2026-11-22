// ModuleScreen.cpp - lesson pages for one chapter: explanation, example code, analogy, exam tip.
#include "Screens.h"
#include "UI.h"
#include <algorithm>

ModuleScreen::ModuleScreen(int moduleId, int page) : id_(moduleId), page_(page) {
    const Module* m = TheApp().content.Get(id_);
    if (m && !m->lessons.empty()) {
        page_ = std::clamp(page_, 0, (int)m->lessons.size() - 1);
        TheApp().progress.MarkLesson(id_, page_);
    }
}

void ModuleScreen::Update(float) {
    const Module* m = TheApp().content.Get(id_);
    if (!m) return;
    int n = (int)m->lessons.size();
    int old = page_;
    if (IsKeyPressed(KEY_RIGHT) && page_ < n - 1) page_++;
    if (IsKeyPressed(KEY_LEFT) && page_ > 0) page_--;
    if (old != page_) { scroll_ = 0; TheApp().progress.MarkLesson(id_, page_); }
    float wheel = GetMouseWheelMove();
    if (wheel != 0) scroll_ = std::clamp(scroll_ - wheel * 40, 0.0f, std::max(0.0f, contentH_ - 548));
}

void ModuleScreen::Draw() {
    App& app = TheApp();
    const Module* m = app.content.Get(id_);
    if (!m) { app.Goto(std::make_unique<MapScreen>()); return; }
    int n = (int)m->lessons.size();

    // ---------------- sidebar
    const ModuleProgress& mp0 = app.progress.Module(id_);
    DrawRectangle(0, 64, 300, kScreenH - 64, Theme::Bg2);
    DrawRectangle(299, 64, 1, kScreenH - 64, Theme::Line);
    UI::ChapterNode({50, 108}, 24, m->color, false, mp0.bestScore >= Progress::kPassMark, m->id);
    UI::Text("CHAPTER " + std::to_string(m->id), 94, 88, 12, m->color, FontKind::MonoBold);
    UI::Text(m->title, 94, 104, 17, Theme::Text, FontKind::SansBold);

    UI::Text("LESSONS", 24, 150, 12, Theme::Muted, FontKind::MonoBold);
    int seen = app.progress.Module(id_).lessonsSeen;
    for (int i = 0; i < n; ++i) {
        Rectangle r = {12, 170.0f + i * 44, 276, 40};
        bool active = i == page_;
        bool hov = UI::Hover(r);
        if (active) UI::Panel(r, Theme::Panel2, Fade(m->color, 0.7f), 0.2f);
        else if (hov) UI::Panel(r, Theme::Panel, {0, 0, 0, 0}, 0.2f);
        if ((seen >> i) & 1) UI::CheckMark(26, r.y + 13, 13, Theme::Green);
        else DrawCircleLines(32, (int)(r.y + 20), 6, Theme::Dim);
        std::string label = std::to_string(i + 1) + ". " + m->lessons[i].title;
        if (UI::Measure(label, 15).x > 230) { while (label.size() > 4 && UI::Measure(label + "...", 15).x > 230) label.pop_back(); label += "..."; }
        UI::Text(label, 50, r.y + 11, 15, active ? Theme::Text : Theme::Muted);
        if (UI::Clicked(r) && i != page_) { page_ = i; scroll_ = 0; app.progress.MarkLesson(id_, page_); }
    }
    float y = 182.0f + n * 44;
    UI::Text("PRACTICE", 24, y, 12, Theme::Muted, FontKind::MonoBold);
    const ModuleProgress& mp = app.progress.Module(id_);
    if (UI::Button({12, y + 20, 276, 46}, "Code Lab: step through", BtnStyle::Secondary, true, 16)) app.Goto(std::make_unique<LabScreen>(id_));
    if (mp.labDone) UI::CheckMark(258, y + 36, 14, Theme::Green);
    if (UI::Button({12, y + 76, 276, 46}, "Chapter Quiz  >", BtnStyle::Primary, true, 17)) app.Goto(std::make_unique<QuizScreen>(QuizScreen::ForModule(id_)));
    std::string best = "Best score: " + std::to_string(mp.bestScore) + "%   ·   attempts: " + std::to_string(mp.attempts);
    UI::Text(best, 24, y + 134, 13, Theme::Muted, FontKind::Mono);

    if (n == 0) { TopBar(m->title, m->topic, m->color); return; }
    const LessonPage& L = m->lessons[page_];

    // ---------------- main content
    const float top = 84, areaH = 548;
    bool hasCode = !L.code.empty();
    float leftX = 324, leftW = hasCode ? 404 : 912;
    float codeX = 746, codeW = 512;

    BeginScissorMode((int)leftX - 4, (int)top, (int)leftW + 8, (int)areaH);
    float cy = top - scroll_;
    UI::Text("LESSON " + std::to_string(page_ + 1) + " OF " + std::to_string(n), leftX, cy, 12, m->color, FontKind::MonoBold);
    cy += 18;
    float th = UI::RichText(L.title, {leftX, cy, leftW, 200}, 27, Theme::Text, false);
    // title in display font (single line titles expected)
    UI::Text(L.title, leftX, cy, 27, Theme::Text, FontKind::Display);
    cy += std::max(th, 36.0f) + 6;
    cy += UI::RichText(L.text, {leftX, cy, leftW, 2000}, 17, Theme::Text) + 12;

    auto card = [&](const std::string& head, const std::string& body, Color c) {
        if (body.empty()) return;
        float h = UI::RichText(body, {leftX + 18, 0, leftW - 34, 1000}, 15, Theme::Muted, false);
        Rectangle r = {leftX, cy, leftW, h + 44};
        UI::Panel(r, Fade(c, 0.08f), Fade(c, 0.5f), 0.08f);
        DrawRectangle((int)r.x, (int)r.y + 6, 3, (int)r.height - 12, c);
        UI::Text(head, leftX + 18, cy + 10, 12, c, FontKind::MonoBold);
        UI::RichText(body, {leftX + 18, cy + 30, leftW - 34, 1000}, 15, Theme::Text);
        cy += r.height + 12;
    };
    card("REAL-WORLD ANALOGY", L.analogy, Theme::Teal);
    card("EXAM TIP", L.tip, Theme::Yellow);
    contentH_ = cy + scroll_ - top;
    EndScissorMode();
    if (contentH_ > areaH) {
        float frac = areaH / contentH_;
        float barH = areaH * frac;
        float barY = top + (areaH - barH) * (scroll_ / (contentH_ - areaH));
        DrawRectangleRounded({leftX + leftW + 6, barY, 4, barH}, 1, 4, Theme::Line);
    }

    if (hasCode) {
        int lines = 1 + (int)std::count(L.code.begin(), L.code.end(), '\n');
        float h = lines * 24.0f + 24;
        int fs = 16;
        if (h > areaH - 30) { h = areaH - 30; fs = 0; }
        UI::Text("EXAMPLE", codeX, top, 12, Theme::Muted, FontKind::MonoBold);
        UI::CodeBlock(L.code, {codeX, top + 20, codeW, h}, fs);
    }

    // ---------------- bottom navigation
    float ny = kScreenH - 62;
    DrawRectangle(300, (int)ny - 10, kScreenW - 300, 1, Theme::Line);
    if (UI::Button({leftX, ny, 130, 42}, "< Previous", BtnStyle::Secondary, page_ > 0, 16)) { page_--; scroll_ = 0; app.progress.MarkLesson(id_, page_); }
    for (int i = 0; i < n; ++i) {
        float dx = 790 - n * 11 + i * 22.0f;
        DrawCircle((int)dx, (int)ny + 21, i == page_ ? 6 : 4, i == page_ ? m->color : Theme::Line);
    }
    if (page_ < n - 1) {
        if (UI::Button({kScreenW - 154, ny, 130, 42}, "Next  >", BtnStyle::Primary, true, 16)) { page_++; scroll_ = 0; app.progress.MarkLesson(id_, page_); }
    } else {
        if (UI::Button({kScreenW - 214, ny, 190, 42}, "Open Code Lab  >", BtnStyle::Primary, true, 16)) app.Goto(std::make_unique<LabScreen>(id_));
    }

    if (TopBar(m->title, m->topic, m->color)) app.Goto(std::make_unique<MapScreen>());
}
