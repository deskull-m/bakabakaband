#!/usr/bin/env python3
"""Compare every old W/S entry with the JSONC migration.

bakaba variant of the upstream hengband#5545 verifier: covers all 9
PlayerSkillKindType kinds (MARTIAL_ARTS..ASSHOLE). Skill kinds absent from the
legacy .txt for a class are treated as (0, 0), which matches the runtime
behavior (consumers read s_start/s_max via std::map::operator[], so an absent
key yields 0) and the value emitted by the migration.

Uses pyjson5 when available; otherwise falls back to a minimal JSONC reader
(strips // line comments and trailing commas).

Usage: python3 tools/verify-class-skill-migration.py --legacy-ref <pre-migration commit>
"""

import argparse
import re
from pathlib import Path
import subprocess

WEAPON_NAMES = ("BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD")
SKILL_NAMES = ("MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD",
               "GROSS_EATING", "SCATOLOGY", "ARMOR", "EVASION", "ASSHOLE")


def load_jsonc(text):
    try:
        import pyjson5
        return pyjson5.decode(text)
    except ImportError:
        import json
        no_block = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        no_line = re.sub(r"//[^\n]*", "", no_block)
        no_trailing = re.sub(r",(\s*[}\]])", r"\1", no_line)
        return json.loads(no_trailing)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-ref", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    legacy = subprocess.check_output(
        ["git", "show", f"{args.legacy_ref}:lib/edit/ClassSkillDefinitions.txt"],
        cwd=root,
    ).decode("utf-8")
    expected = {}
    ids = []
    class_id = None
    for line in legacy.splitlines():
        text = line.split("#", 1)[0].strip()
        if not text:
            continue
        tag, *values = text.split(":")
        values = list(map(int, values))
        if tag == "N":
            class_id = values[0]
            ids.append(class_id)
        elif tag in ("W", "S"):
            key = (class_id, tag, *values[:-2])
            if key in expected:
                raise ValueError(f"Duplicate legacy entry: {key}")
            expected[key] = tuple(values[-2:])
        else:
            raise ValueError(f"Unknown legacy directive: {text}")

    data = load_jsonc((root / "lib/edit/ClassSkillDefinitions.jsonc").read_text(encoding="utf-8"))
    if [entry["id"] for entry in data["classes"]] != ids:
        raise ValueError("Class IDs/order changed")
    actual = {}
    for entry in data["classes"]:
        class_id = entry["id"]
        for tval, name in enumerate(WEAPON_NAMES):
            weapon = entry["weapons"][name]
            for sval, pair in enumerate(zip(weapon["start_ranks"], weapon["max_ranks"], strict=True)):
                actual[(class_id, "W", tval, sval)] = pair
        for kind, name in enumerate(SKILL_NAMES):
            skill = entry["skills"][name]
            pair = (skill["start_exp"], skill["max_exp"])
            # A (0, 0) skill in the JSONC corresponds to an absent legacy entry.
            if pair != (0, 0) or (class_id, "S", kind) in expected:
                actual[(class_id, "S", kind)] = pair
    differences = [key for key in expected.keys() | actual.keys() if expected.get(key) != actual.get(key)]
    if differences:
        raise ValueError(f"Changed/missing entries: {differences[:20]}")
    print(f"Identical: {len(ids)} classes, {len(expected)} weapon/skill entries")


if __name__ == "__main__":
    main()
