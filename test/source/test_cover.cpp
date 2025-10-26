#include <doctest/doctest.h>
#include <cstdint>
#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>
#include <unordered_map>
#include <vector>
#include <utility>

#include <netlistx/netlist.hpp>        // for Netlist, SimpleNetlist, graph_t
#include <netlistx/cover.hpp> // Assuming the converted functions are in this header

// Simple test graph structure
struct TestCoverGraph {
    using node_t = uint32_t;
    std::vector<std::pair<node_t, node_t>> edges_list;
    std::vector<std::vector<node_t>> adjacency;
    size_t num_nodes;
    
    TestCoverGraph(size_t n, const std::vector<std::pair<node_t, node_t>>& edges) 
        : edges_list(edges), adjacency(n), num_nodes(n) {
        for (const auto& edge : edges) {
            adjacency[edge.first].push_back(edge.second);
            adjacency[edge.second].push_back(edge.first);
        }
    }
    
    auto edges() const -> const std::vector<std::pair<node_t, node_t>>& {
        return edges_list;
    }
    
    auto operator[](node_t node) const -> const std::vector<node_t>& {
        return adjacency[node];
    }

    auto begin() const { return py::range<uint32_t>(uint32_t(0), uint32_t(num_nodes)).begin(); }
    auto end() const { return py::range<uint32_t>(uint32_t(0), uint32_t(num_nodes)).end(); }
    
    auto number_of_nodes() const -> size_t {
        return num_nodes;
    }
};

// Mock hypergraph for testing
struct MockHypergraph {
    using node_t = uint32_t;
    using graph_t = TestCoverGraph;
    
    graph_t gr;
    std::vector<node_t> nets;
    
    MockHypergraph(const std::vector<node_t>& net_list, const graph_t& graph) 
        : gr(graph), nets(net_list) {}
};

TEST_CASE("Test pd_cover basic") {
    auto violate_func = []() -> std::vector<std::vector<uint32_t>> {
        return {{0, 1}, {0, 2}, {1, 2}};
    };
    
    py::dict<uint32_t, int> weight;
    weight[0] = 1;
    weight[1] = 2;
    weight[2] = 3;
    
    py::set<uint32_t> soln;
    auto [covered, cost] = pd_cover(violate_func, weight, soln);
    
    CHECK(covered.contains(0));
    CHECK(covered.contains(1));
    CHECK_FALSE(covered.contains(2));
    CHECK(cost == 4); // 1 + 2
}

TEST_CASE("Test min_vertex_cover simple") {
    TestCoverGraph ugraph(3, {{0, 1}, {1, 2}});
    py::dict<uint32_t, int> weight;
    weight[0] = 1;
    weight[1] = 1;
    weight[2] = 1;
    
    py::set<uint32_t> soln;
    auto [covered, cost] = min_vertex_cover(ugraph, weight, soln);
    
    // Verify all edges are covered
    for (const auto& edge : ugraph.edges()) {
        auto expression = covered.contains(edge.first) || covered.contains(edge.second);
        CHECK(expression);
    }
}

TEST_CASE("Test min_hyper_vertex_cover") {
    TestCoverGraph base_graph(3, {});
    MockHypergraph hyprgraph({0, 1}, base_graph); // Nets 0 and 1
    
    // Mock the graph access for nets
    hyprgraph.gr.adjacency = {
        {1, 2}, // Net 0 connects vertices 1, 2
        {0, 1}  // Net 1 connects vertices 0, 1
    };
    
    py::dict<uint32_t, int> weight;
    weight[0] = 1;
    weight[1] = 1;
    weight[2] = 1;
    
    py::set<uint32_t> soln;
    auto [covered, cost] = min_hyper_vertex_cover(hyprgraph, weight, soln);
    
    // Should cover at least one vertex from each net
    CHECK(covered.size() >= 1);
}

TEST_CASE("Test min_cycle_cover triangle") {
    TestCoverGraph ugraph(3, {{0, 1}, {1, 2}, {2, 0}});
    py::dict<uint32_t, int> weight {{0, 1}, {1, 1}, {2, 1}};
    // weight[0] = 1;
    // weight[1] = 1;
    // weight[2] = 1;
    
    py::set<uint32_t> soln;
    auto [covered, cost] = min_cycle_cover(ugraph, weight, soln);
    
    // Should cover at least one vertex to break the cycle
    CHECK(covered.size() >= 1);
    CHECK(cost >= 1);
}

TEST_CASE("Test min_odd_cycle_cover triangle") {
    TestCoverGraph ugraph(3, {{0, 1}, {1, 2}, {2, 0}});
    py::dict<uint32_t, int> weight {{0, 1}, {1, 1}, {2, 1}};
    // weight[0] = 1;
    // weight[1] = 1;
    // weight[2] = 1;
    
    py::set<uint32_t> soln;
    auto [covered, cost] = min_odd_cycle_cover(ugraph, weight, soln);
    
    // Should cover at least one vertex to break the odd cycle
    CHECK(covered.size() >= 1);
    CHECK(cost >= 1);
}

TEST_CASE("Test _construct_cycle") {
    py::dict<uint32_t, BFSInfo<uint32_t>> info;

    info.insert_or_assign(uint32_t(0), BFSInfo<uint32_t>(uint32_t(0), 3));
    info.insert_or_assign(uint32_t(1), BFSInfo<uint32_t>(uint32_t(0), 2));
    info.insert_or_assign(uint32_t(2), BFSInfo<uint32_t>(uint32_t(1), 1));
    info.insert_or_assign(uint32_t(3), BFSInfo<uint32_t>(uint32_t(2), 0));
    
    auto cycle = _construct_cycle<uint32_t>(info, 1, 3);
    
    // Cycle should contain the path
    CHECK(cycle.size() >= 2);
}

TEST_CASE("Test empty graph") {
    TestCoverGraph ugraph(0, {});
    py::dict<uint32_t, int> weight;
    
    auto [covered, cost] = min_vertex_cover(ugraph, weight);
    CHECK(covered.empty());
    CHECK(cost == 0);
    
    auto [covered2, cost2] = min_cycle_cover(ugraph, weight);
    CHECK(covered2.empty());
    CHECK(cost2 == 0);
}

TEST_CASE("Test single vertex") {
    TestCoverGraph ugraph(1, {});
    py::dict<uint32_t, int> weight{{0, 5}};
    // weight[0] = 5;
    
    auto [covered, cost] = min_vertex_cover(ugraph, weight);
    CHECK(covered.empty()); // No edges to cover
    CHECK(cost == 0);
}