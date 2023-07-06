
#pragma once

#include <float.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <map>
#include <set>
#include <vector>

#include "utils/component.h"
#include "utils/config.h"
#include "utils/exception.h"
#include "utils/log.h"
#include "utils/operating_system.h"
#include "utils/stream_handler.h"
#include "graph/cost_edge.h"
#include "graph/face.h"
#include "graph/graph_properties.h"
#include "graph/node.h"

namespace graph
{
////////////////////////////////////////////////////////////////////////////////
/// Identifiers used in the input format for QIO optimization problems.
///
/// Nodes are called "variables" because each is associated with a variable of
/// the optimization problem. Edges are called "terms", because they represent
/// interacting variables in the cost function polynomial.
constexpr char kEdgesInputIdentifier[] = "terms";
constexpr char kNodesInputIdentifier[] = "variables";

template <class EdgeType, class NodeType = ::graph::Node>
class GraphConfiguration
{
 public:
  using EdgeType_T = EdgeType;
  using NodeType_T = NodeType;

  struct Get_Edges
  {
    static std::vector<EdgeType>& get(GraphConfiguration& graph_config)
    {
      return graph_config.edges_;
    }
    static std::string get_key() { return graph::kEdgesInputIdentifier; }
  };

  struct Get_Nodes
  {
    static std::vector<NodeType>& get(GraphConfiguration& graph_config)
    {
      return graph_config.nodes_;
    }
    static std::string get_key() { return graph::kNodesInputIdentifier; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<typename EdgeType::StreamHandler>,
      GraphConfiguration, Get_Edges, true,
      utils::ObjectMemberStreamHandler<
          utils::VectorObjectStreamHandler<typename NodeType::StreamHandler>,
          GraphConfiguration, Get_Nodes, false>>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

 private:
  std::vector<EdgeType> edges_;
  std::vector<NodeType> nodes_;
};

template <typename Edge>
void normalize_edges(std::vector<Edge>& edges,
                     std::vector<std::vector<size_t>>& local_nodes,
                     GraphProperties& properties,
                     std::map<int, int>& node_name_to_id,
                     std::map<int, int>& node_id_to_name)
{
  std::vector<Edge> temp_edges = std::move(edges);

  node_name_to_id.clear();
  node_id_to_name.clear();
  edges.clear();

  if (temp_edges.size() == 0)
  {
    THROW(utils::ValueException, "parameter `", kEdgesInputIdentifier,
          +"`: size must be greater than 0, found 0");
  }

  edges.reserve(temp_edges.size());

  int node_order = 0;

  for (size_t edge_id = 0; edge_id < temp_edges.size(); edge_id++)
  {
    Edge new_edge;
    std::set<int> new_edge_ids;
    const Edge& old_edge = temp_edges[edge_id];
    new_edge.set_cost(old_edge.cost());

    for (int input_id : old_edge.node_ids())
    {
      auto search = node_name_to_id.find(input_id);
      if (search == node_name_to_id.end())
      {
        // this is a new node id found
        node_name_to_id.insert({input_id, node_order});
        node_id_to_name.insert({node_order, input_id});
        local_nodes.push_back(std::vector<size_t>());
        node_order++;
        assert((size_t)node_order == local_nodes.size());
      }
      int internal_id = node_name_to_id[input_id];
      assert(internal_id < node_order);

      if (new_edge_ids.find(internal_id) == new_edge_ids.end())
      {
        new_edge_ids.insert(internal_id);
        new_edge.add_node_id(internal_id);
        local_nodes[internal_id].push_back(edges.size());
      }
      else
      {
        if (!properties.allow_dup_merge_)
        {
          throw utils::DuplicatedVariableException(
              "Duplicated ids detected in term!");
        }
      }
    }
    new_edge.sort_node_ids();
    size_t locality = new_edge.node_ids().size();
    if (locality > properties.max_locality_)
    {
      properties.max_locality_ = locality;
    }
    if (properties.min_locality_ > locality)
    {
      properties.min_locality_ = locality;
    }
    if (locality > 0)
    {
      properties.accumulated_dependent_vars_ += locality * (locality - 1);
      properties.accumulated_dependent_terms_ += locality;
    }
    if (locality > 0)
    {
      double abs_cost = fabs(new_edge.cost());
      if (abs_cost > properties.max_coupling_magnitude_)
      {
        properties.max_coupling_magnitude_ = abs_cost;
      }
      if (abs_cost < properties.min_coupling_magnitude_)
      {
        properties.min_coupling_magnitude_ = abs_cost;
      }
    }
    properties.total_locality_ += locality;

    if (new_edge.nodes_count() > 0)
    {
      edges.push_back(new_edge);
    }
    else
    {
      // in case this is const term (there is no node), add the cost to
      // properties_.const_cost_
      properties.const_cost_ += new_edge.cost();
    }
  }
  assert(local_nodes.size() == (size_t)node_order);
  properties.avg_locality_ =
      (double)(properties.total_locality_) / edges.size();
}

////////////////////////////////////////////////////////////////////////////////
/// Graph representation as a collection of nodes and edges.
///
/// It assumes that:
///
///   * nodes are identified with a NodeId `0..(N-1)` and
///   * edges are identified with an EdgeId `0..(M-1)`,
///
/// where N (M) is the number of nodes (edges). This is a concurrent adjacency
/// list AND edge list representation of the graph (slightly slower than using
/// a single representation, but more versatile).
///
/// NOTE: We use a templated approach such that the accessors for nodes and
/// edges can return the actual type rather than a limited base class view of
/// it.
///
/// Repetition of a `node_id` in an edge is explicitly allowed and implies a
/// duplication of the corresponding `edge_id` in the node. This can be used
/// for, e.g., squared terms in a cost function represented as a graph.
/// Hoewever, this approach is not efficient for very high order terms.
///
template <class Edge, class Node = ::graph::Node>
class Graph : public utils::Component
{
 public:
  Graph() : properties_() {}

  // Interface for stream configuration
  using Configuration_T = GraphConfiguration<Edge, Node>;

  using Edge_T = Edge;
  using Node_T = Node;

  void set_allow_dup_merge(bool value) { properties_.allow_dup_merge_ = value; }

  double get_const_cost() const { return properties_.const_cost_; }

  bool is_empty() const
  {
    assert((!nodes_.empty()) || edges_.empty());
    assert((!edges_.empty()) || nodes_.empty());
    return nodes_.empty();
  }

  /// Get a node by NodeId.
  const Node& node(size_t node_indx) const { return nodes_[node_indx]; }

  /// Access the vector of all edges.
  const std::vector<Node>& nodes() const { return nodes_; }

  /// Get an edge by EdgeId.
  const Edge& edge(size_t edge_id) const { return edges_[edge_id]; }

  /// Access the vector of all edges.
  const std::vector<Edge>& edges() const { return edges_; }

  /// Sort each node and edge.
  ///
  /// This invokes `sort_edge_ids()` and `sort_node_ids()` on all nodes and
  /// edges, respectively. It does **NOT** modify the order of the nodes and
  /// edges in the graph (i.e., the `node_id`s and `edge_id`s are stable under
  /// this operation). The order in which nodes are listed in each edge (and
  /// vice-versa) should not matter for the typical application (and can be
  /// unsorted either from intial input or from calls to
  /// `{add,remove}_{node,edge}`).
  ///
  /// This functionality is provided in case an implementation relies on either
  /// ascending order or being able to identify repetitions easily.
  ///
  /// @see Node::sort_edge_ids
  /// @see Edge::sort_node_ids
  ///
  virtual void sort()
  {
    for (auto& node : nodes_) node.sort_edge_ids();
    for (auto& edge : edges_) edge.sort_node_ids();
  }

  /// Validate the `Graph`.
  ///
  /// Ensure that `Node`s, `Edge`s and their references are consistent.
  ///
  /// @see validate_edge
  /// @see validate_node
  /// @see validate_counts
  ///
  virtual bool validate() const
  {
    bool valid = true;
    for (size_t edge_id = 0; edge_id < edges_.size(); edge_id++)
    {
      valid &= validate_edge(edge_id);
    }
    for (int node_id = 0; node_id < (int)nodes_.size(); node_id++)
    {
      valid &= validate_node(node_id);
    }
    return valid;
  }

  /// Validate a `Node`
  ///
  /// Ensure that
  ///
  ///   * the `Node` participates in at least one `Edge`,
  ///   * all edges listed have a valid `edge_id`s, and
  ///   * the `Node` is referenced in those `Edge`s.
  ///
  virtual bool validate_node(int node_indx) const
  {
    bool valid = true;
    const auto& node = nodes_[node_indx];
    if (node.edge_ids().empty())
    {
      LOG(ERROR, "Node ", node_indx, " is not contained in any edges.");
      valid = false;
    }
    for (size_t edge_id : node.edge_ids())
    {
      if (edge_id >= edges_.size())
      {
        LOG(ERROR, "Invalid edge_id ", edge_id, " on node ", node_indx, ".");
        valid = false;
      }
      valid &= validate_counts(node_indx, edge_id);
    }
    return valid;
  }

  /// Validate an `Edge`
  ///
  /// Ensure that
  ///
  ///   * the `Edge` includes at least one `Node`,
  ///   * all nodes listed have a valid `edge_id`s, and
  ///   * the node is referenced in those `Edge`s.
  ///
  virtual bool validate_edge(size_t edge_id) const
  {
    bool valid = true;
    const auto& edge = edges_[edge_id];
    for (int node_id : edge.node_ids())
    {
      if (node_id < 0 || node_id >= (int)nodes_.size())
      {
        LOG(ERROR, "Invalid node_id ", node_id, " on edge ", edge_id, ".");
        valid = false;
      }
      valid &= validate_counts(node_id, edge_id);
    }
    return valid;
  }

  virtual bool validate_counts(int node_id, size_t edge_id) const
  {
    const auto& node = nodes_[node_id];
    const auto& edge = edges_[edge_id];
    auto node_count =
        std::count(edge.node_ids().begin(), edge.node_ids().end(), node_id);
    auto edge_count =
        std::count(node.edge_ids().begin(), node.edge_ids().end(), edge_id);
    if (node_count != edge_count)
    {
      LOG(ERROR, "Inconsistent id count: node_id ", node_id, " is listed ",
          node_count, " in edge ", edge_id, ", edge_id ", edge_id,
          " is listed ", edge_count, " in node ", node_id, ".");
      return false;
    }
    if (node_count != 1)
    {
      LOG(ERROR, "Nodes should appear at most once in each edge: ", node_count,
          " found.");
      return false;
    }
    return true;
  }

  /// Configure the graph by configuring its components.
  ///
  /// ```
  /// {
  ///   "terms": [...],
  ///   "variables": [...],
  /// }
  /// ```
  ///
  /// NOTE: The group name `terms` is a side effect of the cost_function
  /// input format.
  ///
  void configure(const utils::Json& json) override
  {
    LOG_MEMORY_USAGE("begin of graph configure");
    using matcher::GreaterThan;
    using matcher::SizeIs;

    configure_memory_check(json);

    this->param(json, kEdgesInputIdentifier, edges_)
        .description("edge terms of the graph")
        .required()
        .matches(SizeIs(GreaterThan(0)));
    if (json.HasMember(kNodesInputIdentifier))
    {
      this->param(json, kNodesInputIdentifier, nodes_)
          .description("nodes of the graph")
          .required()
          .matches(SizeIs(GreaterThan(0)));
    }
    LOG_MEMORY_USAGE("end of graph configure");
    init();
  }

  void configure(Configuration_T& config)
  {
    LOG_MEMORY_USAGE("begin of graph configure");
    edges_ = std::move(Configuration_T::Get_Edges::get(config));
    LOG_MEMORY_USAGE("end of graph configure");
    init();
  }

  utils::Structure render() const override
  {
    utils::Structure s;
    s[kEdgesInputIdentifier] = edges_;
    s[kNodesInputIdentifier] = nodes_;
    return s;
  }

  int map_output(int internal_id) const
  {
    assert(internal_id < (int)nodes_.size());
    return node_id_to_name_.at(internal_id);
  }

  const std::map<int, int>& output_map() const { return node_id_to_name_; }

  size_t nodes_size() const { return nodes().size(); }

  size_t edges_size() const { return edges().size(); }

  uint32_t get_locality() const { return properties_.max_locality_; }

  uint32_t get_min_locality() const { return properties_.min_locality_; }

  double get_avg_locality() const { return properties_.avg_locality_; }

  uint64_t get_accumulated_dependent_vars() const
  {
    return properties_.accumulated_dependent_vars_;
  }

  double get_max_coupling_magnitude() const
  {
    return properties_.max_coupling_magnitude_;
  }

  double get_min_coupling_magnitude() const
  {
    return properties_.min_coupling_magnitude_;
  }

  uint64_t get_sum_coefficient_degrees_total() const
  {
    return properties_.total_locality_;
  }

  double get_scale_factor() const { return properties_.scale_factor_; }

  bool is_rescaled() const { return properties_.is_rescaled_; }

  const GraphProperties& get_properties() const { return properties_; }

  void rescale() { properties_.rescale(); }

  double estimate_max_cost_diff() const
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
      for (size_t i = 0; i < nodes_.size(); i++)
      {
        double cost_sum_abs = 0;
        const Node& node_i = node(i);
        for (auto edge_id : node_i.edge_ids())
        {
          const Edge& edge_i = edge(edge_id);
          cost_sum_abs += std::abs(edge_i.cost());
        }
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

  struct Get_Edges
  {
    static std::vector<Edge>& get(Graph& g) { return g.edges_; }
    static std::string get_key() { return kEdgesInputIdentifier; }
  };

  struct Get_Nodes
  {
    static std::vector<Node>& get(Graph& g) { return g.nodes_; }
    static std::string get_key() { return kNodesInputIdentifier; }
  };

  const std::map<int, int>& get_node_name_to_id_map() const
  {
    return node_name_to_id_;
  }

 protected:
  std::vector<Edge> edges_;
  std::vector<Node> nodes_;
  std::map<int, int> node_name_to_id_;
  std::map<int, int> node_id_to_name_;
  GraphProperties properties_;

  /// Build the adjacency list from the edge list.
  ///
  /// This is used when the graph input is represented solely as an edge
  /// list to infer the corresponding adjacencly list representation.
  void populate_node_edge_ids()
  {
    LOG_MEMORY_USAGE("begin of graph nodes extraction");

    std::vector<std::vector<size_t>> local_nodes;
    normalize_edges<Edge>(edges_, local_nodes, properties_, node_name_to_id_,
                          node_id_to_name_);
    nodes_.clear();
    nodes_.reserve(local_nodes.size());
    for (size_t i = 0; i < local_nodes.size(); i++)
    {
      // node name is useless just occupy space, may remove them later.
      nodes_.push_back(Node(local_nodes[i]));
    }

    assert(validate());
    LOG_MEMORY_USAGE("end of graph nodes extraction");
  }

  void configure_memory_check(const utils::Json& json)
  {
    size_t edges_memory_estimation = 0;
    if (json.HasMember(kEdgesInputIdentifier))
    {
      auto it = json.FindMember(kEdgesInputIdentifier);
      if (it->value.IsArray())
      {
        size_t num_edges = it->value.Size();

        #pragma omp parallel for reduction(+ : edges_memory_estimation)
        for (size_t i = 0; i < num_edges; i++)
        {
          size_t n_node_id = Edge::num_nodes(it->value[i]);
          edges_memory_estimation += Edge::memory_estimate(n_node_id);
        }
      }
    }

    size_t memory_estimation = edges_memory_estimation;

    size_t available_memory = utils::get_available_memory();

    if (available_memory < memory_estimation)
      throw utils::MemoryLimitedException(
          "Input problem is too large (too many "
          "terms and/or variables). "
          "Expected to exceed machine's current available memory.");
  }

  void init_memory_check()
  {
    size_t edges_memory_estimation = 0;
    size_t n_connections = 0;
    size_t num_edges = edges_.size();
    #pragma omp parallel for reduction(+ : edges_memory_estimation, n_connections)
    for (size_t i = 0; i < num_edges; i++)
    {
      size_t n_node_id = edges_[i].node_ids().size();
      edges_memory_estimation += Edge::memory_estimate(n_node_id);
      n_connections += n_node_id;
    }

    // Use of lower bound for nodes memory.
    // The more edges in each node in average the better estimation accuracy.
    // For instance, 90% accuracy if we have in average 27 edges in each node,
    // 97% for 100... In the worst case 25% accuracy if 1 edge in each node
    // Accurate node count is needed for better accounting of vectors
    // overheads, which takes memory and time to compute.
    size_t nodes_memory_estimation = Node::memory_estimate(n_connections);

    size_t memory_estimation =
        edges_memory_estimation +     // temp_edges
                                      // in populate_node_edge_ids function
        2 * nodes_memory_estimation;  // nodes_ and local_nodes
                                      // in populate_node_edge_ids function
    size_t available_memory = utils::get_available_memory();

    if (available_memory < memory_estimation)
      throw utils::MemoryLimitedException(
          "Input problem is too large (too many "
          "terms and/or variables). "
          "Expected to exceed machine's current available memory.");
  }

  void init()
  {
    init_memory_check();
    populate_node_edge_ids();
  }
};

}  // namespace graph
