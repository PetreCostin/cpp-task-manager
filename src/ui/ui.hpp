#pragma once

#include <string>
#include "../task/task_manager.hpp"

// ─── Formatting Helpers ───────────────────────────────────────────────────────
int         nameWidth(int cols);
std::string currentTime();
const char* priStr(Priority p);
int         priColor(Priority p);
const char* statusStr(TaskStatus s);
int         statusColor(TaskStatus s);

// ─── Initialization ───────────────────────────────────────────────────────────
void initColors();

// ─── Dialogs ──────────────────────────────────────────────────────────────────
bool dialogAdd(std::string& outName, Priority& outPrio);
bool dialogConfirmDelete(const std::string& name);

// ─── Main Renderer ────────────────────────────────────────────────────────────
void drawUI(const TaskManager& tm, int sel, int scroll,
            const std::string& statusMsg);
