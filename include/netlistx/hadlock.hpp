#pragma once

/**
 * @file hadlock.hpp
 * Hadlock's algorithm: planar MAX-CUT → minimum weight perfect matching on dual.
 *
 * Reference:
 *   Hadlock, F. O. (1975). "Finding a Maximum Cut of a Planar Graph in
 *   Polynomial Time." SIAM Journal on Computing, 4(3), 221-225.
 */

#include <algorithm>
#include <cassert>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>

// Hash for std::pair — needed by py::set<std::pair<...>> (backed by unordered_set)
namespace std {
template <typename T1, typename T2>
struct hash<pair<T1, T2>> {
    size_t operator()(const pair<T1, T2>& p) const {
        return hash<T1>{}(p.first) ^ (hash<T2>{}(p.second) << 1);
    }
};
} // namespace std

namespace netlistx::detail {

constexpr int INF = std::numeric_limits<int>::max() / 2;

template <typename Node>
struct DualEdge {
    int neighbor;
    int weight;
    std::pair<Node, Node> primal;
};

// -------------------------------------------------------------------
// Dual graph construction
//
// Each face → dual vertex. Two dual vertices connect if faces share a
// primal edge. Parallel edges → only the minimum-weight edge is kept.
// -------------------------------------------------------------------
template <typename Node, typename WeightFunc>
auto build_dual(
    const std::vector<std::vector<Node>>& faces, WeightFunc weight
) -> std::vector<std::vector<DualEdge<Node>>> {
    const auto n_face = static_cast<int>(faces.size());

    std::map<std::pair<Node, Node>, std::vector<int>> edge_face_map;
    for (int fi = 0; fi < n_face; ++fi) {
        const auto& f = faces[fi];
        const auto sz = static_cast<int>(f.size());
        for (int i = 0; i < sz; ++i) {
            auto u = f[i];
            auto v = f[(i + 1) % sz];
            if (u > v) std::swap(u, v);
            edge_face_map[{u, v}].push_back(fi);
        }
    }

    std::map<std::pair<int, int>, std::pair<int, std::pair<Node, Node>>> best;
    for (const auto& [primal_key, face_ids] : edge_face_map) {
        if (face_ids.size() < 2) continue;
        const auto& [u, v] = primal_key;
        const int w = weight(u, v);
        for (size_t a = 0; a < face_ids.size(); ++a) {
            for (size_t b = a + 1; b < face_ids.size(); ++b) {
                int fi = face_ids[a];
                int fj = face_ids[b];
                if (fi > fj) std::swap(fi, fj);
                auto it = best.find({fi, fj});
                if (it == best.end() || w < it->second.first) {
                    best[{fi, fj}] = {w, {u, v}};
                }
            }
        }
    }

    std::vector<std::vector<DualEdge<Node>>> dual(n_face);
    for (const auto& [key, info] : best) {
        const auto& [fi, fj] = key;
        const auto& [w, primal] = info;
        dual[fi].push_back({fj, w, primal});
        dual[fj].push_back({fi, w, primal});
    }
    return dual;
}

// -------------------------------------------------------------------
// Dijkstra shortest path
// -------------------------------------------------------------------
template <typename Node>
auto dijkstra(
    const std::vector<std::vector<DualEdge<Node>>>& dual, int src
) -> std::pair<std::vector<int>, std::vector<int>> {
    const int n = static_cast<int>(dual.size());
    std::vector<int> dist(n, INF);
    std::vector<int> prev(n, -1);
    dist[src] = 0;

    using P = std::pair<int, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d != dist[u]) continue;
        for (const auto& e : dual[u]) {
            if (dist[e.neighbor] > d + e.weight) {
                dist[e.neighbor] = d + e.weight;
                prev[e.neighbor] = u;
                pq.push({dist[e.neighbor], e.neighbor});
            }
        }
    }
    return {dist, prev};
}

inline auto reconstruct_path(
    const std::vector<int>& prev, int src, int dst
) -> std::vector<int> {
    std::vector<int> path;
    for (int v = dst; v != -1 && v != src; v = prev[v]) {
        path.push_back(v);
    }
    if (path.empty() && src != dst) return {};
    path.push_back(src);
    std::reverse(path.begin(), path.end());
    return path;
}

// -------------------------------------------------------------------
// Minimum Weight Perfect Matching
// -------------------------------------------------------------------

/**
 * Greedy 2-approximation for MWPM on a metric complete graph.
 */
template <typename Node>
auto greedy_mwpm(
    const std::vector<int>& odd_faces,
    const std::vector<std::vector<int>>& dist
) -> std::vector<std::pair<int, int>> {
    const int n = static_cast<int>(odd_faces.size());
    std::vector<std::tuple<int, int, int>> edges;
    edges.reserve(n * (n - 1) / 2);
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            edges.emplace_back(dist[odd_faces[i]][odd_faces[j]],
                               odd_faces[i], odd_faces[j]);
        }
    }
    std::sort(edges.begin(), edges.end());

    std::unordered_set<int> used;
    std::vector<std::pair<int, int>> matching;
    for (const auto& [w, u, v] : edges) {
        if (used.contains(u) || used.contains(v)) continue;
        used.insert(u);
        used.insert(v);
        matching.emplace_back(u, v);
        if (used.size() == static_cast<size_t>(n)) break;
    }
    return matching;
}

/**
 * Exact MWPM via DP over subsets (n ≤ 18, 2^18 = 262k states).
 */
template <typename Node>
auto exact_mwpm(
    const std::vector<int>& odd_faces,
    const std::vector<std::vector<int>>& dist
) -> std::vector<std::pair<int, int>> {
    const int n = static_cast<int>(odd_faces.size());
    const int size = 1 << n;
    std::vector<int> dp(size, INF);
    dp[0] = 0;

    for (int mask = 0; mask < size; ++mask) {
        if (dp[mask] == INF) continue;
        int first = -1;
        for (int i = 0; i < n; ++i) {
            if (!(mask & (1 << i))) { first = i; break; }
        }
        if (first == -1) continue;

        for (int j = first + 1; j < n; ++j) {
            if (mask & (1 << j)) continue;
            const int new_mask = mask | (1 << first) | (1 << j);
            const int w = dist[odd_faces[first]][odd_faces[j]];
            if (dp[mask] + w < dp[new_mask]) {
                dp[new_mask] = dp[mask] + w;
            }
        }
    }

    // backtrack: reconstruct matched pairs from dp table
    std::vector<std::pair<int, int>> matching;
    int mask = size - 1;
    while (mask) {
        int first = -1;
        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) { first = i; break; }
        }
        for (int j = first + 1; j < n; ++j) {
            if (!(mask & (1 << j))) continue;
            const int prev_mask = mask ^ (1 << first) ^ (1 << j);
            const int w = dist[odd_faces[first]][odd_faces[j]];
            if (dp[mask] == dp[prev_mask] + w) {
                matching.emplace_back(odd_faces[first], odd_faces[j]);
                mask = prev_mask;
                break;
            }
        }
    }
    return matching;
}

/**
 * Dispatch MWPM: exact DP for small n, greedy for large n.
 */
template <typename Node>
auto min_weight_perfect_matching(
    const std::vector<int>& odd_faces,
    const std::vector<std::vector<int>>& dist
) -> std::vector<std::pair<int, int>> {
    const int n = static_cast<int>(odd_faces.size());
    if (n <= 18) {
        return exact_mwpm<Node>(odd_faces, dist);
    }
    return greedy_mwpm<Node>(odd_faces, dist);
}

} // namespace netlistx::detail

// ===================================================================
// Public API
// ===================================================================

/**
 * Solve MAX-CUT for a planar graph using Hadlock's algorithm.
 *
 * @param G       planar graph (must provide `edges()`, node type `node_t`)
 * @param weight  callable `weight(u, v) -> int`
 * @param faces   list of faces, each a cyclic node sequence
 * @return        set of edges belonging to the maximum cut
 */
template <typename Graph, typename WeightFunc>
auto solve_hadlock_max_cut(
    const Graph& G,
    WeightFunc weight,
    const std::vector<std::vector<typename Graph::node_t>>& faces
) -> py::set<std::pair<typename Graph::node_t, typename Graph::node_t>> {
    using node_t = typename Graph::node_t;

    // (1) Build the dual graph
    const auto dual = netlistx::detail::build_dual<node_t>(faces, weight);

    // (2) Identify odd-degree faces
    std::vector<int> odd_faces;
    for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
        if (faces[i].size() % 2 == 1) {
            odd_faces.push_back(i);
        }
    }

    // No odd faces → graph is bipartite → all edges are in the cut
    if (odd_faces.size() < 2) {
        py::set<std::pair<node_t, node_t>> all_edges;
        for (const auto& e : G.edges()) {
            all_edges.insert(e);
        }
        return all_edges;
    }

    // Odd number of odd faces → drop one (planar graphs have even #odd faces)
    if (odd_faces.size() % 2 != 0) {
        odd_faces.pop_back();
        if (odd_faces.size() < 2) {
            py::set<std::pair<node_t, node_t>> all_edges;
            for (const auto& e : G.edges()) {
                all_edges.insert(e);
            }
            return all_edges;
        }
    }

    const int n_odd = static_cast<int>(odd_faces.size());

    // odd_faces[k] → k lookup
    std::map<int, int> odd_idx;
    for (int k = 0; k < n_odd; ++k) {
        odd_idx[odd_faces[k]] = k;
    }

    // (3) All-pairs shortest paths between odd faces
    std::vector<std::vector<int>> dist_mat(n_odd);
    std::vector<std::vector<std::vector<int>>> path_mat(
        n_odd, std::vector<std::vector<int>>(n_odd)
    );

    for (int i = 0; i < n_odd; ++i) {
        auto [dist, prev] = netlistx::detail::dijkstra<node_t>(dual, odd_faces[i]);
        dist_mat[i] = std::move(dist);
        for (int j = 0; j < n_odd; ++j) {
            if (i != j) {
                path_mat[i][j] = netlistx::detail::reconstruct_path(
                    prev, odd_faces[i], odd_faces[j]
                );
            }
        }
    }

    // (4) Minimum weight perfect matching on odd-face distances
    auto matching = netlistx::detail::min_weight_perfect_matching<node_t>(
        odd_faces, dist_mat
    );

    // (5) Collect primal edges excluded from the cut
    // Build dual-edge → primal-edge lookup
    std::map<std::pair<int, int>, std::pair<node_t, node_t>> dedge_primal;
    for (int fi = 0; fi < static_cast<int>(dual.size()); ++fi) {
        for (const auto& e : dual[fi]) {
            int a = fi;
            int b = e.neighbor;
            if (a > b) std::swap(a, b);
            dedge_primal[{a, b}] = e.primal;
        }
    }

    // Walk each matched-pair path in the dual to find primal edges to exclude
    std::set<std::pair<node_t, node_t>> excluded;
    for (const auto& [u_face, v_face] : matching) {
        auto ui = odd_idx.find(u_face);
        auto vi = odd_idx.find(v_face);
        if (ui == odd_idx.end() || vi == odd_idx.end()) continue;

        const auto& path = path_mat[ui->second][vi->second];
        for (size_t k = 0; k + 1 < path.size(); ++k) {
            int a = path[k];
            int b = path[k + 1];
            if (a > b) std::swap(a, b);
            auto it = dedge_primal.find({a, b});
            if (it != dedge_primal.end()) {
                auto [pu, pv] = it->second;
                if (pu > pv) std::swap(pu, pv);
                excluded.insert({pu, pv});
            }
        }
    }

    // (6) Max-cut = all edges not excluded
    py::set<std::pair<node_t, node_t>> cut_edges;
    for (const auto& e : G.edges()) {
        auto [u, v] = e;
        if (u > v) std::swap(u, v);
        if (!excluded.contains({u, v})) {
            cut_edges.insert(e);
        }
    }
    return cut_edges;
}

template <typename Graph, typename WeightFunc>
auto validate_max_cut(
    [[maybe_unused]] const Graph& G,
    const py::set<std::pair<typename Graph::node_t, typename Graph::node_t>>& cut_edges,
    WeightFunc weight
) -> std::pair<bool, int> {
    using node_t = typename Graph::node_t;

    std::map<node_t, std::vector<node_t>> cut_adj;
    int total_weight = 0;
    for (const auto& e : cut_edges) {
        cut_adj[e.first].push_back(e.second);
        cut_adj[e.second].push_back(e.first);
        total_weight += weight(e.first, e.second);
    }

    std::map<node_t, int> colour;
    bool is_bipartite = true;
    for (const auto& [start, _] : cut_adj) {
        if (colour.contains(start)) continue;
        colour[start] = 1;
        std::queue<node_t> q;
        q.push(start);
        while (!q.empty() && is_bipartite) {
            auto u = q.front();
            q.pop();
            for (const auto& v : cut_adj[u]) {
                if (!colour.contains(v)) {
                    colour[v] = (colour[u] == 1) ? 2 : 1;
                    q.push(v);
                } else if (colour[v] == colour[u]) {
                    is_bipartite = false;
                    break;
                }
            }
        }
    }
    return {is_bipartite, total_weight};
}

template <typename Graph>
auto validate_max_cut(
    [[maybe_unused]] const Graph& G,
    const py::set<std::pair<typename Graph::node_t, typename Graph::node_t>>& cut_edges
) -> std::pair<bool, int> {
    return validate_max_cut(G, cut_edges, [](auto, auto) { return 1; });
}
