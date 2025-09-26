// -*- coding: utf-8 -*-
#include <doctest/doctest.h>  // for ResultBuilder, CHECK, TestCase, TEST_CASE
// #include <__config>            // for std
#include <netlistx/netlist.hpp>  // for Netlist, SimpleNetlist
#include <string_view>           // for std::string_view

using namespace std;

extern auto readNetD(std::string_view netDFileName) -> SimpleNetlist;
extern void readAre(SimpleNetlist &hyprgraph, std::string_view areFileName);
extern void writeJSON(std::string_view jsonFileName, const SimpleNetlist &hyprgraph);

TEST_CASE("Test Read Dwarf") {
    auto hyprgraph = readNetD("../../testcases/dwarf1.netD");
    readAre(hyprgraph, "../../testcases/dwarf1.are");

    CHECK(hyprgraph.number_of_modules() == 7);
    CHECK(hyprgraph.number_of_nets() == 5);
    // CHECK(hyprgraph.number_of_pins() == 13);
    CHECK(hyprgraph.get_max_degree() == 3);
    CHECK(hyprgraph.get_max_net_degree() == 3);
    CHECK(!hyprgraph.has_fixed_modules);
    CHECK(hyprgraph.get_module_weight(1) == 2);
}

TEST_CASE("Test Read p1") {
    const auto hyprgraph = readNetD("../../testcases/p1.net");

    CHECK(hyprgraph.number_of_modules() == 833);
    CHECK(hyprgraph.number_of_nets() == 902);
    // CHECK(hyprgraph.number_of_pins() == 2908);
    CHECK(hyprgraph.get_max_degree() == 9);
    CHECK(hyprgraph.get_max_net_degree() == 18);
    CHECK(!hyprgraph.has_fixed_modules);
    CHECK(hyprgraph.get_module_weight(1) == 1);
}

TEST_CASE("Test Read ibm01") {
    const auto hyprgraph = readNetD("../../testcases/ibm01.net");

    CHECK(hyprgraph.number_of_modules() == 12752);
    CHECK(hyprgraph.number_of_nets() == 14111);
    // CHECK(hyprgraph.number_of_pins() == 2908);
    CHECK(hyprgraph.get_max_degree() == 39);
    CHECK(hyprgraph.get_max_net_degree() == 42);
    CHECK(!hyprgraph.has_fixed_modules);
    CHECK(hyprgraph.get_module_weight(1) == 1);
}

TEST_CASE("Test Read ibm02") {
    auto hyprgraph = readNetD("../../testcases/ibm02.netD");
    readAre(hyprgraph, "../../testcases/ibm02.are");

    CHECK(hyprgraph.number_of_modules() == 19601);
    CHECK(hyprgraph.number_of_nets() == 19584);
    // CHECK(hyprgraph.number_of_pins() == 2908);
    CHECK(hyprgraph.get_max_degree() == 69);
    CHECK(hyprgraph.get_max_net_degree() == 134);
    CHECK(!hyprgraph.has_fixed_modules);
    CHECK(hyprgraph.get_module_weight(1) == 64);
}

TEST_CASE("Test Read ibm03") {
    auto hyprgraph = readNetD("../../testcases/ibm03.netD");
    readAre(hyprgraph, "../../testcases/ibm03.are");

    CHECK(hyprgraph.number_of_modules() == 23136);
    CHECK(hyprgraph.number_of_nets() == 27401);
    // CHECK(hyprgraph.number_of_pins() == 2908);
    CHECK(hyprgraph.get_max_degree() == 100);
    CHECK(hyprgraph.get_max_net_degree() == 55);
    CHECK(!hyprgraph.has_fixed_modules);
    CHECK(hyprgraph.get_module_weight(1) == 96);
}


// TEST_CASE("Test Read ibm18") {
//     const auto hyprgraph = readNetD("../../testcases/ibm18.net");
//
//     CHECK(hyprgraph.number_of_modules() == 210613);
//     CHECK(hyprgraph.number_of_nets() == 201920);
//     // CHECK(hyprgraph.number_of_pins() == 2908);
//     CHECK(hyprgraph.get_max_degree() == 97);
//     CHECK(hyprgraph.get_max_net_degree() == 66);
//     CHECK(!hyprgraph.has_fixed_modules);
//     CHECK(hyprgraph.get_module_weight(1) == 1);
// }

TEST_CASE("Test Write Dwarf") {
    auto hyprgraph = readNetD("../../testcases/dwarf1.netD");
    readAre(hyprgraph, "../../testcases/dwarf1.are");
    writeJSON("../../testcases/dwarf1.json", hyprgraph);

    CHECK(hyprgraph.number_of_modules() == 7);
    CHECK(hyprgraph.number_of_nets() == 5);
    // CHECK(hyprgraph.number_of_pins() == 13);
}

TEST_CASE("Test Write p1") {
    const auto hyprgraph = readNetD("../../testcases/p1.net");
    writeJSON("../../testcases/p1.json", hyprgraph);
}