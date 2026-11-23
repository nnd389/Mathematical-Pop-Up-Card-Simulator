#!/usr/bin/env python3
"""Draw the flat pattern the simulator loads.

Usage:
    python3 visualize_flat.py                      # reads popUpCard.txt (FOLD), opens a window
    python3 visualize_flat.py pattern.txt          # another FOLD file (same loader rules as FoldLoader.cpp)
    python3 visualize_flat.py pattern.txt --scale 159
    python3 visualize_flat.py old.cpp              # a hand-written .cpp card (the old main.cpp format) still works
    python3 visualize_flat.py --save flat.png      # write an image instead of opening a window

FOLD files are read exactly the way FoldLoader.cpp reads them: file (x, y, z) -> (x, -z), mechanisms =
connected components ignoring glue springs (GS), outline = walk of each B-edge loop (clockwise, collinear glue targets skipped), creases = merged M/PM/V/PV
chains, glue dots on the higher mechanism pointing at the lower one. FOLD files open with y pointing up
(the loader's convention); .cpp files open y-down as before.

Two views are available: y increasing downwards (SVG style, the default) and y increasing
upwards. Toggle between them with the "Flip y" button or by pressing the T key.
With --save, both views are written (<name>_ydown.png and <name>_yup.png).

What is drawn:
    - each mechanism's outline (vertices in loop order), one colour per mechanism
    - vertex index numbers (v0, v1, ...) next to each vertex
    - mechanism id in the centre of each mechanism (and its Mechanism variable name)
    - creases (solid red = Mountain, dashed blue = Valley)
    - glue dots: a diamond at the glue position (on the mechanism it is glued ONTO),
      coloured like the mechanism that owns the vertex, labelled "m<owner>.v<i>",
      and the owner's vertex is ringed so you can see which vertices are glued.

For .cpp input it parses the source as text, following the shapes the old main.cpp used:
    std::vector<Eigen::Vector3f> NAME = { Eigen::Vector3f{x, y, z}, ... };
    std::vector<Crease> NAME = { Crease {i, j, CreaseType::Mountain}, ... };
    std::vector<GlueDot> NAME = { GlueDot {i, n, Eigen::Vector3f{x, y, z}, ...}, ... };
    Mechanism NAME(points, creases, glues, isBaseCard, id);
"""
import argparse
import json
import math
import re
import sys
from collections import deque

import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
from matplotlib.widgets import Button

COLORS = ["#4c78a8", "#f58518", "#54a24b", "#b279a2", "#e45756", "#72b7b2", "#9d755d", "#bab0ac"]
NUM = r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?"
VEC = rf"Eigen::Vector3f\s*\{{\s*({NUM})\s*,\s*({NUM})\s*,\s*({NUM})\s*\}}"


def strip_comments(src):
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    return re.sub(r"//[^\n]*", "", src)


def block(src, decl_regex):
    """Return {name: text between the outermost braces} for each declaration matching decl_regex."""
    out = {}
    for m in re.finditer(decl_regex, src):
        i = src.index("{", m.end() - 1)
        depth, j = 0, i
        while j < len(src):
            depth += src[j] == "{"
            depth -= src[j] == "}"
            if depth == 0:
                break
            j += 1
        out[m.group(1)] = src[i + 1:j]
    return out


def parse(path):
    src = strip_comments(open(path).read())

    points = {n: [(float(x), float(y)) for x, y, _ in re.findall(VEC, b)]
              for n, b in block(src, r"std::vector<\s*Eigen::Vector3f\s*>\s+(\w+)\s*=\s*\{").items()}
    creases = {n: [(int(i), int(j), t) for i, j, t in re.findall(r"Crease\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*CreaseType::(\w+)", b)]
               for n, b in block(src, r"std::vector<\s*Crease\s*>\s+(\w+)\s*=\s*\{").items()}
    glues = {n: [(int(i), int(k), (float(x), float(y))) for i, k, x, y, _ in
                 re.findall(rf"GlueDot\s*\{{\s*(\d+)\s*,\s*(\d+)\s*,\s*{VEC}", b)]
             for n, b in block(src, r"std::vector<\s*GlueDot\s*>\s+(\w+)\s*=\s*\{").items()}

    mechs = []
    for m in re.finditer(r"\bMechanism\s+(\w+)\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\d+)\s*\)", src):
        name, p, c, g, base, mid = m.groups()
        if p not in points:
            print(f"warning: mechanism {name}: no points vector named '{p}' found", file=sys.stderr)
            continue
        mechs.append(dict(name=name, id=int(mid), base=base == "true", pts=points[p],
                          creases=creases.get(c, []), glues=glues.get(g, [])))
    return mechs


def parse_fold(path, scale=1.0):
    """Python port of FoldLoader.cpp: returns the same mechanism dicts that parse() returns."""
    with open(path) as f:
        d = json.load(f)
    # 3D files are (x, y, z) -> (x, -z); 2D files (makeTowerCard.py) are (x, y) -> (x, -y). Same as FoldLoader.cpp.
    pos = [(v[0] * scale, -v[2 if len(v) >= 3 else 1] * scale) for v in d["vertices_coords"]]
    edges = [tuple(e) for e in d["edges_vertices"]]
    assign = d["edges_assignment"]
    nv = len(pos)

    # components ignoring glue springs
    parent = list(range(nv))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for (a, b), t in zip(edges, assign):
        if t != "GS":
            parent[find(a)] = find(b)

    glue_set = {(min(a, b), max(a, b)) for (a, b), t in zip(edges, assign) if t == "GS"}  # file lists each twice
    adj = {}
    for a, b in glue_set:
        ra, rb = find(a), find(b)
        adj.setdefault(ra, set()).add(rb)
        adj.setdefault(rb, set()).add(ra)

    # mechanism order: base = component of vertex 0, then BFS across glue springs
    base_root = find(0)
    comp = {base_root: 0}
    order = [base_root]
    q = deque([base_root])
    while q:
        r = q.popleft()
        for nb in sorted(adj.get(r, ())):
            if nb not in comp:
                comp[nb] = len(order)
                order.append(nb)
                q.append(nb)
    M = len(order)

    # flatPos = outline only: walk each mechanism's cyclic loop of B edges (see FoldLoader.cpp)
    b_nbr = {v: [] for v in range(nv)}
    fold_deg = [0] * nv
    glue_ends = set()
    for (a, b), t in zip(edges, assign):
        if t == "B":
            b_nbr[a].append(b)
            b_nbr[b].append(a)
        if t in ("M", "PM", "V", "PV"):
            fold_deg[a] += 1
            fold_deg[b] += 1
        if t == "GS":
            glue_ends.update((a, b))
    mech_of = {v: comp[find(v)] for v in range(nv) if find(v) in comp}

    # a glue target in the middle of a straight outline edge is walked past (crease endpoints are kept)
    skip = set()
    for v in range(nv):
        if len(b_nbr[v]) != 2 or fold_deg[v] == 1 or v not in glue_ends:
            continue
        d1 = (pos[b_nbr[v][0]][0] - pos[v][0], pos[b_nbr[v][0]][1] - pos[v][1])
        d2 = (pos[b_nbr[v][1]][0] - pos[v][0], pos[b_nbr[v][1]][1] - pos[v][1])
        cross = d1[0] * d2[1] - d1[1] * d2[0]
        ln = math.hypot(*d1) * math.hypot(*d2)
        if ln > 0 and d1[0] * d2[0] + d1[1] * d2[1] < 0 and abs(cross) / ln < 0.25:
            skip.add(v)

    def walk_past(frm, first):
        prev, cur = frm, first
        while cur in skip:
            nxt = b_nbr[cur][1] if b_nbr[cur][0] == prev else b_nbr[cur][0]
            prev, cur = cur, nxt
        return prev, cur

    local_of = {}
    verts = [[] for _ in range(M)]
    for m in range(M):
        members = [v for v in range(nv) if mech_of.get(v) == m and b_nbr[v]]
        start = next((v for v in members if v not in skip), None)
        if start is None:
            raise SystemExit(f"mechanism {m} has no outline (no B edges)")
        bad = [v for v in members if len(b_nbr[v]) != 2]
        if bad:
            raise SystemExit(f"B edges of mechanism {m} do not form simple loops (vertex {bad[0]})")
        d0, d1 = walk_past(start, b_nbr[start][0]), walk_past(start, b_nbr[start][1])
        prev, cur = d0 if d0[1] <= d1[1] else d1  # head toward the lower-numbered neighbour
        loop = [start]
        while cur != start:
            if len(loop) > nv:
                raise SystemExit("outline walk did not close")
            loop.append(cur)
            raw = b_nbr[cur][1] if b_nbr[cur][0] == prev else b_nbr[cur][0]
            prev, cur = walk_past(cur, raw)
        # every outline runs clockwise (shoelace area <= 0); keep the start vertex at local index 0
        area2 = sum(pos[loop[i]][0] * pos[loop[(i + 1) % len(loop)]][1]
                    - pos[loop[(i + 1) % len(loop)]][0] * pos[loop[i]][1] for i in range(len(loop)))
        if area2 > 0:
            loop = [loop[0]] + loop[:0:-1]
        verts[m] = loop
        for l, v in enumerate(loop):
            local_of[v] = l
        extra = [v for v in members if v not in skip and v not in local_of]
        if extra:
            print(f"warning: mechanism {m} has more than one B-edge loop (vertex {extra[0]}); "
                  "only the first loop is used", file=sys.stderr)

    # creases: merge chains of fold edges
    fadj, mountain_at = {}, {}
    for (a, b), t in zip(edges, assign):
        if t in ("M", "PM", "V", "PV"):
            fadj.setdefault(a, []).append(b)
            fadj.setdefault(b, []).append(a)
            mountain_at[a] = mountain_at[b] = t in ("M", "PM")
    creases = [[] for _ in range(M)]
    seen = set()
    for start in sorted(fadj):
        if start in seen or len(fadj[start]) != 1:
            continue
        prev, cur = -1, start
        seen.add(cur)
        while True:
            nxt = next((n for n in fadj[cur] if n != prev), None)
            if nxt is None:
                break
            prev, cur = cur, nxt
            seen.add(cur)
        a, b = start, cur
        if pos[a][1] > pos[b][1]:
            a, b = b, a  # i = lower end
        creases[mech_of[a]].append((local_of[a], local_of[b], "Mountain" if mountain_at[start] else "Valley"))

    # glue dots: on the higher mechanism, targeting the lower one
    glues = [[] for _ in range(M)]
    # triple points: a child vertex glued to parents that are glued to each other keeps only the nearest parent
    nbrs = {}
    for a, b in glue_set:
        nbrs.setdefault(a, []).append(b)
        nbrs.setdefault(b, []).append(a)
    drop_glue = set()
    for a, nb in nbrs.items():
        parents = [b for b in nb if b in mech_of and mech_of[b] < mech_of.get(a, -1)]
        for b1 in parents:
            for b2 in parents:
                if b1 < b2 and mech_of[b1] != mech_of[b2] and (b1, b2) in glue_set:
                    d = b1 if mech_of[b1] < mech_of[b2] else b2
                    drop_glue.add((min(a, d), max(a, d)))
    for a, b in sorted(glue_set):
        if (a, b) in drop_glue:
            continue
        if mech_of[a] == mech_of[b]:
            continue
        if mech_of[a] < mech_of[b]:
            a, b = b, a  # a = child, b = parent
        glues[mech_of[a]].append((local_of[a], mech_of[b], pos[b]))
    for m in range(M):
        glues[m].sort(key=lambda g: (g[1], pos[verts[m][g[0]]][0]))

    return [dict(name="base" if m == 0 else f"mech{m}", id=m, base=m == 0, pts=[pos[v] for v in verts[m]],
                 creases=creases[m], glues=glues[m]) for m in range(M)]


def draw(mechs, save, y_up=False):
    if save:
        stem = save[:-4] if save.lower().endswith(".png") else save
        for y_up, suffix in ((False, "_ydown"), (True, "_yup")):
            fig, ax = plt.subplots(figsize=(11, 8))
            draw_view(ax, mechs, y_up=y_up, warn=not y_up)
            plt.tight_layout()
            fig.savefig(f"{stem}{suffix}.png", dpi=150)
            print(f"saved {stem}{suffix}.png")
        return

    fig, ax = plt.subplots(figsize=(11, 8.5))
    plt.subplots_adjust(bottom=0.12)
    state = {"y_up": y_up, "warn": True}

    def redraw():
        ax.clear()
        draw_view(ax, mechs, y_up=state["y_up"], warn=state["warn"])
        state["warn"] = False  # only print warnings once
        fig.canvas.draw_idle()

    def toggle(_event=None):
        state["y_up"] = not state["y_up"]
        redraw()

    button = Button(plt.axes([0.42, 0.02, 0.16, 0.06]), "Flip y  (T)")
    button.on_clicked(toggle)
    fig.canvas.mpl_connect("key_press_event", lambda e: toggle() if e.key in ("t", "T") else None)
    redraw()
    plt.show()


def draw_view(ax, mechs, y_up, warn):
    by_id = {m["id"]: m for m in mechs}
    color = {m["id"]: COLORS[k % len(COLORS)] for k, m in enumerate(mechs)}

    for m in mechs:
        c, pts = color[m["id"]], m["pts"]
        ax.add_patch(Polygon(pts, closed=True, facecolor=c, alpha=0.15, edgecolor=c, linewidth=2))

        # creases
        for i, j, t in m["creases"]:
            if i < len(pts) and j < len(pts):
                mtn = t == "Mountain"
                ax.plot([pts[i][0], pts[j][0]], [pts[i][1], pts[j][1]],
                        color="crimson" if mtn else "royalblue", linestyle="-" if mtn else "--", linewidth=2.2, zorder=3)
            else:
                if warn:
                    print(f"warning: mechanism {m['id']} crease ({i},{j}) out of range", file=sys.stderr)

        # vertices + indices
        glued = {i for i, _, _ in m["glues"]}
        for k, (x, y) in enumerate(pts):
            ax.plot(x, y, "o", color=c, markersize=5, zorder=4)
            if k in glued:
                ax.plot(x, y, "o", markerfacecolor="none", markeredgecolor="black", markersize=12, zorder=4)
            ax.annotate(f"v{k}", (x, y), textcoords="offset points", xytext=(6, 6), fontsize=10,
                        fontweight="bold", color=c, zorder=5)

        # mechanism label
        cx = sum(p[0] for p in pts) / len(pts)
        cy = sum(p[1] for p in pts) / len(pts)
        tag = "base card" if m["base"] else "mechanism"
        ax.text(cx, cy, f"M{m['id']}\n({m['name']}, {tag})", ha="center", va="center", fontsize=11,
                color=c, fontweight="bold", zorder=5)

    # glue dots (drawn on the mechanism they are glued onto)
    for m in mechs:
        c, pts = color[m["id"]], m["pts"]
        for i, n, (gx, gy) in m["glues"]:
            ax.plot(gx, gy, "D", markerfacecolor=c, markeredgecolor="black", markersize=9, zorder=6)
            ax.annotate(f"m{m['id']}.v{i}", (gx, gy), textcoords="offset points", xytext=(7, -13),
                        fontsize=9, color=c, zorder=7,
                        bbox=dict(boxstyle="round,pad=0.15", fc="white", ec=c, alpha=0.85))
            if not warn:
                continue
            if n not in by_id:
                print(f"warning: mechanism {m['id']} glue dot targets unknown mechanism {n}", file=sys.stderr)
            elif not _inside(by_id[n]["pts"], gx, gy):
                print(f"note: glue dot m{m['id']}.v{i} at ({gx:.1f}, {gy:.1f}) is outside mechanism {n}'s outline",
                      file=sys.stderr)

    ax.set_aspect("equal")
    ax.autoscale_view()
    if not y_up:
        ax.invert_yaxis()  # SVG style: y grows downward
    ax.grid(True, alpha=0.3)
    ax.set_xlabel("x")
    ax.set_ylabel("y (up)" if y_up else "y (down)")
    ax.set_title(("y increasing upwards" if y_up else "y increasing downwards")
                 + "\nringed vertex = glue dot, diamond = glue position, red solid = mountain, blue dashed = valley",
                 fontsize=10)


def _inside(poly, x, y):
    """Point-in-polygon, counting points on (or within 1% of the size of) the outline as inside."""
    xs, ys = [p[0] for p in poly], [p[1] for p in poly]
    tol = 0.01 * max(max(xs) - min(xs), max(ys) - min(ys))
    ins = False
    for k in range(len(poly)):
        x1, y1 = poly[k]
        x2, y2 = poly[(k + 1) % len(poly)]
        dx, dy = x2 - x1, y2 - y1
        t = 0.0 if dx == dy == 0 else max(0.0, min(1.0, ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy)))
        if ((x - (x1 + t * dx)) ** 2 + (y - (y1 + t * dy)) ** 2) ** 0.5 <= tol:
            return True
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            ins = not ins
    return ins


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", nargs="?", default="popUpCard.txt")
    ap.add_argument("--scale", type=float, default=1.0, help="scale applied to FOLD coordinates (like loadCard's scale)")
    ap.add_argument("--save", metavar="PNG", help="save to an image file instead of opening a window")
    a = ap.parse_args()
    is_cpp = a.file.lower().endswith((".cpp", ".hpp", ".cc", ".h"))
    ms = parse(a.file) if is_cpp else parse_fold(a.file, a.scale)
    if not ms:
        sys.exit(f"no mechanisms found in {a.file}")
    draw(ms, a.save, y_up=not is_cpp)
