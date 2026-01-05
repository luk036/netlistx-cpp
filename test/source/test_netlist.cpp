// -*- coding: utf-8 -*-
#include <doctest/doctest.h>  // for ResultBuilder, CHECK, TestCase

#include <netlistx/netlist.hpp>        // for Netlist, SimpleNetlist, graph_t
#include <xnetwork/classes/graph.hpp>  // for SimpleGraph, Graph
// #include <py2cpp/py2cpp.hpp>
// #include <__config>     // for std
#include <cstdint>      // for uint32_t
#include <utility>      // for pair
#include <vector>       // for vector

using namespace std;

// using graph_t =
//     boost::adjacency_list<boost::hash_setS, boost::vecS, boost::undirectedS>;
// using node_t = typename boost::graph_traits<graph_t>::vertex_descriptor;
// using edge_t = typename boost::graph_traits<graph_t>::edge_iterator;

/**
 * @brief Create a test netlist object
 *
 * @return Netlist
 */
auto create_dwarf() -> SimpleNetlist {
    using Edge = pair<uint32_t, uint32_t>;
    const auto num_nodes = 13U;
    enum nodes { mod0, mod1, mod2, mod3, pad1, pad2, pad3, net1, net2, net3, net4, net5, net6 };
    // static vector<nodes> module_name_list = {mod1, mod2, mod3};
    // static vector<nodes> net__name_list = {net1, net2, net3};

    // char name[] = "ABCDE";
    vector<Edge> edge_array{Edge(pad1, net1), Edge(mod0, net1), Edge(mod1, net1), Edge(mod0, net2), Edge(mod2, net2),
                            Edge(mod3, net2), Edge(mod1, net3), Edge(mod2, net3), Edge(mod3, net3), Edge(mod2, net4),
                            Edge(pad2, net4), Edge(mod3, net5), Edge(pad3, net5), Edge(mod0, net6)};
    // index_t indices[] = {0, 1, 2, 3, 4, 5};
    // int num_arcs = sizeof(edge_array) / sizeof(Edge);
    // auto R = py::range(num_nodes);
    // graph_t g{R, R};
    xnetwork::SimpleGraph g(num_nodes);
    for (const auto &e : edge_array) {
        g.add_edge(e.first, e.second);
    }
    // using node_t = typename boost::graph_traits<graph_t>::vertex_descriptor;
    // using IndexMap =
    //     typename boost::property_map<graph_t, boost::vertex_index_t>::type;
    // IndexMap index = boost::get(boost::vertex_index, g);
    // auto gr = py::GraphAdaptor<graph_t>(std::move(g));

    // vector<node_t> module_list(7);
    // vector<node_t> net_list(5);
    vector<unsigned int> module_weight = {1, 3, 4, 2, 0, 0, 0};
    // auto hyprgraph = Netlist{std::move(g), py::range(7), py::range(7, 13),
    // py::range(7),
    //                  py::range(-7, 6)};
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

    // char name[] = "ABCDE";
    auto edge_array = vector<Edge>{Edge(mod1, net1), Edge(mod1, net2), Edge(mod2, net1),
                                   Edge(mod2, net2), Edge(mod3, net2), Edge(mod1, net3)};
    // index_t indices[] = {0, 1, 2, 3, 4, 5};
    // auto num_arcs = sizeof(edge_array) / sizeof(Edge);
    // auto g = graph_t{edge_array, edge_array + num_arcs, num_nodes};
    // auto gr = py::GraphAdaptor<graph_t>{std::move(g)};
    // const auto R = py::range(num_nodes);
    graph_t g(num_nodes);
    for (const auto &e : edge_array) {
        g.add_edge(e.first, e.second);
    }

    auto module_weight = vector<unsigned int>{3, 4, 2};
    auto hyprgraph = SimpleNetlist{std::move(g), 3, 3};
    hyprgraph.module_weight = std::move(module_weight);
    return hyprgraph;
}

TEST_CASE("Test Netlist") {
    const auto hyprgraph = create_test_netlist();

    CHECK(hyprgraph.number_of_modules() == 3);
    CHECK(hyprgraph.number_of_nets() == 3);
    // CHECK(hyprgraph.number_of_pins() == 6);
    CHECK(hyprgraph.get_max_degree() == 3);
    CHECK(hyprgraph.get_max_net_degree() == 3);
    CHECK(!hyprgraph.has_fixed_modules);
}

TEST_CASE("Test dwarf") {
    // static_assert(sizeof(double*) == 8);
    const auto hyprgraph = create_dwarf();

    CHECK(hyprgraph.number_of_modules() == 7);
    CHECK(hyprgraph.number_of_nets() == 6);
    // CHECK(hyprgraph.number_of_pins() == 14);
    CHECK(hyprgraph.get_max_degree() == 3);
    CHECK(hyprgraph.get_max_net_degree() == 3);
    CHECK(!hyprgraph.has_fixed_modules);
    CHECK(hyprgraph.get_module_weight(1) == 3U);
}
