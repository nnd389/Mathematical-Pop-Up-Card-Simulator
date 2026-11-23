#include "FoldLoader.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <iostream>

using json = nlohmann::json;

namespace {

int findRoot(std::vector<int>& parent, int x) {
    while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
    return x;
}

bool isFold(const std::string& a) { return a == "M" || a == "PM" || a == "V" || a == "PV"; }
bool isMountain(const std::string& a) { return a == "M" || a == "PM"; }

} // namespace

std::vector<Mechanism> loadCard(const std::string& path, float scale) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("loadCard: cannot open " + path);
    json j = json::parse(in);

    // ---- vertices: file (x, y, z) -> card (x, -z, 0) ----
    std::vector<Eigen::Vector3f> pos;
    for (const auto& v : j.at("vertices_coords")) {
        // 3D files are (x, y, z) with the flat pattern in the x-z plane; 2D files are (x, y) in the pattern plane
        // (makeTowerCard.py writes these). Either way the second pattern coordinate is negated into card y.
        if (!v.is_array() || v.size() < 2) throw std::runtime_error("loadCard: vertex needs at least 2 coordinates");
        float x = v[0].get<float>(), z = v[v.size() >= 3 ? 2 : 1].get<float>();
        pos.emplace_back(x * scale, -z * scale, 0.0f);
    }
    const int V = static_cast<int>(pos.size());

    std::vector<std::pair<int,int>> edges;
    for (const auto& e : j.at("edges_vertices")) edges.emplace_back(e[0].get<int>(), e[1].get<int>());
    std::vector<std::string> assign = j.at("edges_assignment").get<std::vector<std::string>>();
    if (assign.size() != edges.size())
        throw std::runtime_error("loadCard: edges_assignment and edges_vertices differ in length");

    // ---- components (ignoring GS edges) ----
    std::vector<int> parent(V);
    std::iota(parent.begin(), parent.end(), 0);
    for (size_t e = 0; e < edges.size(); e++) {
        if (assign[e] == "GS") continue;
        parent[findRoot(parent, edges[e].first)] = findRoot(parent, edges[e].second);
    }

    // ---- glue springs, deduplicated (the file lists each in both directions) ----
    std::set<std::pair<int,int>> glueSet;
    for (size_t e = 0; e < edges.size(); e++) {
        if (assign[e] != "GS") continue;
        glueSet.insert({std::min(edges[e].first, edges[e].second), std::max(edges[e].first, edges[e].second)});
    }

    // ---- order mechanisms: base = component of vertex 0, then BFS across glue springs ----
    std::vector<int> compOfRoot(V, -1);
    std::vector<int> compOrder; // roots in mechanism order
    std::map<int, std::set<int>> compAdj;
    for (auto& g : glueSet) {
        int a = findRoot(parent, g.first), b = findRoot(parent, g.second);
        compAdj[a].insert(b); compAdj[b].insert(a);
    }
    {
        std::queue<int> q;
        int baseRoot = findRoot(parent, 0);
        compOfRoot[baseRoot] = 0; compOrder.push_back(baseRoot); q.push(baseRoot);
        while (!q.empty()) {
            int r = q.front(); q.pop();
            for (int nb : compAdj[r]) {
                if (compOfRoot[nb] != -1) continue;
                compOfRoot[nb] = static_cast<int>(compOrder.size());
                compOrder.push_back(nb); q.push(nb);
            }
        }
    }
    const int M = static_cast<int>(compOrder.size());

    // ---- outlines: walk the cyclic loop of B edges of each mechanism ----
    // flatPos is the OUTLINE only. Local index l is the l-th vertex met walking the loop, starting at the loop's
    // lowest-numbered vertex and heading toward whichever neighbour has the lower index (so a loop listed
    // 0,1,2,... in the file keeps those indices). Interior vertices (e.g. glue targets on the base card) get
    // no local index: their positions are only used as GlueDot::gluePosition.
    std::vector<std::vector<int>> bNbr(V);
    std::vector<int> foldDeg(V, 0);
    std::set<int> glueEnds;
    for (size_t e = 0; e < edges.size(); e++) {
        int a = edges[e].first, b = edges[e].second;
        if (assign[e] == "B") { bNbr[a].push_back(b); bNbr[b].push_back(a); }
        if (isFold(assign[e])) { foldDeg[a]++; foldDeg[b]++; }
        if (assign[e] == "GS") { glueEnds.insert(a); glueEnds.insert(b); }
    }

    // A glue target lying in the middle of a straight outline edge (e.g. the top of a tower piece that a child
    // attaches to) is on the loop but is not a corner of the mechanism: walk past it. Crease endpoints are kept
    // even when collinear (the crease meets the outline mid-edge).
    std::vector<bool> skip(V, false);
    for (int v = 0; v < V; v++) {
        if (bNbr[v].size() != 2 || foldDeg[v] == 1 || !glueEnds.count(v)) continue;
        Eigen::Vector3f d1 = pos[bNbr[v][0]] - pos[v], d2 = pos[bNbr[v][1]] - pos[v];
        float cross = d1.x() * d2.y() - d1.y() * d2.x();
        float len = d1.norm() * d2.norm();
        if (len > 0 && d1.dot(d2) < 0 && std::abs(cross) / len < 0.25f) skip[v] = true; // within ~14 degrees of straight
    }

    std::vector<int> mechOf(V, -1), localOf(V, -1); // localOf stays -1 for interior vertices
    for (int v = 0; v < V; v++) mechOf[v] = compOfRoot[findRoot(parent, v)];
    // (mechOf is the mechanism number, or -1 for a component with no glue connection to the base card)

    std::vector<std::vector<int>> globalVerts(M);   // local index -> global index
    std::vector<int> outlineCount(M, 0);
    // next non-skipped vertex when leaving `from` toward `first` along the loop
    auto walkPast = [&](int from, int first) {
        int prev = from, cur = first;
        while (skip[cur]) {
            int next = bNbr[cur][0] == prev ? bNbr[cur][1] : bNbr[cur][0];
            prev = cur; cur = next;
        }
        return std::make_pair(prev, cur); // (vertex before cur, cur)
    };
    for (int m = 0; m < M; m++) {
        int start = -1;
        for (int v = 0; v < V; v++)
            if (mechOf[v] == m && !bNbr[v].empty() && !skip[v]) { start = v; break; }
        if (start < 0) throw std::runtime_error("loadCard: mechanism " + std::to_string(m) + " has no outline (no B edges)");
        for (int v = 0; v < V; v++)
            if (mechOf[v] == m && !bNbr[v].empty() && bNbr[v].size() != 2)
                throw std::runtime_error("loadCard: B edges of mechanism " + std::to_string(m) + " do not form simple loops (vertex " + std::to_string(v) + ")");

        auto d0 = walkPast(start, bNbr[start][0]), d1 = walkPast(start, bNbr[start][1]);
        auto dir = d0.second <= d1.second ? d0 : d1; // head toward the lower-numbered neighbour
        std::vector<int>& loop = globalVerts[m];
        loop.push_back(start);
        int prev = dir.first, cur = dir.second;
        while (cur != start) {
            if (loop.size() > static_cast<size_t>(V)) throw std::runtime_error("loadCard: outline walk did not close");
            loop.push_back(cur);
            // step to the next vertex along the loop, away from `prev`
            int raw = bNbr[cur][0] == prev ? bNbr[cur][1] : bNbr[cur][0];
            auto nx = walkPast(cur, raw);
            prev = nx.first; cur = nx.second;
        }
        // Make every outline run the same way round (clockwise in card coordinates, like the hand-built cards):
        // the file may number a mechanism the opposite way. Keep the start vertex at local index 0.
        double area2 = 0; // twice the signed area (shoelace); > 0 means counterclockwise
        for (size_t l = 0; l < loop.size(); l++) {
            const auto& p = pos[loop[l]]; const auto& q = pos[loop[(l + 1) % loop.size()]];
            area2 += double(p.x()) * q.y() - double(q.x()) * p.y();
        }
        if (area2 > 0) std::reverse(loop.begin() + 1, loop.end());
        for (size_t l = 0; l < loop.size(); l++) localOf[loop[l]] = static_cast<int>(l);
        outlineCount[m] = static_cast<int>(loop.size());

        for (int v = 0; v < V; v++)
            if (mechOf[v] == m && !bNbr[v].empty() && !skip[v] && localOf[v] < 0)
                std::cerr << "loadCard: warning: mechanism " << m << " has more than one B-edge loop (vertex " << v
                          << " is on a different loop); only the first loop is used as the outline\n";
    }

    // ---- creases: merge chains of fold edges into one crease per chain ----
    std::vector<std::vector<Crease>> creases(M);
    {
        std::map<int, std::vector<int>> adj; // global vertex -> fold neighbours
        std::map<int, bool> mountainAt;       // per vertex, type of an edge touching it
        for (size_t e = 0; e < edges.size(); e++) {
            if (!isFold(assign[e])) continue;
            adj[edges[e].first].push_back(edges[e].second);
            adj[edges[e].second].push_back(edges[e].first);
            mountainAt[edges[e].first] = mountainAt[edges[e].second] = isMountain(assign[e]);
        }
        std::set<int> seen;
        for (auto& kv : adj) {
            int start = kv.first;
            if (seen.count(start) || kv.second.size() != 1) continue; // start only at chain endpoints
            int prev = -1, cur = start;
            seen.insert(cur);
            while (true) {
                int next = -1;
                for (int nb : adj[cur]) if (nb != prev) { next = nb; break; }
                if (next == -1) break;
                prev = cur; cur = next; seen.insert(cur);
            }
            int a = start, b = cur;
            if (pos[a].y() > pos[b].y()) std::swap(a, b); // i = lower end (the bottom), like Crease{0,3}
            int m = mechOf[a];
            if (localOf[a] < 0 || localOf[b] < 0)
                throw std::runtime_error("loadCard: a crease endpoint is not on the mechanism outline");
            if (m != mechOf[b]) throw std::runtime_error("loadCard: a crease spans two mechanisms");
            creases[m].push_back(Crease{localOf[a], localOf[b],
                                        mountainAt[start] ? CreaseType::Mountain : CreaseType::Valley});
        }
        for (int m = 0; m < M; m++)
            if (creases[m].empty())
                throw std::runtime_error("loadCard: mechanism " + std::to_string(m) + " has no crease");
    }

    // ---- glue dots: on the higher-numbered mechanism, pointing at the lower one ----
    std::vector<std::vector<GlueDot>> glues(M);
    // If a child vertex is glued to several vertices that are themselves glued to each other (a triple point,
    // e.g. 30-15, 30-21 and 15-21), they all describe one attachment: keep only the nearest parent (highest mechanism
    // index below the child) so the child gets a single glue dot there.
    std::set<std::pair<int,int>> dropGlue;
    {
        std::map<int, std::vector<int>> nbrs; // vertex -> glue partners
        for (auto& g : glueSet) { nbrs[g.first].push_back(g.second); nbrs[g.second].push_back(g.first); }
        for (auto& kv : nbrs) {
            int a = kv.first;
            std::vector<int> parents;
            for (int b : kv.second) if (mechOf[b] >= 0 && mechOf[b] < mechOf[a]) parents.push_back(b);
            for (int b1 : parents) for (int b2 : parents) {
                if (b1 >= b2 || mechOf[b1] == mechOf[b2]) continue;
                if (!glueSet.count({std::min(b1, b2), std::max(b1, b2)})) continue; // parents not glued together
                int drop = mechOf[b1] < mechOf[b2] ? b1 : b2;
                dropGlue.insert({std::min(a, drop), std::max(a, drop)});
            }
        }
    }
    for (auto& g : glueSet) {
        if (dropGlue.count(g)) continue;
        int a = g.first, b = g.second;
        if (mechOf[a] == mechOf[b]) continue;
        if (mechOf[a] < mechOf[b]) std::swap(a, b); // a = child, b = parent
        if (localOf[a] < 0)
            throw std::runtime_error("loadCard: a glue spring attaches to an interior vertex of its child mechanism");
        glues[mechOf[a]].push_back(GlueDot{localOf[a], mechOf[b], pos[b], Eigen::Vector3f::Zero()});
    }
    for (int m = 0; m < M; m++) {
        const auto& gv = globalVerts[m];
        std::stable_sort(glues[m].begin(), glues[m].end(), [&](const GlueDot& p, const GlueDot& q) {
            if (p.n != q.n) return p.n < q.n;
            return pos[gv[p.i]].x() < pos[gv[q.i]].x(); // left to right, as findGlueTabs assumes
        });
    }

    // ---- build ----
    std::vector<Mechanism> out;
    for (int m = 0; m < M; m++) {
        std::vector<Eigen::Vector3f> flat;
        for (int g : globalVerts[m]) flat.push_back(pos[g]);
        out.emplace_back(flat, creases[m], glues[m], m == 0, m, outlineCount[m]);
    }
    return out;
}
