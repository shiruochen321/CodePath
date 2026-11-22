// Content.cpp - parser for the plain-text content format (see assets/content/FORMAT.md).
#include "Content.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace {
    std::string Trim(const std::string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    // Remove leading/trailing blank lines but keep indentation of code.
    std::string TrimBlock(const std::string& s) {
        size_t start = 0;
        while (start < s.size()) {
            size_t nl = s.find('\n', start);
            std::string line = s.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
            if (!Trim(line).empty() || nl == std::string::npos) break;
            start = nl + 1;
        }
        std::string out = s.substr(start);
        while (!out.empty() && (out.back() == '\n' || out.back() == ' ' || out.back() == '\r' || out.back() == '\t')) out.pop_back();
        return out;
    }

    std::vector<std::string> Split(const std::string& s, char delim) {
        std::vector<std::string> parts;
        std::string cur;
        for (char c : s) {
            if (c == delim) { parts.push_back(cur); cur.clear(); }
            else cur += c;
        }
        parts.push_back(cur);
        return parts;
    }

    std::string Unescape(const std::string& s) {
        std::string out;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') { out += '\n'; ++i; }
            else out += s[i];
        }
        return out;
    }

    Color ParseColor(const std::string& s) {
        auto p = Split(s, ',');
        if (p.size() < 3) return {255, 138, 31, 255};
        return {(unsigned char)std::stoi(p[0]), (unsigned char)std::stoi(p[1]), (unsigned char)std::stoi(p[2]), 255};
    }
}

bool ContentLibrary::LoadDirectory(const std::string& dir) {
    modules_.clear();
    FilePathList files = LoadDirectoryFiles(dir.c_str());
    std::vector<std::string> paths;
    for (unsigned i = 0; i < files.count; ++i) {
        std::string p = files.paths[i];
        std::string name = GetFileName(p.c_str());
        if (name.rfind("module", 0) == 0 && IsFileExtension(p.c_str(), ".txt")) paths.push_back(p);
    }
    UnloadDirectoryFiles(files);
    std::sort(paths.begin(), paths.end());
    for (auto& p : paths)
        if (!LoadFile(p)) return false;
    std::sort(modules_.begin(), modules_.end(), [](const Module& a, const Module& b) { return a.id < b.id; });
    if (modules_.empty()) { error_ = "No module*.txt files found in " + dir; return false; }
    return true;
}

const Module* ContentLibrary::Get(int id) const {
    for (auto& m : modules_) if (m.id == id) return &m;
    return nullptr;
}

int ContentLibrary::TotalQuestions() const {
    int n = 0;
    for (auto& m : modules_) n += (int)m.questions.size();
    return n;
}

bool ContentLibrary::LoadFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) { error_ = "Cannot open " + path; return false; }

    Module mod;
    enum class Ctx { None, Lesson, Lab, Question } ctx = Ctx::None;
    std::string* block = nullptr;     // multi-line field currently being filled
    std::string answerRaw;
    int lineNo = 0;

    auto finishQuestion = [&]() {
        if (ctx != Ctx::Question || mod.questions.empty()) return;
        Question& q = mod.questions.back();
        if (q.type == QType::Fill) {
            for (auto& a : Split(answerRaw, '|')) { std::string t = Trim(a); if (!t.empty()) q.accepted.push_back(t); }
        } else {
            q.answer = answerRaw.empty() ? 0 : std::stoi(Trim(answerRaw));
        }
        answerRaw.clear();
    };

    std::string line;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 2 && line[0] == '@' && line[1] == '@') continue;   // comment
        if (line.empty() || line[0] != '@') {
            if (block) { *block += line; *block += '\n'; }
            continue;
        }
        // directive
        size_t sp = line.find(' ');
        std::string key = line.substr(1, sp == std::string::npos ? std::string::npos : sp - 1);
        std::string arg = sp == std::string::npos ? "" : Trim(line.substr(sp + 1));
        block = nullptr;

        try {
            if (key == "MODULE") mod.id = std::stoi(arg);
            else if (key == "CHAPTER") mod.title = arg;
            else if (key == "TOPIC") mod.topic = arg;
            else if (key == "BLURB") mod.blurb = arg;
            else if (key == "COLOR") mod.color = ParseColor(arg);
            else if (key == "LESSON") { finishQuestion(); ctx = Ctx::Lesson; mod.lessons.push_back({}); mod.lessons.back().title = arg; }
            else if (key == "LAB") { finishQuestion(); ctx = Ctx::Lab; mod.lab.title = arg; }
            else if (key == "Q") {
                finishQuestion();
                ctx = Ctx::Question;
                Question q; q.module = mod.id;
                if (arg == "MCQ") q.type = QType::MCQ;
                else if (arg == "OUTPUT") q.type = QType::Output;
                else if (arg == "FILL") q.type = QType::Fill;
                else if (arg == "BUG") q.type = QType::Bug;
                else throw std::runtime_error("unknown question type " + arg);
                mod.questions.push_back(q);
            }
            else if (key == "TEXT" && ctx == Ctx::Lesson) block = &mod.lessons.back().text;
            else if (key == "ANALOGY" && ctx == Ctx::Lesson) block = &mod.lessons.back().analogy;
            else if (key == "TIP" && ctx == Ctx::Lesson) block = &mod.lessons.back().tip;
            else if (key == "INTRO" && ctx == Ctx::Lab) block = &mod.lab.intro;
            else if (key == "CODE") {
                if (ctx == Ctx::Lesson) block = &mod.lessons.back().code;
                else if (ctx == Ctx::Lab) block = &mod.lab.code;
                else if (ctx == Ctx::Question) block = &mod.questions.back().code;
            }
            else if (key == "PROMPT" && ctx == Ctx::Question) block = &mod.questions.back().prompt;
            else if (key == "EXPLAIN" && ctx == Ctx::Question) block = &mod.questions.back().explain;
            else if (key == "OPT" && ctx == Ctx::Question) mod.questions.back().options.push_back(Unescape(arg));
            else if (key == "ANSWER" && ctx == Ctx::Question) answerRaw = arg;
            else if (key == "STEP" && ctx == Ctx::Lab) {
                // @STEP line | a=1; b=2 | Bin=3; Bin2=4 | output\n | note
                auto parts = Split(arg, '|');
                while (parts.size() < 5) parts.push_back("");
                LabStep st;
                st.line = std::stoi(Trim(parts[0]));
                for (auto& kv : Split(parts[1], ';')) {
                    std::string t = Trim(kv); if (t.empty()) continue;
                    size_t eq = t.find('=');
                    st.vars.push_back({Trim(t.substr(0, eq)), eq == std::string::npos ? "" : Trim(t.substr(eq + 1))});
                }
                for (auto& kv : Split(parts[2], ';')) {
                    std::string t = Trim(kv); if (t.empty()) continue;
                    size_t eq = t.find('=');
                    st.bins.push_back({Trim(t.substr(0, eq)), eq == std::string::npos ? 0 : std::stoi(Trim(t.substr(eq + 1)))});
                }
                // output keeps leading spaces after the first one
                std::string o = parts[3];
                if (!o.empty() && o[0] == ' ') o.erase(0, 1);
                while (!o.empty() && o.back() == ' ') o.pop_back();
                st.output = Unescape(o);
                st.note = Trim(parts[4]);
                mod.lab.steps.push_back(st);
            }
            else throw std::runtime_error("unexpected directive @" + key);
        } catch (const std::exception& e) {
            error_ = path + ":" + std::to_string(lineNo) + ": " + e.what();
            return false;
        }
    }
    finishQuestion();

    // tidy blocks
    for (auto& l : mod.lessons) { l.text = TrimBlock(l.text); l.code = TrimBlock(l.code); l.analogy = TrimBlock(l.analogy); l.tip = TrimBlock(l.tip); }
    mod.lab.intro = TrimBlock(mod.lab.intro);
    mod.lab.code = TrimBlock(mod.lab.code);
    for (auto& q : mod.questions) { q.prompt = TrimBlock(q.prompt); q.code = TrimBlock(q.code); q.explain = TrimBlock(q.explain); }

    if (mod.id <= 0) { error_ = path + ": missing @MODULE"; return false; }
    modules_.push_back(std::move(mod));
    return true;
}
