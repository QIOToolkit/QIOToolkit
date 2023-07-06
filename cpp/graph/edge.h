
#pragma once

#include <string>
#include <vector>

#include "utils/component.h"
#include "utils/exception.h"
#include "utils/log.h"
#include "utils/operating_system.h"
#include "utils/stream_handler.h"

namespace graph
{
////////////////////////////////////////////////////////////////////////////////
/// Base representation of an edge.
///
/// An edge consists of a list of ids of nodes that are part of it. For a
/// regular graph, this list of nodes would always have lenght 2; in the case
/// of a hypergraph it will typically be 2+. There is currently no support for
/// directed graphs.
///
/// NOTE: A node does not know its own id as this information can change when
/// the graph is mutated.
///
class Edge : public utils::Component
{
 public:
  /// Default constructor for an empty edge.
  Edge();

  /// Constructor with node list initialization.
  Edge(const std::vector<int>& node_ids);

  /// Accessor for the list of node ids.
  const std::vector<int>& node_ids() const;

  /// Remove all `node_id`s from this edge.
  void clear_node_ids();

  /// Order the `node_id`s ascendingly.
  void sort_node_ids();

  /// Append a node at the end of the `node_id` list.
  void add_node_id(int node_id);

  /// Find and remove a node from the list.
  ///
  /// This will remove exactly one instance if one is found,
  /// zero otherwise.
  void remove_node_id(int node_id);

  /// Serialize the edge's node_ids under ["ids"].
  void configure(const utils::Json& json) override;

  static size_t num_nodes(const utils::Json& json);

  utils::Structure render() const override;

  utils::Structure get_status() const override;

  virtual void rescale(double scale_factor)
  {
    LOG(ERROR, "rescale by ", scale_factor, " is not supported!");
    throw utils::NotImplementedException("rescale not implemented!");
  }

  size_t nodes_count() const;

  static size_t memory_estimate(size_t num_nodes)
  {
    return sizeof(Edge) + utils::vector_values_memory_estimate<int>(num_nodes);
  }

  struct Get_Node_Ids
  {
    static std::vector<int>& get(Edge& e) { return e.node_ids_; }
    static std::string get_key() { return "ids"; }
  };

  using MembersStreamHandler =
      utils::ObjectMemberStreamHandler<utils::VectorStreamHandler<int>, Edge,
                                      Get_Node_Ids, true>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

 protected:
  /// The list of nodes participating in this edge.
  std::vector<int> node_ids_;
};

}  // namespace graph
