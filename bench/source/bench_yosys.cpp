#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <netlistx/readwrite.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using Clock = chrono::high_resolution_clock;
using Duration = chrono::duration<double, milli>;

struct BenchResult {
    string name;
    double min_ms;
    double max_ms;
    double mean_ms;
    size_t modules;
    size_t nets;
};

auto measure(const string& name, const string& path, auto (*fn)(string_view), int iterations)
    -> BenchResult {
    vector<double> times;
    times.reserve(iterations);

    auto netlist = fn(path);
    auto m = netlist.number_of_modules();
    auto n = netlist.number_of_nets();

    for (int i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        auto result = fn(path);
        auto end = Clock::now();
        (void)result;
        times.push_back(Duration(end - start).count());
    }

    sort(times.begin(), times.end());
    double sum = 0;
    for (double t : times) sum += t;
    double mean = sum / static_cast<double>(iterations);

    return {name, times.front(), times.back(), mean, m, n};
}

int main() {
    const int iters = 50;

    struct {
        string path;
        string label;
    } files[] = {
        {"../../testcases/yosys_and2.json", "yosys_and2.json (small)"},
        {"../../yosys_testcases/sphere_netlist.json", "sphere_netlist.json (482 KB)"},
        {"../../yosys_testcases/sphere3hopf_netlist_simple.json",
         "sphere3hopf_netlist_simple.json (528 KB)"},
    };

    cout << fixed << setprecision(2);
    cout << "File                              Method      Min(ms)   Max(ms)   Mean(ms)"
            "   Modules   Nets\n";
    cout << string(90, '-') << '\n';

    for (auto& f : files) {
        auto r1 = measure("DOM", f.path, read_yosys_json, iters);
        auto r2 = measure("SAX", f.path, read_yosys_json_sax, iters);

        cout << left << setw(34) << f.label << setw(12) << "DOM" << setw(10) << r1.min_ms
             << setw(10) << r1.max_ms << setw(10) << r1.mean_ms << setw(10) << r1.modules
             << r1.nets << '\n';
        cout << left << setw(34) << "" << setw(12) << "SAX" << setw(10) << r2.min_ms
             << setw(10) << r2.max_ms << setw(10) << r2.mean_ms << setw(10) << r2.modules
             << r2.nets << '\n';

        double ratio = r1.mean_ms / r2.mean_ms;
        cout << left << setw(34) << "" << setw(12) << (ratio > 1.0 ? "SAX wins" : "DOM wins")
             << "ratio: " << (ratio > 1.0 ? ratio : 1.0 / ratio) << "x" << '\n';
        cout << '\n';
    }

    return 0;
}
