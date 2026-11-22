# Content file format

Every `moduleN.txt` file in this folder is one chapter. The app loads them at start-up
(`src/Content.cpp`), so you can add or fix lessons and questions **without recompiling**.

Lines starting with `@` are directives. Everything else is the text of the block above it.
Lines starting with `@@` are comments.

```
@MODULE 3                      chapter number (controls order and unlocking)
@CHAPTER Loops                 title shown on the path
@TOPIC for, while & do-while   subtitle
@BLURB One-line description shown in the tooltip
@COLOR 255,200,61              accent colour (r,g,b)

@LESSON Lesson title
@TEXT        (multi-line)  explanation. Supports `inline code`, **bold**, "- " bullets
@CODE        (multi-line)  example program
@ANALOGY     (multi-line)  real-world analogy card
@TIP         (multi-line)  exam tip card

@LAB Lab title
@INTRO       (multi-line)
@CODE        (multi-line)  program to step through
@STEP line | var=value; var2=value | Block=3; Block2=5 | console output\n | explanation

@Q MCQ | OUTPUT | FILL | BUG
@PROMPT      (multi-line)
@CODE        (multi-line, optional)
@OPT option text            (MCQ / OUTPUT, repeat; use \n for multi-line output)
@ANSWER 0                   option index (0-based) for MCQ/OUTPUT,
                            1-based line number for BUG,
                            accepted answers separated by | for FILL
@EXPLAIN     (multi-line)
```

Notes
- Options are shuffled every attempt, so never write "all of the above".
- FILL answers are compared ignoring spaces and a trailing `;`.
- `|` cannot appear inside a FILL answer or a `@STEP` field (it is the separator).
- Exam 1 practice draws from chapters 1-4, Exam 2 practice from chapters 5-7
  (see `QuizScreen::ForExam`).
