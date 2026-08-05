#include <netlistx/version.h>

#include <iostream>

auto main() -> int {
    const auto ok = (NETLISTX_VERSION_MAJOR >= 1);
    std::cout << "netlistx installed test: version " << NETLISTX_VERSION << "\n";
    return ok ? 0 : 1;
}
