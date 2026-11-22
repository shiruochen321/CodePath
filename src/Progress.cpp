// Progress.cpp - save/load learner progress using std::ifstream / std::ofstream.
#include "Progress.h"
#include <fstream>
#include <sstream>
#include <algorithm>

void Progress::Load(const std::string& path) {
    path_ = path;
    std::ifstream in(path);
    if (!in) return;   // first run: nothing saved yet
    std::string key;
    while (in >> key) {
        if (key == "xp") in >> xp;
        else if (key == "unlockAll") { int v; in >> v; unlockAll = v != 0; }
        else if (key == "exam") { int id, v; in >> id >> v; if (id >= 1 && id <= 2) examBest[id] = v; }
        else if (key == "module") {
            int id, lab; ModuleProgress m;
            in >> id >> m.bestScore >> m.attempts >> m.lessonsSeen >> lab;
            m.labDone = lab != 0;
            modules_[id] = m;
        } else {
            std::string rest; std::getline(in, rest);   // skip unknown keys
        }
    }
}

void Progress::Save() const {
    if (path_.empty()) return;
    std::ofstream out(path_);
    if (!out) return;
    out << "xp " << xp << "\n";
    out << "unlockAll " << (unlockAll ? 1 : 0) << "\n";
    out << "exam 1 " << examBest[1] << "\n";
    out << "exam 2 " << examBest[2] << "\n";
    for (const auto& [id, m] : modules_)
        out << "module " << id << " " << m.bestScore << " " << m.attempts << " " << m.lessonsSeen << " " << (m.labDone ? 1 : 0) << "\n";
}

void Progress::Reset() {
    modules_.clear();
    examBest[1] = examBest[2] = 0;
    xp = 0;
    unlockAll = false;
    Save();
}

bool Progress::IsUnlocked(int id) const {
    if (unlockAll || id <= 1) return true;
    auto it = modules_.find(id - 1);
    return it != modules_.end() && it->second.bestScore >= kPassMark;
}

void Progress::RecordQuiz(int moduleId, int percent, int correct) {
    ModuleProgress& m = modules_[moduleId];
    m.attempts++;
    m.bestScore = std::max(m.bestScore, percent);
    xp += correct * kXpPerCorrect;
    Save();
}

void Progress::RecordExam(int examId, int percent, int correct) {
    if (examId >= 1 && examId <= 2) examBest[examId] = std::max(examBest[examId], percent);
    xp += correct * kXpPerCorrect;
    Save();
}

void Progress::MarkLesson(int moduleId, int page) {
    int bit = 1 << page;
    ModuleProgress& m = modules_[moduleId];
    if (!(m.lessonsSeen & bit)) { m.lessonsSeen |= bit; Save(); }
}

void Progress::MarkLab(int moduleId) {
    ModuleProgress& m = modules_[moduleId];
    if (!m.labDone) { m.labDone = true; Save(); }
}

int Progress::LessonsSeenCount(int moduleId) const {
    auto it = modules_.find(moduleId);
    if (it == modules_.end()) return 0;
    int n = 0, v = it->second.lessonsSeen;
    while (v) { n += v & 1; v >>= 1; }
    return n;
}
