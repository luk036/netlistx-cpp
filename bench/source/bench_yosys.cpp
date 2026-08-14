#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <cstdint>
#include <iostream>
#include <netlistx/readwrite.hpp>
#include <string>
#include <string_view>

using namespace std;

int main() {
    struct {
        string path;
        string label;
    } files[] = {
        {"../../testcases/yosys_and2.json", "yosys_and2.json (small)"},
        {"../../yosys_testcases/sphere_netlist.json", "sphere_netlist.json (482 KB)"},
        {"../../yosys_testcases/sphere3hopf_netlist_simple.json",
         "sphere3hopf_netlist_simple.json (528 KB)"},
    };

    ankerl::nanobench::Bench bench;
    bench.title("Yosys JSON parsing").unit("op").warmup(100).epochs(50).minEpochIterations(10);

    for (auto& f : files) {
        auto netlist = read_yosys_json(f.path);
        cout << f.label << ": modules=" << netlist.number_of_modules()
             << " nets=" << netlist.number_of_nets() << '\n';

        bench.run("DOM " + f.label, [&] {
            auto result = read_yosys_json(f.path);
            ankerl::nanobench::doNotOptimizeAway(result);
        });

        bench.run("SAX " + f.label, [&] {
            auto result = read_yosys_json_sax(f.path);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    return 0;
}
