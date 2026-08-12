#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <cstdint>
#include <iostream>
#include <netlistx/cover.hpp>  // min_hyper_vertex_cover (over hypergraph)
#include <netlistx/netlist.hpp>
#include <netlistx/netlist_algo.hpp>
#include <netlistx/readwrite.hpp>  // read_yosys_json, read_yosys_json_sax
#include <random>
#include <string>
#include <xnetwork/classes/graph.hpp>
#include <xnetwork/graph_algo.hpp>  // min_vertex_cover_fast, min_maximal_independent_set

using namespace std;

// -----------------------------------------------------------------------
// Deterministic test: min_vertex_cover_fast on a simple line graph
// Line: 0-1, 1-2, 2-3, 3-4 (5 nodes, 4 edges, unit weights)
// Expected: cover size = 2, cost = 2 (optimal cover: {1, 3})
// -----------------------------------------------------------------------
auto bench_vertex_cover_fast_line(ankerl::nanobench::Bench& bench) {
    auto ugraph = xnetwork::SimpleGraph(5);
    ugraph.add_edge(0, 1);
    ugraph.add_edge(1, 2);
    ugraph.add_edge(2, 3);
    ugraph.add_edge(3, 4);

    py::dict<uint32_t, int> weight;
    for (uint32_t i = 0; i < 5; ++i) weight[i] = 1;

    size_t result_size = 0;
    int result_cost = 0;
    {
        py::set<uint32_t> coverset;
        auto [sol, cost] = min_vertex_cover_fast(ugraph, weight, coverset);
        result_size = sol.size();
        result_cost = cost;
    }
    cout << "min_vertex_cover_fast (line graph, unit weights):\n";
    cout << "  Cover set size: " << result_size << ", cost: " << result_cost << "\n";

    bench.run("min_vertex_cover_fast (line)", [&] {
        py::set<uint32_t> coverset;
        auto [sol, cost] = min_vertex_cover_fast(ugraph, weight, coverset);
        ankerl::nanobench::doNotOptimizeAway(sol);
        ankerl::nanobench::doNotOptimizeAway(cost);
    });
}

// -----------------------------------------------------------------------
// min_hyper_vertex_cover on inverter netlist
// Inverter: modules 0-2, nets 3-4, edges: n0-p1, n0-a0, n1-a0, n1-p2
// -----------------------------------------------------------------------
auto bench_hyper_vertex_cover_inverter(ankerl::nanobench::Bench& bench) {
    auto ugraph = xnetwork::SimpleGraph(5);
    ugraph.add_edge(3, 1);  // n0-p1
    ugraph.add_edge(3, 0);  // n0-a0
    ugraph.add_edge(4, 0);  // n1-a0
    ugraph.add_edge(4, 2);  // n1-p2

    auto netlist = SimpleNetlist(ugraph, 3u, 2u);

    py::dict<uint32_t, int> weight;
    for (uint32_t i = 0; i < 3; ++i) weight[i] = 1;

    cout << "min_hyper_vertex_cover (inverter netlist, unit weights):\n";
    bench.run("min_hyper_vertex_cover (inverter)", [&] {
        py::set<uint32_t> coverset;
        auto [sol, cost] = min_hyper_vertex_cover(netlist, weight, coverset);
        ankerl::nanobench::doNotOptimizeAway(sol);
        ankerl::nanobench::doNotOptimizeAway(cost);
    });
}

// -----------------------------------------------------------------------
// min_maximal_matching on inverter netlist
// -----------------------------------------------------------------------
auto bench_maximal_matching_inverter(ankerl::nanobench::Bench& bench) {
    auto ugraph = xnetwork::SimpleGraph(5);
    ugraph.add_edge(3, 1);
    ugraph.add_edge(3, 0);
    ugraph.add_edge(4, 0);
    ugraph.add_edge(4, 2);

    auto netlist = SimpleNetlist(ugraph, 3u, 2u);

    py::dict<uint32_t, unsigned int> weight;
    for (uint32_t i = 3; i < 5; ++i) weight[i] = 1;

    cout << "min_maximal_matching (inverter netlist, unit weights):\n";
    bench.run("min_maximal_matching (inverter)", [&] {
        py::set<uint32_t> matchset;
        py::set<uint32_t> dep;
        auto cost = min_maximal_matching(netlist, weight, matchset, dep);
        ankerl::nanobench::doNotOptimizeAway(cost);
    });
}

// -----------------------------------------------------------------------
// Yosys JSON parsing performance
// -----------------------------------------------------------------------
auto bench_yosys_json(ankerl::nanobench::Bench& bench, const string& path, const string& label) {
    bench.run("DOM " + label, [&] {
        auto result = read_yosys_json(path);
        ankerl::nanobench::doNotOptimizeAway(result);
    });

    bench.run("SAX " + label, [&] {
        auto result = read_yosys_json_sax(path);
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

int main() {
    cout << "=== Cross-Project Benchmarks (netlistx-cpp / xnetwork-cpp) ===\n\n";

    ankerl::nanobench::Bench bench;
    bench.title("Graph algorithms").unit("op").warmup(100).epochs(50).minEpochIterations(1000);

    bench_vertex_cover_fast_line(bench);
    cout << "\n";
    bench_hyper_vertex_cover_inverter(bench);
    cout << "\n";
    bench_maximal_matching_inverter(bench);

    cout << "\n=== Yosys JSON Parsing ===\n";
    ankerl::nanobench::Bench bench2;
    bench2.title("Yosys JSON parsing").unit("op").warmup(100).epochs(50).minEpochIterations(10);

    struct {
        string path;
        string label;
    } files[] = {
        {"../../testcases/yosys_and2.json", "yosys_and2.json (tiny)"},
        {"../../yosys_testcases/sphere_netlist.json", "sphere_netlist.json (482 KB)"},
        {"../../yosys_testcases/sphere3hopf_netlist_simple.json",
         "sphere3hopf_netlist_simple.json (528 KB)"},
    };

    for (auto& f : files) {
        bench_yosys_json(bench2, f.path, f.label);
    }

    return 0;
}
