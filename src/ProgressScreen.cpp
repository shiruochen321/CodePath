// ProgressScreen.cpp - per-chapter progress, exam practice bests and badges.
#include "Screens.h"
#include "UI.h"

void ProgressScreen::Update(float) {}

void ProgressScreen::Draw() {
    App& app = TheApp();
    Progress& P = app.progress;
    const auto& mods = app.content.Modules();

    UI::Text("CHAPTERS", 24, 80, 12, Theme::Muted, FontKind::MonoBold);
    float cw = (1232 - 3 * 14) / 4.0f, ch = 156;
    int passed = 0, labs = 0;
    for (int i = 0; i < (int)mods.size() && i < 7; ++i) {
        const Module& m = mods[i];
        const ModuleProgress& mp = P.Module(m.id);
        bool open = P.IsUnlocked(m.id);
        bool done = mp.bestScore >= Progress::kPassMark;
        if (done) passed++;
        if (mp.labDone) labs++;
        Rectangle r = {24 + (i % 4) * (cw + 14), 100 + (i / 4) * (ch + 14), cw, ch};
        UI::Panel(r, Theme::Panel, open ? Fade(m.color, 0.6f) : Theme::Line, 0.06f);
        UI::ChapterNode({r.x + 38, r.y + 38}, 22, m.color, !open, done, m.id);
        UI::Text("CHAPTER " + std::to_string(m.id), r.x + 74, r.y + 18, 11, open ? m.color : Theme::Dim, FontKind::MonoBold);
        UI::Text(m.title, r.x + 74, r.y + 34, 16, open ? Theme::Text : Theme::Dim, FontKind::SansBold);
        UI::Text(m.topic, r.x + 16, r.y + 70, 13, Theme::Muted);
        UI::Text("Best", r.x + 16, r.y + 96, 13, Theme::Muted);
        UI::ProgressBar({r.x + 60, r.y + 101, cw - 130, 7}, mp.bestScore / 100.0f, done ? Theme::Green : Theme::Orange);
        UI::Text(std::to_string(mp.bestScore) + "%", r.x + cw - 58, r.y + 94, 15, Theme::Text, FontKind::MonoBold);
        std::string line = std::to_string(P.LessonsSeenCount(m.id)) + "/" + std::to_string(m.lessons.size()) + " lessons   lab " +
                           (mp.labDone ? "done" : "--") + "   " + std::to_string(mp.attempts) + " tries";
        UI::Text(line, r.x + 16, r.y + 124, 13, Theme::Dim, FontKind::Mono);
        if (open && UI::Clicked(r)) app.Goto(std::make_unique<ModuleScreen>(m.id));
    }
    // summary card in the 8th slot
    {
        Rectangle r = {24 + 3 * (cw + 14), 100 + ch + 14, cw, ch};
        UI::Panel(r, Theme::Bg2, Theme::Line, 0.06f);
        UI::Text("EXAM PRACTICE BESTS", r.x + 16, r.y + 16, 11, Theme::Muted, FontKind::MonoBold);
        UI::Text("Exam 1", r.x + 16, r.y + 44, 15, Theme::Text);
        UI::ProgressBar({r.x + 84, r.y + 50, cw - 150, 7}, P.examBest[1] / 100.0f, Theme::Orange);
        UI::Text(std::to_string(P.examBest[1]) + "%", r.x + cw - 58, r.y + 42, 15, Theme::Text, FontKind::MonoBold);
        UI::Text("Exam 2", r.x + 16, r.y + 76, 15, Theme::Text);
        UI::ProgressBar({r.x + 84, r.y + 82, cw - 150, 7}, P.examBest[2] / 100.0f, Theme::Red);
        UI::Text(std::to_string(P.examBest[2]) + "%", r.x + cw - 58, r.y + 74, 15, Theme::Text, FontKind::MonoBold);
        UI::Text(std::to_string(passed) + "/" + std::to_string(mods.size()) + " chapters passed", r.x + 16, r.y + 116, 13, Theme::Muted, FontKind::Mono);
    }

    // badges
    float by = 100 + 2 * (ch + 14) + 10;
    UI::Text("BADGES", 24, by, 12, Theme::Muted, FontKind::MonoBold);
    struct Badge { const char* name; const char* how; bool earned; Color c; };
    Badge badges[] = {
        {"Hello, World", "Answer your first question correctly", P.xp > 0, Theme::Orange},
        {"Compiler Friend", "Pass chapter 1", P.Module(1).bestScore >= Progress::kPassMark, Theme::Teal},
        {"Loop Master", "Pass chapter 3", P.Module(3).bestScore >= Progress::kPassMark, Theme::Yellow},
        {"Pointer Pro", "Pass chapter 5", P.Module(5).bestScore >= Progress::kPassMark, Theme::Red},
        {"Debugger", "Finish 4 Code Labs", labs >= 4, Theme::Blue},
        {"Exam 1 Ready", "Score 70%+ in Exam 1 practice", P.examBest[1] >= 70, Theme::Orange},
        {"Exam 2 Ready", "Score 70%+ in Exam 2 practice", P.examBest[2] >= 70, Theme::Red},
        {"Graduate", "Pass all 7 chapters", passed >= (int)mods.size() && !mods.empty(), Theme::Green},
    };
    float bw = (1232 - 7 * 10) / 8.0f;
    for (int i = 0; i < 8; ++i) {
        Rectangle r = {24 + i * (bw + 10), by + 20, bw, 116};
        const Badge& b = badges[i];
        UI::Panel(r, b.earned ? Fade(b.c, 0.12f) : Theme::Panel, b.earned ? b.c : Theme::Line, 0.1f);
        Vector2 c = {r.x + r.width / 2, r.y + 34};
        if (b.earned) {
            DrawPoly(c, 6, 22, 30, b.c);
            UI::Star(c, 11, Theme::Bg);
        } else {
            DrawPolyLinesEx(c, 6, 22, 30, 2, Theme::Dim);
            UI::Padlock(c.x - 9, c.y - 11, 18, Theme::Dim);
        }
        UI::TextCentered(b.name, {r.x, r.y + 64, r.width, 18}, 14, b.earned ? Theme::Text : Theme::Muted, FontKind::SansBold);
        UI::RichText(b.how, {r.x + 10, r.y + 84, r.width - 20, 40}, 12, Theme::Dim);
    }

    // bottom actions
    float y = kScreenH - 62;
    UI::Star({36, y + 22}, 12, Theme::Yellow);
    UI::Text(std::to_string(P.xp) + " XP earned in total", 56, y + 12, 17, Theme::Yellow, FontKind::MonoBold);
    if (UI::Button({kScreenW - 520, y, 250, 42}, P.unlockAll ? "Demo mode: ON (all open)" : "Demo mode: unlock all",
                   P.unlockAll ? BtnStyle::Success : BtnStyle::Secondary, true, 15)) {
        P.unlockAll = !P.unlockAll; P.Save();
    }
    if (!confirmReset_) {
        if (UI::Button({kScreenW - 258, y, 234, 42}, "Reset progress", BtnStyle::Ghost, true, 15)) confirmReset_ = true;
    } else {
        if (UI::Button({kScreenW - 258, y, 140, 42}, "Yes, wipe it", BtnStyle::Danger, true, 15)) { P.Reset(); confirmReset_ = false; }
        if (UI::Button({kScreenW - 110, y, 86, 42}, "Cancel", BtnStyle::Ghost, true, 15)) confirmReset_ = false;
    }

    if (TopBar("Progress & Badges", "Your learning record. Click a chapter to revisit it.", Theme::Yellow, "Path"))
        app.Goto(std::make_unique<MapScreen>());
}
