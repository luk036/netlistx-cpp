#include <doctest/doctest.h>

#include <cstdint>
#include <netlistx/netlist.hpp>  // for SimpleNetlist, graph_t
#include <netlistx/rand_cover.hpp>
#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>
#include <utility>
#include <xnetwork/classes/graph.hpp>  // for SimpleGraph

// ============================================================================
// rand_hyper_vertex_cover
// ============================================================================

TEST_CASE("rand_hyper_vertex_cover simple") {
    // 3 vertices (0,1,2) + 2 nets (3,4) = 5 total nodes
    xnetwork::SimpleGraph g(5);
    // Net 0 (node 3) connects vertices 1, 2
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    // Net 1 (node 4) connects vertices 0, 1
    g.add_edge(4, 0);
    g.add_edge(4, 1);
    SimpleNetlist hyprgraph(std::move(g), 3, 2);

    py::dict<uint32_t, int> weight{{0, 1}, {1, 1}, {2, 1}};

    auto [soln, cost] = rand_hyper_vertex_cover(hyprgraph, weight, 42);
    // Must cover each net
    for (const auto& net : hyprgraph.nets) {
        bool covered = false;
        for (const auto& v : hyprgraph.gr[net]) {
            if (soln.contains(v)) {
                covered = true;
                break;
            }
        }
        CHECK(covered);
    }
    CHECK_GE(cost, 1);
}

TEST_CASE("rand_hyper_vertex_cover empty") {
    xnetwork::SimpleGraph g(0);
    SimpleNetlist hyprgraph(std::move(g), 0, 0);
    py::dict<uint32_t, int> weight;

    auto [soln, cost] = rand_hyper_vertex_cover(hyprgraph, weight, 0);
    CHECK(soln.empty());
    CHECK_EQ(cost, 0);
}

TEST_CASE("rand_hyper_vertex_cover deterministic") {
    // 3 vertices (0,1,2) + 1 net (3) = 4 total nodes
    xnetwork::SimpleGraph g(4);
    // Single net connecting all 3 vertices
    g.add_edge(3, 0);
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    SimpleNetlist hyprgraph(std::move(g), 3, 1);

    py::dict<uint32_t, int> weight{{0, 1}, {1, 1}, {2, 1}};

    auto [soln1, cost1] = rand_hyper_vertex_cover(hyprgraph, weight, 99);
    auto [soln2, cost2] = rand_hyper_vertex_cover(hyprgraph, weight, 99);

    CHECK_EQ(soln1, soln2);
    CHECK_EQ(cost1, cost2);
}

// ============================================================================
// rand_hyper_vertex_cover_mt
// ============================================================================

TEST_CASE("rand_hyper_vertex_cover_mt simple") {
    // 3 vertices (0,1,2) + 2 nets (3,4) = 5 total nodes
    xnetwork::SimpleGraph g(5);
    // Net 0 (node 3) connects vertices 1, 2
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    // Net 1 (node 4) connects vertices 0, 1
    g.add_edge(4, 0);
    g.add_edge(4, 1);
    SimpleNetlist hyprgraph(std::move(g), 3, 2);

    py::dict<uint32_t, int> weight{{0, 1}, {1, 1}, {2, 1}};

    auto [soln, cost] = rand_hyper_vertex_cover_mt(hyprgraph, weight, 64, 42);
    for (const auto& net : hyprgraph.nets) {
        bool covered = false;
        for (const auto& v : hyprgraph.gr[net]) {
            if (soln.contains(v)) {
                covered = true;
                break;
            }
        }
        CHECK(covered);
    }
    CHECK_GE(cost, 1);
}

TEST_CASE("rand_hyper_vertex_cover_mt weighted") {
    // 2 vertices (0,1) + 1 net (2) = 3 total nodes
    xnetwork::SimpleGraph g(3);
    // Single net connecting 0 and 1
    g.add_edge(2, 0);
    g.add_edge(2, 1);
    SimpleNetlist hyprgraph(std::move(g), 2, 1);

    py::dict<uint32_t, int> weight{{0, 100}, {1, 1}};

    // With many trials, the lighter vertex (1) should be picked
    auto [soln, cost] = rand_hyper_vertex_cover_mt(hyprgraph, weight, 128, 7);
    CHECK_EQ(soln.size(), 1);
    CHECK(soln.contains(1));
    CHECK_EQ(cost, 1);
}
