// Progress.h - learner progress, persisted to a small key/value text file.
#pragma once
#include <string>
#include <map>

struct ModuleProgress {
    int bestScore = 0;        // percent
    int attempts = 0;
    int lessonsSeen = 0;      // bit mask of viewed lesson pages
    bool labDone = false;
};

class Progress {
public:
    static constexpr int kPassMark = 60;

    void Load(const std::string& path);
    void Save() const;
    void Reset();

    ModuleProgress& Module(int id) { return modules_[id]; }
    bool IsUnlocked(int id) const;
    void RecordQuiz(int moduleId, int percent, int correct);
    void RecordExam(int examId, int percent, int correct);
    void MarkLesson(int moduleId, int page);
    void MarkLab(int moduleId);
    int LessonsSeenCount(int moduleId) const;

    int examBest[3] = {0, 0, 0};   // index 1 and 2 used
    int xp = 0;                    // experience points: 10 per correct answer
    static constexpr int kXpPerCorrect = 10;
    bool unlockAll = false;        // demo / lecturer mode

private:
    std::map<int, ModuleProgress> modules_;
    std::string path_;
};
