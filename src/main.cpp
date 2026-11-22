// CodePath - an interactive C / C++ learning app built with raylib (SETU Software Development).
// Learners follow a path of chapters: each chapter has lessons, a step-through Code Lab
// that visualises memory, and a quiz. Passing a quiz (60%+) unlocks the next chapter.
// Timed Exam 1 / Exam 2 practice draws mixed questions from the relevant chapters.
//
// Team: Ruochen Shi (C00304527) & Oliwier Nowak (C00307825)
#include "App.h"

int main(int argc, char** argv) {
    App& app = TheApp();
    if (!app.Init(argc, argv)) return 1;
    app.Run();
    app.Shutdown();
    return 0;
}
