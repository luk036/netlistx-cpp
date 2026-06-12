#include <algorithm>
#include <netlistx/netlist.hpp>
#include <netlistx/netlist_algo.hpp>
#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>
#include <unordered_map>
#include <utility>

// Implementation of min_maximal_matching using a primal-dual approximation.
// Uses dep_count[] for O(1) dependency checks instead of O(deg(net)) scans.
template <typename Gnl, typename C1, typename C2>
auto min_maximal_matching(const Gnl& hyprgraph, const C1& weight, C2& matchset, C2& dep) ->
    typename C1::mapped_type {
    using T = typename C1::mapped_type;

    // dep_count[net] > 0  iff  at least one vertex of net is in dep.
    // Replaces the O(deg(net2)) std::any_of scan with an O(1) dict lookup.
    std::unordered_map<typename Gnl::node_t, int> dep_count;

    auto cover = [&](const auto& net) {
        for (const auto& v : hyprgraph.gr[net]) {
            dep.insert(v);
            for (const auto& net2 : hyprgraph.gr[v]) {
                ++dep_count[net2];
            }
        }
    };

    auto gap = weight;
    [[maybe_unused]] auto total_dual_cost = T(0);
    auto total_primal_cost = T(0);
    for (const auto& net : hyprgraph.nets) {
        if (dep_count[net] > 0) {
            continue;
        }
        if (matchset.contains(net)) {  // pre-define independent
            cover(net);
            continue;
        }
        auto min_val = gap[net];
        auto min_net = net;
        for (const auto& v : hyprgraph.gr[net]) {
            for (const auto& net2 : hyprgraph.gr[v]) {
                if (dep_count[net2] > 0) {
                    continue;
                }
                if (min_val > gap[net2]) {
                    min_val = gap[net2];
                    min_net = net2;
                }
            }
        }
        cover(min_net);
        matchset.insert(min_net);
        total_primal_cost += weight[min_net];
        total_dual_cost += min_val;
        if (min_net != net) {
            gap[net] -= min_val;
            for (const auto& v : hyprgraph.gr[net]) {
                for (const auto& net2 : hyprgraph.gr[v]) {
                    gap[net2] -= min_val;
                }
            }
        }
    }
    // assert(total_dual_cost <= total_primal_cost);
    return total_primal_cost;
}

template <typename Gnl, typename C1>
auto min_maximal_matching(const Gnl& hyprgraph, const C1& weight)
    -> std::pair<py::set<typename Gnl::node_t>, typename C1::mapped_type> {
    py::set<typename Gnl::node_t> matchset{};
    py::set<typename Gnl::node_t> dep{};
    auto cost = min_maximal_matching(hyprgraph, weight, matchset, dep);
    return std::make_pair(matchset, cost);
}

// Explicit instantiations for SimpleNetlist
using node_t = SimpleNetlist::node_t;

template auto min_maximal_matching<SimpleNetlist, py::dict<node_t, unsigned int>, py::set<node_t>>(
    const SimpleNetlist&, const py::dict<node_t, unsigned int>&, py::set<node_t>&, py::set<node_t>&)
    -> unsigned int;

template auto min_maximal_matching<SimpleNetlist, py::dict<node_t, unsigned int>>(
    const SimpleNetlist&, const py::dict<node_t, unsigned int>&)
    -> std::pair<py::set<node_t>, unsigned int>;
