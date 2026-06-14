// -*- coding: utf-8 -*-
#include <doctest/doctest.h>  // for ResultBuilder, CHECK, TestCase

#include <cstdint>                     // for uint32_t
#include <netlistx/netlist.hpp>        // for Netlist, SimpleNetlist, graph_t
#include <utility>                     // for pair
#include <vector>                      // for vector
#include <xnetwork/classes/graph.hpp>  // for SimpleGraph, Graph

using namespace std;

/**
 * @brief Create a test netlist (the "dwarf" benchmark).
 *
 * Constructs a small bipartite graph with 7 module nodes (4 cells + 3 pads)
 * and 6 net nodes, used as a minimal test case for partitioning algorithms.
 *
 * @return SimpleNetlist A pre-built test netlist with module weights assigned
 */
auto create_dwarf() -> SimpleNetlist {
    using Edge = pair<uint32_t, uint32_t>;
    const auto num_nodes = 13U;
    enum nodes { mod0, mod1, mod2, mod3, pad1, pad2, pad3, net1, net2, net3, net4, net5, net6 };
    vector<Edge> edge_array{Edge(pad1, net1), Edge(mod0, net1), Edge(mod1, net1), Edge(mod0, net2),
                            Edge(mod2, net2), Edge(mod3, net2), Edge(mod1, net3), Edge(mod2, net3),
                            Edge(mod3, net3), Edge(mod2, net4), Edge(pad2, net4), Edge(mod3, net5),
                            Edge(pad3, net5), Edge(mod0, net6)};
    xnetwork::SimpleGraph g(num_nodes);
    for (const auto& e : edge_array) {
        g.add_edge(e.first, e.second);
    }
    vector<unsigned int> module_weight = {1, 3, 4, 2, 0, 0, 0};
    SimpleNetlist hyprgraph(std::move(g), 7, 6);

    hyprgraph.module_weight = module_weight;
    hyprgraph.num_pads = 3;
    return hyprgraph;
}

/**
 * @brief Create a test netlist object
 *
 * @return Netlist
 */
auto create_test_netlist() -> SimpleNetlist {
    using Edge = pair<uint32_t, uint32_t>;
    auto num_nodes = 6U;
    enum nodes { mod1, mod2, mod3, net1, net2, net3 };

    auto edge_array = vector<Edge>{Edge(mod1, net1), Edge(mod1, net2), Edge(mod2, net1),
                                   Edge(mod2, net2), Edge(mod3, net2), Edge(mod1, net3)};
    graph_t g(num_nodes);
    for (const auto& e : edge_array) {
        g.add_edge(e.first, e.second);
    }

    auto module_weight = vector<unsigned int>{3, 4, 2};
    auto hyprgraph = SimpleNetlist{std::move(g), 3, 3};
    hyprgraph.module_weight = std::move(module_weight);
    return hyprgraph;
}

TEST_CASE("Test Netlist") {
    const auto hyprgraph = create_test_netlist();

    CHECK_EQ(hyprgraph.number_of_modules(), 3);
    CHECK_EQ(hyprgraph.number_of_nets(), 3);
    CHECK_EQ(hyprgraph.get_max_degree(), 3);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 3);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
}

TEST_CASE("Test dwarf") {
    const auto hyprgraph = create_dwarf();

    CHECK_EQ(hyprgraph.number_of_modules(), 7);
    CHECK_EQ(hyprgraph.number_of_nets(), 6);
    CHECK_EQ(hyprgraph.get_max_degree(), 3);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 3);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 3U);
}
