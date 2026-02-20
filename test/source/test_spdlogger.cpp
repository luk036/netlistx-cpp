// -*- coding: utf-8 -*-
#include <doctest/doctest.h>
#include <netlistx/logger.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <fstream>

TEST_CASE("spdlogger integration: Basic logging") {
    std::cout << "Testing spdlogger integration..." << std::endl;

    // Test the wrapper function
    netlistx::log_with_spdlog("Test message from spdlogger integration test");

    // Test multiple log calls
    netlistx::log_with_spdlog("Creating a simple netlist");
    netlistx::log_with_spdlog("Processing netlist data");
    netlistx::log_with_spdlog("Netlist processing completed");

    // Verify log file was created
    std::ifstream log_file("netlistx.log");
    CHECK(log_file.good());
    log_file.close();

    std::cout << "Logged messages to netlistx.log" << std::endl;
}

TEST_CASE("spdlogger integration: Direct spdlog usage") {
    std::cout << "Testing direct spdlog usage..." << std::endl;

    // Test direct spdlog API (control test)
    auto logger = spdlog::basic_logger_mt("direct_test", "direct_test.log");
    logger->set_level(spdlog::level::info);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
    logger->info("Direct spdlog message 1");
    logger->info("Direct spdlog message 2");
    logger->flush();

    // Verify log file was created
    std::ifstream log_file("direct_test.log");
    CHECK(log_file.good());
    log_file.close();

    std::cout << "Direct spdlog test completed" << std::endl;

    // Cleanup
    spdlog::drop("direct_test");
}