#include <netlistx/netlist.hpp>

// Explicit instantiation of Netlist for xnetwork::SimpleGraph (i.e. SimpleNetlist).
// Ensures the template is compiled in this translation unit rather than in every header include.
template struct Netlist<xnetwork::SimpleGraph>;
