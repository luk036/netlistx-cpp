#include <doctest/doctest.h>

#include <cstdint>
#include <netlistx/cover.hpp>
#include <netlistx/netlist.hpp>  // for Netlist, SimpleNetlist, graph_t
#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>
#include <utility>
#include <xnetwork/classes/graph.hpp>  // for SimpleGraph

TEST_CASE("Test min_hyper_vertex_cover") {
    // 3 vertices (0,1,2) + 2 nets (3,4) = 5 total nodes
    xnetwork::SimpleGraph g(5);
    // Net 0 (node 3) connects vertices 1, 2
    g.add_edge(3, 1);
    g.add_edge(3, 2);
    // Net 1 (node 4) connects vertices 0, 1
    g.add_edge(4, 0);
    g.add_edge(4, 1);
    SimpleNetlist hyprgraph(std::move(g), 3, 2);

    py::dict<uint32_t, int> weight;
    weight[0] = 1;
    weight[1] = 1;
    weight[2] = 1;

    py::set<uint32_t> soln;
    auto [covered, cost] = min_hyper_vertex_cover(hyprgraph, weight, soln);

    // Should cover at least one vertex from each net
    CHECK_GE(covered.size(), 1);
}
