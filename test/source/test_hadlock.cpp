#include <doctest/doctest.h>

#include <cstdint>
#include <netlistx/hadlock.hpp>
#include <py2cpp/set.hpp>
#include <utility>
#include <vector>

// ===================================================================
// Test graph — minimal mock following test_graph_algo / test_cover pattern
// ===================================================================
struct TestGraph {
    using node_t = uint32_t;
    std::vector<std::pair<node_t, node_t>> edges_list;
    std::vector<std::vector<node_t>> adjacency;

    TestGraph(uint32_t num_nodes,
              std::vector<std::pair<node_t, node_t>> edges)
        : edges_list(std::move(edges)), adjacency(num_nodes) {
        for (const auto& e : edges_list) {
            adjacency[e.first].push_back(e.second);
            adjacency[e.second].push_back(e.first);
        }
    }

    auto edges() const -> const std::vector<std::pair<node_t, node_t>>& {
        return edges_list;
    }
};

// ===================================================================
// Helper weight lambdas
// ===================================================================
const auto unit_weight = [](uint32_t, uint32_t) -> int { return 1; };

// ===================================================================
// Tests: solve_hadlock_max_cut
// ===================================================================

TEST_CASE("Triangle") {
    // 3-cycle: w(0,1)=5, w(1,2)=10, w(2,0)=3
    TestGraph G(3, {{0, 1}, {1, 2}, {2, 0}});
    auto weight = [](uint32_t u, uint32_t v) -> int {
        if ((u == 0 && v == 1) || (u == 1 && v == 0)) return 5;
        if ((u == 1 && v == 2) || (u == 2 && v == 1)) return 10;
        if ((u == 2 && v == 0) || (u == 0 && v == 2)) return 3;
        return 1;
    };
    // Two faces (inner + outer), each with 3 nodes (odd)
    std::vector<std::vector<uint32_t>> faces = {{0, 1, 2}, {0, 2, 1}};

    auto cut = solve_hadlock_max_cut(G, weight, faces);
    auto [ok, val] = validate_max_cut(G, cut, weight);

    CHECK(ok);
    CHECK_EQ(val, 15); // 5 + 10
}

TEST_CASE("Square with diagonal") {
    // Square 1-2-3-4-1 (weights 5,10,5,10) + diagonal 1-3 (weight 2)
    // Planar embedding: two triangles + outer quad
    TestGraph G(5, {{1, 2}, {2, 3}, {3, 4}, {4, 1}, {1, 3}});
    auto weight = [](uint32_t u, uint32_t v) -> int {
        if ((u == 1 && v == 2) || (u == 2 && v == 1)) return 5;
        if ((u == 2 && v == 3) || (u == 3 && v == 2)) return 10;
        if ((u == 3 && v == 4) || (u == 4 && v == 3)) return 5;
        if ((u == 4 && v == 1) || (u == 1 && v == 4)) return 10;
        if ((u == 1 && v == 3) || (u == 3 && v == 1)) return 2;
        return 1;
    };
    // Face 0 = triangle 1-2-3 (odd), Face 1 = triangle 1-3-4 (odd),
    // Face 2 = outer quad 1-4-3-2 (even)
    std::vector<std::vector<uint32_t>> faces = {
        {1, 2, 3}, {1, 3, 4}, {1, 4, 3, 2}
    };

    auto cut = solve_hadlock_max_cut(G, weight, faces);
    auto [ok, val] = validate_max_cut(G, cut, weight);

    CHECK(ok);
    CHECK_EQ(val, 30); // 5 + 10 + 5 + 10
}

TEST_CASE("Grid 2x2 is bipartite — all edges in cut") {
    // (0,0)->0, (0,1)->1, (1,0)->2, (1,1)->3
    TestGraph G(4, {{0, 1}, {0, 2}, {1, 3}, {2, 3}});
    // Inner face 0-1-3-2 (even), outer face 0-2-3-1 (even)
    std::vector<std::vector<uint32_t>> faces = {
        {0, 1, 3, 2}, {0, 2, 3, 1}
    };

    auto cut = solve_hadlock_max_cut(G, unit_weight, faces);

    // All 4 edges should be in the cut
    CHECK_EQ(cut.size(), 4);
    auto [ok, val] = validate_max_cut(G, cut, unit_weight);
    CHECK(ok);
    CHECK_EQ(val, 4);
}

TEST_CASE("Empty graph") {
    TestGraph G(0, {});
    std::vector<std::vector<uint32_t>> faces;

    auto cut = solve_hadlock_max_cut(G, unit_weight, faces);
    CHECK(cut.empty());
}

TEST_CASE("Single edge") {
    TestGraph G(2, {{0, 1}});
    // Two faces, each a digon [0,1] — both even (2 nodes)
    std::vector<std::vector<uint32_t>> faces = {{0, 1}, {1, 0}};

    auto weight = [](uint32_t, uint32_t) -> int { return 7; };
    auto cut = solve_hadlock_max_cut(G, weight, faces);

    CHECK_EQ(cut.size(), 1);
    auto [ok, val] = validate_max_cut(G, cut, weight);
    CHECK(ok);
    CHECK_EQ(val, 7);
}

TEST_CASE("Unit weight triangle") {
    TestGraph G(3, {{0, 1}, {1, 2}, {2, 0}});
    std::vector<std::vector<uint32_t>> faces = {{0, 1, 2}, {0, 2, 1}};

    auto cut = solve_hadlock_max_cut(G, unit_weight, faces);
    auto [ok, val] = validate_max_cut(G, cut, unit_weight);

    CHECK(ok);
    // Total = 3, min weight edge excluded = 1, cut weight = 2
    CHECK_EQ(val, 2);
}

// ===================================================================
// Tests: validate_max_cut
// ===================================================================

TEST_CASE("Valid cut validates as bipartite") {
    TestGraph G(5, {{1, 2}, {2, 3}, {3, 4}, {4, 1}, {1, 3}});
    py::set<std::pair<uint32_t, uint32_t>> cut;
    cut.insert({1, 2});
    cut.insert({2, 3});
    cut.insert({3, 4});
    cut.insert({4, 1});

    auto [ok, val] = validate_max_cut(
        G, cut, [](uint32_t, uint32_t) { return 1; });
    CHECK(ok);
    CHECK_EQ(val, 4);
}

TEST_CASE("Cut containing a triangle is invalid") {
    TestGraph G(3, {{0, 1}, {1, 2}, {2, 0}});
    py::set<std::pair<uint32_t, uint32_t>> cut;
    cut.insert({0, 1});
    cut.insert({1, 2});
    cut.insert({2, 0});

    auto [ok, val] = validate_max_cut(G, cut, unit_weight);
    CHECK_FALSE(ok);
}

TEST_CASE("Unit weight overload") {
    TestGraph G(2, {{0, 1}});
    py::set<std::pair<uint32_t, uint32_t>> cut;
    cut.insert({0, 1});

    auto [ok, val] = validate_max_cut(G, cut);
    CHECK(ok);
    CHECK_EQ(val, 1);
}
