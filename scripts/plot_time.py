import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data", "experiments.csv")
OUT = os.path.join(ROOT, "report", "img")


def load():
    with open(DATA, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main():
    os.makedirs(OUT, exist_ok=True)
    rows = load()
    limits = [20, 50, 100]

    fig, ax = plt.subplots(figsize=(9, 5.5), dpi=200)
    markers = ["o", "s", "^"]
    for i, lim in enumerate(limits):
        vals = [float(r["timeMs"]) for r in rows if int(r["limit"]) == lim]
        ax.plot([lim] * len(vals), vals, marker=markers[i], color="black", linestyle="none",
                markersize=6, label=f"запуски 1-3, limit={lim}")
    means = [sum(float(r["timeMs"]) for r in rows if int(r["limit"]) == lim) / 3 for lim in limits]
    ax.plot(limits, means, color="black", linewidth=1.4, label="среднее по 3 запускам")
    for lim, m in zip(limits, means):
        ax.text(lim, m + 0.4, f"{m:.2f}", ha="center", fontsize=8)
    ax.set_xlabel("Ограничение выборки, событий")
    ax.set_ylabel("Время выполнения выборки, мс")
    ax.set_title("Зависимость времени чтения журнала Application\nот размера выборки (20, 50, 100 событий)")
    ax.legend(frameon=True, edgecolor="black", fontsize=8)
    ax.grid(alpha=0.3, color="0.7")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "plot_time.png"), facecolor="white")
    print("saved plot_time.png")


if __name__ == "__main__":
    main()
