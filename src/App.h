// App.h - CodePath application shell: owns content, progress and the active screen.
// Team: Ruochen Shi (C00304527) & Oliwier Nowak (C00307825)
#pragma once
#include "Content.h"
#include "Progress.h"
#include <memory>
#include <string>

constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;
inline const char* kTeamNames = "Ruochen Shi (C00304527) & Oliwier Nowak (C00307825)";

class Screen {
public:
    virtual ~Screen() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;
    virtual const char* Name() const = 0;
};

class App {
public:
    bool Init(int argc, char** argv);
    void Run();
    void Shutdown();

    void Goto(std::unique_ptr<Screen> next) { pending_ = std::move(next); }
    void Quit() { quit_ = true; }

    ContentLibrary content;
    Progress progress;
    std::string assetDir;
    float time = 0;      // seconds since start, for animations

private:
    void RunTour(const std::string& outDir);
    std::unique_ptr<Screen> current_, pending_;
    bool quit_ = false;
    std::string tourDir_;
};

App& TheApp();

// Shared header bar for inner screens. Returns true when "back" was clicked.
bool TopBar(const std::string& title, const std::string& subtitle, Color accent, const std::string& backLabel = "Path");
