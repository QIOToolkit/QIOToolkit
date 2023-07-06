
#include "graph/properties.h"

#include <omp.h>

#include <algorithm>
#include <set>
#include <vector>

#include "utils/exception.h"
namespace graph
{
GraphAttributes get_graph_attributes(const utils::Json& cost_function)
{
  size_t max_nodes_in_term = 0;
  bool slc_term_present = false;
  if (cost_function.IsObject() && cost_function.HasMember("terms"))
  {
    const auto& terms = cost_function["terms"].GetArray();
#ifdef _MSC_VER
    #pragma omp parallel
    {
      // reduction min or max is not implemented yet in all compilers
      size_t max_nodes_in_term_local = 0;
      #pragma omp for
      for (size_t i = 0; i < terms.Size(); i++)
      {
        const auto& term = terms[i];
        if (term.IsObject() && term.HasMember("ids") && term["ids"].IsArray())
        {
          size_t nodes_in_term = (size_t)term["ids"].GetArray().Size();
          max_nodes_in_term_local = nodes_in_term > max_nodes_in_term_local
                                        ? nodes_in_term
                                        : max_nodes_in_term_local;
        }
      }
      #pragma omp critical
      max_nodes_in_term = max_nodes_in_term_local > max_nodes_in_term
                              ? max_nodes_in_term_local
                              : max_nodes_in_term;
    }
#else
    #pragma omp parallel for reduction(max : max_nodes_in_term)
    for (size_t i = 0; i < terms.Size(); i++)
    {
      const auto& term = terms[i];
      if (term.IsObject() && term.HasMember("ids") && term["ids"].IsArray())
      {
        size_t nodes_in_term = (size_t)term["ids"].GetArray().Size();
        max_nodes_in_term = nodes_in_term > max_nodes_in_term
                                ? nodes_in_term
                                : max_nodes_in_term;
      }
    }
#endif
  }
  // For slc terms  maximum possible value of max_nodes_in_term is 2.
  // It is very unlikely to have total max_nodes_in_term from
  // "terms" and "terms_slc" less then 2 for real models.
  // Also max_nodes_in_term is used to select storage type in models,
  // where any value less then 256 will lead to same storage selection.
  if (cost_function.IsObject() && cost_function.HasMember("terms_slc"))
  {
    slc_term_present = true;
    max_nodes_in_term = 2 > max_nodes_in_term ? 2 : max_nodes_in_term;
  }
  return {max_nodes_in_term, slc_term_present};
}

template <class EdgesType>
size_t compute_max_nodes_in_term(EdgesType& edges)
{
  size_t max_nodes_in_term = 0;
#ifdef _MSC_VER
  #pragma omp parallel
  {
    // reduction min or max is not implemented yet in all compilers
    size_t max_nodes_in_term_local = 0;
    #pragma omp for
    for (size_t i = 0; i < edges.size(); i++)
    {
      size_t nodes_in_term = edges[i].node_ids().size();
      max_nodes_in_term_local = nodes_in_term > max_nodes_in_term_local
                                    ? nodes_in_term
                                    : max_nodes_in_term_local;
    }
    #pragma omp critical
    max_nodes_in_term = max_nodes_in_term_local > max_nodes_in_term
                            ? max_nodes_in_term_local
                            : max_nodes_in_term;
  }
#else
  #pragma omp parallel for reduction(max : max_nodes_in_term)
  for (size_t i = 0; i < edges.size(); i++)
  {
    size_t nodes_in_term = edges[i].node_ids().size();
    max_nodes_in_term =
        nodes_in_term > max_nodes_in_term ? nodes_in_term : max_nodes_in_term;
  }
#endif
  return max_nodes_in_term;
}

GraphAttributes get_graph_attributes(
    const std::vector<::graph::CostEdge<double>>& edges)
{
  size_t max_nodes_in_term = compute_max_nodes_in_term(edges);
  bool slc_term_present = false;
  return {max_nodes_in_term, slc_term_present};
}

GraphAttributes get_graph_attributes(const std::vector<FacedGraph::Edge>& edges,
                                     const std::vector<SCL_Term>& slc_terms)
{
  size_t max_nodes_in_term = compute_max_nodes_in_term(edges);
  bool slc_term_present = false;
  if (slc_terms.size() > 0)
  {
    slc_term_present = true;
    max_nodes_in_term = 2 > max_nodes_in_term ? 2 : max_nodes_in_term;
  }
  return {max_nodes_in_term, slc_term_present};
}

size_t get_graph_node_count(const std::vector<::graph::CostEdge<double>>& edges)
{
  std::vector<std::set<int>> nodes;
  nodes.resize(omp_get_max_threads());

  #pragma omp parallel for
  for (size_t i = 0; i < edges.size(); i++)
  {
    std::set<int>& local_nodes = nodes[omp_get_thread_num()];
    for (int node_id : edges[i].node_ids())
    {
      local_nodes.insert(node_id);
    }
  }

  for (size_t i = 1; i < nodes.size(); i++)
  {
    nodes[0].insert(nodes[i].begin(), nodes[i].end());
    std::set<int> deleted;
    std::swap(nodes[i], deleted);
  }
  return nodes[0].size();
}

}  // namespace graph