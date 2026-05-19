#include <doctest/doctest.h>

#include <netlistx/netlist.hpp>
#include <netlistx/readwrite.hpp>
#include <sstream>
#include <string_view>

using namespace std;

TEST_CASE("Test Read Yosys JSON") {
    auto hyprgraph = read_yosys_json("../../testcases/yosys_and2.json");

    CHECK_EQ(hyprgraph.number_of_modules(), 5);  // 2 cells + 3 ports
    CHECK_EQ(hyprgraph.number_of_nets(), 4);     // nets 0, 1, 2, 3
    CHECK_EQ(hyprgraph.num_pads, 3);             // 3 I/O ports
    CHECK(hyprgraph.has_fixed_modules);

    CHECK_EQ(hyprgraph.get_module_weight(0), 1);  // and1
    CHECK_EQ(hyprgraph.get_module_weight(1), 1);  // buf1
    CHECK_EQ(hyprgraph.get_module_weight(2), 0);  // port a
    CHECK_EQ(hyprgraph.get_module_weight(3), 0);  // port b
    CHECK_EQ(hyprgraph.get_module_weight(4), 0);  // port y
}

TEST_CASE("Test Read Dwarf") {
    auto hyprgraph = readNetD("../../testcases/dwarf1.netD");
    readAre(hyprgraph, "../../testcases/dwarf1.are");

    CHECK_EQ(hyprgraph.number_of_modules(), 7);
    CHECK_EQ(hyprgraph.number_of_nets(), 5);
    CHECK_EQ(hyprgraph.get_max_degree(), 3);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 3);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 2);
}

TEST_CASE("Test Read p1") {
    const auto hyprgraph = readNetD("../../testcases/p1.net");

    CHECK_EQ(hyprgraph.number_of_modules(), 833);
    CHECK_EQ(hyprgraph.number_of_nets(), 902);
    CHECK_EQ(hyprgraph.get_max_degree(), 9);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 18);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 1);
}

TEST_CASE("Test Read ibm01") {
    const auto hyprgraph = readNetD("../../testcases/ibm01.net");

    CHECK_EQ(hyprgraph.number_of_modules(), 12752);
    CHECK_EQ(hyprgraph.number_of_nets(), 14111);
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
    CHECK_EQ(hyprgraph.get_max_degree(), 100);
    CHECK_EQ(hyprgraph.get_max_net_degree(), 55);
    CHECK_FALSE(hyprgraph.has_fixed_modules);
    CHECK_EQ(hyprgraph.get_module_weight(1), 96);
}

TEST_CASE("Test Write Dwarf") {
    auto hyprgraph = readNetD("../../testcases/dwarf1.netD");
    readAre(hyprgraph, "../../testcases/dwarf1.are");
    writeJSON("../../testcases/dwarf1.json", hyprgraph);

    CHECK_EQ(hyprgraph.number_of_modules(), 7);
    CHECK_EQ(hyprgraph.number_of_nets(), 5);
}

TEST_CASE("Test Write p1") {
    const auto hyprgraph = readNetD("../../testcases/p1.net");
    writeJSON("../../testcases/p1.json", hyprgraph);
}

TEST_CASE("detect_input_format") {
    CHECK_EQ(detect_input_format("test.hgr"), InputFormat::hmetis);
    CHECK_EQ(detect_input_format("test.graph"), InputFormat::hmetis);
    CHECK_EQ(detect_input_format("test.json"), InputFormat::json);
    CHECK_EQ(detect_input_format("test.net"), InputFormat::netD);
    CHECK_EQ(detect_input_format("test.dimacs"), InputFormat::dimacs);
    CHECK_EQ(detect_input_format("unknown"), InputFormat::auto_detect);
}

TEST_CASE("write_hmetis_partition") {
    vector<uint8_t> part = {0, 1, 0, 1, 0};
    ostringstream oss;
    write_hmetis_partition(part, oss);
    CHECK_EQ(oss.str(), "0\n1\n0\n1\n0\n");
}

TEST_CASE("write_json_partition") {
    vector<uint8_t> part = {0, 1, 0, 1, 0};
    ostringstream oss;
    write_json_partition(part, oss);
    CHECK_EQ(oss.str(), "[0, 1, 0, 1, 0]\n");
}

TEST_CASE("write_partition") {
    vector<uint8_t> part = {0, 1, 0, 1, 0};

    ostringstream oss1;
    write_partition(part, oss1, OutputFormat::hmetis);
    CHECK_EQ(oss1.str(), "0\n1\n0\n1\n0\n");

    ostringstream oss2;
    write_partition(part, oss2, OutputFormat::json);
    CHECK_EQ(oss2.str(), "[0, 1, 0, 1, 0]\n");
}
