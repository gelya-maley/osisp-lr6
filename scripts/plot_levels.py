import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data", "experiments.csv")
OUT = os.path.join(ROOT, "report", "img")

NAMES = {"1": "Critical", "2": "Error", "3": "Warning", "4": "Information", "5": "Verbose"}
FILLS = ["0.15", "0.35", "0.55", "0.8", "white"]
HATCH = ["", "", "", "", "xx"]


def load():
    with open(DATA, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main():
    os.makedirs(OUT, exist_ok=True)
    rows = [r for r in load() if int(r["limit"]) == 100]
    totals = {lv: sum(int(r[f"L{lv}"]) for r in rows) for lv in range(1, 6)}

    fig, ax = plt.subplots(figsize=(9, 5.5), dpi=200)
    xs = range(1, 6)
    ax.bar(xs, [totals[lv] for lv in xs], color=[FILLS[lv - 1] for lv in xs],
           edgecolor="black", hatch=[HATCH[lv - 1] for lv in xs])
    for lv in xs:
        ax.text(lv, totals[lv] + 1, str(totals[lv]), ha="center", fontsize=9)
    ax.set_xticks(list(xs))
    ax.set_xticklabels([f"{lv} — {NAMES[str(lv)]}" for lv in xs])
    ax.set_xlabel("Уровень важности события")
    ax.set_ylabel("Число событий в трёх выборках по 100")
    ax.set_title("Распределение событий журнала Application по уровням\n(выборка последних 100 событий, 3 запуска)")
    ax.grid(axis="y", alpha=0.3, color="0.7")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "plot_levels.png"), facecolor="white")
    print("saved plot_levels.png")


if __name__ == "__main__":
    main()
