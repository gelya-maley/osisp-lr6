#include "common.h"
#include <cstdio>
#include <cstdlib>
#include <mutex>

static std::string g_role = "EVTVIEW";
static std::mutex g_log_mutex;
static LARGE_INTEGER g_qpc_freq = {0};
static std::string g_log_file;
static bool g_log_stdout = true;

void log_init(const std::string& role) {
    g_role = role;
    SetConsoleOutputCP(CP_UTF8);
}

void set_log_stdout(bool on) {
    g_log_stdout = on;
}

void log_print(const std::string& event) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[512];
    snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] %s %s",
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
             g_role.c_str(), event.c_str());
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (g_log_stdout) printf("%s\n", buf);
    else fprintf(stderr, "%s\n", buf);
    fflush(stdout);
    if (!g_log_file.empty()) {
        FILE* f = fopen(g_log_file.c_str(), "a");
        if (f) { fprintf(f, "%s\n", buf); fclose(f); }
    }
}

std::string win_error_text(DWORD code) {
    wchar_t* msg = nullptr;
    DWORD len = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                               nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPWSTR)&msg, 0, nullptr);
    std::string out;
    if (len && msg) {
        std::wstring w(msg, len);
        while (!w.empty() && (w.back() == L'\r' || w.back() == L'\n' || w.back() == L' ')) w.pop_back();
        out = to_narrow(w);
    }
    if (msg) LocalFree(msg);
    if (out.empty()) out = "unknown error code";
    return out;
}

void log_win_error(const std::string& func, const std::string& op, DWORD code) {
    log_print("ERROR func=" + func + " op=" + op + " code=" + std::to_string(code) +
              " desc=\"" + win_error_text(code) + "\"");
}

void qpc_init() {
    QueryPerformanceFrequency(&g_qpc_freq);
}

double qpc_now_ms() {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)g_qpc_freq.QuadPart;
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](int& i) -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--log") o.log_name = next(i);
        else if (k == "--limit") o.limit = atoi(next(i).c_str());
        else if (k == "--format") o.format = next(i);
        else if (k == "--query") o.query = next(i);
        else if (k == "--log-file") g_log_file = next(i);
    }
    return o;
}

EvtGuard::~EvtGuard() {
    if (h) EvtClose(h);
}

void EvtGuard::reset(EVT_HANDLE k) {
    if (h && h != k) EvtClose(h);
    h = k;
}

EVT_HANDLE EvtGuard::get() const { return h; }

std::wstring to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w((size_t)n > 0 ? n - 1 : 0, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

std::string to_narrow(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s((size_t)n > 0 ? n - 1 : 0, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}

std::string level_name(DWORD level) {
    switch (level) {
    case 1: return "Critical";
    case 2: return "Error";
    case 3: return "Warning";
    case 4: return "Information";
    case 5: return "Verbose";
    default: return "Level" + std::to_string(level);
    }
}
