"""Locate machine-local artifacts without adding them to the source checkout."""
import os
from pathlib import Path
import sys


def workspace_root() -> Path:
    repo = Path(__file__).resolve().parents[1]
    return Path(os.environ.get("D2R_MODS_WORKSPACE", str(repo.parent / "D2R-Mods-Local"))).expanduser().resolve()


def add_python_dependencies() -> None:
    dependencies = workspace_root() / "dependencies" / "python-libs"
    if dependencies.is_dir():
        sys.path.insert(0, str(dependencies))


if __name__ == "__main__":
    print(workspace_root())
