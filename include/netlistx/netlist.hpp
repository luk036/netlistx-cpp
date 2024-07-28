#pragma once

#include <cstddef>           // for size_t
#include <cstdint>           // for uint32_t, uint8_t
#include <py2cpp/dict.hpp>   // for dict
#include <py2cpp/range.hpp>  // for range, _iterator, iterable_wra...
#include <py2cpp/set.hpp>    // for set
#include <type_traits>       // for move
#include <vector>            // for vector

// using node_t = int;

// struct PartInfo
// {
//     std::vector<std::uint8_t> part;
//     py::set<node_t> extern_nets;
// };

/**
 * @brief Represents a netlist, which is implemented using a graph-like data structure.
 *
 * The `Netlist` struct contains various properties and data structures to represent the netlist,
 * including:
 * - `gr`: The underlying graph-like data structure used to represent the netlist.
 * - `modules`: A view of the module nodes in the graph.
 * - `nets`: A view of the net nodes in the graph.
 * - `num_modules`: The number of module nodes in the netlist.
 * - `num_nets`: The number of net nodes in the netlist.
 * - `num_pads`: The number of pad nodes in the netlist.
 * - `max_degree`: The maximum degree of any node in the netlist.
 * - `max_net_degree`: The maximum degree of any net node in the netlist.
 * - `module_weight`: A vector of weights for each module node.
 * - `has_fixed_modules`: A flag indicating whether the netlist has any fixed module nodes.
 * - `module_fixed`: A set of fixed module nodes.
 */
template <typename graph_t> struct Netlist {
  using nodeview_t = typename graph_t::nodeview_t;
  using node_t = typename graph_t::node_t;
  using index_t = typename nodeview_t::key_type;
  // using graph_t = xnetwork::Graph<graph_t>;

  graph_t gr;
  nodeview_t modules;
  nodeview_t nets;
  size_t num_modules{};
  size_t num_nets{};
  size_t num_pads = 0U;
  size_t max_degree{};
  size_t max_net_degree{};
  // std::uint8_t cost_model = 0;
  std::vector<unsigned int> module_weight;
  bool has_fixed_modules{};
  py::set<node_t> module_fixed;

public:
  /**
   * @brief Constructs a new Netlist object.
   *
   * This constructor initializes a Netlist object with the provided graph, module nodes, and net
   * nodes.
   *
   * @param[in] gr The graph representing the netlist.
   * @param[in] modules The module nodes in the netlist.
   * @param[in] nets The net nodes in the netlist.
   */
  Netlist(graph_t gr, const nodeview_t &modules, const nodeview_t &nets);

  /**
   * @brief Construct a new Netlist object
   *
   * @param[in] gr The graph representing the netlist.
   * @param[in] numModules The number of modules in the netlist.
   * @param[in] numNets The number of nets in the netlist.
   */
  Netlist(graph_t gr, uint32_t numModules, uint32_t numNets);

  /// Returns an iterator to the beginning of the modules nodeview.
  auto begin() const { return this->modules.begin(); }

  /// Returns an iterator to the end of the modules nodeview.
  auto end() const { return this->modules.end(); }

  /**
   * @brief Get the number of modules in the netlist.
   *
   * @return The number of modules in the netlist.
   */
  auto number_of_modules() const -> size_t { return this->num_modules; }

  /**
   * @brief Get the number of nets in the netlist.
   *
   * @return The number of nets in the netlist.
   */
  auto number_of_nets() const -> size_t { return this->num_nets; }

  /**
   * @brief Get the number of nodes in the netlist graph.
   *
   * @return The number of nodes in the netlist graph.
   */
  auto number_of_nodes() const -> size_t { return this->gr.number_of_nodes(); }

  // /**
  //  * @brief
  //  *
  //  * @return index_t
  //  */
  // auto number_of_pins() const -> index_t { return
  // this->gr.number_of_edges(); }

  /**
   * @brief Get the maximum degree of any node in the netlist.
   *
   * @return The maximum degree of any node in the netlist.
   */
  auto get_max_degree() const -> size_t { return this->max_degree; }

  /**
   * @brief Get the max net degree
   *
   * @return size_t The maximum degree of any net in the netlist.
   */
  auto get_max_net_degree() const -> size_t { return this->max_net_degree; }

  /**
   * @brief Get the module weight
   *
   * @param[in] v The module node
   * @return unsigned int The weight of the module
   */
  auto get_module_weight(const node_t &v) const -> unsigned int {
    return this->module_weight.empty() ? 1U : this->module_weight[v];
  }

  /**
   * @brief Get the net weight
   *
   * @param[in] net The net to get the weight for
   * @return uint32_t The weight of the net
   */
  auto get_net_weight(const node_t & /*net*/) const -> uint32_t {
    // return this->net_weight.is_empty() ? 1
    //                                 :
    //                                 this->net_weight[this->net_map[net]];
    return 1U;
  }
};

/**
 * @brief Constructs a Netlist object from the given graph, module nodes, and net nodes.
 *
 * @param gr The graph representing the netlist.
 * @param modules The module nodes in the netlist.
 * @param nets The net nodes in the netlist.
 *
 * The constructor initializes the Netlist object with the provided graph, modules, and nets. It
 * also calculates the maximum degree of the modules and nets, and sets flags indicating whether the
 * modules have fixed positions.
 */
template <typename graph_t>
Netlist<graph_t>::Netlist(graph_t gr, const nodeview_t &modules, const nodeview_t &nets)
    : gr{std::move(gr)},
      modules{modules},
      nets{nets},
      num_modules(modules.size()),
      num_nets(nets.size()) {
  this->has_fixed_modules = (!this->module_fixed.empty());

  // Some compilers does not accept py::range()->iterator as a forward
  // iterator auto deg_cmp = [this](const node_t& v, const node_t& w) ->
  // index_t {
  //     return this->gr.degree(v) < this->gr.degree(w);
  // };
  // const auto result1 =
  //     std::max_element(this->modules.begin(), this->modules.end(),
  //     deg_cmp);
  // this->max_degree = this->gr.degree(*result1);
  // const auto result2 =
  //     std::max_element(this->nets.begin(), this->nets.end(), deg_cmp);
  // this->max_net_degree = this->gr.degree(*result2);

  this->max_degree = 0U;
  for (const auto &v : this->modules) {
    if (this->max_degree < this->gr.degree(v)) {
      this->max_degree = this->gr.degree(v);
    }
  }

  this->max_net_degree = 0U;
  for (const auto &net : this->nets) {
    if (this->max_net_degree < this->gr.degree(net)) {
      this->max_net_degree = this->gr.degree(net);
    }
  }
}

template <typename graph_t>
Netlist<graph_t>::Netlist(graph_t gr, uint32_t numModules, uint32_t numNets)
    : Netlist{std::move(gr), py::range(numModules), py::range(numModules, numModules + numNets)} {}

#include <xnetwork/classes/graph.hpp>  // for Graph, Graph<>::nodeview_t

// using RngIter = decltype(py::range(1));
using graph_t = xnetwork::SimpleGraph;
using index_t = uint32_t;
using SimpleNetlist = Netlist<graph_t>;

template <typename Node> struct Snapshot {
  py::set<Node> extern_nets;
  py::dict<index_t, std::uint8_t> extern_modules;
};
