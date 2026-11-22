// Content.h - learning content model (lessons, quizzes, trace labs) loaded from text files.
#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <utility>

struct LessonPage {
    std::string title;
    std::string text;      // explanation (rich text)
    std::string code;      // example program
    std::string analogy;   // real-world analogy
    std::string tip;       // exam tip
};

enum class QType { MCQ, Output, Fill, Bug };

struct Question {
    QType type = QType::MCQ;
    int module = 0;                       // 1-based module id
    std::string prompt;
    std::string code;
    std::vector<std::string> options;     // MCQ / Output
    int answer = 0;                       // option index (MCQ/Output) or 1-based line (Bug)
    std::vector<std::string> accepted;    // Fill answers
    std::string explain;
};

struct LabStep {
    int line = 1;                                             // 1-based line being executed
    std::vector<std::pair<std::string, std::string>> vars;    // variable watch
    std::vector<std::pair<std::string, int>> bins;            // drawn as crate stacks
    std::string output;                                       // console output produced by this step
    std::string note;                                         // explanation
};

struct TraceLab {
    std::string title;
    std::string intro;
    std::string code;
    std::vector<LabStep> steps;
};

struct Module {
    int id = 0;
    std::string title;     // e.g. "Getting Started"
    std::string topic;     // e.g. "Variables, Types & I/O"
    std::string blurb;
    Color color = {255, 138, 31, 255};
    std::vector<LessonPage> lessons;
    std::vector<Question> questions;
    TraceLab lab;
};

class ContentLibrary {
public:
    bool LoadDirectory(const std::string& dir);   // loads module*.txt sorted by name
    const std::vector<Module>& Modules() const { return modules_; }
    const Module* Get(int id) const;
    const std::string& Error() const { return error_; }
    int TotalQuestions() const;

private:
    bool LoadFile(const std::string& path);
    std::vector<Module> modules_;
    std::string error_;
};
