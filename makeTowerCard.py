"""Generate an SVG cut/fold pattern for a multi-level "tower" pop-up card.

Python port of TowerCard.m, generalised to any number of levels.

    from makeTowerCard import makeTowerCard
    makeTowerCard(levels=4, theta=80, scale=100)      # -> popup_card.svg
"""
import json
import math

import numpy as np

# Colours (match the MATLAB plot: k, b, r, y, c)
BOUNDARY = "black"
# FOLD assignment -> SVG colour for creases
CREASE_COLORS = {
    "V": "blue",                       # base card
    "PM": "rgb(255,0,155)",            # A/B on odd levels (pre-creased mountain, pink)
    "PV": "rgb(155,0,255)",            # C mechanisms; A/B on even levels (pre-creased valley, purple)
}
FACET = "yellow"
GLUE = "cyan"


def makeTowerCard(levels, theta, scale, filename="popup_card.svg",
                  fold_filename="popUpCard.fold", stroke_width=1.5, margin=0.5):
    """Write an SVG and a FOLD-format file of a tower card pattern.

    Returns (svg_filename, fold_filename).

    levels : int >= 1, number of mechanism levels
    theta  : float in degrees, 0 < theta < 90
    scale  : float > 0, multiplier applied to all coordinates
    """
    if int(levels) != levels or levels < 1:
        raise ValueError("levels must be an integer >= 1")
    if not 0 < theta < 90:
        raise ValueError("theta must satisfy 0 < theta < 90 (degrees)")
    if not scale > 0:
        raise ValueError("scale must be positive")
    n = int(levels)

    # ---- parameters -------------------------------------------------------
    d = 1.0
    t2, t3 = 2 * d, 3 * d
    alpha = math.pi / 2 - math.radians(theta)
    x0, y0 = d * math.cos(alpha), d * math.sin(alpha)
    w = math.sin(alpha) + math.cos(alpha)
    s = math.cos(alpha) / w

    def P(*rows):
        return np.array(rows, dtype=float)

    def shift(pts, dx, dy):
        return pts + np.array([dx, dy])

    flip_y = np.array([[1.0, 0.0], [0.0, -1.0]])

    # ---- base card --------------------------------------------------------
    BC = shift(P([0, 0], [-w, 0], [-w, 2 * w], [0, 2 * w], [w, 2 * w], [w, 0]), -t3, 0)
    BC_crease = P(BC[0], BC[3])
    glue_tri = P([-x0, y0], [0, 0], [x0, y0])
    g1 = shift(glue_tri, -t3, x0)        # lower
    g2 = shift(glue_tri, -t3, x0 + d)    # upper
    BC_left = P(BC[3], g2[0], g2[1], g1[0], g1[1], BC[1], g1[0], g2[0], BC[2])
    BC_right = P(BC[3], g2[2], g2[1], g1[2], g1[1], BC[5], g1[2], g2[2], BC[4])

    # ---- mechanisms (A, B, C per level; index 0 = level 1) ----------------
    A0 = P([0, 0], [-d, 0], [-d, d], [0, d], [d, d], [d, 0])
    A = [A0]
    B = [shift(A0, 0, t2)]
    C = [shift(P([0, 0], [-x0, y0], [-x0, y0 + d], [0, d], [x0, y0 + d], [x0, y0]), 0, -t2)]
    for i in range(1, n):
        A.append(shift(s * A[i - 1], t3, 0))
        B.append(shift(A[i], 0, t2))
        C.append(shift(s * C[i - 1] @ flip_y, t3, -t2))

    C_crease = [P(c[0], c[3]) for c in C]       # taken from the unmodified C
    C_draw = list(C)                            # C polygons as drawn
    facets1 = {}                                # level index -> glue tab pieces
    facets2 = {}
    for i in range(1, n):
        prev, cur = C[i - 1], C[i]
        Cm = cur - (cur[0] - prev[3])
        facets1[i] = P(prev[2], Cm[1], prev[3], Cm[5], prev[4])
        facets2[i] = P(Cm[2], Cm[3], Cm[4])
        C_draw[i - 1] = P(prev[0], facets2[i][0], prev[1], prev[2], prev[3],
                          prev[4], prev[5], facets2[i][2])

    # ---- springs: list of (point, point) ----------------------------------
    springs = []

    def pair(p, q):
        springs.append((p, q))

    # base card <-> level 1
    pair(BC_left[1], B[0][1]); pair(BC_left[2], B[0][0]); pair(BC_right[1], B[0][5])
    pair(BC_left[3], A[0][1]); pair(BC_left[4], A[0][0]); pair(BC_right[3], A[0][5])

    for i in range(n):
        L = i + 1
        # glue tabs of level L's C onto level L-1's C, sprung to level L's A/B
        if i >= 1:
            f1_to, f2_to = (B[i], A[i]) if L % 2 == 0 else (A[i], B[i])
            for k, j in enumerate((1, 0, 5)):
                pair(facets1[i][k + 1], f1_to[j])
            for k, j in enumerate((1, 0, 5)):
                pair(facets2[i][k], f2_to[j])
        # vertical springs between C_L and A_L/B_L, present on every level.
        # Indexed into the unmodified C so the top level (no extra tab
        # vertices) works too; these are the same points as MATLAB's
        # C(4,5,6) / C(3,1,7) on the modified 8-vertex C.
        c = C[i]
        first_to, second_to = (B[i], A[i]) if L % 2 == 1 else (A[i], B[i])
        for k, ci in enumerate((2, 3, 4)):
            pair(c[ci], first_to[k + 2])
        for k, ci in enumerate((1, 0, 5)):
            pair(c[ci], second_to[k + 2])

    # ---- gather drawing primitives (math coords, y up) --------------------
    polygons = [BC]
    creases = [(BC_crease, "V")]
    for i in range(n):
        polygons += [A[i], B[i], C_draw[i]]
        ab_kind = "PM" if (i + 1) % 2 == 1 else "PV"   # odd levels PM, even PV
        creases += [(P(A[i][0], A[i][3]), ab_kind),
                    (P(B[i][0], B[i][3]), ab_kind),
                    (C_crease[i], "PV")]
    tabs = [BC_left, BC_right]
    for i in range(1, n):
        tabs += [facets1[i], facets2[i]]
    # extra facets (MATLAB A*_facets, B*_facets, C*_facets3, C<last>_facets)
    for i in range(n):
        tabs += [A[i][[1, 3, 5]], B[i][[1, 3, 5]]]
    for i in range(1, n):
        prev = C[i - 1]
        tabs.append(P(prev[1], facets1[i][1], facets2[i][0], prev[3],
                      facets2[i][2], facets1[i][3], prev[5]))
    tabs.append(C[n - 1][[1, 3, 5]])

    # ---- write SVG ---------------------------------------------------------
    def tx(pts):
        pts = np.asarray(pts) * scale
        return [(x, -y) for x, y in pts]          # flip y for SVG

    allpts = np.vstack(polygons + tabs + [np.array(sp) for sp in springs])
    pad = margin * scale
    xs, ys = allpts[:, 0] * scale, -allpts[:, 1] * scale
    minx, miny = xs.min() - pad, ys.min() - pad
    width, height = xs.max() - xs.min() + 2 * pad, ys.max() - ys.min() + 2 * pad

    def fmt(pts):
        return " ".join(f"{x:.3f},{y:.3f}" for x, y in tx(pts))

    out = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{minx:.3f} {miny:.3f} '
        f'{width:.3f} {height:.3f}" width="{width:.3f}" height="{height:.3f}">',
        f'<g fill="none" stroke-width="{stroke_width}" stroke-linejoin="round">',
    ]
    for poly in polygons:
        out.append(f'<polygon points="{fmt(poly)}" stroke="{BOUNDARY}"/>')
    for seg, kind in creases:
        out.append(f'<polyline points="{fmt(seg)}" stroke="{CREASE_COLORS[kind]}"/>')
    for tab in tabs:
        out.append(f'<polyline points="{fmt(tab)}" stroke="{FACET}"/>')
    for p, q in springs:
        out.append(f'<polyline points="{fmt(P(p, q))}" stroke="{GLUE}" '
                   f'stroke-width="{stroke_width * 4 / 3:.3f}"/>')
    out += ["</g>", "</svg>"]

    with open(filename, "w") as f:
        f.write("\n".join(out))

    # ---- write FOLD file ---------------------------------------------------
    TOL = 1e-8
    verts = []          # unscaled [x, y]; multiplied by scale on output

    def vid(pt):
        pt = np.asarray(pt, dtype=float)
        if verts:
            dist = np.linalg.norm(np.array(verts) - pt, axis=1)
            k = int(dist.argmin())
            if dist[k] < TOL:
                return k
        verts.append(pt)
        return len(verts) - 1

    raw = []            # (i, j, assignment)
    for poly in polygons:                              # closed outlines
        for k in range(len(poly)):
            raw.append((vid(poly[k]), vid(poly[(k + 1) % len(poly)]), "B"))
    for seg, kind in creases:
        raw.append((vid(seg[0]), vid(seg[1]), kind))
    for tab in tabs:                                   # open polylines
        for k in range(len(tab) - 1):
            raw.append((vid(tab[k]), vid(tab[k + 1]), "F"))
    for p, q in springs:
        raw.append((vid(p), vid(q), "GS"))
    V = np.array(verts)

    def split(i, j):
        """Vertex ids along segment i-j, including any vertex lying on it."""
        d = V[j] - V[i]
        L2 = d @ d
        t = (V - V[i]) @ d / L2
        off = np.abs((V - V[i]) @ np.array([-d[1], d[0]])) / np.sqrt(L2)
        on = [k for k in range(len(V)) if k not in (i, j)
              and off[k] < TOL and 1e-12 < t[k] < 1 - 1e-12]
        on.sort(key=lambda k: t[k])
        return [i] + on + [j]

    edges, assign, seen = [], [], set()
    for i, j, a in raw:
        chain = [i, j] if a == "GS" else split(i, j)  # springs are not split
        for u, v in zip(chain, chain[1:]):
            if u != v and (min(u, v), max(u, v)) not in seen:
                seen.add((min(u, v), max(u, v)))
                edges.append([u, v])
                assign.append(a)
                if a == "GS":                           # springs go both ways
                    edges.append([v, u])
                    assign.append(a)

    # faces: walk the planar graph of every non-spring edge
    nbrs = {}
    for (u, v), a in zip(edges, assign):
        if a != "GS":
            nbrs.setdefault(u, []).append(v)
            nbrs.setdefault(v, []).append(u)
    for u in nbrs:                                     # sort CCW by angle
        nbrs[u].sort(key=lambda v: math.atan2(V[v][1] - V[u][1], V[v][0] - V[u][0]))
    used, faces = set(), []
    for u in nbrs:
        for v in nbrs[u]:
            if (u, v) in used:
                continue
            face, a, b = [], u, v
            while (a, b) not in used:
                used.add((a, b))
                face.append(a)
                ring = nbrs[b]                          # next edge: clockwise of a
                a, b = b, ring[(ring.index(a) - 1) % len(ring)]
            area = 0.5 * sum(V[face[k]][0] * V[face[(k + 1) % len(face)]][1]
                             - V[face[(k + 1) % len(face)]][0] * V[face[k]][1]
                             for k in range(len(face)))
            if area > 1e-12:                           # drop the outer faces
                faces.append(face)

    # Any face that is not yet a triangle is ear-clipped; each new diagonal is
    # added as a facet ("F") edge so faces_vertices stays consistent with edges.
    def cross(o, a, b):
        return (V[a][0] - V[o][0]) * (V[b][1] - V[o][1]) - (V[a][1] - V[o][1]) * (V[b][0] - V[o][0])

    def triangulate(face):
        face, tris = list(face), []
        while len(face) > 3:
            m = len(face)
            for k in range(m):
                a, b, c = face[k - 1], face[k], face[(k + 1) % m]
                if cross(a, b, c) <= TOL:               # reflex/collinear: not an ear
                    continue
                if any(cross(a, b, q) > 0 and cross(b, c, q) > 0 and cross(c, a, q) > 0
                       for q in face if q not in (a, b, c)):
                    continue
                tris.append([a, b, c])
                key = (min(a, c), max(a, c))
                if key not in seen:
                    seen.add(key)
                    edges.append([a, c])
                    assign.append("F")
                del face[k]
                break
            else:
                raise RuntimeError("could not triangulate a face")
        return tris + [face]

    faces = [t for f in faces for t in (triangulate(f) if len(f) > 3 else [f])]

    # flip y so the FOLD file looks like the SVG; reverse each face to stay CCW
    verts = (V * scale * np.array([1, -1])).tolist()
    faces = [f[::-1] for f in faces]

    fold = {
        "file_spec": 1.1,
        "file_creator": "makeTowerCard.py",
        "file_author": "Nina De La Torre",
        "file_classes": ["gluePattern"],
        "frame_title": f"TowerLevel{n}Theta{theta:g}Scale{scale:g}",
        "frame_classes": ["FoldedForm"],
        "frame_attributes": ["2D"],
        "frame_unit": "Unit",
        "vertices_coords": verts,
        "edges_vertices": edges,
        "edges_assignment": assign,
        "faces_vertices": faces,
    }
    with open(fold_filename, "w") as f:
        json.dump(fold, f, indent=4)
    return filename, fold_filename


if __name__ == "__main__":
    print(makeTowerCard(4, 80, 100))
