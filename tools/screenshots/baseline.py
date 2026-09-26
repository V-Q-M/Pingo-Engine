#!/usr/bin/env python3
"""Puts a fresh copy of assets/ into the state the documentation starts from.

The developer keeps extra scenes in assets/ while working (copies, tests). The
screenshots of docs/index.html show the game with its three fixed scenes, so
every other scene is removed from the copy: from scenes.json and its files.
Only ever pass the copy inside a run folder, never the real assets/.
"""
import json
import os
import sys

KEEP = ["main_menu", "hub", "combat"]


def main() -> None:
    assets = sys.argv[1]
    path = os.path.join(assets, "scenes", "scenes.json")

    with open(path, encoding="utf-8") as file:
        data = json.load(file)

    kept = []

    for scene in data["scenes"]:
        if scene["id"] in KEEP:
            kept.append(scene)
            continue

        for name in (f"scenes/{scene['id']}.json", f"maps/{scene['id']}.txt"):
            try:
                os.remove(os.path.join(assets, name))
            except FileNotFoundError:
                pass

    data["scenes"] = kept

    with open(path, "w", encoding="utf-8") as file:
        json.dump(data, file, indent=2)
        file.write("\n")


main()
