// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>
#include <vector>

#include "utils/exception.h"
#include "utils/utils.h"
#include "graph/cost_edge.h"
#include "graph/face.h"
#include "graph/faced_graph.h"
#include "graph/graph.h"
#include "graph/properties.h"
#include "markov/model.h"

#define MIN_DELTA_DEFAULT 0.1

namespace model
{

constexpr char kInitialConfigurationInputIdentifier[] = "initial_configuration";  
////////////////////////////////////////////////////////////////////////////////
/// Graph Model Configuration
///
/// This is a base class for an intermidiate loading of data for
/// various Graph Models. Graph Models will be configured form this class by
/// moving its data.
///
class GraphModelConfiguration
    : public BaseModelConfiguration,
      public graph::GraphConfiguration<::graph::CostEdge<double>>
{
 public:
  using Node = ::graph::Node;
  using Edge = ::graph::CostEdge<double>;

  struct Get_Initial_Configuration
  {
    static std::vector<std::pair<int, int>>& get(
        class GraphModelConfiguration& config)
    {
      return config.initial_configuration_;
    }
    static std::string get_key()
    {
      return model::kInitialConfigurationInputIdentifier;
    }
  };

  using MembersStreamHandler = 
      utils::ObjectMemberStreamHandler<
          utils::VectorObjectStreamHandler<typename Edge::StreamHandler>,
          GraphModelConfiguration, Get_Edges, true,
      utils::ObjectMemberStreamHandler<
          utils::VectorObjectStreamHandler<typename Node::StreamHandler>,
          GraphModelConfiguration, Get_Nodes, false,
      utils::ObjectMemberStreamHandler<
          utils::PairVectorStreamHandler<int, int>, 
          GraphModelConfiguration, Get_Initial_Configuration, false,
      BaseModelConfiguration::MembersStreamHandler>>>;

  using StreamHandler = ModelStreamHandler<GraphModelConfiguration>;

  void map_initial_configuration(
      const std::map<int, int>& node_name_to_id_map,
      std::vector<std::pair<int, int>>& id_val_configuration)
  {
    const auto& name_val_configuration = initial_configuration_;
    if (name_val_configuration.size() != 0)
    {
      id_val_configuration.clear();
      id_val_configuration.reserve(name_val_configuration.size());
      for (const auto& name_val : name_val_configuration)
      {
        const auto name_id = node_name_to_id_map.find(name_val.first);
        if (name_id == node_name_to_id_map.end())
        {
          // Node name is not found
          THROW(utils::ValueException,
                "parameter `initial_configuration` have node name: ",
                name_val.first, " which is not found in model node names");
        }
        id_val_configuration.push_back({name_id->second, name_val.second});
      }
      if (id_val_configuration.size() != node_name_to_id_map.size())
      {
        THROW(utils::ValueException,
              "parameter `initial_configuration` have size: ",
              id_val_configuration.size(),
              " which is different then model number of nodes: ",
              node_name_to_id_map.size());
      }
    }
  }

 private:
  std::vector<std::pair<int, int>> initial_configuration_;
};


////////////////////////////////////////////////////////////////////////////////
/// Graph Model
///
/// This is a base class for models with a cost function that can be expressed
/// as a graph. It takes care of the boiler plate to initialize and access the
/// different graph components.
///
/// NOTE: Currently, it is limited to a graph with blank nodes and edges that
/// have a double-valued cost attributes. If necessary, this can be made
/// configurable in the future -- either by extending the list of template
/// arguments here or by using multiple inheritance (i.e., removing the
/// ::markov::Model base class from GraphModel).
///
template <class State_T, class Transition_T, class Cost_T = double>
class GraphModel : public ::markov::Model<State_T, Transition_T, Cost_T>
{
 public:
  // Using directives to simplify the notation below.
  // Make these template arguments if they need to be configurable.
  using Node = ::graph::Node;
  using Edge = ::graph::CostEdge<double>;
  using Graph = ::graph::Graph<Edge, Node>;

  // Interface for external model configuration functions.
  using Configuration_T = GraphModelConfiguration;

  /// Return a vector of all the nodes.
  inline const std::vector<Node>& nodes() const { return graph_.nodes(); }

  /// Return the number of nodes.
  inline size_t node_count() const { return graph_.nodes().size(); }

  /// Return a specific node by NodeId.
  inline const Node& node(size_t id) const { return graph_.nodes()[id]; }

  /// Return a vector of all the edges.
  inline const std::vector<Edge>& edges() const { return graph_.edges(); }

  /// Return the number of edges.
  inline size_t edge_count() const { return graph_.edges().size(); }

  /// Return a specific edge by EdgeId.
  inline const Edge& edge(size_t id) const { return graph_.edges()[id]; }

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

  // Return the typical number of markov states considered one `sweep`.
  // For graph models, this corresponds to the number of variables which
  // are typically associated with nodes.
  size_t get_sweep_size() const override { return nodes().size(); }

  // Return number of edges entries in the model
  size_t get_term_count() const override { return edges().size(); }

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
  Graph graph_;
  std::vector<std::pair<int, int>> initial_configuration_;
};

class CostCache
{
 public:
  CostCache() : value(0) {}
  CostCache(double v) : value(v) {}
  CostCache(double v, std::vector<double> c) : value(v), cache(c) {}

  operator double() const { return value; }

  utils::Structure render() const { return value; }

  double value;
  std::vector<double> cache;
};

inline CostCache operator+(const CostCache& c1, const CostCache& c2)
{
  return CostCache(c1.value + c2.value, c2.cache);
}
inline void operator+=(CostCache& c1, const CostCache& c2)
{
  c1.value += c2.value;
  if (c1.cache.size() == c2.cache.size())
  {
    for (size_t i = 0; i < c1.cache.size(); i++)
    {
      c1.cache[i] += c2.cache[i];
    }
  }
}
inline void operator/=(CostCache& c, const double& factor)
{
  c.value /= factor;
}
inline bool operator<(const CostCache& c1, const CostCache& c2)
{
  return c1.value < c2.value;
}
inline bool operator<(const CostCache& c, const double& d)
{
  return c.value < d;
}
inline bool operator>(const CostCache& c1, const CostCache& c2)
{
  return c1.value > c2.value;
}

////////////////////////////////////////////////////////////////////////////////
/// Fced Graph Model Configuration
///
/// This is a base class for an intermidiate loading of data for
/// various Faced Graph Models. Graph Models will be configured form this class
/// by moving its data.
///
class FacedGraphModelConfiguration : public BaseModelConfiguration,
                                     public ::graph::FacedGraph::Configuration_T
{
 public:
  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<
          typename ::graph::SCL_Term::StreamHandler>,
      FacedGraphModelConfiguration, Get_SLC_Terms, false,
      utils::ObjectMemberStreamHandler<
          utils::VectorObjectStreamHandler<
              typename ::graph::FacedGraph::Configuration_T::EdgeType_T::
                  StreamHandler>,
          FacedGraphModelConfiguration, Get_Edges, true,
          utils::ObjectMemberStreamHandler<
              utils::VectorObjectStreamHandler<
                  typename ::graph::FacedGraph::Configuration_T::NodeType_T::
                      StreamHandler>,
              FacedGraphModelConfiguration, Get_Nodes, false,
              BaseModelConfiguration::MembersStreamHandler>>>;

  using StreamHandler = ModelStreamHandler<FacedGraphModelConfiguration>;
};

////////////////////////////////////////////////////////////////////////////////
/// Faced Graph Model
///
/// This is a base class for models with a cost function that can be expressed
/// as a graph with faces.
template <class State_T, class Transition_T, class Cost_T = CostCache>
class FacedGraphModel : public ::markov::Model<State_T, Transition_T, Cost_T>
{
 public:
  // Using directives to simplify the notation below.
  using Node = ::graph::NodeWithFaces;
  using Edge = ::graph::EdgeWithFace<double>;
  using Face = ::graph::Face<double>;
  using Graph = ::graph::FacedGraph;
  // Interface for external model configuration functions.
  using Configuration_T = FacedGraphModelConfiguration;

  /// Return a vector of all the nodes.
  inline const std::vector<Node>& nodes() const { return graph_.nodes(); }

  /// Return the number of nodes.
  inline size_t node_count() const { return graph_.nodes().size(); }

  /// Return a specific node by node ID.
  inline const Node& node(size_t id) const { return graph_.nodes()[id]; }

  /// Return a vector of all the edges.
  inline const std::vector<Edge>& edges() const { return graph_.edges(); }

  /// Return a specific edge by edge ID.
  inline const Edge& edge(size_t id) const { return graph_.edges()[id]; }

  /// Return the number of edges.
  inline size_t edge_count() const { return graph_.edges().size(); }

  /// Return a vector of all the faces.
  inline const std::vector<Face>& faces() const { return graph_.faces(); }

  /// Return a specific edge by face ID.
  inline const Face& face(size_t id) const { return graph_.faces()[id]; }

  /// Configure the graph from input
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

  void configure(Configuration_T& configuration)
  {
    ::markov::Model<State_T, Transition_T, Cost_T>::configure(configuration);
    graph_.configure(configuration);
  }

  Cost_T calculate_cost_difference(const State_T&,
                                   const Transition_T&) const override
  {
    throw utils::NotImplementedException(
        "Cost difference calculation for \
        Grouped model requires a non-null cost pointer.");
    return Cost_T(0);
  }

  virtual Cost_T calculate_cost_difference(const State_T& state,
                                           const Transition_T& transition,
                                           const Cost_T* cost) const = 0;

  virtual void apply_transition(const Transition_T& transition, State_T& state,
                                Cost_T* cost) const = 0;

  // Return the typical number of markov states considered one `sweep`.
  // For graph models, this corresponds to the number of variables which
  // are typically associated with nodes.
  size_t get_sweep_size() const override { return nodes().size(); }

  // Return number of edges entries in the model
  size_t get_term_count() const override { return edges().size(); }

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

  Cost_T get_scale_factor() const override { return graph_.get_scale_factor(); }

  Cost_T get_const_cost() const override { return graph_.get_const_cost(); }

  bool is_empty() const override { return graph_.is_empty(); }

  /// Manage the coefficient collation logic for a single term with degree at
  /// most 2.
  void collate(std::map<int, double>& coefficients, const double& term_cost,
               const std::vector<int>& term_nodes, int node_id,
               const graph::FaceType& ftype) const
  {
    int paired_id;
    // Select input value for coefficient map
    // The output value at node_id represents the coefficient on x_i^2
    // The output value at node_count() represents the coefficient on x_i
    if (term_nodes.size() == 2)
    {
      if (term_nodes[0] != node_id)
      {
        paired_id = term_nodes[0];
      }
      else
      {
        paired_id = term_nodes[1];
      }
    }
    else if (ftype == graph::FaceType::SquaredLinearCombination &&
             term_nodes.size() == 1)
    {
      // In an SLC face, a linear term expands to a quadratic term
      paired_id = term_nodes[0];
    }
    else
    {
      // In an SLC face, a constant term expands to a linear term
      paired_id = node_count();
    }
    // Add value to coefficient map at other node ID
    if (coefficients.find(paired_id) == coefficients.end())
    {
      coefficients.insert({paired_id, term_cost});
    }
    else
    {
      coefficients[paired_id] += term_cost;
    }
  }

  /// Compute an upper bound to the maximum cost difference
  /// from a single state flip using the triangle inequality.
  /// Implicitly expands SLC terms in cost function to obtain
  /// a tighter bound at the cost of additional memory.
  virtual double estimate_max_cost_diff() const override
  {
    double max_diff = 0;
#ifdef _MSC_VER
    // reduction min or max is not implemented yet in all compilers
    #pragma omp parallel
    {
      double max_diff_local = 0;
      #pragma omp for
#else
    #pragma omp parallel for reduction(max : max_diff)
#endif
      for (size_t node_id = 0; node_id < node_count(); node_id++)
      {
        const Node& n = node(node_id);
        // For input i, quad_coeffs maps to the sum of coefficients
        // on terms with form x_i * x_node_id.
        // Input nodes_count corresponds to sum of coefficients
        // on terms with form x_node_id.
        std::map<int, double> quad_coeffs;
        double cost_sum_abs = 0;
        for (size_t j = 0; j < n.face_ids().size(); j++)
        {
          int face_id = n.face_ids()[j];
          const Face& f = face(face_id);
          switch (f.type())
          {
            case graph::FaceType::Combination:
              // Collate quadratic term coefficients
              for (size_t k = 0; k < n.edge_ids(face_id).size(); k++)
              {
                const Edge& e = edge(n.edge_ids(face_id)[k]);
                double edge_contr = f.cost() * e.cost();
                std::vector<int> adj_nodes = e.node_ids();
                if (adj_nodes.size() <= 2)
                {
                  // Only collate terms with degree at most 2
                  collate(quad_coeffs, edge_contr, adj_nodes, node_id,
                          graph::FaceType::Combination);
                }
                else
                {
                  // Add absolute value directly to running estimate
                  cost_sum_abs += std::abs(edge_contr);
                }
              }
              break;
            case graph::FaceType::SquaredLinearCombination:
              // Validated face ensures given node appears once and linearly
              double c = edge(n.edge_ids(face_id)[0]).cost();
              double node_contr = f.cost() * c;
              // Iterate over all edges in face, since all are
              // incident to given node in expanded form
              for (size_t k = 0; k < f.edge_ids().size(); k++)
              {
                const Edge& e = edge(f.edge_ids()[k]);
                double edge_contr = 2 * node_contr * e.cost();
                collate(quad_coeffs, edge_contr, e.node_ids(), node_id,
                        graph::FaceType::SquaredLinearCombination);
              }
              // Amend double-count of term with node_id**2
              // Does not depend on spin vs. binary setting
              quad_coeffs[node_id] -= node_contr * c;
              break;
          }
        }
        // Add map values to estimate
        for (auto& collated_coeff : quad_coeffs)
        {
          cost_sum_abs += std::abs(collated_coeff.second);
        }
        quad_coeffs.clear();
#ifdef _MSC_VER
        max_diff_local =
            cost_sum_abs > max_diff_local ? cost_sum_abs : max_diff_local;
      }
      #pragma omp critical
      max_diff = max_diff_local > max_diff ? max_diff_local : max_diff;
    }
#else
      max_diff = cost_sum_abs > max_diff ? cost_sum_abs : max_diff;
    }
#endif
    return max_diff;
  }

 protected:
  Graph graph_;

  virtual void insert_val_to_bbstrees(
      double val, std::multiset<double>& tree_A,
      std::multiset<double>* tree_B = nullptr) const = 0;

  virtual void amend_collation(std::map<int, double>& coeffs,
                               int node_id) const = 0;

  /// Simulate the expanded form of grouped terms to collate
  /// quadratic and linear coefficients of terms including
  /// a given node ID to populate balanced binary search trees
  /// of term weights for maximum cost difference estimation.
  /// This function is used by children models to implement
  /// estimate_min_cost_diff by overriding the functions
  /// insert_val_to_bbstrees and amend_collation
  void populate_bbstrees(int node_id, std::multiset<double>& tree_A,
                         std::multiset<double>* tree_B = nullptr) const
  {
    const Node& n = node(node_id);
    // For input i, quad_coeffs maps to the sum of coefficients
    // on terms with form x_i * x_node_id.
    // Input nodes_count corresponds to sum of coefficients
    // on terms with form x_node_id.
    std::map<int, double> quad_coeffs;
    for (size_t j = 0; j < n.face_ids().size(); j++)
    {
      int face_id = n.face_ids()[j];
      const Face& f = face(face_id);
      switch (f.type())
      {
        case graph::FaceType::Combination:
          // Collate quadratic term coefficients
          for (size_t k = 0; k < n.edge_ids(face_id).size(); k++)
          {
            const Edge& e = edge(n.edge_ids(face_id)[k]);
            double edge_contr = f.cost() * e.cost();
            std::vector<int> adj_nodes = e.node_ids();
            if (adj_nodes.size() <= 2)
            {
              // Only collate terms with degree at most 2
              collate(quad_coeffs, edge_contr, adj_nodes, node_id,
                      graph::FaceType::Combination);
            }
            else
            {
              this->insert_val_to_bbstrees(edge_contr, tree_A, tree_B);
            }
          }
          break;
        case graph::FaceType::SquaredLinearCombination:
          // Validated face ensures given node appears once and linearly
          double c = edge(n.edge_ids(face_id)[0]).cost();
          double node_contr = f.cost() * c;
          // Iterate over all edges in face, since all are
          // incident to given node in expanded form
          for (size_t k = 0; k < f.edge_ids().size(); k++)
          {
            const auto& e = edge(f.edge_ids()[k]);
            double edge_contr = 2 * node_contr * e.cost();
            collate(quad_coeffs, edge_contr, e.node_ids(), node_id,
                    graph::FaceType::SquaredLinearCombination);
          }
          break;
      }
    }
    this->amend_collation(quad_coeffs, node_id);
    for (auto& collated_coeff : quad_coeffs)
    {
      this->insert_val_to_bbstrees(collated_coeff.second, tree_A, tree_B);
    }
    quad_coeffs.clear();
  }
};

}  // namespace model