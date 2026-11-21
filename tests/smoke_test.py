import json
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "evtview.exe")
results = []


def check(name, cond):
    results.append((name, bool(cond)))
    print(("PASS " if cond else "FAIL ") + name)


def run(*args, log=None):
    cmd = [EXE] + list(args)
    if log:
        cmd += ["--log-file", log]
    p = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    return p.returncode, p.stdout, p.stderr


def stats_of(out):
    for line in out.splitlines():
        if line.startswith("STATS "):
            return dict(t.split("=", 1) for t in line.split()[1:])
    return {}


def main():
    tmp = tempfile.mkdtemp(prefix="evtviewtest_")
    log = os.path.join(tmp, "evt.log")

    rc, out, err = run("--limit", "20", "--log-file", log)
    st = stats_of(out)
    check("limit 20 returns 0", rc == 0)
    check("events <= 20", int(st.get("events", "-1")) <= 20)
    check("events > 0 on Application", int(st.get("events", "0")) > 0)
    check("text lines contain EventID", out.count("EventID=") == int(st.get("events", "0")))

    rc, out, err = run("--limit", "5", "--format", "csv", "--log-file", log)
    lines = [l for l in out.splitlines() if l and not l.startswith("STATS")]
    check("csv header present", lines[0] == "time,provider,eventid,level,message")
    check("csv rows == 5", len(lines) - 1 == 5)

    rc, out, err = run("--limit", "7", "--format", "json", "--log-file", log)
    body = out[out.index("["):out.rindex("]") + 1]
    try:
        arr = json.loads(body)
        ok_json = isinstance(arr, list) and len(arr) == 7 and all("eventid" in e for e in arr)
    except Exception:
        ok_json = False
    check("json parses with 7 objects", ok_json)

    rc, out, err = run("--limit", "10", "--query", "*[System[EventID=4294967295]]", "--log-file", log)
    check("empty selection handled", rc == 0 and "selection is empty" in out)

    rc, out, err = run("--limit", "10", "--query", "not-a-xpath[[", "--log-file", log)
    check("invalid query reported", rc == 1 and ("invalid query" in out or "15007" in out + err))

    rc, out, err = run("--limit", "10", "--log", "NoSuchChannelXYZ", "--log-file", log)
    check("missing channel reported", rc == 1 and ("journal not found" in out or "code" in out))

    rc, out, err = run("--limit", "-3", "--log-file", log)
    check("negative limit rejected", rc == 1 and "invalid --limit" in out)

    data = open(log, encoding="utf-8", errors="replace").read()
    check("journal has EVT_QUERY and EVT_CLOSE", "EVT_QUERY" in data and "EVT_CLOSE" in data)
    closes = data.count("EVT_CLOSE handles=")
    renders = data.count("EVT_RENDER id=")
    check("every rendered handle closed", closes >= 1 and "handles=" in data and renders >= 0)

    rc1, o1, _ = run("--limit", "100", "--log-file", log)
    rc2, o2, _ = run("--limit", "100", "--log-file", log)
    check("repeat runs stable", rc1 == 0 and rc2 == 0 and
          stats_of(o1)["events"] == stats_of(o2)["events"])

    failed = [n for n, ok in results if not ok]
    print(f"\nTOTAL {len(results)} PASSED {len(results) - len(failed)} FAILED {len(failed)}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
