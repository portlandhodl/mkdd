#!/usr/bin/env python3
"""List unmatched functions from report.json.

Usage: python tools/list_unmatched.py [file-substring]
With no args, prints per-file summary sorted by unmatched bytes.
With a substring, prints unmatched functions of matching files.
"""
import json
import sys

d = json.load(open("build/MarioClub_us/report.json"))
sub = sys.argv[1] if len(sys.argv) > 1 else None

if sub is None:
    rows = []
    for u in d["units"]:
        m = u["measures"]
        try:
            total = int(m.get("total_code", 0))
            matched = int(m.get("matched_code", 0))
        except (TypeError, ValueError):
            continue
        if total == 0 or matched == total:
            continue
        rows.append((total - matched, matched, total, m.get("total_functions", 0), m.get("matched_functions", 0), u["name"]))
    rows.sort(reverse=True)
    for r in rows:
        print(f"{r[0]:7d} | {r[1]:7d}/{r[2]:7d} | f {r[4]:3d}/{r[3]:3d} | {r[5]}")
else:
    for u in d["units"]:
        if sub.lower() not in u["name"].lower():
            continue
        print(f"== {u['name']} ({u['measures']['matched_code_percent']:.1f}% code, {u['measures']['matched_functions']}/{u['measures']['total_functions']} funcs)")
        for f in u.get("functions", []):
            pct = f.get("fuzzy_match_percent", 0)
            if pct < 99.9:
                size = int(f.get("size", 0))
                demangled = f.get("metadata", {}).get("demangled_name", "")
                print(f"  {pct:6.2f}% {size:6d}  {f['name']}  {demangled}")
