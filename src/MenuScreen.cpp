// MenuScreen.cpp - title screen with a self-typing code editor.
#include "Screens.h"
#include "UI.h"
#include <cmath>
#include <algorithm>

namespace {
    const char* kDemo =
R"(#include <stdio.h>

int main(void) {
    int score = 0;
    for (int ch = 1; ch <= 7; ch++) {
        score += 10;      // learn, practise
    }
    printf("Ready for the exam: %d XP\n", score);
    return 0;
})";
}

void MenuScreen::Update(float dt) {
    t_ += dt;
    if (IsKeyPressed(KEY_ENTER)) TheApp().Goto(std::make_unique<MapScreen>());
}

void MenuScreen::Draw() {
    App& app = TheApp();
    for (int y = 20; y < kScreenH; y += 32)
        for (int x = 20; x < kScreenW; x += 32) DrawPixel(x, y, Theme::Line);

    // ---- right: editor window that types the demo program, then "runs" it
    Rectangle win = {680, 110, 540, 400};
    DrawRectangleRounded({win.x + 6, win.y + 10, win.width, win.height}, 0.04f, 6, Fade(BLACK, 0.4f));
    UI::Panel(win, Theme::Panel, Theme::Line, 0.04f);
    DrawCircle((int)win.x + 20, (int)win.y + 18, 6, Theme::Red);
    DrawCircle((int)win.x + 38, (int)win.y + 18, 6, Theme::Yellow);
    DrawCircle((int)win.x + 56, (int)win.y + 18, 6, Theme::Green);
    UI::Text("chapter1.c", win.x + 78, win.y + 9, 14, Theme::Muted, FontKind::Mono);

    std::string full = kDemo;
    float cycle = std::fmod(t_, 14.0f);
    size_t shown = std::min(full.size(), (size_t)(cycle * 28));
    std::string typed = full.substr(0, shown);
    int curLine = 1 + (int)std::count(typed.begin(), typed.end(), '\n');
    UI::CodeBlock(typed.empty() ? " " : typed, {win.x + 12, win.y + 38, win.width - 24, 268}, 16, shown < full.size() ? curLine : -1, false, Theme::Orange);

    // terminal under the editor
    Rectangle term = {win.x + 12, win.y + 316, win.width - 24, 72};
    DrawRectangleRounded(term, 0.1f, 6, {5, 8, 12, 255});
    UI::Text("$ gcc chapter1.c -o chapter1 && ./chapter1", term.x + 12, term.y + 10, 14, Theme::Dim, FontKind::Mono);
    if (shown >= full.size()) {
        UI::Text("Ready for the exam: 70 XP", term.x + 12, term.y + 36, 16, {170, 230, 170, 255}, FontKind::Mono);
    } else if (std::fmod(t_, 1.0f) < 0.5f) {
        DrawRectangle((int)term.x + 12, (int)term.y + 38, 9, 16, Theme::Dim);
    }

    // memory strip under the window: the "score" variable growing
    UI::Text("MEMORY", win.x, win.y + win.height + 30, 12, Theme::Muted, FontKind::MonoBold);
    int score = shown >= full.size() ? 7 : std::clamp((int)((cycle - 4.0f) * 1.2f), 0, 7);
    for (int i = 0; i < 7; ++i) {
        float x = win.x + 70 + i * 50;
        if (i < score) UI::Block(x, win.y + win.height + 20, 38, i % 2 ? Theme::Teal : Theme::Blue, "10");
        else DrawRectangleRoundedLinesEx({x, win.y + win.height + 20, 38, 38}, 0.25f, 4, 1, Theme::Line);
    }

    // ---- left: title block
    UI::Tag(80, 104, "SETU  ·  SOFTWARE DEVELOPMENT  ·  C / C++", Theme::Panel2, Theme::Muted, 13);
    UI::Text("Code", 76, 136, 96, Theme::Text, FontKind::Display);
    float cw = UI::Measure("Code", 96, FontKind::Display).x;
    UI::Text("Path", 76 + cw + 4, 136, 96, Theme::Orange, FontKind::Display);
    UI::Text("Interactive C & C++ learning app", 80, 240, 22, Theme::Text, FontKind::SansBold);
    UI::RichText("Short lessons, a step-through Code Lab that shows you what happens in memory, and quizzes modelled on **Coding Exam 1 and 2**.",
                 {80, 276, 540, 80}, 18, Theme::Muted);

    float bx = 80, by = 384;
    if (UI::Button({bx, by, 250, 54}, "Start learning  >", BtnStyle::Primary, true, 21)) app.Goto(std::make_unique<MapScreen>());
    if (UI::Button({bx + 266, by, 250, 54}, "Exam 1 practice", BtnStyle::Secondary, true, 18)) app.Goto(std::make_unique<QuizScreen>(QuizScreen::ForExam(1)));
    if (UI::Button({bx, by + 68, 250, 46}, "Progress & badges", BtnStyle::Secondary, true, 17)) app.Goto(std::make_unique<ProgressScreen>());
    if (UI::Button({bx + 266, by + 68, 250, 46}, "Exam 2 practice", BtnStyle::Secondary, true, 17)) app.Goto(std::make_unique<QuizScreen>(QuizScreen::ForExam(2)));
    if (UI::Button({bx, by + 128, 120, 40}, "Quit", BtnStyle::Ghost, true, 16)) app.Quit();

    // footer
    DrawRectangle(0, kScreenH - 48, kScreenW, 1, Theme::Line);
    int modules = (int)app.content.Modules().size();
    int lessons = 0;
    for (auto& m : app.content.Modules()) lessons += (int)m.lessons.size();
    std::string stats = std::to_string(modules) + " chapters  ·  " + std::to_string(lessons) + " lessons  ·  " +
                        std::to_string(app.content.TotalQuestions()) + " questions  ·  " + std::to_string(app.progress.xp) + " XP earned";
    UI::Text(stats, 80, kScreenH - 32, 15, Theme::Muted, FontKind::Mono);
    std::string team = kTeamNames;
    UI::Text(team, kScreenW - 40 - UI::Measure(team, 15).x, kScreenH - 32, 15, Theme::Dim);
}
