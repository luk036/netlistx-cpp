#include <algorithm>
// #include <range/v3/algorithm/any_of.hpp>
// #include <range/v3/algorithm/min_element.hpp>
#include <tuple>

/**
 * @brief Solves the minimum weighted vertex cover problem using the primal-dual paradigm.
 *
 * This function takes a hypergraph, a weight function, and a set of pre-covered vertices. It
 * computes the minimum weighted vertex cover by iterating over the hypergraph's nets and
 * selecting the vertex with the minimum weight gap to add to the cover set. The total primal
 * and dual costs are computed and returned.
 *
 * @tparam Gnl The type of the hypergraph.
 * @tparam C1 The type of the weight function.
 * @tparam C2 The type of the cover set.
 * @param hyprgraph The input hypergraph.
 * @param weight The weight function.
 * @param[in,out] coverset The set of pre-covered vertices, which will be updated with the
 *                         solution set.
 * @return The total primal cost of the minimum weighted vertex cover.
 */
template <typename Gnl, typename C1, typename C2>
auto min_vertex_cover(const Gnl &hyprgraph, const C1 &weight, C2 &coverset) ->
    typename C1::mapped_type {
  using T = typename C1::mapped_type;
  auto in_coverset = [&](const auto &v) { return coverset.contains(v); };
  auto total_dual_cost = T(0);
  static_assert(sizeof total_dual_cost >= 0, "maybe unused");
  auto total_primal_cost = T(0);
  auto gap = weight;
  for (const auto &net : hyprgraph.nets) {
    if (std::any_of(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(), in_coverset)) {
      continue;
    }

    auto min_vtx
        = *std::min_element(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(),
                            [&](const auto &v1, const auto &v2) { return gap[v1] < gap[v2]; });
    auto min_val = gap[min_vtx];
    coverset.insert(min_vtx);
    total_primal_cost += weight[min_vtx];
    total_dual_cost += min_val;
    for (const auto &u : hyprgraph.gr[net]) {
      gap[u] -= min_val;
    }
  }

  assert(total_dual_cost <= total_primal_cost);
  return total_primal_cost;
}

/**
 * @brief Solves the minimum weighted maximal matching problem using the primal-dual paradigm.
 *
 * This function takes a hypergraph, a weight function, and two output parameters: a matching set
 * and a dependency set. It computes the minimum weighted maximal matching by iterating over the
 * hypergraph's nets and greedily selecting the minimum-weight net that does not conflict with the
 * current dependency set. The total primal cost of the minimum weighted maximal matching is
 * returned.
 *
 * @tparam Gnl The type of the hypergraph.
 * @tparam C1 The type of the weight function.
 * @tparam C2 The type of the matching set and dependency set.
 * @param hyprgraph The input hypergraph.
 * @param weight The weight function.
 * @param[in,out] matchset The output matching set.
 * @param[in,out] dep The output dependency set.
 * @return The total primal cost of the minimum weighted maximal matching.
 */
template <typename Gnl, typename C1, typename C2>
auto min_maximal_matching(const Gnl &hyprgraph, const C1 &weight, C2 &matchset, C2 &dep) ->
    typename C1::mapped_type {
  auto cover = [&](const auto &net) {
    for (const auto &v : hyprgraph.gr[net]) {
      dep.insert(v);
    }
  };

  auto in_dep = [&](const auto &v) { return dep.contains(v); };

  // auto any_of_dep = [&](const auto& net) {
  //     return ranges::any_of(
  //         hyprgraph.gr[net], [&](const auto& v) { return dep.contains(v); });
  // };

  using T = typename C1::mapped_type;

  auto gap = weight;
  auto total_dual_cost = T(0);
  static_assert(sizeof total_dual_cost >= 0, "maybe unused");
  auto total_primal_cost = T(0);
  for (const auto &net : hyprgraph.nets) {
    if (std::any_of(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(), in_dep)) {
      continue;
    }
    if (matchset.contains(net)) {  // pre-define independant
      cover(net);
      continue;
    }
    auto min_val = gap[net];
    auto min_net = net;
    for (const auto &v : hyprgraph.gr[net]) {
      for (const auto &net2 : hyprgraph.gr[v]) {
        if (std::any_of(hyprgraph.gr[net2].begin(), hyprgraph.gr[net2].end(), in_dep)) {
          continue;
        }
        if (min_val > gap[net2]) {
          min_val = gap[net2];
          min_net = net2;
        }
      }
    }
    cover(min_net);
    matchset.insert(min_net);
    total_primal_cost += weight[min_net];
    total_dual_cost += min_val;
    if (min_net != net) {
      gap[net] -= min_val;
      for (const auto &v : hyprgraph.gr[net]) {
        for (const auto &net2 : hyprgraph.gr[v]) {
          gap[net2] -= min_val;
        }
      }
    }
  }
  // assert(total_dual_cost <= total_primal_cost);
  return total_primal_cost;
}
