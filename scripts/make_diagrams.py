import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "report", "img")


def box(ax, x, y, w, h, text, fs=9):
    b = FancyBboxPatch((x, y), w, h, boxstyle="square,pad=0.04", fc="white", ec="black", lw=1.1)
    ax.add_patch(b)
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs)


def arrow(ax, x1, y1, x2, y2, label="", ly=0.09):
    a = FancyArrowPatch((x1, y1), (x2, y2), arrowstyle="-|>", mutation_scale=13,
                        lw=1.0, color="black")
    ax.add_patch(a)
    if label:
        ax.text((x1 + x2) / 2, (y1 + y2) / 2 + ly, label, ha="center", fontsize=8)


def architecture():
    fig, ax = plt.subplots(figsize=(10.5, 6.8), dpi=200)
    box(ax, 0.4, 5.2, 2.5, 0.8, "Разбор аргументов\n--log --limit --format\n--query --log-file")
    box(ax, 3.9, 5.2, 2.6, 0.8, "Открытие журнала\nEvtQuery (канал или XPath,\nобратный порядок)")
    box(ax, 7.5, 5.2, 2.7, 0.8, "Пакетное чтение\nEvtNext по 64 дескриптора,\nERROR_NO_MORE_ITEMS")
    box(ax, 7.5, 3.9, 2.7, 0.8, "Рендер события\nEvtRenderEventXml,\nрост буфера по коду ошибки")
    box(ax, 3.9, 3.9, 2.6, 0.8, "Разбор XML и сообщение\nProvider, EventID, Level,\nEvtFormatMessage + publisher")
    box(ax, 3.9, 2.6, 2.6, 0.8, "Формат вывода\ntext / csv / json,\nограничение --limit")
    box(ax, 0.4, 2.6, 2.5, 0.8, "Журнал утилиты\nEVT_QUERY, EVT_BATCH,\nEVT_RENDER, EVT_CLOSE")
    box(ax, 7.5, 2.6, 2.7, 0.8, "Освобождение ресурсов\nEvtClose на каждый\nдескриптор (RAII EvtGuard)")
    arrow(ax, 2.9, 5.6, 3.9, 5.6)
    arrow(ax, 6.5, 5.6, 7.5, 5.6)
    arrow(ax, 8.85, 5.2, 8.85, 4.7)
    arrow(ax, 7.5, 4.3, 6.5, 4.3)
    arrow(ax, 5.2, 3.9, 5.2, 3.4)
    arrow(ax, 3.9, 3.0, 2.9, 3.0, "события")
    arrow(ax, 6.5, 3.0, 7.5, 3.0, "EvtClose")
    ax.set_xlim(0, 10.6)
    ax.set_ylim(2.2, 6.3)
    ax.axis("off")
    ax.set_title("Архитектура утилиты просмотра событий журнала Application", fontsize=11)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "architecture.png"), facecolor="white", bbox_inches="tight")
    plt.close(fig)
    print("saved architecture.png")


def flow():
    fig, ax = plt.subplots(figsize=(10.5, 5.2), dpi=200)
    steps = [
        "Пуск\nразбор аргументов",
        "EvtQuery\nканал Application\nили XPath",
        "EvtNext\nпакет до 64\nдескрипторов",
        "EvtRenderEventXml\nразбор Provider,\nEventID, Level, Time",
        "EvtFormatMessage\nили пометка\n«нет сообщения»",
        "Вывод записи\ntext / csv / json\nсчётчик <= limit",
        "EvtClose\nдескриптора события",
    ]
    x = 0.2
    w = 1.32
    gap = 0.16
    for i, text in enumerate(steps):
        box(ax, x, 2.2, w, 1.15, text, fs=7.5)
        if i:
            arrow(ax, x - gap, 2.77, x, 2.77)
        x += w + gap
    box(ax, 3.4, 0.6, 3.4, 0.8, "Пакет исчерпан?\nнет — следующий EvtNext;\nда — STATS и EvtClose набора", fs=8)
    arrow(ax, 5.1, 2.2, 5.1, 1.4)
    arrow(ax, 6.8, 1.0, 8.6, 1.0, "")
    arrow(ax, 8.9, 1.4, 8.9, 2.2, "")
    ax.text(9.0, 0.7, "к блоку вывода", fontsize=8)
    ax.set_xlim(0, x)
    ax.set_ylim(0.2, 3.8)
    ax.axis("off")
    ax.set_title("Блок-схема алгоритма выборки последних N событий журнала", fontsize=11)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "flow.png"), facecolor="white", bbox_inches="tight")
    plt.close(fig)
    print("saved flow.png")


def main():
    os.makedirs(OUT, exist_ok=True)
    architecture()
    flow()


if __name__ == "__main__":
    main()
