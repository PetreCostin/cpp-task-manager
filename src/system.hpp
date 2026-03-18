#pragma once
#include "task.hpp"
#include <string>

// ─── Formatting Helpers ───────────────────────────────────────────────────────
std::string   currentTime();
const char*   priStr(Priority p);
const char*   statusStr(TaskStatus s);
int           nameWidth(int cols);
