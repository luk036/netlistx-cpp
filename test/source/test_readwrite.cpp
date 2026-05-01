// -*- coding: utf-8 -*-
#include <doctest/doctest.h>  // for ResultBuilder, CHECK, TestCase, TEST_CASE
// #include <__config>            // for std
#include <netlistx/netlist.hpp>  // for Netlist, SimpleNetlist
#include <string_view>           // for std::string_view

using namespace std;

extern auto readNetD(std::string_view netDFileName) -> SimpleNetlist;
extern void readAre(SimpleNetlist& hyprgraph, std::string_view areFileName);
extern void writeJSON(std::string_view jsonFileName, const SimpleNetlist& hyprgraph);

TEST_CASE("Test Read Dwarf") {
    auto hyprgraph = readNetD("../../testcases/dwarf1.netD");
    readAre(hyprgraph, "../../testcases/dwarf1.are");

    CHECK_EQ(hyprgraph.number_of_modules(), 7);
    CHECK_EQ(hyprgraph.number_of_nets(), 5);
    // CHECK_EQ(hyprgraph.number_of_pins(), 13);
    CHECK_EQ(hyprgraph.get_max_degree(), 3);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 3);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 2);
}

TEST_CASE("Test Read p1") {
    const auto hyprgraph = readNetD("../../testcases/p1.net");

    CHECK_EQ(hyprgraph.number_of_modules(), 833);
    CHECK_EQ(hyprgraph.number_of_nets(), 902);
    // CHECK_EQ(hyprgraph.number_of_pins(), 2908);
    CHECK_EQ(hyprgraph.get_max_degree(), 9);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 18);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 1);
}

TEST_CASE("Test Read ibm01") {
    const auto hyprgraph = readNetD("../../testcases/ibm01.net");

    CHECK_EQ(hyprgraph.number_of_modules(), 12752);
    CHECK_EQ(hyprgraph.number_of_nets(), 14111);
    // CHECK_EQ(hyprgraph.number_of_pins(), 2908);
    CHECK_EQ(hyprgraph.get_max_degree(), 39);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 42);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 1);
}

TEST_CASE("Test Read ibm02") {
    auto hyprgraph = readNetD("../../testcases/ibm02.netD");
    readAre(hyprgraph, "../../testcases/ibm02.are");

    CHECK_EQ(hyprgraph.number_of_modules(), 19601);
    CHECK_EQ(hyprgraph.number_of_nets(), 19584);
    // CHECK_EQ(hyprgraph.number_of_pins(), 2908);
    CHECK_EQ(hyprgraph.get_max_degree(), 69);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 134);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 64);
}

TEST_CASE("Test Read ibm03") {
    auto hyprgraph = readNetD("../../testcases/ibm03.netD");
    readAre(hyprgraph, "../../testcases/ibm03.are");

    CHECK_EQ(hyprgraph.number_of_modules(), 23136);
    CHECK_EQ(hyprgraph.number_of_nets(), 27401);
    // CHECK_EQ(hyprgraph.number_of_pins(), 2908);
    CHECK_EQ(hyprgraph.get_max_degree(), 100);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 55);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 96);
}

// TEST_CASE("Test Read ibm18") {
//     const auto hyprgraph = readNetD("../../testcases/ibm18.net");
//
//     CHECK_EQ(hyprgraph.number_of_modules(), 210613);
//     CHECK_EQ(hyprgraph.number_of_nets(), 201920);
//     // CHECK_EQ(hyprgraph.number_of_pins(), 2908);
//     CHECK_EQ(hyprgraph.get_max_degree(), 97);
//     CHECK_EQ(hyprgraph.get_max_net_degree(), 66);
//     CHECK_FALSE(hyprgraph.has_fixed_modules);
//     CHECK_EQ(hyprgraph.get_module_weight(1), 1);
// }

TEST_CASE("Test Write Dwarf") {
    auto hyprgraph = readNetD("../../testcases/dwarf1.netD");
    readAre(hyprgraph, "../../testcases/dwarf1.are");
    writeJSON("../../testcases/dwarf1.json", hyprgraph);

    CHECK_EQ(hyprgraph.number_of_modules(), 7);
    CHECK_EQ(hyprgraph.number_of_nets(), 5);
    // CHECK_EQ(hyprgraph.number_of_pins(), 13);
}

TEST_CASE("Test Write p1") {
    const auto hyprgraph = readNetD("../../testcases/p1.net");
    writeJSON("../../testcases/p1.json", hyprgraph);
}