import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DST = ROOT / "report" / "listings"

FILES = {
    ROOT / "src" / "evtview.cpp": DST / "evtview.cpp",
    ROOT / "scripts" / "plot_time.py": DST / "plot_time.py",
    ROOT / "scripts" / "plot_levels.py": DST / "plot_levels.py",
    ROOT / "diagrams" / "flow.puml": DST / "flow.puml",
}


def main():
    DST.mkdir(parents=True, exist_ok=True)
    for src, dst in FILES.items():
        shutil.copyfile(src, dst)
        print("synced", dst.name)


if __name__ == "__main__":
    main()
