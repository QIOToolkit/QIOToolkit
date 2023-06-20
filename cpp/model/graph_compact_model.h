// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "utils/exception.h"
#include "utils/utils.h"
#include "graph/cost_edge.h"
#include "graph/graph.h"
#include "graph/graph_compact.h"
#include "graph/properties.h"
#include "markov/model.h"
#include "model/graph_model.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// Graph Compact Model
///
/// This is with a cost function that can be expressed
/// as a compact graph. GraphCompactModel is samilar to GraphModel except
/// GraphCompactModel use a compact representation and it only allow sequential
/// visiting
///
template <class State_T, class Transition_T, typename Element_T,
          class Cost_T = double>
class GraphCompactModel : public ::markov::Model<State_T, Transition_T, Cost_T>
{
 public:
  using Configuration_T = GraphModelConfiguration;
  using Edge_T = typename graph::CompactGraph<Element_T>::Edge_T;
  GraphCompactModel() {}
  virtual ~GraphCompactModel(){};

  /// Configure the graph from JSON document
  void configure(const utils::Json& json) override
  {
    ::markov::Model<State_T, Transition_T, Cost_T>::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kCostFunction))
    {
      THROW(utils::MissingInputException, "The configuration of the ",
            this->get_identifier(),
            " model must contain a `cost_function` entry.");
    }
    graph_.configure(json[utils::kCostFunction]);
  }

  /// Configure the graph from preloaded graph
  void configure(Configuration_T& configuration)
  {
    ::markov::Model<State_T, Transition_T>::configure(configuration);
    graph_.configure(configuration);

    configuration.map_initial_configuration(graph_.get_node_name_to_id_map(),
                                            initial_configuration_);
  }

  void init() override { graph_.init(); }

  size_t get_sweep_size() const override { return node_count(); }

  size_t get_term_count() const override { return edge_count(); }

  /// Return the number of nodes.
  inline size_t node_count() const { return graph_.nodes_size(); }

  /// Return the number of edges.
  inline size_t edge_count() const { return graph_.edges_size(); }

  inline const Edge_T& edge(size_t edge_id) const
  {
    return graph_.edge(edge_id);
  }

  /// Fill the graph benchmarking properties.
  utils::Structure get_benchmark_properties() const override
  {
    utils::Structure s = ::markov::Model<State_T, Transition_T,
                                        Cost_T>::get_benchmark_properties();
    s["graph"] = ::graph::get_properties(graph_);
    return s;
  }

  bool is_rescaled() const override { return graph_.is_rescaled(); }

  void rescale() override { graph_.rescale(); }

  double get_scale_factor() const override { return graph_.get_scale_factor(); }

  double get_const_cost() const override { return graph_.get_const_cost(); }

  bool is_empty() const override { return graph_.is_empty(); }

  virtual double estimate_max_cost_diff() const override
  {
    return graph_.estimate_max_cost_diff();
  }

  bool has_initial_configuration() const override
  {
    return initial_configuration_.size() > 0;
  }

  const std::vector<std::pair<int, int>>& get_initial_configuration() const
  {
    return initial_configuration_;
  }

 protected:
  graph::CompactGraph<Element_T> graph_;
  std::vector<std::pair<int, int>> initial_configuration_;
};
}  // namespace model