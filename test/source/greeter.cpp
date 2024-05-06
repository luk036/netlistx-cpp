#include <doctest/doctest.h>
#include <netlistx/greeter.h>
#include <netlistx/version.h>

#include <string>

TEST_CASE("NetlistX") {
  using namespace netlistx;

  NetlistX netlistx("Tests");

  CHECK(netlistx.greet(LanguageCode::EN) == "Hello, Tests!");
  CHECK(netlistx.greet(LanguageCode::DE) == "Hallo Tests!");
  CHECK(netlistx.greet(LanguageCode::ES) == "¡Hola Tests!");
  CHECK(netlistx.greet(LanguageCode::FR) == "Bonjour Tests!");
}

TEST_CASE("NetlistX version") {
  static_assert(std::string_view(NETLISTX_VERSION) == std::string_view("1.0"));
  CHECK(std::string(NETLISTX_VERSION) == std::string("1.0"));
}
