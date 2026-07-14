#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <netlistx/netlist.hpp>
#include <netlistx/netlist_algo.hpp>
#include <netlistx/cover.hpp>         // min_hyper_vertex_cover (over hypergraph)
#include <netlistx/readwrite.hpp>     // read_yosys_json, read_yosys_json_sax
#include <xnetwork/graph_algo.hpp>    // min_vertex_cover_fast, min_maximal_independent_set
#include <xnetwork/classes/graph.hpp>

using namespace std;
using Clock = chrono::high_resolution_clock;
using Duration = chrono::duration<double, milli>;

// -----------------------------------------------------------------------
// Deterministic test: min_vertex_cover_fast on a simple line graph
// Line: 0-1, 1-2, 2-3, 3-4 (5 nodes, 4 edges, unit weights)
// Expected: cover size = 2, cost = 2 (optimal cover: {1, 3})
// -----------------------------------------------------------------------
auto bench_vertex_cover_fast_line(int iterations) {
    auto ugraph = xnetwork::SimpleGraph(5);
    ugraph.add_edge(0, 1);
    ugraph.add_edge(1, 2);
    ugraph.add_edge(2, 3);
    ugraph.add_edge(3, 4);

    py::dict<uint32_t, int> weight;
    for (uint32_t i = 0; i < 5; ++i) weight[i] = 1;

    vector<double> times;
    times.reserve(iterations);

    size_t result_size = 0;
    int result_cost = 0;

    for (int i = 0; i < iterations; ++i) {
        py::set<uint32_t> coverset;
        auto start = Clock::now();
        auto [sol, cost] = min_vertex_cover_fast(ugraph, weight, coverset);
        auto end = Clock::now();
        times.push_back(Duration(end - start).count());
        if (i == 0) { result_size = sol.size(); result_cost = cost; }
    }

    sort(times.begin(), times.end());
    double sum = 0;
    for (double t : times) sum += t;
    double mean = sum / iterations;

    cout << "min_vertex_cover_fast (line graph, unit weights):\n";
    cout << "  Cover set size: " << result_size << ", cost: " << result_cost << "\n";
    cout << "  Min: " << times.front() << " ms, Max: " << times.back()
         << " ms, Mean: " << mean << " ms\n";

    return mean;
}

// -----------------------------------------------------------------------
// min_hyper_vertex_cover on inverter netlist
// Inverter: modules 0-2, nets 3-4, edges: n0-p1, n0-a0, n1-a0, n1-p2
// -----------------------------------------------------------------------
auto bench_hyper_vertex_cover_inverter(int iterations) {
    auto ugraph = xnetwork::SimpleGraph(5);
    ugraph.add_edge(3, 1);  // n0-p1
    ugraph.add_edge(3, 0);  // n0-a0
    ugraph.add_edge(4, 0);  // n1-a0
    ugraph.add_edge(4, 2);  // n1-p2

    auto netlist = SimpleNetlist(ugraph, 3u, 2u);

    py::dict<uint32_t, int> weight;
    for (uint32_t i = 0; i < 3; ++i) weight[i] = 1;

    vector<double> times;
    times.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        py::set<uint32_t> coverset;
        auto start = Clock::now();
        auto [sol, cost] = min_hyper_vertex_cover(netlist, weight, coverset);
        auto end = Clock::now();
        times.push_back(Duration(end - start).count());
    }

    sort(times.begin(), times.end());
    double sum = 0;
    for (double t : times) sum += t;
    double mean = sum / iterations;

    cout << "min_hyper_vertex_cover (inverter netlist, unit weights):\n";
    cout << "  Min: " << times.front() << " ms, Max: " << times.back()
         << " ms, Mean: " << mean << " ms\n";

    return mean;
}

// -----------------------------------------------------------------------
// min_maximal_matching on inverter netlist
// -----------------------------------------------------------------------
auto bench_maximal_matching_inverter(int iterations) {
    auto ugraph = xnetwork::SimpleGraph(5);
    ugraph.add_edge(3, 1);
    ugraph.add_edge(3, 0);
    ugraph.add_edge(4, 0);
    ugraph.add_edge(4, 2);

    auto netlist = SimpleNetlist(ugraph, 3u, 2u);

    py::dict<uint32_t, unsigned int> weight;
    for (uint32_t i = 3; i < 5; ++i) weight[i] = 1;

    vector<double> times;
    times.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        py::set<uint32_t> matchset;
        py::set<uint32_t> dep;
        auto start = Clock::now();
        [[maybe_unused]] auto cost = min_maximal_matching(netlist, weight, matchset, dep);
        auto end = Clock::now();
        times.push_back(Duration(end - start).count());
    }

    sort(times.begin(), times.end());
    double sum = 0;
    for (double t : times) sum += t;
    double mean = sum / iterations;

    cout << "min_maximal_matching (inverter netlist, unit weights):\n";
    cout << "  Min: " << times.front() << " ms, Max: " << times.back()
         << " ms, Mean: " << mean << " ms\n";

    return mean;
}

// -----------------------------------------------------------------------
// Yosys JSON parsing performance
// -----------------------------------------------------------------------
double bench_yosys_dom(const string& path, int iterations) {
    vector<double> times;
    times.reserve(iterations);
    for (int i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        auto result = read_yosys_json(path);
        auto end = Clock::now();
        (void)result;
        times.push_back(Duration(end - start).count());
    }
    sort(times.begin(), times.end());
    double sum = 0;
    for (double t : times) sum += t;
    return sum / iterations;
}

double bench_yosys_sax(const string& path, int iterations) {
    vector<double> times;
    times.reserve(iterations);
    for (int i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        auto result = read_yosys_json_sax(path);
        auto end = Clock::now();
        (void)result;
        times.push_back(Duration(end - start).count());
    }
    sort(times.begin(), times.end());
    double sum = 0;
    for (double t : times) sum += t;
    return sum / iterations;
}

int main() {
    const int iters = 100;
    cout << fixed << setprecision(4);
    cout << "=== Cross-Project Benchmarks (netlistx-cpp / xnetwork-cpp) ===\n\n";

    auto vc_time = bench_vertex_cover_fast_line(iters);
    cout << "\n";
    auto hvc_time = bench_hyper_vertex_cover_inverter(iters);
    cout << "\n";
    auto match_time = bench_maximal_matching_inverter(iters);
    cout << "\n";

    cout << "=== Yosys JSON Parsing ===\n";
    struct { string path; string label; } files[] = {
        {"../../testcases/yosys_and2.json", "yosys_and2.json (tiny)"},
        {"../../yosys_testcases/sphere_netlist.json", "sphere_netlist.json (482 KB)"},
        {"../../yosys_testcases/sphere3hopf_netlist_simple.json",
         "sphere3hopf_netlist_simple.json (528 KB)"},
    };

    for (auto& f : files) {
        auto dom = bench_yosys_dom(f.path, 50);
        auto sax = bench_yosys_sax(f.path, 50);
        cout << left << setw(40) << f.label
             << "DOM: " << setw(10) << dom << " ms"
             << " SAX: " << setw(10) << sax << " ms"
             << " Speedup: " << (dom / sax) << "x\n";
    }

    cout << "\n=== Summary ===\n";
    cout << "min_vertex_cover_fast (line): " << vc_time << " ms\n";
    cout << "min_hyper_vertex_cover (inverter): " << hvc_time << " ms\n";
    cout << "min_maximal_matching (inverter): " << match_time << " ms\n";

    return 0;
}
