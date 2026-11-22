// Screens.h - every screen of the app. Each is a Screen subclass (see App.h).
#pragma once
#include "App.h"
#include <vector>
#include <string>

// ---------------------------------------------------------------- Menu
class MenuScreen : public Screen {
public:
    void Update(float dt) override;
    void Draw() override;
    const char* Name() const override { return "menu"; }
private:
    float t_ = 0;
};

// ---------------------------------------------------------------- Learning path
class MapScreen : public Screen {
public:
    MapScreen();
    void Update(float dt) override;
    void Draw() override;
    const char* Name() const override { return "map"; }
private:
    std::vector<Vector2> pos_;   // chapter node positions
    int hover_ = -1;
    float t_ = 0;
};

// ---------------------------------------------------------------- Chapter (lessons)
class ModuleScreen : public Screen {
public:
    explicit ModuleScreen(int moduleId, int page = 0);
    void Update(float dt) override;
    void Draw() override;
    const char* Name() const override { return "module"; }
private:
    int id_;
    int page_;
    float scroll_ = 0, contentH_ = 0;
};

// ---------------------------------------------------------------- Trace lab
class LabScreen : public Screen {
public:
    explicit LabScreen(int moduleId);
    void Update(float dt) override;
    void Draw() override;
    const char* Name() const override { return "lab"; }
    void JumpTo(int step) { step_ = step; }
private:
    std::string ConsoleUpTo(int step) const;
    int id_;
    int step_ = -1;          // -1 = intro, before first line
    bool playing_ = false;
    float timer_ = 0;
    std::vector<float> shown_;   // animated crate counts
};

// ---------------------------------------------------------------- Quiz / exam practice
struct QuizConfig {
    std::string title;
    std::string subtitle;
    int moduleId = 0;        // >0 module quiz
    int examId = 0;          // >0 exam drill
    float timeLimit = 0;     // seconds, 0 = untimed
    Color color = {255, 138, 31, 255};
    std::vector<Question> questions;
};

class QuizScreen : public Screen {
public:
    explicit QuizScreen(QuizConfig cfg);
    void Update(float dt) override;
    void Draw() override;
    const char* Name() const override { return "quiz"; }

    // used by the screenshot tour
    void DebugAnswer(bool correct);
    void DebugFinish();
    void DebugType(const std::string& s) { input_ = s; }
    void DebugSkipTo(QType t);

    static QuizConfig ForModule(int moduleId);
    static QuizConfig ForExam(int examId);

private:
    void Submit(bool correct, int chosen);
    void Next();
    void Finish();
    bool CheckFill(const std::string& in) const;
    void DrawQuestion(Rectangle area);
    void DrawFeedback(Rectangle area);
    void DrawResults();
    void DrawTrack(Rectangle r);

    QuizConfig cfg_;
    int index_ = 0;
    bool answered_ = false;
    bool lastCorrect_ = false;
    int chosen_ = -1;
    int correct_ = 0;
    std::vector<int> results_;    // 1 correct, 0 wrong, per question
    std::vector<int> order_;      // shuffled option order for current question
    std::string input_;
    float elapsed_ = 0;
    bool finished_ = false;
    bool unlockedNext_ = false;
    float anim_ = 0;              // feedback animation
    float scrollReview_ = 0;
    float feedbackScroll_ = 0;
};

// ---------------------------------------------------------------- Progress
class ProgressScreen : public Screen {
public:
    void Update(float dt) override;
    void Draw() override;
    const char* Name() const override { return "progress"; }
private:
    bool confirmReset_ = false;
};
