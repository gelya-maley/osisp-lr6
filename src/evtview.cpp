#include "common.h"
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <vector>

struct EventInfo {
    std::string time;
    std::string provider;
    std::string eventid;
    std::string level;
    DWORD levelNum;
    std::string message;
};

static std::wstring xml_attr(const std::wstring& xml, const wchar_t* tag, const wchar_t* attr) {
    size_t t = xml.find(tag);
    if (t == std::wstring::npos) return L"";
    size_t end = xml.find(L'>', t);
    if (end == std::wstring::npos) return L"";
    std::wstring a = attr;
    a += L"=\"";
    size_t p = xml.find(a, t);
    if (p == std::wstring::npos || p > end) return L"";
    p += a.size();
    size_t q = xml.find(L'"', p);
    if (q == std::wstring::npos || q > end) return L"";
    return xml.substr(p, q - p);
}

static std::wstring xml_elem(const std::wstring& xml, const wchar_t* name) {
    std::wstring open = L"<";
    open += name;
    open += L">";
    size_t p = xml.find(open);
    if (p == std::wstring::npos) return L"";
    p += open.size();
    std::wstring close = L"</";
    close += name;
    close += L">";
    size_t q = xml.find(close, p);
    if (q == std::wstring::npos) return L"";
    return xml.substr(p, q - p);
}

static bool render_xml(EVT_HANDLE ev, std::wstring& xml) {
    DWORD used = 0, props = 0;
    BOOL ok = EvtRender(nullptr, ev, EvtRenderEventXml, 0, nullptr, &used, &props);
    if (!ok && GetLastError() != ERROR_INSUFFICIENT_BUFFER) return false;
    if (used == 0) return false;
    std::vector<wchar_t> buf(used);
    ok = EvtRender(nullptr, ev, EvtRenderEventXml, used, buf.data(), &used, &props);
    if (!ok) return false;
    xml.assign(buf.data());
    return true;
}

static std::string format_message(EVT_HANDLE ev, const std::wstring& provider) {
    if (provider.empty()) return "";
    EVT_HANDLE pub = EvtOpenPublisherMetadata(nullptr, provider.c_str(), nullptr, 0, 0);
    if (!pub) return "";
    EvtGuard pg(pub);
    DWORD size = 0;
    BOOL ok = EvtFormatMessage(pub, ev, 0, 0, nullptr, EvtFormatMessageEvent, 0, nullptr, &size);
    if (!ok && GetLastError() != ERROR_INSUFFICIENT_BUFFER) return "";
    if (size == 0) return "";
    std::vector<wchar_t> buf(size);
    ok = EvtFormatMessage(pub, ev, 0, 0, nullptr, EvtFormatMessageEvent, size, buf.data(), &size);
    if (!ok) return "";
    std::wstring w(buf.data());
    for (auto& ch : w) if (ch == L'\r' || ch == L'\n' || ch == L'\t') ch = L' ';
    return to_narrow(w);
}

static std::string iso_to_local(const std::string& iso) {
    SYSTEMTIME utc{};
    unsigned y, mo, d, h, mi, s;
    if (sscanf_s(iso.c_str(), "%u-%u-%uT%u:%u:%u", &y, &mo, &d, &h, &mi, &s) != 6) return iso;
    utc.wYear = (WORD)y; utc.wMonth = (WORD)mo; utc.wDay = (WORD)d;
    utc.wHour = (WORD)h; utc.wMinute = (WORD)mi; utc.wSecond = (WORD)s;
    FILETIME ft{}, local{};
    if (!SystemTimeToFileTime(&utc, &ft)) return iso;
    if (!FileTimeToLocalFileTime(&ft, &local)) return iso;
    SYSTEMTIME loc{};
    if (!FileTimeToSystemTime(&local, &loc)) return iso;
    char buf[32];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
             loc.wYear, loc.wMonth, loc.wDay, loc.wHour, loc.wMinute, loc.wSecond);
    return buf;
}

static std::string csv_escape(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

static std::string json_escape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += (char)c; }
        else if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
        else out += (char)c;
    }
    return out;
}

int main(int argc, char** argv) {
    Options o = parse_args(argc, argv);
    log_init("EVTVIEW");
    set_log_stdout(o.format == "text");
    qpc_init();
    if (o.limit < 0) {
        log_print("ERROR op=parse code=BAD_LIMIT desc=limit must be non-negative");
        printf("invalid --limit value\n");
        return 1;
    }
    double t0 = qpc_now_ms();

    DWORD flags = EvtQueryChannelPath | EvtQueryReverseDirection;
    const wchar_t* query = nullptr;
    std::wstring wlog, wquery;
    wlog = to_wide(o.log_name);
    if (!o.query.empty()) {
        wquery = to_wide(o.query);
        query = wquery.c_str();
    }
    EVT_HANDLE rs = EvtQuery(nullptr, wlog.c_str(), query, flags);
    if (!rs) {
        DWORD e = GetLastError();
        log_win_error("EvtQuery", "open " + o.log_name, e);
        if (e == ERROR_ACCESS_DENIED) printf("journal unavailable: insufficient rights to read %s\n", o.log_name.c_str());
        else if (e == ERROR_EVT_CHANNEL_NOT_FOUND) printf("journal not found: %s\n", o.log_name.c_str());
        else if (e == ERROR_EVT_INVALID_QUERY) printf("invalid query: %s\n", o.query.c_str());
        else printf("failed to open journal %s: code %lu\n", o.log_name.c_str(), e);
        log_print("STOP result=1");
        return 1;
    }
    EvtGuard rg(rs);
    log_print("EVT_QUERY log=" + o.log_name + " query=" + (o.query.empty() ? std::string("*") : o.query) +
              " reverse=1 status=OK");

    if (o.format == "csv") printf("time,provider,eventid,level,message\n");
    else if (o.format == "json") printf("[\n");

    const DWORD BATCH = 64;
    std::vector<EVT_HANDLE> batch(BATCH);
    int shown = 0;
    DWORD errors = 0;
    std::map<DWORD, int> levels;
    bool first_json = true;

    while (shown < o.limit) {
        DWORD got = 0;
        BOOL ok = EvtNext(rs, BATCH, batch.data(), INFINITE, 0, &got);
        if (!ok) {
            DWORD e = GetLastError();
            if (e == ERROR_NO_MORE_ITEMS) break;
            log_win_error("EvtNext", "batch", e);
            ++errors;
            break;
        }
        log_print("EVT_BATCH count=" + std::to_string(got) + " status=OK");
        for (DWORD i = 0; i < got && shown < o.limit; ++i) {
            EvtGuard eg(batch[i]);
            std::wstring xml;
            if (!render_xml(batch[i], xml)) {
                log_win_error("EvtRender", "xml", GetLastError());
                ++errors;
                continue;
            }
            EventInfo ei{};
            ei.provider = to_narrow(xml_attr(xml, L"Provider", L"Name"));
            ei.time = iso_to_local(to_narrow(xml_attr(xml, L"TimeCreated", L"SystemTime")));
            ei.eventid = to_narrow(xml_elem(xml, L"EventID"));
            ei.levelNum = (DWORD)atoi(to_narrow(xml_elem(xml, L"Level")).c_str());
            ei.level = level_name(ei.levelNum);
            ei.message = format_message(batch[i], to_wide(ei.provider));
            if (ei.message.empty()) ei.message = "(no message: publisher metadata unavailable)";
            levels[ei.levelNum]++;
            if (o.format == "csv") {
                printf("%s,%s,%s,%s,%s\n",
                       csv_escape(ei.time).c_str(), csv_escape(ei.provider).c_str(),
                       csv_escape(ei.eventid).c_str(), csv_escape(ei.level).c_str(),
                       csv_escape(ei.message).c_str());
            } else if (o.format == "json") {
                printf("%s  {\"time\": \"%s\", \"provider\": \"%s\", \"eventid\": \"%s\", \"level\": \"%s\", \"message\": \"%s\"}",
                       first_json ? "" : ",\n", json_escape(ei.time).c_str(), json_escape(ei.provider).c_str(),
                       json_escape(ei.eventid).c_str(), json_escape(ei.level).c_str(), json_escape(ei.message).c_str());
                first_json = false;
            } else {
                printf("[%s] %-11s EventID=%-6s Provider=%s %s\n",
                       ei.time.c_str(), ei.level.c_str(), ei.eventid.c_str(),
                       ei.provider.c_str(), ei.message.c_str());
            }
            log_print("EVT_RENDER id=" + ei.eventid + " level=" + ei.level + " status=OK");
            ++shown;
        }
    }
    double t1 = qpc_now_ms();
    if (o.format == "json") printf("\n]\n");
    log_print("EVT_CLOSE handles=" + std::to_string(shown + 1) + " status=OK");
    if (shown == 0) {
        printf("selection is empty\n");
        log_print("EVT_EMPTY status=OK");
    }
    std::string lv;
    for (auto& kv : levels) lv += (lv.empty() ? "" : ",") + std::to_string(kv.first) + ":" + std::to_string(kv.second);
    char buf[512];
    snprintf(buf, sizeof(buf), "STATS limit=%d events=%d timeMs=%.3f errors=%d levels=%s",
             o.limit, shown, t1 - t0, errors, lv.c_str());
    log_print(buf);
    printf("%s\n", buf);
    log_print("STOP result=0");
    return 0;
}
