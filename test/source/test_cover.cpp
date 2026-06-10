#include <doctest/doctest.h>

#include <cstdint>
#include <netlistx/cover.hpp>
#include <netlistx/netlist.hpp>  // for Netlist, SimpleNetlist, graph_t
#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>
#include <utility>
#include <xnetwork/classes/graph.hpp>  // for SimpleGraph

static void verify_cover_valid(const SimpleNetlist& hyprgraph,
                                const py::set<uint32_t>& coverset) {
    for (const auto& net : hyprgraph.nets) {
        bool net_covered = false;
        for (const auto& v : hyprgraph.gr[net]) {
            if (coverset.contains(v)) {
                net_covered = true;
                break;
            }
        }
        CHECK_MESSAGE(net_covered, "Net ", net, " must be covered by the vertex cover");
    }
}

TEST_CASE("Test min_hyper_vertex_cover") {
    // 3 vertices (0,1,2) + 2 nets (3,4) = 5 total nodes
    xnetwork::SimpleGraph g(5);
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    g.add_edge(4, 0);
    g.add_edge(4, 1);
    SimpleNetlist hyprgraph(std::move(g), 3, 2);

    py::dict<uint32_t, int> weight;
    weight[0] = 1;
    weight[1] = 1;
    weight[2] = 1;

    py::set<uint32_t> soln;
    auto [covered, cost] = min_hyper_vertex_cover(hyprgraph, weight, soln);

    verify_cover_valid(hyprgraph, covered);
    CHECK_GE(cost, 1);
}

TEST_CASE("Test min_hyper_vertex_cover with pre-covered vertices") {
    // 3 vertices (0,1,2) + 2 nets (3,4) = 5 total nodes
    xnetwork::SimpleGraph g(5);
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    g.add_edge(4, 0);
    g.add_edge(4, 1);
    SimpleNetlist hyprgraph(std::move(g), 3, 2);

    py::dict<uint32_t, int> weight;
    weight[0] = 10;
    weight[1] = 1;
    weight[2] = 1;

    // Pre-cover vertex 0 (high weight, but forced to be in cover)
    py::set<uint32_t> soln;
    soln.insert(0);
    auto [covered, cost] = min_hyper_vertex_cover(hyprgraph, weight, soln);

    verify_cover_valid(hyprgraph, covered);
    CHECK(covered.contains(0));
    CHECK_LE(cost, 12);  // at most weight[0] + weight[1] or weight[0] + weight[2]
}

TEST_CASE("Test min_hyper_vertex_cover convenience overload") {
    xnetwork::SimpleGraph g(5);
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    g.add_edge(4, 0);
    g.add_edge(4, 1);
    SimpleNetlist hyprgraph(std::move(g), 3, 2);

    py::dict<uint32_t, int> weight;
    weight[0] = 1;
    weight[1] = 1;
    weight[2] = 1;

    auto [covered, cost] = min_hyper_vertex_cover(hyprgraph, weight);

    verify_cover_valid(hyprgraph, covered);
    CHECK_GT(covered.size(), 0U);
    CHECK_GE(cost, 1);
}

TEST_CASE("Test min_hyper_vertex_cover weighted") {
    xnetwork::SimpleGraph g(3);
    g.add_edge(2, 0);
    g.add_edge(2, 1);
    SimpleNetlist hyprgraph(std::move(g), 2, 1);

    py::dict<uint32_t, int> weight;
    weight[0] = 100;
    weight[1] = 1;

    py::set<uint32_t> soln;
    auto [covered, cost] = min_hyper_vertex_cover(hyprgraph, weight, soln);

    verify_cover_valid(hyprgraph, covered);
    CHECK_EQ(cost, 1);
    CHECK(covered.contains(1));
    CHECK_EQ(covered.size(), 1);
}
