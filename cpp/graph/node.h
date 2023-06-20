// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>
#include <vector>

#include "utils/component.h"
#include "utils/operating_system.h"
#include "utils/stream_handler.h"

namespace graph
{
////////////////////////////////////////////////////////////////////////////////
/// Base class for graph nodes.
///
/// A node's core property is the list of edges it participates
/// in. This is a list of numeric `edge_id`s which only makes
/// sense in the context of the `Graph` the note belongs to.
///
///   * The node does not know its own `NodeId` as this information might change
///     when the graph is manipulated (nodes ids are modified to ensure they
///     form a contiguous set 0..N).
///
///   * The node does not perform any validation on the list of edge_id's it is
///     given; Ensuring validity of these is within the responsibility of the
///     `Graph`.
///
///   * If you need additional properties on the node, you can inherit from
///     this base class (make sure to also overload `configure` accordingly).
///
class Node : public utils::Component
{
 public:
  /// Create an empty node. (Not participating in any edges).
  Node();

  /// Create a node participating in a given set of edges.
  Node(const std::vector<size_t>& edge_ids);

  /// Return the list of edges this node participates in.
  virtual const std::vector<size_t>& edge_ids() const;

  /// Remove all `edge_id`s. This should typically be invoked from the `Graph`
  /// [not directly], as remove `edge_id`s implies the node's `NodeId` should
  /// be removed from the corresponding edges.
  ///
  /// @see Graph::remove_node
  ///
  void clear_edge_ids();

  /// Ensure `edge_id`s are in ascending order.
  ///
  virtual void sort_edge_ids();

  /// Add an edge to this node.
  virtual void add_edge_id(size_t edge_id);

  /// Remove an edge from this node.
  virtual void remove_edge_id(size_t edge_id);

  /// Read/Write this node from/to a serializer.
  void configure(const utils::Json& json) override;

  utils::Structure render() const override;

  utils::Structure get_status() const override;

  static size_t memory_estimate(size_t num_edges)
  {
    return sizeof(Node) +
           utils::vector_values_memory_estimate<size_t>(num_edges);
  }

  struct Get_Edge_Ids
  {
    static std::vector<size_t>& get(Node& node) { return node.edge_ids_; }
    static std::string get_key() { return "ids"; }
  };

  using MembersStreamHandler =
      utils::ObjectMemberStreamHandler<utils::VectorStreamHandler<size_t>, Node,
                                      Get_Edge_Ids>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

 protected:
  /// List of ids of edges this node participates in.
  std::vector<size_t> edge_ids_;
};

/// This node class extends the regular Node class.
/// The key differences are that NodeWithFaces objects maintain a list
/// of face IDs to which the node belongs and sorts its edge IDS
/// according to the faces to which the corresponding edges belong.
/// No validation on such edges belonging to the faces is handled here.
/// Though NodeWithFaces inherits edge_ids_ from Node, it goes unused.
class NodeWithFaces : public Node
{
 public:
  /// Create a node participating in a given set of edges.
  NodeWithFaces(const std::vector<size_t>& edge_ids = {});

  /// Return the list of edge IDs this node participates in, in the first face.
  const std::vector<size_t>& edge_ids() const override;
  /// Return the list of edge IDs this node participates in for a particular
  /// face.
  const std::vector<size_t>& edge_ids(int face_id) const;
  /// Return the list of edge ID lists (by face) this node participates in.
  const std::vector<std::vector<size_t>>& edge_ids_by_face() const;
  /// Return the list of face IDs this node participates in.
  const std::vector<int>& face_ids() const;

  /// Ensure edge IDs are in ascending order.
  void sort_edge_ids() override;
  /// Add an edge to this node at a particular face.
  void add_edge_id(size_t edge_id) override;
  void add_edge_id(size_t edge_id, int face_id);

  /// Ensure face IDs are in ascending order.
  void sort_face_ids();
  /// Add a face to this node.
  void add_face_id(int face_id);

  /// Search vector of face IDs for a particular ID.
  bool contains_face_id(int face_id);

  size_t edges_count() const;
  size_t faces_count() const;

  static size_t memory_estimate(size_t num_edges)
  {
    return sizeof(NodeWithFaces) +
           utils::vector_values_memory_estimate<size_t>(num_edges);
  }

  using StreamHandler = utils::ObjectStreamHandler<Node::MembersStreamHandler,
                                                  false, NodeWithFaces>;

 protected:
  /// List of ids of edges this node participates in, organized by face.
  std::vector<std::vector<size_t>> edge_ids_by_face_;
  /// List of ids of faces this node participates in.
  std::vector<int> face_ids_;
};

}  // namespace graph
