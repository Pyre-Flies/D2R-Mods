"""Read-only source-table evidence for Defense reports; no native/runtime claims."""
import argparse
import csv
import hashlib
import json
from pathlib import Path


def read(path):
    with path.open(encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def audit(root):
    properties = read(root / "properties.txt")
    armor = read(root / "armor.txt")
    stats = read(root / "itemstatcost.txt")
    ids = {row["Stat"]: row.get("*ID", row.get("ID")) for row in stats if row.get("Stat")}
    defense = []
    for row in properties:
        components = [
            {"function": row.get(f"func{n}"), "stat": row.get(f"stat{n}"),
             "stat_id": ids.get(row.get(f"stat{n}")), "set": row.get(f"set{n}")}
            for n in range(1, 8)
            if "armor" in row.get(f"stat{n}", "")
        ]
        if components:
            defense.append({"code": row["code"], "range_mode": row.get("uiRangeType"),
                            "components": components})
    belts = []
    for row in armor:
        if row.get("type") != "belt":
            continue
        lo, hi = row.get("minac", ""), row.get("maxac", "")
        belts.append({"name": row.get("name"), "code": row.get("code"),
                      "minimum": lo, "maximum": hi,
                      "fixed_base_defense": bool(lo and hi and int(lo) == int(hi))})
    return {"evidence": "source tables only; compiled selection and visible tooltip unverified",
            "sha256": {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                       for name in ("properties.txt", "armor.txt", "itemstatcost.txt")},
            "defense_properties": defense, "belts": belts}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("excel", type=Path, help="Explicit active mod data/global/excel directory")
    args = parser.parse_args()
    print(json.dumps(audit(args.excel), indent=2))
