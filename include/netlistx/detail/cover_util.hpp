#pragma once

/**
 * @file cover_util.hpp
 * @brief Internal helpers shared by the hypergraph covering algorithms
 *
 * Free templates that de-duplicate the "is a net covered?" scans and the
 * reverse-delete post-processing pass used by <netlistx/netlist_algo.hpp>,
 * <netlistx/cover.hpp> and <netlistx/rand_cover.hpp>. They live in
 * netlistx::detail and are not part of the public algorithm API.
 */

#include <utility>  // for forward
#include <vector>   // for vector

namespace netlistx::detail {

    /**
     * @brief True if some vertex of a net is already in the solution set.
     *
     * @tparam Hypergraph Graph-like type exposing gr[net] adjacency ranges
     * @tparam Net Net (hyperedge) id type
     * @tparam SolutionSet Cover set with a contains() query
     * @param[in] hyprgraph The hypergraph
     * @param[in] net The net to test
     * @param[in] soln The cover set to test against
     * @return true if at least one vertex of the net belongs to @p soln
     */
    template <typename Hypergraph, typename Net, typename SolutionSet>
    auto net_is_covered(const Hypergraph& hyprgraph, const Net& net, const SolutionSet& soln)
        -> bool {
        for (const auto& vtx : hyprgraph.gr[net]) {
            if (soln.contains(vtx)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief True if every non-empty net has a vertex in the solution set.
     *
     * Empty nets are considered covered (they contain no vertex to cover).
     *
     * @tparam Hypergraph Graph-like type exposing nets and gr[net] adjacency ranges
     * @tparam SolutionSet Cover set with a contains() query
     * @param[in] hyprgraph The hypergraph
     * @param[in] soln The cover set to test against
     * @return true if the solution covers all non-empty nets
     */
    template <typename Hypergraph, typename SolutionSet>
    auto all_nets_covered(const Hypergraph& hyprgraph, const SolutionSet& soln) -> bool {
        for (const auto& net : hyprgraph.nets) {
            if (!net_is_covered(hyprgraph, net, soln) && !hyprgraph.gr[net].empty()) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Reverse-delete post-processing for a covering solution.
     *
     * Iterates the vertices in reverse insertion order, temporarily removing each
     * one and keeping the removal only while the solution remains valid.
     *
     * @tparam Node Vertex type
     * @tparam SolutionSet Cover set supporting erase()/insert()
     * @tparam Validator Callable returning true if the current solution is valid
     * @param[in,out] soln Mutable cover set (modified in place)
     * @param[in] added_order Vertices in the order they were added
     * @param[in] is_valid Validation callable, invoked after each removal
     */
    template <typename Node, typename SolutionSet, typename Validator>
    void reverse_delete(SolutionSet& soln, const std::vector<Node>& added_order,
                        Validator&& is_valid) {
        for (auto it = added_order.rbegin(); it != added_order.rend(); ++it) {
            soln.erase(*it);
            if (!std::forward<Validator>(is_valid)()) {
                soln.insert(*it);
            }
        }
    }

}  // namespace netlistx::detail
