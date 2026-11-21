import csv
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "evtview.exe")
DATA = os.path.join(ROOT, "data")
RUNS = 3

CONFIGS = [("A", 20), ("B", 50), ("C", 100)]


def parse_stats(out):
    for line in out.splitlines():
        if line.startswith("STATS "):
            return dict(t.split("=", 1) for t in line.split()[1:])
    return {}


def main():
    os.makedirs(DATA, exist_ok=True)
    rows = []
    for cfg, limit in CONFIGS:
        for run in range(1, RUNS + 1):
            log = os.path.join(DATA, f"run_{cfg}_{run}.log")
            p = subprocess.run([EXE, "--limit", str(limit), "--format", "csv",
                                "--log-file", log],
                               capture_output=True, text=True, cwd=ROOT)
            s = parse_stats(p.stdout)
            if not s:
                print("NO STATS", p.stdout, p.stderr)
                sys.exit(1)
            levels = dict(kv.split(":") for kv in s.get("levels", "").split(",") if kv)
            row = {
                "config": cfg, "limit": limit, "run": run,
                "events": s["events"], "timeMs": s["timeMs"], "errors": s["errors"],
            }
            for lv in range(1, 6):
                row[f"L{lv}"] = levels.get(str(lv), "0")
            rows.append(row)
            print(f"config={cfg} run={run} events={row['events']} time={row['timeMs']}ms levels={levels}")
            sys.stdout.flush()
            time.sleep(0.3)
    out = os.path.join(DATA, "experiments.csv")
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print("saved " + out)


if __name__ == "__main__":
    main()
