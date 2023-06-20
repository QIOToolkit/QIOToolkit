// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <set>

#include "utils/component.h"
#include "utils/json.h"
#include "graph/cost_edge.h"
#include "graph/faced_graph.h"
#include "observe/observable.h"

namespace graph
{
class GraphAttributes
{
 public:
  size_t max_nodes_in_term;
  bool slc_term_present;
};

GraphAttributes get_graph_attributes(const utils::Json& cost_function);

GraphAttributes get_graph_attributes(
    const std::vector<::graph::CostEdge<double>>& edges);

GraphAttributes get_graph_attributes(const std::vector<FacedGraph::Edge>& edges,
                                     const std::vector<SCL_Term>& slc_terms);

size_t get_graph_node_count(
    const std::vector<::graph::CostEdge<double>>& edges);

////////////////////////////////////////////////////////////////////////////////
/// graph::Property interface
///
/// An API to calculate properties of a graph `g`. Each implementation is
/// expected to implement a `compute` method that returns the evaluated property
/// as a structure.
///
template <class Graph>
class Property
{
 public:
  virtual ~Property() {}
  virtual utils::Structure compute(const Graph& g) const = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// graph::Size
///
/// Denote the number of nodes and edges in the graph.
template <class Graph>
class Size : public Property<Graph>
{
 public:
  utils::Structure compute(const Graph& g) const override
  {
    utils::Structure s;
    s["nodes"] = g.nodes_size();
    s["edges"] = g.edges_size();
    s[utils::kMaxLocality] = g.get_locality();
    s[utils::kMinLocality] = g.get_min_locality();
    s[utils::kAvgLocality] = g.get_avg_locality();
    s[utils::kTotalLocality] = g.get_sum_coefficient_degrees_total();
    s[utils::kAccumDependentVars] = g.get_accumulated_dependent_vars();
    s[utils::kMaxCouplingMagnitude] = g.get_max_coupling_magnitude();
    s[utils::kMinCouplingMagnitude] = g.get_min_coupling_magnitude();
    return s;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Degree
///
/// Histogram and average degree of the nodes in the graph.
///
template <class Graph>
class Degree : public Property<Graph>
{
 public:
  utils::Structure compute(const Graph& g) const override
  {
    observe::BinWidthDistribution degree(1);
    for (const auto& node : g.nodes())
    {
      degree.record(node.edge_ids().size());
    }
    return degree.render();
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Locality
///
/// Histogram and average number of nodes participating in an edge of the graph.
/// (This is relevant for graph with hyper-edges, i.e., when more than 2 nodes
/// can participate in an edge).
///
template <class Graph>
class Locality : public Property<Graph>
{
 public:
  utils::Structure compute(const Graph& g) const override
  {
    observe::BinWidthDistribution locality(1);
    for (const auto& edge : g.edges())
    {
      locality.record(edge.node_ids().size());
    }
    return locality.render();
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Clustering coefficient (global and average of local).
///
/// NOTE: There are open questions on how to generalize the clustering
/// coefficient for weighter graphs and hyper edges.
///
template <class Graph>
class Clustering : public Property<Graph>
{
 public:
  utils::Structure compute(const Graph& g) const override
  {
    size_t N = g.nodes().size();
    std::vector<std::set<size_t>> neighbors(N);
    for (size_t i = 0; i < N; i++)
    {
      for (size_t edge_id : g.node(i).edge_ids())
      {
        const auto& n = g.edge(edge_id).node_ids();
        neighbors[i].insert(n.begin(), n.end());
      }
      neighbors[i].erase(i);
    }

    // NOTE: This implementation also counts triples/triangles within
    // the same hyperedge.
    size_t total_triples = 0;
    size_t total_triangles = 0;
    observe::Average2 local_coefficient;
    for (size_t i = 0; i < N; i++)
    {
      size_t d = neighbors[i].size();
      if (d < 2)
      {
        local_coefficient.record(0);
        continue;
      }
      size_t triples = d * (d - 1) / 2;
      size_t triangles = 0;
      for (size_t j : neighbors[i])
      {
        for (size_t k : neighbors[i])
        {
          if (k < j) continue;
          if (neighbors[j].find(k) != neighbors[j].end())
          {
            triangles++;
          }
        }
      }
      local_coefficient.record(static_cast<double>(triangles) /
                               static_cast<double>(triples));
      total_triples += triples;
      total_triangles += triangles;
    }
    utils::Structure s;
    s["triangles"] = total_triangles / 3;
    s["triples"] = total_triples;
    s["global"] = static_cast<double>(total_triangles) /
                  static_cast<double>(total_triples);
    s["local"] = local_coefficient.render();
    return s;
  }
};

namespace
{
// Default to edge weight 1 for non-specialized templating
template <class Edge>
double get_cost(const Edge&)
{
  return 1;
}

template <class T>
double get_cost(const CostEdge<T>& edge)
{
  return edge.cost();
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/// Coupling strength statistics
///
/// Produces an average and distribution of the coupling strength.
///
template <class Graph>
class Couplings : public Property<Graph>
{
 public:
  utils::Structure compute(const Graph& g) const override
  {
    double bin_width = 1;
    if (!g.edges().empty())
    {
      double low = get_cost(g.edge(0));
      double high = get_cost(g.edge(0));
      for (const auto& edge : g.edges())
      {
        low = std::min(low, get_cost(edge));
        high = std::max(high, get_cost(edge));
      }
      if (low < high)
      {
        bin_width = 1e-9;
        while (true)
        {
          if ((high - low) / bin_width <= 32) break;
          bin_width *= 2.0;
          if ((high - low) / bin_width <= 32) break;
          bin_width *= 2.5;
          if ((high - low) / bin_width <= 32) break;
          bin_width *= 2.0;
        }
      }
    }
    observe::BinWidthDistribution couplings(bin_width);
    for (const auto& edge : g.edges())
    {
      couplings.record(get_cost(edge));
    }
    return couplings.render();
  }
};

namespace
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
std::string to_lower(std::string data)
{
  for (auto& c : data) c = (char)::tolower(c);
  return data;
}
#pragma clang diagnostic pop

}  // namespace

#define SET_PROPERTY(Property) \
  properties[to_lower(#Property)] = Property<Graph>().compute(g)

/// Compute the collection of implemented graph properties for `g`.
template <class Graph>
utils::Structure get_properties(const Graph& g)
{
  utils::Structure properties;
  SET_PROPERTY(Size);
  /*
   * Graph properties temporarily disabled (because they are SLOW)
   *
   *
  SET_PROPERTY(Degree);
  SET_PROPERTY(Locality);
  SET_PROPERTY(Clustering);
  SET_PROPERTY(Couplings);  // relevant for edge weighted graphs
  */
  return properties;
}

#undef SET_PROPERTY

}  // namespace graph
