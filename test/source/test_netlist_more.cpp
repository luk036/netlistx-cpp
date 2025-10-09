#include <doctest/doctest.h>
#include <netlistx/netlist.hpp>

extern auto create_test_netlist() -> SimpleNetlist;

TEST_CASE("Test Netlist degrees") {
    const auto hyprgraph = create_test_netlist();

    CHECK(hyprgraph.gr.degree(0) == 3);
    CHECK(hyprgraph.gr.degree(1) == 2);
    CHECK(hyprgraph.gr.degree(2) == 1);
    CHECK(hyprgraph.gr.degree(3) == 2);
    CHECK(hyprgraph.gr.degree(4) == 3);
    CHECK(hyprgraph.gr.degree(5) == 1);
}

TEST_CASE("Test Netlist module weights") {
    auto hyprgraph = create_test_netlist();

    CHECK(hyprgraph.get_module_weight(0) == 3);
    CHECK(hyprgraph.get_module_weight(1) == 4);
    CHECK(hyprgraph.get_module_weight(2) == 2);

    hyprgraph.set_module_weight(0, 5);
    CHECK(hyprgraph.get_module_weight(0) == 5);
}
