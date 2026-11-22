// MapScreen.cpp - the learning path. Chapters unlock as quizzes are passed.
#include "Screens.h"
#include "UI.h"
#include <cmath>
#include <algorithm>

namespace {
    const float kNodeR = 36;

    struct Seg { Vector2 p1, c2, c3, p4; };
    Seg MakeSeg(Vector2 a, Vector2 b) {
        float mx = (a.x + b.x) / 2;
        return {a, {mx, a.y}, {mx, b.y}, b};
    }

    void DrawSeg(const Seg& s, bool open) {
        DrawSplineSegmentBezierCubic(s.p1, s.c2, s.c3, s.p4, 10, open ? Theme::Panel2 : Theme::Bg2);
        const int N = 36;
        for (int i = 0; i < N; i += 2) {
            Vector2 a = GetSplinePointBezierCubic(s.p1, s.c2, s.c3, s.p4, (float)i / N);
            Vector2 b = GetSplinePointBezierCubic(s.p1, s.c2, s.c3, s.p4, (float)(i + 1) / N);
            DrawLineEx(a, b, 3, open ? Fade(Theme::Orange, 0.8f) : Fade(Theme::Dim, 0.35f));
        }
    }

    void Trophy(Vector2 c, Color col) {
        DrawRectangleRounded({c.x - 22, c.y - 30, 44, 34}, 0.4f, 6, col);
        DrawRing({c.x - 22, c.y - 18}, 6, 10, 90, 270, 12, col);
        DrawRing({c.x + 22, c.y - 18}, 6, 10, -90, 90, 12, col);
        DrawRectangle((int)c.x - 5, (int)c.y + 2, 10, 14, col);
        DrawRectangleRounded({c.x - 18, c.y + 14, 36, 10}, 0.4f, 4, col);
        UI::Star({c.x, c.y - 14}, 9, Theme::Bg);
    }
}

MapScreen::MapScreen() {
    pos_ = {{140, 210}, {400, 160}, {660, 220}, {930, 160}, {1120, 380}, {840, 440}, {540, 450}};
}

void MapScreen::Update(float dt) { t_ += dt; }

void MapScreen::Draw() {
    App& app = TheApp();
    const auto& mods = app.content.Modules();
    for (int y = 80; y < kScreenH; y += 40)
        for (int x = 20; x < kScreenW; x += 40) DrawRectangle(x, y, 2, 2, Theme::Bg2);

    int n = std::min((int)mods.size(), (int)pos_.size());
    Vector2 goal = {220, 470};
    bool allDone = n > 0;
    for (int i = 0; i < n; ++i) if (app.progress.Module(mods[i].id).bestScore < Progress::kPassMark) allDone = false;

    for (int i = 0; i + 1 < n; ++i) DrawSeg(MakeSeg(pos_[i], pos_[i + 1]), app.progress.IsUnlocked(mods[i + 1].id));
    if (n > 0) DrawSeg(MakeSeg(pos_[n - 1], goal), allDone);

    // which chapter is "current": first unlocked chapter not yet passed
    int current = -1;
    for (int i = 0; i < n; ++i)
        if (app.progress.IsUnlocked(mods[i].id) && app.progress.Module(mods[i].id).bestScore < Progress::kPassMark) { current = i; break; }

    // goal
    Trophy(goal, allDone ? Theme::Yellow : Theme::Line);
    UI::TextCentered("EXAM READY", {goal.x - 80, goal.y + 34, 160, 18}, 14, allDone ? Theme::Yellow : Theme::Dim, FontKind::SansBold);
    UI::TextCentered(allDone ? "All chapters passed!" : "Pass every chapter", {goal.x - 90, goal.y + 54, 180, 16}, 13, Theme::Muted);

    hover_ = -1;
    for (int i = 0; i < n; ++i) {
        const Module& m = mods[i];
        bool open = app.progress.IsUnlocked(m.id);
        const ModuleProgress& mp = app.progress.Module(m.id);
        bool done = mp.bestScore >= Progress::kPassMark;
        Vector2 c = pos_[i];
        bool hov = UI::Hover({c.x - 90, c.y - kNodeR - 6, 180, kNodeR * 2 + 76});
        if (hov) hover_ = i;
        float r = kNodeR + ((hov && open) ? 3.0f : 0.0f);
        float pulse = (i == current) ? std::fmod(t_ * 0.8f, 1.0f) : 0.0f;
        UI::ChapterNode(c, r, m.color, !open, done, m.id, pulse);

        Color tc = open ? Theme::Text : Theme::Dim;
        UI::TextCentered(m.title, {c.x - 100, c.y + kNodeR + 12, 200, 18}, 15, tc, FontKind::SansBold);
        UI::TextCentered(m.topic, {c.x - 110, c.y + kNodeR + 31, 220, 16}, 13, open ? Theme::Muted : Theme::Dim);
        if (open) {
            Rectangle bar = {c.x - 40, c.y + kNodeR + 54, 80, 5};
            UI::ProgressBar(bar, mp.bestScore / 100.0f, done ? Theme::Green : Theme::Orange);
        }
        if (i == current) UI::Tag(c.x - 36, c.y - kNodeR - 30, "YOU ARE HERE", Theme::Orange, Theme::Bg, 11);
        if (hov && open && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) app.Goto(std::make_unique<ModuleScreen>(m.id));
    }

    // tooltip
    if (hover_ >= 0) {
        const Module& m = mods[hover_];
        bool open = app.progress.IsUnlocked(m.id);
        float w = 320;
        float x = pos_[hover_].x + kNodeR + 24;
        if (x + w > kScreenW - 10) x = pos_[hover_].x - kNodeR - 24 - w;
        float y = pos_[hover_].y - kNodeR;
        std::string body = open ? m.blurb : "Locked. Score " + std::to_string(Progress::kPassMark) + "% or more in the previous chapter's quiz to unlock it.";
        float h = UI::RichText(body, {x + 16, y, w - 32, 400}, 15, Theme::Muted, false);
        UI::Panel({x, y, w, h + 100}, Theme::Panel, m.color, 0.06f);
        UI::Text("CHAPTER " + std::to_string(m.id), x + 16, y + 14, 12, m.color, FontKind::MonoBold);
        UI::Text(m.title, x + 16, y + 28, 18, Theme::Text, FontKind::SansBold);
        UI::RichText(body, {x + 16, y + 56, w - 32, 400}, 15, Theme::Muted);
        if (open) {
            const ModuleProgress& mp = app.progress.Module(m.id);
            std::string s = std::to_string(app.progress.LessonsSeenCount(m.id)) + "/" + std::to_string(m.lessons.size()) + " lessons  ·  lab " +
                            (mp.labDone ? "done" : "to do") + "  ·  best " + std::to_string(mp.bestScore) + "%";
            UI::Text(s, x + 16, y + h + 68, 13, Theme::Yellow, FontKind::Mono);
        }
    }

    // bottom action bar
    DrawRectangle(0, kScreenH - 74, kScreenW, 74, Theme::Bg2);
    DrawRectangle(0, kScreenH - 74, kScreenW, 1, Theme::Line);
    UI::Text("EXAM PRACTICE", 24, kScreenH - 56, 12, Theme::Muted, FontKind::MonoBold);
    UI::Text("12 mixed questions, 10 min", 24, kScreenH - 38, 13, Theme::Dim);
    if (UI::Button({230, kScreenH - 58, 230, 42}, "Exam 1  (chapters 1-4)", BtnStyle::Primary, true, 15)) app.Goto(std::make_unique<QuizScreen>(QuizScreen::ForExam(1)));
    if (UI::Button({472, kScreenH - 58, 230, 42}, "Exam 2  (chapters 5-7)", BtnStyle::Primary, true, 15)) app.Goto(std::make_unique<QuizScreen>(QuizScreen::ForExam(2)));
    if (UI::Button({kScreenW - 206, kScreenH - 58, 182, 42}, "Progress", BtnStyle::Secondary, true, 15)) app.Goto(std::make_unique<ProgressScreen>());

    if (TopBar("Learning Path", "Each chapter: lessons -> Code Lab -> quiz. Score " + std::to_string(Progress::kPassMark) + "% to unlock the next one.", Theme::Orange, "Menu"))
        app.Goto(std::make_unique<MenuScreen>());
}
