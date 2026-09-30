#!/usr/bin/env python3
"""
visualize_card.py

Parses the stdout of the Mathematical-Pop-Up-Card-Simulator and plots each
mechanism's vertices, both in 3D (folded states) and 2D (the flat pattern).

It looks for text of this shape (whatever else is interleaved with it, e.g.
printCurrentPoints() output, is ignored):

    Mechanism 0 vertices:
    Vertex 0 is: (0, 0, 0)
    Vertex 1 is: (-1, 0, 0)
    ...

    Mechanism 0 creases:
    Crease 0 3 Valley

Vertices within a mechanism are assumed to be connected cyclically
(0 -> 1 -> 2 -> ... -> n-1 -> 0), per the simulator's convention. Crease lines
(if present in the input) are drawn as dashed segments between the two
vertices they connect, colored by fold type (mountain = red, valley = blue).
NOTE: crease lines require your C++ program to actually print crease data —
see the printCreases()/printCurrentCreases() note below if you don't have
that yet; without it, this script still works fine, it just won't have any
creases to draw.

USAGE
-----
Build and run your simulator, capturing its output, then feed that to this
script:

    ./popUp > run.txt
    python3 visualize_card.py run.txt

or pipe directly:

    ./popUp | python3 visualize_card.py

If your program prints multiple snapshots (e.g. once before actuation and
once after, like main.cpp currently does), this script splits them into
separate snapshots automatically. By default it shows the LAST snapshot.
Use --snapshot to pick a different one, or --all to see every snapshot
side by side.

By convention, snapshot 0 is assumed to be the FLAT pattern (printed before
any actuation), so it's rendered as a 2D plot (z is ignored). Every other
snapshot is rendered in 3D. Use --flat-snapshot to change which index counts
as flat, or --no-flat-2d to render everything in 3D.

OPTIONS
-------
    --snapshot N     plot snapshot index N (0 = first). Default: last snapshot.
    --all            plot every snapshot in a grid of subplots instead of one.
    --flat-snapshot N  which snapshot index is the flat pattern, rendered in
                       2D (default: 0).
    --no-flat-2d     disable the 2D flat-pattern rendering; always use 3D.
    --wireframe      draw edges only, no translucent face fill.
    --no-labels      hide vertex index labels.
    --save PATH      save the figure to PATH instead of (or in addition to)
                      showing it interactively.
    --no-show        don't open an interactive window (useful with --save).

Requires: matplotlib (pip install matplotlib)
"""

import argparse
import re
import sys

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

VERTICES_HEADER_RE = re.compile(r"Mechanism\s+(-?\d+)\s+vertices:")
CREASES_HEADER_RE = re.compile(r"Mechanism\s+(-?\d+)\s+creases:")
VERTEX_RE = re.compile(
    r"Vertex\s+(-?\d+)\s+is:\s*\(\s*"
    r"([-+0-9.eE]+)\s*,\s*"
    r"([-+0-9.eE]+)\s*,\s*"
    r"([-+0-9.eE]+)\s*\)"
)
CREASE_RE = re.compile(
    r"Crease\s+(-?\d+)\s+(-?\d+)\s+(Mountain|Valley)", re.IGNORECASE
)

CREASE_COLORS = {"mountain": "#d62728", "valley": "#1f77b4"}


def parse_snapshots(text):
    """
    Returns a list of snapshots. Each snapshot is a dict:
        { mechanism_id: {"vertices": [(idx, x, y, z), ...],
                          "creases":  [(i, j, "mountain"|"valley"), ...]} }
    A new snapshot starts whenever a mechanism's "vertices" header repeats
    (i.e. the program printed "Mechanism <id> vertices:" for that id again).
    """
    snapshots = []
    current = {}
    current_mech = None
    current_section = None

    def ensure_mech(mech_id):
        if mech_id not in current:
            current[mech_id] = {"vertices": [], "creases": []}

    for line in text.splitlines():
        vh = VERTICES_HEADER_RE.search(line)
        if vh:
            mech_id = int(vh.group(1))
            if mech_id in current and current[mech_id]["vertices"]:
                # seen this mechanism's vertices before -> new snapshot
                snapshots.append(current)
                current = {}
            ensure_mech(mech_id)
            current_mech = mech_id
            current_section = "vertices"
            continue

        ch = CREASES_HEADER_RE.search(line)
        if ch:
            mech_id = int(ch.group(1))
            ensure_mech(mech_id)
            current_mech = mech_id
            current_section = "creases"
            continue

        if current_mech is None:
            continue

        if current_section == "vertices":
            vm = VERTEX_RE.search(line)
            if vm:
                idx = int(vm.group(1))
                x, y, z = (float(vm.group(k)) for k in (2, 3, 4))
                current[current_mech]["vertices"].append((idx, x, y, z))
        elif current_section == "creases":
            cm = CREASE_RE.search(line)
            if cm:
                i, j = int(cm.group(1)), int(cm.group(2))
                ctype = cm.group(3).lower()
                current[current_mech]["creases"].append((i, j, ctype))

    if current:
        snapshots.append(current)

    for snap in snapshots:
        for mech in snap.values():
            mech["vertices"].sort(key=lambda t: t[0])

    return snapshots


def plot_snapshot(ax, snapshot, wireframe=False, show_labels=True, is_2d=False):
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]

    all_points = []
    legend_handles = []
    crease_types_seen = set()

    for plot_idx, mech_id in enumerate(sorted(snapshot.keys())):
        mech = snapshot[mech_id]
        verts = mech["vertices"]
        if not verts:
            continue

        color = colors[plot_idx % len(colors)]
        pos_by_idx = {v[0]: (v[1], v[2], v[3]) for v in verts}
        xs = [v[1] for v in verts]
        ys = [v[2] for v in verts]
        zs = [v[3] for v in verts]
        all_points.extend(zip(xs, ys, zs))

        # cyclic closure: append the first point again at the end
        loop_x, loop_y, loop_z = xs + [xs[0]], ys + [ys[0]], zs + [zs[0]]

        if is_2d:
            ax.plot(loop_x, loop_y, color=color, linewidth=2, marker="o", markersize=4)
            if not wireframe and len(verts) >= 3:
                ax.fill(xs, ys, color=color, alpha=0.25)
            if show_labels:
                for i, x, y, _ in verts:
                    ax.text(x, y, str(i), color=color, fontsize=9)
        else:
            ax.plot(loop_x, loop_y, loop_z, color=color, linewidth=2, marker="o", markersize=4)
            if not wireframe and len(verts) >= 3:
                face = list(zip(xs, ys, zs))
                poly = Poly3DCollection([face], alpha=0.25)
                poly.set_facecolor(color)
                ax.add_collection3d(poly)
            if show_labels:
                for i, x, y, z in verts:
                    ax.text(x, y, z, str(i), color=color, fontsize=9)

        legend_handles.append(Line2D([0], [0], color=color, linewidth=2, marker="o",
                                      markersize=4, label=f"Mechanism {mech_id}"))

        # draw crease lines
        for i, j, ctype in mech["creases"]:
            if i not in pos_by_idx or j not in pos_by_idx:
                continue
            xi, yi, zi = pos_by_idx[i]
            xj, yj, zj = pos_by_idx[j]
            crease_color = CREASE_COLORS.get(ctype, "black")
            crease_types_seen.add(ctype)
            if is_2d:
                ax.plot([xi, xj], [yi, yj], color=crease_color, linewidth=2.5,
                        linestyle="--", zorder=5)
            else:
                ax.plot([xi, xj], [yi, yj], [zi, zj], color=crease_color,
                        linewidth=2.5, linestyle="--", zorder=5)

    for ctype in sorted(crease_types_seen):
        legend_handles.append(Line2D([0], [0], color=CREASE_COLORS.get(ctype, "black"),
                                      linewidth=2.5, linestyle="--",
                                      label=f"{ctype.capitalize()} crease"))

    if is_2d:
        _set_equal_aspect_2d(ax, all_points)
        ax.set_xlabel("X")
        ax.set_ylabel("Y")
    else:
        _set_equal_aspect_3d(ax, all_points)
        ax.set_xlabel("X")
        ax.set_ylabel("Y")
        ax.set_zlabel("Z")

    if legend_handles:
        ax.legend(handles=legend_handles, loc="upper left", fontsize=8)


def _set_equal_aspect_3d(ax, points):
    """matplotlib 3D axes don't auto-scale equally; force a cube-shaped
    view so the card's proportions aren't distorted."""
    if not points:
        return
    xs, ys, zs = zip(*points)
    x_range = max(xs) - min(xs)
    y_range = max(ys) - min(ys)
    z_range = max(zs) - min(zs)
    max_range = max(x_range, y_range, z_range, 1e-6) / 2.0

    x_mid = (max(xs) + min(xs)) / 2.0
    y_mid = (max(ys) + min(ys)) / 2.0
    z_mid = (max(zs) + min(zs)) / 2.0

    ax.set_xlim(x_mid - max_range, x_mid + max_range)
    ax.set_ylim(y_mid - max_range, y_mid + max_range)
    ax.set_zlim(z_mid - max_range, z_mid + max_range)


def _set_equal_aspect_2d(ax, points):
    """Equal-scale 2D axes (z is ignored -- this is the flat pattern)."""
    if points:
        xs, ys, _ = zip(*points)
        x_range = max(xs) - min(xs)
        y_range = max(ys) - min(ys)
        pad = max(x_range, y_range, 1e-6) * 0.1
        x_mid = (max(xs) + min(xs)) / 2.0
        y_mid = (max(ys) + min(ys)) / 2.0
        half = max(x_range, y_range, 1e-6) / 2.0 + pad
        ax.set_xlim(x_mid - half, x_mid + half)
        ax.set_ylim(y_mid - half, y_mid + half)
    ax.set_aspect("equal", adjustable="box")


def make_axes(fig, rows, cols, index, is_2d):
    if is_2d:
        return fig.add_subplot(rows, cols, index)
    return fig.add_subplot(rows, cols, index, projection="3d")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("infile", nargs="?", type=argparse.FileType("r"), default=sys.stdin,
                         help="file with the program's printed output (default: stdin)")
    parser.add_argument("--snapshot", type=int, default=None,
                         help="which snapshot to plot (0-indexed). Default: last one found.")
    parser.add_argument("--all", action="store_true",
                         help="plot every snapshot found, in a grid of subplots.")
    parser.add_argument("--flat-snapshot", type=int, default=0,
                         help="snapshot index treated as the flat pattern and drawn in 2D (default: 0).")
    parser.add_argument("--no-flat-2d", action="store_true",
                         help="disable 2D rendering of the flat snapshot; always use 3D.")
    parser.add_argument("--wireframe", action="store_true",
                         help="draw edges only, skip the translucent face fill.")
    parser.add_argument("--no-labels", action="store_true",
                         help="hide vertex index labels.")
    parser.add_argument("--save", type=str, default=None,
                         help="save the figure to this path.")
    parser.add_argument("--no-show", action="store_true",
                         help="don't open an interactive window.")
    args = parser.parse_args()

    text = args.infile.read()
    snapshots = parse_snapshots(text)

    if not snapshots:
        print("No 'Mechanism <id> vertices:' / 'Vertex <i> is: (x, y, z)' lines found in the input.",
              file=sys.stderr)
        print("Make sure you're piping in the output of printCurrentVertices().", file=sys.stderr)
        sys.exit(1)

    show_labels = not args.no_labels

    def is_flat(idx):
        return (not args.no_flat_2d) and idx == args.flat_snapshot

    if args.all:
        n = len(snapshots)
        cols = min(n, 3)
        rows = (n + cols - 1) // cols
        fig = plt.figure(figsize=(6 * cols, 5 * rows))
        for i, snap in enumerate(snapshots):
            flat = is_flat(i)
            ax = make_axes(fig, rows, cols, i + 1, flat)
            ax.set_title(f"Snapshot {i}" + (" (flat pattern)" if flat else ""))
            plot_snapshot(ax, snap, wireframe=args.wireframe, show_labels=show_labels, is_2d=flat)
    else:
        snap_idx = args.snapshot if args.snapshot is not None else len(snapshots) - 1
        if not (0 <= snap_idx < len(snapshots)):
            print(f"--snapshot {snap_idx} out of range (found {len(snapshots)} snapshot(s), 0..{len(snapshots)-1})",
                  file=sys.stderr)
            sys.exit(1)
        flat = is_flat(snap_idx)
        fig = plt.figure(figsize=(8, 7))
        ax = make_axes(fig, 1, 1, 1, flat)
        ax.set_title(f"Snapshot {snap_idx} of {len(snapshots)}" + (" (flat pattern)" if flat else ""))
        plot_snapshot(ax, snapshots[snap_idx], wireframe=args.wireframe, show_labels=show_labels, is_2d=flat)

    fig.tight_layout()

    if args.save:
        fig.savefig(args.save, dpi=150)
        print(f"Saved figure to {args.save}")

    if not args.no_show:
        plt.show()


if __name__ == "__main__":
    main()
