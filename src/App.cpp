// App.cpp - window setup, main loop, screen switching and the shared top bar.
#include "App.h"
#include "Screens.h"
#include "UI.h"
#include <cstring>
#include <functional>
#include <vector>

static App g_app;
App& TheApp() { return g_app; }

static std::string FindAssets() {
    std::string appDir = GetApplicationDirectory();
    const std::string candidates[] = {"assets", "../assets", "../../assets", appDir + "assets", appDir + "../assets", appDir + "../../assets"};
    for (const auto& c : candidates)
        if (DirectoryExists((c + "/content").c_str())) return c;
    return "assets";
}

bool App::Init(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--tour") == 0 && i + 1 < argc) tourDir_ = argv[++i];

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(kScreenW, kScreenH, "CodePath  |  Interactive C & C++ Learning App");
    SetExitKey(KEY_NULL);   // ESC is used for "back"
    SetTargetFPS(60);

    assetDir = FindAssets();
    UI::Init(assetDir);
    if (!content.LoadDirectory(assetDir + "/content")) {
        TraceLog(LOG_ERROR, "Content error: %s", content.Error().c_str());
    }
    std::string save = tourDir_.empty() ? std::string(GetApplicationDirectory()) + "progress.sav" : tourDir_ + "/tour_progress.sav";
    progress.Load(save);

    current_ = std::make_unique<MenuScreen>();
    return true;
}

void App::Run() {
    if (!tourDir_.empty()) { RunTour(tourDir_); return; }
    while (!WindowShouldClose() && !quit_) {
        float dt = GetFrameTime();
        time += dt;
        UI::BeginFrame();
        if (!content.Error().empty()) {
            BeginDrawing();
            ClearBackground(Theme::Bg);
            UI::Text("Could not load learning content", 60, 60, 32, Theme::Red, FontKind::Display);
            UI::RichText(content.Error() + "\n\nMake sure the `assets` folder sits next to the executable.", {60, 120, 1100, 400}, 20, Theme::Text);
            EndDrawing();
            continue;
        }
        current_->Update(dt);
        BeginDrawing();
        ClearBackground(Theme::Bg);
        current_->Draw();
        EndDrawing();
        UI::EndFrame();
        if (pending_) current_ = std::move(pending_);
    }
}

void App::Shutdown() {
    progress.Save();
    current_.reset();
    pending_.reset();
    UI::Shutdown();
    CloseWindow();
}

// Automated screenshot tour, used to produce the screencast stills and to smoke-test every screen:
//   ./SupplyChainPP --tour shots
void App::RunTour(const std::string& outDir) {
    progress.Reset();
    progress.unlockAll = false;
    progress.RecordQuiz(1, 90, 9);
    progress.RecordQuiz(2, 70, 7);
    progress.RecordQuiz(3, 80, 8);
    progress.MarkLesson(1, 0); progress.MarkLesson(1, 1); progress.MarkLesson(1, 2); progress.MarkLab(1);
    progress.MarkLesson(4, 0);

    struct Shot { std::string name; std::function<std::unique_ptr<Screen>()> make; int frames; };
    std::vector<Shot> shots = {
        {"01_menu", [] { return std::make_unique<MenuScreen>(); }, 600},
        {"02_map", [] { return std::make_unique<MapScreen>(); }, 60},
        {"03_lesson", [] { return std::make_unique<ModuleScreen>(5, 1); }, 10},
        {"04_lesson_m1", [] { return std::make_unique<ModuleScreen>(1, 0); }, 10},
        {"05_lab", [] { auto s = std::make_unique<LabScreen>(3); s->JumpTo(6); return s; }, 50},
        {"06_quiz", [] { return std::make_unique<QuizScreen>(QuizScreen::ForModule(2)); }, 10},
        {"07_quiz_feedback", [] { auto s = std::make_unique<QuizScreen>(QuizScreen::ForModule(2)); s->DebugAnswer(true); s->DebugAnswer(false); return s; }, 40},
        {"08_quiz_bug", [] { auto s = std::make_unique<QuizScreen>(QuizScreen::ForModule(5)); s->DebugSkipTo(QType::Bug); return s; }, 10},
        {"09_quiz_fill", [] { auto s = std::make_unique<QuizScreen>(QuizScreen::ForModule(4)); s->DebugSkipTo(QType::Fill); s->DebugType("arr[2]"); return s; }, 10},
        {"10_quiz_results", [] { auto s = std::make_unique<QuizScreen>(QuizScreen::ForModule(4)); s->DebugFinish(); return s; }, 60},
        {"11_exam", [] { return std::make_unique<QuizScreen>(QuizScreen::ForExam(1)); }, 30},
        {"14_progress", [] { return std::make_unique<ProgressScreen>(); }, 10},
    };
    for (int m = 1; m <= 7; ++m) {
        shots.push_back({"20_lesson_m" + std::to_string(m), [m] { return std::make_unique<ModuleScreen>(m, 0); }, 5});
        shots.push_back({"21_lab_m" + std::to_string(m), [m] { auto s = std::make_unique<LabScreen>(m); s->JumpTo(1000); return s; }, 70});
    }
    for (auto& shot : shots) {
        current_ = shot.make();
        for (int f = 0; f < shot.frames; ++f) {
            time += 1.0f / 60.0f;
            UI::BeginFrame();
            current_->Update(1.0f / 60.0f);
            BeginDrawing();
            ClearBackground(Theme::Bg);
            current_->Draw();
            EndDrawing();
            UI::EndFrame();
        }
        Image img = LoadImageFromScreen();   // TakeScreenshot() always writes to the working dir
        ExportImage(img, (outDir + "/" + shot.name + ".png").c_str());
        UnloadImage(img);
    }
}

bool TopBar(const std::string& title, const std::string& subtitle, Color accent, const std::string& backLabel) {
    DrawRectangle(0, 0, kScreenW, 64, Theme::Bg2);
    DrawRectangle(0, 63, kScreenW, 1, Theme::Line);
    DrawRectangle(0, 0, 6, 64, accent);
    bool back = UI::Button({20, 14, 92, 36}, "< " + backLabel, BtnStyle::Ghost, true, 16);
    if (IsKeyPressed(KEY_ESCAPE)) back = true;
    UI::Text(title, 132, 10, 24, Theme::Text, FontKind::Display);
    UI::Text(subtitle, 132, 38, 15, Theme::Muted);

    // XP counter
    std::string xp = std::to_string(TheApp().progress.xp);
    Vector2 m = UI::Measure(xp, 22, FontKind::MonoBold);
    float x = kScreenW - 24 - m.x;
    UI::Text(xp, x, 12, 22, Theme::Yellow, FontKind::MonoBold);
    UI::Text("XP earned", kScreenW - 24 - UI::Measure("XP earned", 13).x, 38, 13, Theme::Muted);
    UI::Star({x - 20, 26}, 12, Theme::Yellow);
    return back;
}
