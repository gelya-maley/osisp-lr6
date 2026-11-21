#pragma once
#include <windows.h>
#include <winevt.h>
#include <string>
#include <vector>
#include <map>

#define STUDENT_ID 71

void log_init(const std::string& role);
void set_log_stdout(bool on);
void log_print(const std::string& event);
std::string win_error_text(DWORD code);
void log_win_error(const std::string& func, const std::string& op, DWORD code);
void qpc_init();
double qpc_now_ms();

struct Options {
    std::string log_name = "Application";
    int limit = 20;
    std::string format = "text";
    std::string query;
    std::string log_file;
};

Options parse_args(int argc, char** argv);

struct EvtGuard {
    EVT_HANDLE h = nullptr;
    EvtGuard() = default;
    explicit EvtGuard(EVT_HANDLE k) : h(k) {}
    ~EvtGuard();
    void reset(EVT_HANDLE k);
    EVT_HANDLE get() const;
};

std::wstring to_wide(const std::string& s);
std::string to_narrow(const std::wstring& s);
std::string level_name(DWORD level);
