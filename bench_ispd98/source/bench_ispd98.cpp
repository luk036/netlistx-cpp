#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <netlistx/cover.hpp>
#include <netlistx/netlist.hpp>
#include <netlistx/netlist_algo.hpp>
#include <netlistx/rand_cover.hpp>
#include <netlistx/readwrite.hpp>

namespace {

using std::string;

template <typename Fn> auto timed_ms(Fn&& fn) -> double {
    const auto t0 = std::chrono::steady_clock::now();
    std::forward<Fn>(fn)();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

struct Result {
    string name;
    bool ok{false};
    size_t modules{0};
    size_t nets{0};
    size_t pd_size{0};
    size_t greedy_size{0};
    size_t pitt1_size{0};
    size_t pitt64_size{0};
    unsigned long long pd_cost{0};
    unsigned long long greedy_cost{0};
    unsigned long long pitt1_cost{0};
    unsigned long long pitt64_cost{0};
    double pd_ms{0.0};
    double greedy_ms{0.0};
    double pitt1_ms{0.0};
    double pitt64_ms{0.0};
};

auto exists(const string& path) -> bool {
    std::ifstream f(path);
    return f.good();
}

auto run(const string& name, const string& net_path, const string& are_path) -> Result {
    Result r;
    r.name = name;
    try {
        auto nl = readNetD(net_path);
        readAre(nl, are_path);
        r.modules = nl.number_of_modules();
        r.nets = nl.number_of_nets();

        py::dict<uint32_t, unsigned int> weight;
        py::dict<uint32_t, unsigned int> weight_pos;
        for (uint32_t v = 0; v < r.modules; ++v) {
            const auto w = nl.get_module_weight(v);
            weight[v] = w;
            weight_pos[v] = std::max<unsigned int>(1U, w);
        }

        if (r.modules <= 50000U) {
            py::set<uint32_t> cover;
            std::pair<py::set<uint32_t>, unsigned int> res;
            r.pd_ms = timed_ms([&] { res = min_hyper_vertex_cover(nl, weight, cover); });
            r.pd_size = res.first.size();
            r.pd_cost = res.second;
        }

        {
            py::set<uint32_t> cover;
            r.greedy_ms = timed_ms([&] { r.greedy_cost = min_vertex_cover(nl, weight, cover); });
            r.greedy_size = cover.size();
        }

        if (r.modules <= 50000U) {
            r.pitt1_ms = timed_ms([&] {
                auto res = rand_hyper_vertex_cover(nl, weight_pos, std::optional<unsigned int>{42});
                r.pitt1_size = res.first.size();
                r.pitt1_cost = res.second;
            });

            r.pitt64_ms = timed_ms([&] {
                auto res = rand_hyper_vertex_cover_mt(nl, weight_pos, 64, 0);
                r.pitt64_size = res.first.size();
                r.pitt64_cost = res.second;
            });
        }

        r.ok = true;
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << name << ": " << ex.what() << "\n";
    }
    return r;
}

}

int main(int argc, char** argv) {
    const string base = (argc > 1) ? string(argv[1]) : string("testcases");

    const struct {
        string name;
        string net;
        string are;
    } cases[] = {
        {"ibm01", base + "/ibm01.net", base + "/ibm01.are"},
        {"ibm02", base + "/ibm02.net", base + "/ibm02.are"},
        {"ibm03", base + "/ibm03.netD", base + "/ibm03.are"},
        {"ibm18", base + "/ibm18.net", base + "/ibm18.are"},
    };

    std::vector<Result> results;
    for (const auto& c : cases) {
        if (!exists(c.net) || !exists(c.are)) {
            std::cerr << "[skip] " << c.name << " (missing " << c.net << " or " << c.are << ")\n";
            continue;
        }
        std::cerr << "[run ] " << c.name << " ...\n";
        results.push_back(run(c.name, c.net, c.are));
    }

    std::cout << "| Circuit | modules | nets | PD cover | PD cost | PD ms | greedy cost | "
                 "greedy ms | Pitt(1) cost | Pitt(64) cost | Pitt(64) ms |\n";
    std::cout << "|:--------|--------:|-----:|---------:|--------:|------:|------------:|"
                 "---------:|-------------:|--------------:|------------:|\n";
    for (const auto& r : results) {
        if (!r.ok) {
            std::cout << "| " << r.name << " | FAILED | | | | | | | | | |\n";
            continue;
        }
        const auto show = [](unsigned long long value, size_t size) -> string {
            return size == 0 ? string("-") : std::to_string(value);
        };
        const auto show_ms = [](double ms, size_t size) -> string {
            return size == 0 ? string("-") : std::to_string(ms);
        };
        std::cout << "| " << r.name << " | " << r.modules << " | " << r.nets << " | "
                  << (r.pd_size == 0 ? string("-") : std::to_string(r.pd_size)) << " | "
                  << show(r.pd_cost, r.pd_size) << " | " << show_ms(r.pd_ms, r.pd_size) << " | "
                  << r.greedy_cost << " | " << r.greedy_ms << " | "
                  << show(r.pitt1_cost, r.pitt1_size) << " | "
                  << show(r.pitt64_cost, r.pitt64_size) << " | "
                  << show_ms(r.pitt64_ms, r.pitt64_size) << " |\n";
    }
    return 0;
}
