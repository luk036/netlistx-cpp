#include <doctest/doctest.h>  // for TestCase, TEST_CASE

#include <netlistx/netlist.hpp>       // for Netlist, Netlist<>::nodeview_t
#include <netlistx/netlist_algo.hpp>  // for min_maximal_matching, min_vertex_...
#include <py2cpp/dict.hpp>            // for dict
#include <py2cpp/range.hpp>           // for _iterator, iterable_wrapper
#include <py2cpp/set.hpp>             // for set
#include <string_view>                // for std::string_view

using namespace std;

extern auto create_test_netlist() -> SimpleNetlist;  // import create_test_netlist
extern auto create_dwarf() -> SimpleNetlist;         // import create_dwarf
extern auto readNetD(std::string_view netDFileName) -> SimpleNetlist;
extern void readAre(SimpleNetlist& hyprgraph, std::string_view areFileName);

using node_t = SimpleNetlist::node_t;

TEST_CASE("Test min_vertex_cover dwarf") {
    const auto hyprgraph = create_dwarf();
    py::dict<node_t, unsigned int> weight{};
    py::set<node_t> covset{};
    for (auto node : hyprgraph) {
        weight[node] = 1;
    }
    const auto cost = min_vertex_cover(hyprgraph, weight, covset);

    for (const auto& net : hyprgraph.nets) {
        bool covered = false;
        for (const auto& v : hyprgraph.gr[net]) {
            if (covset.contains(v)) {
                covered = true;
                break;
            }
        }
        CHECK_MESSAGE(covered, "Net ", net, " should be covered by min vertex cover");
    }
    CHECK_GT(cost, 0U);
}

//
// Primal-dual algorithm for minimum vertex cover problem
//

TEST_CASE("Test min_maximal_matching dwarf") {
    const auto hyprgraph = create_dwarf();
    py::dict<node_t, unsigned int> weight{};
    py::set<node_t> matchset{};
    py::set<node_t> dep{};
    for (auto net : hyprgraph.nets) {
        weight[net] = 1;
    }
    const auto cost = min_maximal_matching(hyprgraph, weight, matchset, dep);

    for (const auto& net : hyprgraph.nets) {
        if (matchset.contains(net)) {
            continue;
        }
        bool covered_by_dep = false;
        for (const auto& v : hyprgraph.gr[net]) {
            if (dep.contains(v)) {
                covered_by_dep = true;
                break;
            }
        }
        CHECK_MESSAGE(covered_by_dep, "Net ", net, " should be covered by dep in maximal matching");
    }
    for (const auto& n : matchset) {
        CHECK_GE(n, hyprgraph.number_of_modules());
        CHECK_LT(n, hyprgraph.number_of_modules() + hyprgraph.number_of_nets());
    }
    CHECK_GT(cost, 0U);
}

TEST_CASE("Test min_maximal_matching convenience overload") {
    const auto hyprgraph = create_dwarf();
    py::dict<node_t, unsigned int> weight{};
    for (auto net : hyprgraph.nets) {
        weight[net] = 1;
    }
    const auto [matchset, cost] = min_maximal_matching(hyprgraph, weight);

    for (const auto& net : hyprgraph.nets) {
        bool covered = matchset.contains(net);
        if (!covered) {
            for (const auto& net2 : hyprgraph.nets) {
                if (matchset.contains(net2)) {
                    for (const auto& v : hyprgraph.gr[net2]) {
                        for (const auto& v2 : hyprgraph.gr[net]) {
                            if (v == v2) {
                                covered = true;
                                break;
                            }
                        }
                        if (covered) break;
                    }
                }
                if (covered) break;
            }
        }
        CHECK_MESSAGE(covered, "Net ", net, " should be covered in maximal matching");
    }
    CHECK_GT(cost, 0U);
    CHECK_FALSE(matchset.empty());
}
