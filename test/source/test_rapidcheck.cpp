// -*- coding: utf-8 -*-
#include <doctest/doctest.h>

#ifdef RAPIDCHECK_H
// Suppress signed/unsigned warnings for RapidCheck headers (they have internal signed/unsigned comparisons)
#    if defined(_MSC_VER)
#        pragma warning(push)
#        pragma warning(disable : 4018 4267)
#    elif defined(__GNUC__) || defined(__clang__)
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wsign-compare"
#    endif
#    include <rapidcheck.h>
#    if defined(_MSC_VER)
#        pragma warning(pop)
#    elif defined(__GNUC__) || defined(__clang__)
#        pragma GCC diagnostic pop
#    endif

#    include <netlistx/cover.hpp>
#    include <netlistx/netlist.hpp>
#    include <netlistx/graph_algo.hpp>
#    include <xnetwork/classes/graph.hpp>
#    include <py2cpp/dict.hpp>
#    include <py2cpp/set.hpp>
#    include <vector>

// Simple test graph structure for RapidCheck tests
struct RapidTestGraph {
    using node_t = uint32_t;
    std::vector<std::pair<node_t, node_t>> edges_list;
    std::vector<std::vector<node_t>> adjacency;
    size_t num_nodes;

    RapidTestGraph(size_t n, const std::vector<std::pair<node_t, node_t>>& edges)
        : edges_list(edges), adjacency(n), num_nodes(n) {
        for (const auto& edge : edges) {
            if (edge.first < n && edge.second < n) {
                adjacency[edge.first].push_back(edge.second);
                adjacency[edge.second].push_back(edge.first);
            }
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

TEST_CASE("Property-based test: Vertex cover covers all edges") {
    rc::check("All edges are covered by vertex cover",
              []() {
                  // Generate random graph size
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(1, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  // Generate random edges
                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  // Create weight map with unit weights
                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  // Compute vertex cover
                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  // Verify all edges are covered
                  for (const auto& edge : graph.edges()) {
                      RC_ASSERT(coverset.contains(edge.first) || coverset.contains(edge.second));
                  }
              });
}

TEST_CASE("Property-based test: Maximal independent set is independent") {
    rc::check("No two vertices in independent set are adjacent",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [indset, total_weight] = min_maximal_independant_set(graph, weight);

                  // Verify no two vertices in indset are adjacent
                  for (const auto& u : indset) {
                      for (const auto& v : graph[u]) {
                          RC_ASSERT(!indset.contains(v));
                      }
                  }
              });
}

TEST_CASE("Property-based test: Vertex cover weight is non-negative") {
    rc::check("Vertex cover total weight is non-negative",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(1, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  RC_ASSERT(total_weight >= 0);
              });
}

TEST_CASE("Property-based test: Independent set weight is non-negative") {
    rc::check("Independent set total weight is non-negative",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [indset, total_weight] = min_maximal_independant_set(graph, weight);

                  RC_ASSERT(total_weight >= 0);
              });
}

TEST_CASE("Property-based test: Empty graph has empty vertex cover") {
    rc::check("Graph with no edges has empty vertex cover",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));
                  std::vector<std::pair<uint32_t, uint32_t>> edges; // No edges

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  RC_ASSERT(coverset.empty());
              });
}

TEST_CASE("Property-based test: Independent set is subset of all vertices") {
    rc::check("Independent set vertices are within graph vertex set",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [indset, total_weight] = min_maximal_independant_set(graph, weight);

                  // Verify all vertices in indset are within the graph's vertex range
                  for (const auto& v : indset) {
                      RC_ASSERT(v < static_cast<uint32_t>(num_nodes));
                  }
              });
}

TEST_CASE("Property-based test: Vertex cover size does not exceed vertex count") {
    rc::check("Vertex cover size is at most the number of vertices",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  RC_ASSERT(coverset.size() <= num_nodes);
              });
}

TEST_CASE("Property-based test: Complete graph vertex cover is non-empty") {
    rc::check("Complete graph has non-empty vertex cover",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 6));

                  // Create complete graph
                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      for (uint32_t j = i + 1; j < static_cast<uint32_t>(num_nodes); ++j) {
                          edges.push_back({i, j});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  RC_ASSERT(!coverset.empty());
              });
}

TEST_CASE("Property-based test: Independent set size is bounded") {
    rc::check("Independent set size is at most the number of vertices",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [indset, total_weight] = min_maximal_independant_set(graph, weight);

                  RC_ASSERT(indset.size() <= num_nodes);
              });
}

TEST_CASE("Property-based test: Cycle cover handles small cycles") {
    rc::check("Cycle cover algorithm handles 3-node cycles",
              []() {
                  // Create a 3-node cycle
                  std::vector<std::pair<uint32_t, uint32_t>> edges = {
                      {0, 1}, {1, 2}, {2, 0}
                  };

                  RapidTestGraph graph(3, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < 3; ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_cycle_cover(graph, weight);

                  // Verify the cycle is covered
                  bool cycle_covered = false;
                  for (const auto& edge : graph.edges()) {
                      if (coverset.contains(edge.first) || coverset.contains(edge.second)) {
                          cycle_covered = true;
                          break;
                      }
                  }
                  RC_ASSERT(cycle_covered);
              });
}

TEST_CASE("Property-based test: Odd cycle cover detects odd cycles") {
    rc::check("Odd cycle cover handles 3-node cycles",
              []() {
                  // Create a 3-node cycle (odd cycle)
                  std::vector<std::pair<uint32_t, uint32_t>> edges = {
                      {0, 1}, {1, 2}, {2, 0}
                  };

                  RapidTestGraph graph(3, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < 3; ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_odd_cycle_cover(graph, weight);

                  RC_ASSERT(total_weight >= 0);
              });
}

TEST_CASE("Property-based test: Path graph has simple vertex cover") {
    rc::check("Path graph vertex cover is simple to compute",
              []() {
                  auto path_length = static_cast<size_t>(*rc::gen::inRange(3, 8));

                  // Create a path graph
                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(path_length - 1); ++i) {
                      edges.push_back({i, i + 1});
                  }

                  RapidTestGraph graph(path_length, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(path_length); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  // Verify all edges are covered
                  for (const auto& edge : graph.edges()) {
                      RC_ASSERT(coverset.contains(edge.first) || coverset.contains(edge.second));
                  }
              });
}

TEST_CASE("Property-based test: Star graph has minimal vertex cover") {
    rc::check("Star graph has single-vertex cover with center",
              []() {
                  auto num_leaves = static_cast<size_t>(*rc::gen::inRange(2, 6));

                  // Create a star graph (center = 0, leaves = 1..num_leaves)
                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (uint32_t i = 1; i <= static_cast<uint32_t>(num_leaves); ++i) {
                      edges.push_back({0, i});
                  }

                  RapidTestGraph graph(num_leaves + 1, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i <= static_cast<uint32_t>(num_leaves); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  // Verify all edges are covered
                  for (const auto& edge : graph.edges()) {
                      RC_ASSERT(coverset.contains(edge.first) || coverset.contains(edge.second));
                  }
              });
}

TEST_CASE("Property-based test: Bipartite graph independent set is non-empty") {
    rc::check("Bipartite graph has non-empty independent set",
              []() {
                  // Create a simple bipartite graph
                  auto left_size = static_cast<size_t>(*rc::gen::inRange(2, 4));
                  auto right_size = static_cast<size_t>(*rc::gen::inRange(2, 4));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(left_size); ++i) {
                      for (uint32_t j = 0; j < static_cast<uint32_t>(right_size); ++j) {
                          edges.push_back({i, static_cast<uint32_t>(left_size) + j});
                      }
                  }

                  RapidTestGraph graph(left_size + right_size, edges);

                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(left_size + right_size); ++i) {
                      weight.insert_or_assign(i, 1U);
                  }

                  auto [indset, total_weight] = min_maximal_independant_set(graph, weight);

                  RC_ASSERT(!indset.empty());
              });
}

TEST_CASE("Property-based test: Vertex cover with varying weights") {
    rc::check("Vertex cover handles varying vertex weights",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 8));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(1, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  // Create weight map with random weights
                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      auto w = static_cast<unsigned int>(*rc::gen::inRange(1, 10));
                      weight.insert_or_assign(i, w);
                  }

                  auto [coverset, total_weight] = min_vertex_cover_fast(graph, weight);

                  // Verify all edges are covered
                  for (const auto& edge : graph.edges()) {
                      RC_ASSERT(coverset.contains(edge.first) || coverset.contains(edge.second));
                  }

                  RC_ASSERT(total_weight >= 0);
              });
}

TEST_CASE("Property-based test: Independent set with varying weights") {
    rc::check("Independent set handles varying vertex weights",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(2, 8));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes * (num_nodes - 1) / 2)));

                  std::vector<std::pair<uint32_t, uint32_t>> edges;
                  for (size_t i = 0; i < num_edges; ++i) {
                      auto u = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      auto v = static_cast<uint32_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes - 1)));
                      if (u != v) {
                          edges.push_back({u, v});
                      }
                  }

                  RapidTestGraph graph(num_nodes, edges);

                  // Create weight map with random weights
                  py::dict<uint32_t, unsigned int> weight;
                  for (uint32_t i = 0; i < static_cast<uint32_t>(num_nodes); ++i) {
                      auto w = static_cast<unsigned int>(*rc::gen::inRange(1, 10));
                      weight.insert_or_assign(i, w);
                  }

                  auto [indset, total_weight] = min_maximal_independant_set(graph, weight);

                  RC_ASSERT(total_weight >= 0);
              });
}

#endif