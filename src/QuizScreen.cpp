// QuizScreen.cpp - chapter quizzes and timed exam practice. Each correct answer earns XP.
// Four question types: multiple choice, predict-the-output, fill-in-the-code, find-the-bug.
#include "Screens.h"
#include "UI.h"
#include <algorithm>
#include <random>
#include <numeric>
#include <cmath>
#include <cstdio>

namespace {
    std::mt19937& Rng() { static std::mt19937 r{std::random_device{}()}; return r; }

    const char* TypeName(QType t) {
        switch (t) {
            case QType::MCQ: return "MULTIPLE CHOICE";
            case QType::Output: return "PREDICT THE OUTPUT";
            case QType::Fill: return "FILL IN THE CODE";
            case QType::Bug: return "FIND THE BUG";
        }
        return "";
    }
    Color TypeColor(QType t) {
        switch (t) {
            case QType::MCQ: return Theme::Blue;
            case QType::Output: return Theme::Teal;
            case QType::Fill: return Theme::Yellow;
            case QType::Bug: return Theme::Red;
        }
        return Theme::Text;
    }
    std::string Normalize(const std::string& s) {
        std::string out;
        for (char c : s) if (c != ' ' && c != '\t') out += c;
        while (!out.empty() && out.back() == ';') out.pop_back();
        return out;
    }
    std::string CodeLine(const std::string& code, int line) {
        int cur = 1; std::string out;
        for (char c : code) { if (c == '\n') { if (cur == line) return out; cur++; out.clear(); } else out += c; }
        return cur == line ? out : "";
    }
}

QuizConfig QuizScreen::ForModule(int moduleId) {
    QuizConfig cfg;
    const Module* m = TheApp().content.Get(moduleId);
    if (!m) return cfg;
    cfg.moduleId = moduleId;
    cfg.title = "Chapter Quiz  ·  " + m->title;
    cfg.subtitle = m->topic + "  ·  score " + std::to_string(Progress::kPassMark) + "% to unlock the next chapter";
    cfg.color = m->color;
    cfg.questions = m->questions;
    std::shuffle(cfg.questions.begin(), cfg.questions.end(), Rng());
    return cfg;
}

QuizConfig QuizScreen::ForExam(int examId) {
    QuizConfig cfg;
    cfg.examId = examId;
    cfg.timeLimit = 10 * 60;
    cfg.color = examId == 1 ? Theme::Orange : Theme::Red;
    cfg.title = examId == 1 ? "Exam 1 Practice  ·  C fundamentals" : "Exam 2 Practice  ·  Functions, pointers, structs, files & C++";
    cfg.subtitle = "12 mixed questions, 10 minutes on the clock - just like the real thing";
    std::vector<Question> pool;
    for (const Module& m : TheApp().content.Modules()) {
        bool inExam = examId == 1 ? m.id <= 4 : m.id >= 5;
        if (inExam) pool.insert(pool.end(), m.questions.begin(), m.questions.end());
    }
    std::shuffle(pool.begin(), pool.end(), Rng());
    if (pool.size() > 12) pool.resize(12);
    cfg.questions = pool;
    return cfg;
}

QuizScreen::QuizScreen(QuizConfig cfg) : cfg_(std::move(cfg)) {
    results_.assign(cfg_.questions.size(), -1);
    Next();
    index_ = 0;
}

// prepare the question at index_ (called after index_ changes)
void QuizScreen::Next() {
    answered_ = false; chosen_ = -1; input_.clear(); feedbackScroll_ = 0;
    order_.clear();
    if (index_ < (int)cfg_.questions.size()) {
        const Question& q = cfg_.questions[index_];
        order_.resize(q.options.size());
        std::iota(order_.begin(), order_.end(), 0);
        std::shuffle(order_.begin(), order_.end(), Rng());
    }
}

bool QuizScreen::CheckFill(const std::string& in) const {
    const Question& q = cfg_.questions[index_];
    std::string n = Normalize(in);
    for (const auto& a : q.accepted) if (Normalize(a) == n) return true;
    return false;
}

void QuizScreen::Submit(bool correct, int chosen) {
    if (answered_ || finished_) return;
    answered_ = true;
    lastCorrect_ = correct;
    chosen_ = chosen;
    results_[index_] = correct ? 1 : 0;
    if (correct) correct_++;
    anim_ = 0;
}

void QuizScreen::Finish() {
    if (finished_) return;
    finished_ = true;
    Progress& p = TheApp().progress;
    int total = std::max(1, (int)cfg_.questions.size());
    int pct = (int)std::round(100.0 * correct_ / total);
    if (cfg_.moduleId > 0) {
        bool wasOpen = p.IsUnlocked(cfg_.moduleId + 1);
        p.RecordQuiz(cfg_.moduleId, pct, correct_);
        unlockedNext_ = !wasOpen && p.IsUnlocked(cfg_.moduleId + 1) && TheApp().content.Get(cfg_.moduleId + 1);
    } else if (cfg_.examId > 0) {
        p.RecordExam(cfg_.examId, pct, correct_);
    }
}

void QuizScreen::DebugAnswer(bool correct) {
    if (index_ >= (int)cfg_.questions.size()) return;
    if (answered_) { index_++; Next(); }
    const Question& q = cfg_.questions[index_];
    int pick = q.answer;
    if (!correct) pick = (q.type == QType::Bug) ? q.answer + 1 : (q.answer + 1) % std::max<size_t>(1, q.options.size());
    if (q.type == QType::Fill && !correct) input_ = "???";
    Submit(correct, pick);
    anim_ = 1;
}

void QuizScreen::DebugFinish() {
    for (int i = 0; i < (int)cfg_.questions.size(); ++i) { index_ = i; Next(); Submit(i % 4 != 2, -1); }
    index_ = (int)cfg_.questions.size() - 1;
    Finish();
}

void QuizScreen::DebugSkipTo(QType t) {
    for (int i = 0; i < (int)cfg_.questions.size(); ++i)
        if (cfg_.questions[i].type == t) { std::swap(cfg_.questions[0], cfg_.questions[i]); break; }
    index_ = 0; Next();
}

void QuizScreen::Update(float dt) {
    anim_ = std::min(1.0f, anim_ + dt * 2.5f);
    if (finished_) {
        float w = GetMouseWheelMove();
        if (w != 0) scrollReview_ = std::max(0.0f, scrollReview_ - w * 40);
        return;
    }
    if (cfg_.questions.empty()) return;
    elapsed_ += dt;
    if (cfg_.timeLimit > 0 && elapsed_ >= cfg_.timeLimit) { Finish(); return; }

    const Question& q = cfg_.questions[index_];
    if (!answered_) {
        if (q.type == QType::Fill) {
            int c;
            while ((c = GetCharPressed()) > 0) if (c >= 32 && c < 127 && input_.size() < 60) input_ += (char)c;
            if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !input_.empty()) input_.pop_back();
            if (IsKeyPressed(KEY_ENTER) && !input_.empty()) Submit(CheckFill(input_), -1);
        } else if (q.type == QType::MCQ || q.type == QType::Output) {
            for (int k = 0; k < (int)order_.size() && k < 4; ++k)
                if (IsKeyPressed(KEY_ONE + k) || IsKeyPressed(KEY_A + k)) Submit(order_[k] == q.answer, order_[k]);
        }
    } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (index_ + 1 < (int)cfg_.questions.size()) { index_++; Next(); }
        else Finish();
    }
}

void QuizScreen::DrawTrack(Rectangle r) {
    // progress track: one dot per question, green/red once answered, current one pulses
    int n = (int)cfg_.questions.size();
    UI::Panel(r, Theme::Bg2, Theme::Line, 0.1f);
    UI::Text("PROGRESS", r.x + 16, r.y + 12, 11, Theme::Muted, FontKind::MonoBold);
    float x0 = r.x + 30, x1 = r.x + r.width - 300;
    float cy = r.y + r.height / 2 + 8;
    float step = n > 1 ? (x1 - x0) / (n - 1) : 0;
    DrawLineEx({x0, cy}, {x1, cy}, 4, Theme::Panel2);
    int answered = 0;
    for (int i = 0; i < n; ++i) if (results_[i] != -1) answered = i + 1;
    if (answered > 0) DrawLineEx({x0, cy}, {x0 + step * (answered - 1), cy}, 4, cfg_.color);
    for (int i = 0; i < n; ++i) {
        Vector2 c = {x0 + step * i, cy};
        if (results_[i] == 1) { DrawCircleV(c, 12, Theme::Green); UI::CheckMark(c.x - 6, c.y - 6, 12, Theme::Bg); }
        else if (results_[i] == 0) { DrawCircleV(c, 12, Theme::Red); UI::CrossMark(c.x - 5, c.y - 5, 10, Theme::Text); }
        else if (i == index_ && !finished_) {
            float p = std::fmod(TheApp().time, 1.0f);
            DrawCircleV(c, 12 + p * 8, Fade(cfg_.color, 0.3f * (1 - p)));
            DrawCircleV(c, 12, cfg_.color);
            UI::TextCentered(std::to_string(i + 1), {c.x - 12, c.y - 12, 24, 24}, 13, Theme::Bg, FontKind::MonoBold);
        } else {
            DrawCircleV(c, 11, Theme::Panel);
            DrawCircleLinesV(c, 11, Theme::Line);
            UI::TextCentered(std::to_string(i + 1), {c.x - 12, c.y - 12, 24, 24}, 12, Theme::Dim, FontKind::Mono);
        }
    }
    // score box
    std::string sc = std::to_string(correct_) + " / " + std::to_string(n);
    UI::Text("SCORE", r.x + r.width - 120, r.y + 14, 11, Theme::Muted, FontKind::MonoBold);
    UI::Text(sc, r.x + r.width - 120, r.y + 32, 28, Theme::Text, FontKind::MonoBold);
}

void QuizScreen::DrawQuestion(Rectangle area) {
    const Question& q = cfg_.questions[index_];
    Color tc = TypeColor(q.type);
    UI::Text("QUESTION " + std::to_string(index_ + 1) + " / " + std::to_string(cfg_.questions.size()), area.x, area.y, 13, Theme::Muted, FontKind::MonoBold);
    UI::Tag(area.x + 132, area.y - 4, TypeName(q.type), Fade(tc, 0.18f), tc, 12);
    if (cfg_.examId > 0) {
        const Module* m = TheApp().content.Get(q.module);
        if (m) UI::Tag(area.x + 152 + UI::Measure(TypeName(q.type), 12, FontKind::SansBold).x + 14, area.y - 4, "Ch " + std::to_string(m->id) + ": " + m->title, Theme::Panel2, Theme::Muted, 12);
    }
    float y = area.y + 30;
    y += UI::RichText(q.prompt, {area.x, y, area.width, 200}, 20, Theme::Text) + 12;

    bool showFeedback = answered_;
    float bottom = showFeedback ? kScreenH - 170 : kScreenH - 24;
    Vector2 mouse = GetMousePosition(); (void)mouse;

    if (q.type == QType::Bug) {
        UI::Text("Click the line that contains the bug", area.x, y, 14, Theme::Red, FontKind::SansBold);
        y += 24;
        float h = std::min(bottom - y, 24.0f * (1 + std::count(q.code.begin(), q.code.end(), '\n')) + 24);
        int fs = h < 24.0f * (1 + std::count(q.code.begin(), q.code.end(), '\n')) + 24 ? 0 : 16;
        int clicked = UI::CodeBlock(q.code, {area.x, y, 760, h}, fs, answered_ ? q.answer : -1, !answered_, Theme::Green,
                                    answered_ && chosen_ != q.answer ? chosen_ : -1, Theme::Red);
        if (clicked > 0) Submit(clicked == q.answer, clicked);
        // side hint
        Rectangle side = {area.x + 784, y, area.width - 784, 150};
        UI::Panel(side, Theme::Panel, Theme::Line, 0.08f);
        UI::Text("CODE REVIEW", side.x + 16, side.y + 14, 12, Theme::Red, FontKind::MonoBold);
        UI::RichText("You are the reviewer. Read the program carefully and click the line that would cause wrong output, a crash or a compile error.",
                     {side.x + 16, side.y + 36, side.width - 32, 120}, 15, Theme::Muted);
        return;
    }

    bool hasCode = !q.code.empty();
    float optX = area.x, optW = area.width;
    if (hasCode) {
        int lines = 1 + (int)std::count(q.code.begin(), q.code.end(), '\n');
        float h = std::min(bottom - y, lines * 24.0f + 24);
        int fs = (lines * 24.0f + 24 > bottom - y) ? 0 : 16;
        UI::CodeBlock(q.code, {area.x, y, 600, h}, fs);
        optX = area.x + 624; optW = area.width - 624;
    }

    if (q.type == QType::Fill) {
        float fy = y;
        UI::Text("Type the missing code, then press Enter", optX, fy, 14, Theme::Yellow, FontKind::SansBold);
        Rectangle box = {optX, fy + 26, optW, 56};
        Color border = answered_ ? (lastCorrect_ ? Theme::Green : Theme::Red) : Theme::Yellow;
        DrawRectangleRounded(box, 0.15f, 6, Theme::CodeBg);
        DrawRectangleRoundedLinesEx(box, 0.15f, 6, 2, border);
        UI::Text(input_, box.x + 16, box.y + 16, 22, Theme::Text, FontKind::Mono);
        if (!answered_ && std::fmod(TheApp().time, 1.0f) < 0.55f) {
            float cx = box.x + 18 + UI::Measure(input_, 22, FontKind::Mono).x;
            DrawRectangle((int)cx, (int)box.y + 14, 2, 28, Theme::Yellow);
        }
        if (input_.empty() && !answered_) UI::Text("e.g.  " + std::string("i++"), box.x + 16, box.y + 16, 22, Theme::Dim, FontKind::Mono);
        if (!answered_ && UI::Button({optX, box.y + 72, 180, 44}, "Check  >", BtnStyle::Primary, !input_.empty(), 17))
            Submit(CheckFill(input_), -1);
        if (answered_ && !lastCorrect_ && !q.accepted.empty()) {
            UI::Text("Accepted answer:", optX, box.y + 74, 14, Theme::Muted);
            UI::Text(q.accepted[0], optX + 130, box.y + 72, 18, Theme::Green, FontKind::MonoBold);
        }
        return;
    }

    // MCQ / Output options
    int nOpt = (int)order_.size();
    bool grid = !hasCode && nOpt == 4;
    float gap = 12;
    float colW = grid ? (optW - gap) / 2 : optW;
    float oy = y;
    std::vector<float> heights(nOpt);
    float rowH[2] = {0, 0};
    for (int k = 0; k < nOpt; ++k) {
        const std::string& text = q.options[order_[k]];
        heights[k] = std::max(52.0f, UI::RichText(text, {0, 0, colW - 70, 400}, 17, Theme::Text, false) + 26);
        if (grid) rowH[k / 2] = std::max(rowH[k / 2], heights[k]);
    }
    for (int k = 0; k < nOpt; ++k) {
        int optIdx = order_[k];
        float h = grid ? rowH[k / 2] : heights[k];
        float x = grid ? optX + (k % 2) * (colW + gap) : optX;
        float yy = grid ? oy + (k / 2) * (rowH[0] + gap) : oy;
        if (!grid) oy += h + gap;
        Rectangle r = {x, yy, colW, h};
        bool hov = !answered_ && UI::Hover(r);
        Color fill = Theme::Panel, border = Theme::Line, badge = Theme::Panel2, badgeText = Theme::Muted;
        if (hov) { fill = Theme::Panel2; border = Theme::Orange; badge = Theme::Orange; badgeText = Theme::Bg; }
        if (answered_) {
            if (optIdx == q.answer) { fill = Fade(Theme::Green, 0.16f); border = Theme::Green; badge = Theme::Green; badgeText = Theme::Bg; }
            else if (optIdx == chosen_) { fill = Fade(Theme::Red, 0.16f); border = Theme::Red; badge = Theme::Red; badgeText = Theme::Text; }
            else { border = Fade(Theme::Line, 0.5f); }
        }
        UI::Panel(r, fill, border, 0.12f);
        Rectangle b = {r.x + 12, r.y + (h - 30) / 2, 30, 30};
        DrawRectangleRounded(b, 0.3f, 4, badge);
        UI::TextCentered(std::string(1, (char)('A' + k)), b, 15, badgeText, FontKind::MonoBold);
        bool mono = q.type == QType::Output;
        if (mono) {
            // outputs are shown verbatim in monospace (may contain \n)
            std::string t = q.options[optIdx];
            float ty = r.y + (h - 20 * (1 + std::count(t.begin(), t.end(), '\n'))) / 2;
            std::string line;
            for (size_t i = 0; i <= t.size(); ++i) {
                if (i == t.size() || t[i] == '\n') { UI::Text(line, r.x + 56, ty, 17, Theme::Text, FontKind::Mono); ty += 20; line.clear(); }
                else line += t[i];
            }
        } else {
            float th = UI::RichText(q.options[optIdx], {r.x + 56, 0, colW - 70, 400}, 17, Theme::Text, false);
            UI::RichText(q.options[optIdx], {r.x + 56, r.y + (h - th) / 2, colW - 70, 400}, 17, Theme::Text);
        }
        if (hov && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) Submit(optIdx == q.answer, optIdx);
    }
    if (!answered_) UI::Text("Tip: press 1-4 or A-D", optX, std::max(oy, y + rowH[0] * 2 + 24) + 4, 13, Theme::Dim);
}

void QuizScreen::DrawFeedback(Rectangle r) {
    const Question& q = cfg_.questions[index_];
    Color c = lastCorrect_ ? Theme::Green : Theme::Red;
    float slide = (1 - anim_) * 40;
    r.y += slide;
    UI::Panel(r, Theme::Panel, c, 0.08f);
    DrawRectangle((int)r.x, (int)r.y + 8, 4, (int)r.height - 16, c);
    if (lastCorrect_) UI::CheckMark(r.x + 22, r.y + 18, 22, c); else UI::CrossMark(r.x + 24, r.y + 20, 18, c);
    UI::Text(lastCorrect_ ? "Correct!  +10 XP" : "Not quite", r.x + 58, r.y + 16, 22, c, FontKind::Display);
    std::string sub;
    if (!lastCorrect_ && q.type == QType::Bug) sub = "Faulty line " + std::to_string(q.answer) + ":  " + CodeLine(q.code, q.answer);
    if (!sub.empty()) UI::Text(sub, r.x + 280, r.y + 22, 15, Theme::Muted, FontKind::Mono);

    Rectangle body = {r.x + 58, r.y + 52, r.width - 300, r.height - 60};
    BeginScissorMode((int)body.x, (int)body.y, (int)body.width, (int)body.height);
    UI::RichText(q.explain, body, 16, Theme::Text);
    EndScissorMode();

    bool last = index_ + 1 >= (int)cfg_.questions.size();
    if (UI::Button({r.x + r.width - 220, r.y + r.height - 62, 200, 46}, last ? "See results" : "Next question  >", BtnStyle::Primary, true, 17)) {
        if (!last) { index_++; Next(); } else Finish();
    }
    UI::Text("Enter", r.x + r.width - 220, r.y + r.height - 82, 12, Theme::Dim, FontKind::Mono);
}

void QuizScreen::DrawResults() {
    App& app = TheApp();
    int total = std::max(1, (int)cfg_.questions.size());
    int pct = (int)std::round(100.0 * correct_ / total);
    bool pass = pct >= Progress::kPassMark;
    Color c = pass ? Theme::Green : Theme::Red;

    // left: report card
    Rectangle card = {24, 168, 470, 530};
    UI::Panel(card, Theme::Panel, Theme::Line, 0.05f);
    UI::Text("RESULTS", card.x + 24, card.y + 22, 13, Theme::Muted, FontKind::MonoBold);
    std::string big = std::to_string(pct) + "%";
    UI::Text(big, card.x + 24, card.y + 44, 84, c, FontKind::Display);
    UI::Text(std::to_string(correct_) + " of " + std::to_string(total) + " correct  ·  +" + std::to_string(correct_ * Progress::kXpPerCorrect) + " XP", card.x + 28, card.y + 140, 17, Theme::Text);
    if (cfg_.timeLimit > 0) {
        int secs = (int)std::min(elapsed_, cfg_.timeLimit);
        UI::Text("Time: " + std::to_string(secs / 60) + "m " + std::to_string(secs % 60) + "s", card.x + 28, card.y + 166, 15, Theme::Muted, FontKind::Mono);
    }
    // stamp
    float sa = std::min(1.0f, anim_ * 1.5f);
    Rectangle stamp = {card.x + 280, card.y + 60, 168, 70};
    DrawRectangleRoundedLinesEx(stamp, 0.15f, 6, 4, Fade(c, sa));
    UI::TextCentered(pass ? "PASSED" : "RETRY", stamp, 28, Fade(c, sa), FontKind::Display);

    float y = card.y + 200;
    if (cfg_.moduleId > 0) {
        std::string msg;
        if (unlockedNext_) msg = "Chapter " + std::to_string(cfg_.moduleId + 1) + " unlocked! It is now open on your learning path.";
        else if (pass) msg = "Chapter passed. Best score: " + std::to_string(app.progress.Module(cfg_.moduleId).bestScore) + "%.";
        else msg = "You need " + std::to_string(Progress::kPassMark) + "% to unlock the next chapter. Review the lessons and the Code Lab, then try again - the questions come in a new order.";
        y += UI::RichText(msg, {card.x + 24, y, card.width - 48, 100}, 16, pass ? Theme::Text : Theme::Muted) + 16;
    } else {
        y += UI::RichText("Best Exam " + std::to_string(cfg_.examId) + " practice score: **" + std::to_string(app.progress.examBest[cfg_.examId]) + "%**. Every attempt draws a new random set of questions.",
                          {card.x + 24, y, card.width - 48, 100}, 16, Theme::Text) + 16;
    }
    // breakdown per type
    int tot[4] = {0}, ok[4] = {0};
    for (int i = 0; i < (int)cfg_.questions.size(); ++i) { int t = (int)cfg_.questions[i].type; tot[t]++; if (results_[i] == 1) ok[t]++; }
    for (int t = 0; t < 4; ++t) {
        if (!tot[t]) continue;
        UI::Text(TypeName((QType)t), card.x + 24, y, 12, TypeColor((QType)t), FontKind::MonoBold);
        UI::ProgressBar({card.x + 200, y + 4, 180, 8}, ok[t] / (float)tot[t], TypeColor((QType)t));
        UI::Text(std::to_string(ok[t]) + "/" + std::to_string(tot[t]), card.x + 396, y - 1, 14, Theme::Text, FontKind::Mono);
        y += 26;
    }

    float by = card.y + card.height - 66;
    if (UI::Button({card.x + 24, by, 130, 44}, "Retry", BtnStyle::Secondary, true, 16)) {
        app.Goto(std::make_unique<QuizScreen>(cfg_.moduleId > 0 ? ForModule(cfg_.moduleId) : ForExam(cfg_.examId)));
    }
    if (cfg_.moduleId > 0 && unlockedNext_) {
        if (UI::Button({card.x + 166, by, 280, 44}, "Go to chapter " + std::to_string(cfg_.moduleId + 1) + "  >", BtnStyle::Success, true, 16))
            app.Goto(std::make_unique<ModuleScreen>(cfg_.moduleId + 1));
    } else if (UI::Button({card.x + 166, by, 280, 44}, "Back to path", BtnStyle::Primary, true, 16)) {
        app.Goto(std::make_unique<MapScreen>());
    }

    // right: review of mistakes
    Rectangle rev = {514, 168, 742, 530};
    UI::Panel(rev, Theme::Bg2, Theme::Line, 0.05f);
    int wrong = 0;
    for (int r : results_) if (r != 1) wrong++;
    UI::Text("REVIEW YOUR MISTAKES  (" + std::to_string(wrong) + ")", rev.x + 20, rev.y + 18, 13, Theme::Muted, FontKind::MonoBold);
    BeginScissorMode((int)rev.x, (int)rev.y + 44, (int)rev.width, (int)rev.height - 50);
    float ry = rev.y + 48 - scrollReview_;
    if (wrong == 0) UI::RichText("No mistakes - perfect score!", {rev.x + 20, ry, rev.width - 40, 60}, 17, Theme::Green);
    for (int i = 0; i < (int)cfg_.questions.size(); ++i) {
        if (results_[i] == 1) continue;
        const Question& q = cfg_.questions[i];
        std::string ans;
        if (q.type == QType::Fill) ans = q.accepted.empty() ? "" : "`" + q.accepted[0] + "`";
        else if (q.type == QType::Bug) ans = "line " + std::to_string(q.answer) + ": `" + CodeLine(q.code, q.answer) + "`";
        else if (q.answer < (int)q.options.size()) { std::string o = q.options[q.answer]; std::replace(o.begin(), o.end(), '\n', ' '); ans = q.type == QType::Output ? "`" + o + "`" : o; }
        if (results_[i] == -1) ans += "  (not answered)";
        float h1 = UI::RichText(q.prompt, {rev.x + 40, 0, rev.width - 60, 300}, 15, Theme::Text, false);
        float h2 = UI::RichText("Answer: " + ans, {rev.x + 40, 0, rev.width - 60, 300}, 15, Theme::Green, false);
        UI::CrossMark(rev.x + 20, ry + 4, 10, Theme::Red);
        UI::RichText(q.prompt, {rev.x + 40, ry, rev.width - 60, 300}, 15, Theme::Text);
        UI::RichText("Answer: " + ans, {rev.x + 40, ry + h1 + 2, rev.width - 60, 300}, 15, {120, 210, 140, 255});
        ry += h1 + h2 + 18;
        DrawRectangle((int)rev.x + 20, (int)ry - 9, (int)rev.width - 40, 1, Theme::Line);
    }
    float contentH = ry + scrollReview_ - (rev.y + 48);
    scrollReview_ = std::min(scrollReview_, std::max(0.0f, contentH - (rev.height - 60)));
    EndScissorMode();
}

void QuizScreen::Draw() {
    App& app = TheApp();
    DrawTrack({24, 76, 1232, 80});

    // score / timer on the belt panel
    if (cfg_.timeLimit > 0 && !finished_) {
        float left = std::max(0.0f, cfg_.timeLimit - elapsed_);
        int s = (int)std::ceil(left);
        char buf[16]; snprintf(buf, sizeof buf, "%02d:%02d", s / 60, s % 60);
        Color tc = left < 60 ? Theme::Red : Theme::Yellow;
        UI::Text("TIME LEFT", 990, 90, 11, Theme::Muted, FontKind::MonoBold);
        UI::Text(buf, 990, 108, 28, tc, FontKind::MonoBold);
    }

    if (cfg_.questions.empty()) {
        UI::Text("No questions in this set yet.", 40, 200, 22, Theme::Muted);
    } else if (finished_) {
        DrawResults();
    } else {
        DrawQuestion({24, 182, 1232, 0});
        if (answered_) DrawFeedback({24, (float)kScreenH - 158, 1232, 146});
    }

    if (TopBar(cfg_.title, cfg_.subtitle, cfg_.color, cfg_.moduleId ? "Chapter" : "Path")) {
        if (cfg_.moduleId > 0) app.Goto(std::make_unique<ModuleScreen>(cfg_.moduleId));
        else app.Goto(std::make_unique<MapScreen>());
    }
}
