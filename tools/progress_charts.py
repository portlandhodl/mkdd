#!/usr/bin/env python3
"""Render README progress charts (light + dark SVG) from objdiff report.json files.

    python tools/progress_charts.py --fork build/MarioClub_us/report.json \
        --upstream <upstream checkout>/build/MarioClub_us/report.json --out docs/images
"""
import argparse
import collections
import json
import os
from html import escape

FONT = 'system-ui, -apple-system, "Segoe UI", Helvetica, Arial, sans-serif'

THEMES = {
    "light": dict(
        surface="#fcfcfb", ink="#0b0b0b", ink2="#52514e", muted="#898781",
        grid="#e1e0d9", base="#c3c2b7", track="#eceae4", border="rgba(11,11,11,0.10)",
        upstream="#b4b2a9", fork="#2a78d6",
        linked="#256abf", matched="#86b6ef",
        cat=["#2a78d6", "#eb6834", "#1baf7a"], on_fill="#ffffff",
    ),
    "dark": dict(
        surface="#1a1a19", ink="#f0efec", ink2="#c3c2b7", muted="#898781",
        grid="#2c2c2a", base="#383835", track="#33332f", border="rgba(255,255,255,0.10)",
        upstream="#5f5e59", fork="#3987e5",
        linked="#6da7ec", matched="#256abf",
        cat=["#3987e5", "#d95926", "#199e70"], on_fill="#ffffff",
    ),
}

GAME_AREAS = ("Kameda", "Sato", "Shiraiwa", "Kaneshige", "Osako", "Inagaki", "Yamamoto", "Kawano")


def area_of(name):
    top = name.split("/")[1]
    if top == "JSystem":
        return "JSystem"
    if top == "dolphin":
        return "Dolphin SDK"
    if top in ("MSL_C", "Runtime"):
        return "MSL / Runtime"
    if top in ("debugger", "gamedev"):
        return "Debugger / gamedev"
    if top in GAME_AREAS:
        return top
    return None


def group_of(area):
    if area in GAME_AREAS:
        return "Game code"
    if area == "JSystem":
        return "JSystem"
    return "SDK / runtime"


def num(m, k):
    return float(m.get(k, 0) or 0)


def load(path):
    with open(path) as f:
        return json.load(f)


def kb(n):
    return f"{n / 1024:,.0f} KB" if n >= 10240 else f"{n / 1024:.1f} KB"


class Svg:
    def __init__(self, w, h, t, title, desc):
        self.w, self.h, self.t = w, h, t
        self.parts = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" '
            f'role="img" aria-labelledby="t d" font-family=\'{FONT}\'>',
            f"<title id=\"t\">{escape(title)}</title><desc id=\"d\">{escape(desc)}</desc>",
            f'<rect width="{w}" height="{h}" rx="12" fill="{t["surface"]}" stroke="{t["border"]}"/>',
        ]

    def add(self, s):
        self.parts.append(s)

    def text(self, x, y, s, size=13, fill=None, weight=400, anchor="start", extra=""):
        fill = fill or self.t["ink"]
        self.add(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" font-weight="{weight}" '
                 f'fill="{fill}" text-anchor="{anchor}" {extra}>{escape(s)}</text>')

    def bar(self, x, y, w, h, fill, r=4):
        # rounded data end on the right, square at the baseline
        if w <= 0:
            return
        r = min(r, w, h / 2)
        self.add(f'<path d="M{x:.1f},{y:.1f}h{w - r:.1f}a{r},{r} 0 0 1 {r},{r}v{h - 2 * r:.1f}'
                 f'a{r},{r} 0 0 1 -{r},{r}h-{w - r:.1f}z" fill="{fill}"/>')

    def rect(self, x, y, w, h, fill, rx=0, extra=""):
        if w > 0 and h > 0:
            self.add(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" fill="{fill}" {extra}/>')

    def swatch(self, x, y, fill, label):
        self.rect(x, y - 9, 12, 12, fill, rx=3)
        self.text(x + 18, y + 1, label, size=12, fill=self.t["ink2"])
        return x + 18 + 7.2 * len(label) + 22

    def save(self, path):
        self.add("</svg>")
        with open(path, "w") as f:
            f.write("\n".join(self.parts) + "\n")


def chart_overview(fork, up, t, path):
    W, H = 880, 448
    fm, um = fork["measures"], up["measures"]
    s = Svg(W, H, t, "Matching progress: this fork vs upstream",
            f"Code matched {fm['matched_code_percent']:.2f}% vs {um['matched_code_percent']:.2f}% upstream.")
    s.text(32, 44, "Matching progress", size=20, weight=650)
    s.text(32, 66, "MarioClub_us · share of original code bytes reproduced exactly", size=13, fill=t["ink2"])

    # hero stats
    stats = [
        ("Code matched", f"{fm['matched_code_percent']:.2f}%", f"upstream {um['matched_code_percent']:.2f}%"),
        ("Functions matched", f"{fm['matched_functions']:,} / {fm['total_functions']:,}",
         f"upstream {um['matched_functions']:,}"),
        ("Units fully linked", f"{fm['complete_units']} / {fm['total_units']}",
         f"upstream {um['complete_units']}"),
    ]
    for i, (lab, val, sub) in enumerate(stats):
        x = 32 + i * 282
        s.text(x, 104, lab, size=12, fill=t["ink2"])
        s.text(x, 136, val, size=28, weight=650)
        s.text(x, 156, sub, size=12, fill=t["muted"])

    # grouped bars by category
    rows = [("All code", fm, um)]
    upc = {c["id"]: c["measures"] for c in up["categories"]}
    for c in fork["categories"]:
        rows.append((c["name"], c["measures"], upc.get(c["id"], {})))
    x0, x1 = 150, W - 96
    pw = x1 - x0
    top = 206
    rh = 38
    for gx in (0, 25, 50, 75, 100):
        X = x0 + pw * gx / 100
        s.add(f'<line x1="{X:.1f}" y1="{top - 10}" x2="{X:.1f}" y2="{top + rh * len(rows) - 6}" '
              f'stroke="{t["grid"] if gx else t["base"]}" stroke-width="1"/>')
        s.text(X, top + rh * len(rows) + 10, f"{gx}%", size=11, fill=t["muted"], anchor="middle")
    for i, (name, f, u) in enumerate(rows):
        y = top + i * rh
        s.text(x0 - 14, y + 17, name, size=13, anchor="end", weight=600 if i == 0 else 400)
        uv, fv = num(u, "matched_code_percent"), num(f, "matched_code_percent")
        s.bar(x0, y, pw * uv / 100, 11, t["upstream"])
        s.bar(x0, y + 13, pw * fv / 100, 11, t["fork"])
        s.text(x0 + pw * uv / 100 + 6, y + 10, f"{uv:.1f}%", size=11, fill=t["ink2"])
        s.text(x0 + pw * fv / 100 + 6, y + 23, f"{fv:.1f}%", size=11, weight=600)
    ly = H - 22
    nx = s.swatch(32, ly, t["fork"], "This fork")
    s.swatch(nx, ly, t["upstream"], "Upstream doldecomp/mkdd")
    s.save(path)


def area_rows(rep):
    agg = collections.defaultdict(lambda: dict(total=0.0, matched=0.0, complete=0.0, units=0, done=0))
    for u in rep["units"]:
        a = area_of(u["name"])
        m = u["measures"]
        if not a or not num(m, "total_code"):
            continue
        x = agg[a]
        x["total"] += num(m, "total_code")
        x["matched"] += num(m, "matched_code")
        x["complete"] += num(m, "complete_code")
        x["units"] += 1
        x["done"] += 1 if u["metadata"].get("complete") else 0
    return agg


def chart_areas(fork, up, t, path):
    fa, ua = area_rows(fork), area_rows(up)
    order = sorted(fa, key=lambda a: (group_of(a) != "Game code", -fa[a]["total"]))
    W = 880
    top, rh = 96, 34
    H = top + rh * len(order) + 64
    s = Svg(W, H, t, "Progress by area",
            "Per-area share of code bytes linked from matching source, matched but not yet linked, "
            "and unmatched, with the upstream matched share marked.")
    s.text(32, 44, "Progress by area", size=20, weight=650)
    s.text(32, 66, "Bar = all code in the area · tick = upstream matched share", size=13, fill=t["ink2"])
    x0, x1 = 248, W - 236
    pw = x1 - x0
    prev_group = None
    for i, a in enumerate(order):
        y = top + i * rh
        g = group_of(a)
        if prev_group and g != prev_group:
            s.add(f'<line x1="32" y1="{y - 8}" x2="{W - 32}" y2="{y - 8}" stroke="{t["grid"]}"/>')
        prev_group = g
        f = fa[a]
        u = ua.get(a, dict(total=f["total"], matched=0))
        s.text(32, y + 15, a, size=13, weight=600)
        s.text(x0 - 14, y + 15, kb(f["total"]), size=11, fill=t["muted"], anchor="end")
        lw = pw * f["complete"] / f["total"]
        mw = pw * max(f["matched"] - f["complete"], 0) / f["total"]
        s.rect(x0, y, pw, 20, t["track"], rx=4)
        s.rect(x0, y, lw, 20, t["linked"], rx=4)
        if mw > 2:
            s.rect(x0 + lw + (2 if lw else 0), y, mw - (2 if lw else 0), 20, t["matched"], rx=4)
        ux = x0 + pw * u["matched"] / u["total"]
        s.add(f'<line x1="{ux:.1f}" y1="{y - 4}" x2="{ux:.1f}" y2="{y + 24}" stroke="{t["ink"]}" stroke-width="2"/>')
        pct = 100 * f["matched"] / f["total"]
        upct = 100 * u["matched"] / u["total"]
        s.text(x1 + 14, y + 15, f"{pct:.1f}%", size=13, weight=650)
        s.text(x1 + 66, y + 15, f"upstream {upct:.0f}% · {f['done']}/{f['units']} units", size=11, fill=t["ink2"])
    ly = H - 24
    nx = s.swatch(32, ly, t["linked"], "Linked from matching source")
    nx = s.swatch(nx, ly, t["matched"], "Matched, not yet linked")
    nx = s.swatch(nx, ly, t["track"], "Not matched")
    s.add(f'<line x1="{nx}" y1="{ly - 10}" x2="{nx}" y2="{ly + 4}" stroke="{t["ink"]}" stroke-width="2"/>')
    s.text(nx + 8, ly + 1, "Upstream matched", size=12, fill=t["ink2"])
    s.save(path)


def squarify(items, x, y, w, h):
    """items: list of (value, payload) sorted desc. Returns [(x,y,w,h,payload)]."""
    out = []
    items = [it for it in items if it[0] > 0]
    total = sum(v for v, _ in items)
    if not items or w <= 0 or h <= 0:
        return out
    scale = w * h / total
    rects = [(v * scale, p) for v, p in items]

    def worst(row, side):
        s_ = sum(a for a, _ in row)
        return max(max(side * side * a / (s_ * s_), (s_ * s_) / (side * side * a)) for a, _ in row)

    while rects:
        side = min(w, h)
        row = [rects.pop(0)]
        while rects and worst(row + [rects[0]], side) <= worst(row, side):
            row.append(rects.pop(0))
        s_ = sum(a for a, _ in row)
        if w >= h:
            cw = s_ / h
            cy = y
            for a, p in row:
                ch = a / cw
                out.append((x, cy, cw, ch, p))
                cy += ch
            x += cw
            w -= cw
        else:
            ch = s_ / w
            cx = x
            for a, p in row:
                cw_ = a / ch
                out.append((cx, y, cw_, ch, p))
                cx += cw_
            y += ch
            h -= ch
    return out


def chart_remaining(fork, t, path, top_n=60):
    units = []
    for u in fork["units"]:
        a = area_of(u["name"])
        m = u["measures"]
        rem = num(m, "total_code") - num(m, "matched_code")
        if a and rem > 0:
            units.append((rem, dict(name=u["name"].split("/")[-1], area=a, rem=rem,
                                    pct=num(m, "matched_code_percent"))))
    units.sort(key=lambda it: -it[0])
    total_rem = sum(v for v, _ in units)
    head, tail = units[:top_n], units[top_n:]
    if tail:
        head.append((sum(v for v, _ in tail), dict(name=f"{len(tail)} more units", area=None,
                                                   rem=sum(v for v, _ in tail), pct=None)))
    W, H = 880, 560
    s = Svg(W, H, t, "What is left to match",
            f"Treemap of the {kb(total_rem)} of code not yet matched, one box per unit, sized by unmatched bytes.")
    s.text(32, 44, "What is left to match", size=20, weight=650)
    s.text(32, 66, f"{kb(total_rem)} of code still unmatched · box area = unmatched bytes per unit",
           size=13, fill=t["ink2"])
    gx, gy, gw, gh = 32, 84, W - 64, H - 84 - 48
    colors = {"Game code": t["cat"][0], "JSystem": t["cat"][1], "SDK / runtime": t["cat"][2]}
    for (x, y, w, h, p) in squarify(head, gx, gy, gw, gh):
        fill = colors[group_of(p["area"])] if p["area"] else t["upstream"]
        tip = f'{p["name"]} — {kb(p["rem"])} unmatched' + (f', {p["pct"]:.1f}% matched' if p["pct"] is not None else "")
        s.add(f'<g><title>{escape(tip)}</title>')
        s.rect(x + 1, y + 1, w - 2, h - 2, fill, rx=3)
        s.add("</g>")
        ink = t["on_fill"] if p["area"] else t["ink"]
        if w > 64 and h > 30:
            label = p["name"]
            maxc = int((w - 12) / 6.6)
            if len(label) > maxc:
                label = label[: max(maxc - 1, 3)] + "…"
            s.text(x + 7, y + 17, label, size=11, fill=ink, weight=600)
            if h > 46:
                s.text(x + 7, y + 31, kb(p["rem"]), size=10, fill=ink, extra='fill-opacity="0.85"')
    ly = H - 20
    nx = 32
    for g, c in colors.items():
        nx = s.swatch(nx, ly, c, g)
    s.swatch(nx, ly, t["upstream"], "Smaller units, grouped")
    s.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fork", default="build/MarioClub_us/report.json")
    ap.add_argument("--upstream", required=True)
    ap.add_argument("--out", default="docs/images")
    a = ap.parse_args()
    fork, up = load(a.fork), load(a.upstream)
    os.makedirs(a.out, exist_ok=True)
    for mode, t in THEMES.items():
        chart_overview(fork, up, t, os.path.join(a.out, f"progress-overview-{mode}.svg"))
        chart_areas(fork, up, t, os.path.join(a.out, f"progress-areas-{mode}.svg"))
        chart_remaining(fork, t, os.path.join(a.out, f"progress-remaining-{mode}.svg"))


if __name__ == "__main__":
    main()
