#pragma once

// ─── Color Pair IDs ───────────────────────────────────────────────────────────
#define CP_HEADER   1   // header / shortcut bars  (black on cyan)
#define CP_SELECTED 2   // selected row            (black on white)
#define CP_HIGH     3   // HIGH priority           (red   on default)
#define CP_MEDIUM   4   // MEDIUM priority         (yellow on default)
#define CP_LOW      5   // LOW priority            (green  on default)
#define CP_DONE     6   // DONE status             (green  on default)
#define CP_NORMAL   7   // ordinary text           (white  on default)
#define CP_INPROG   8   // IN PROGRESS status      (yellow on default)
#define CP_STATS    9   // stats-bar labels        (cyan   on default)
#define CP_KEY      10  // shortcut key glyph      (white on cyan)
#define CP_TITLE    11  // column header row       (cyan   on default)
#define CP_DIALOG   12  // dialog background       (white  on default)

// ─── Application Constants ────────────────────────────────────────────────────
constexpr int MIN_COLS         = 55;   // minimum terminal width
constexpr int MIN_ROWS         = 8;    // minimum terminal height
constexpr int STATUS_DURATION  = 20;   // status message display ticks
constexpr int HALFDELAY_TENTHS = 5;    // halfdelay units (0.5 s refresh)
