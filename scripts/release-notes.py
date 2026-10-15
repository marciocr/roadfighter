#!/usr/bin/env python3
"""Prints the GitHub release notes for a version, in Markdown.

The changes come from that version's section of the CHANGELOG
("**version: X.Y.Z**" followed by " * item" lines, possibly continued on
indented lines, up to a blank line); a fixed Downloads section follows.

Usage: scripts/release-notes.py 0.5.0
"""

import os
import re
import sys

DOWNLOADS = """## Downloads

- **RoadFighter-x86_64.AppImage**: make it executable (`chmod +x`) and run it; needs glibc 2.34 or newer.
- **RoadFighter.flatpak**: `flatpak install --user RoadFighter.flatpak` (needs the `org.freedesktop.Platform//25.08` runtime from Flathub).
"""


def changelog_items(changelog, version):
    header = re.compile(r"^\*\*version: " + re.escape(version) + r"\*\*")
    items = []
    in_section = False

    for line in changelog.splitlines():
        if not in_section:
            in_section = bool(header.match(line))
            continue
        if not line.strip():
            break
        if line.lstrip().startswith("* "):
            items.append(line.lstrip()[2:].strip())
        elif items:
            items[-1] += " " + line.strip()

    return items


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)

    version = sys.argv[1]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    with open(os.path.join(root, "CHANGELOG"), encoding="utf-8") as f:
        items = changelog_items(f.read(), version)

    if not items:
        sys.exit(f"CHANGELOG has no entries for version {version}")

    print("\n".join("- " + item for item in items))
    print()
    print(DOWNLOADS, end="")


if __name__ == "__main__":
    main()
