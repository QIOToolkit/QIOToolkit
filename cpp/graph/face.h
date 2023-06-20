// Copyright (c) Microsoft. All Rights Reserved.
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
enum class FaceType
{
  Combination,
  SquaredLinearCombination
};
////////////////////////////////////////////////////////////////////////////////
/// Base representation of a face.
///
/// A face consists of a (face) type, a weight cost, and a list of component
/// edge IDs
///
template <typename Cost = double>
class Face : public utils::Component
{
 public:
  /// Default constructor for an empty face.
  Face() : type_(FaceType::Combination), cost_(1.0) {}
  /// Constructor with type.
  Face(const FaceType type) : type_(type), cost_(1.0) {}
  /// Constructor with cost.
  Face(const Cost c) : type_(FaceType::Combination), cost_(c) {}
  /// Constructor with type and cost.
  Face(const FaceType type, const Cost c) : type_(type), cost_(c) {}
  /// Constructor with edge ID list initialization.
  Face(const std::vector<int>& edge_ids)
      : type_(FaceType::Combination), cost_(1.0), edge_ids_(edge_ids)
  {
  }
  /// Constructor with type, cost, and edge ID list initialization.
  Face(const FaceType type, const Cost c, const std::vector<int>& edge_ids)
      : type_(type), cost_(c), edge_ids_(edge_ids)
  {
  }

  /// Accessor for the list of edge IDs.
  const std::vector<int>& edge_ids() const { return edge_ids_; }
  /// Accessor for face type.
  const FaceType& type() const { return type_; }

  /// Accessor for the cost.
  Cost cost() const { return cost_; }
  /// Setter for the cost.
  void set_cost(Cost cost) { cost_ = cost; }

  /// Order the list of edge IDs ascendingly.
  void sort_edge_ids() { std::sort(edge_ids_.begin(), edge_ids_.end()); }
  /// Append an edge ID to the end of the edge ID list.
  void add_edge_id(int edge_id) { edge_ids_.push_back(edge_id); }
  /// Find and remove an edge ID from the edge ID list.
  void remove_edge_id(int edge_id)
  {
    auto it = std::find(edge_ids_.begin(), edge_ids_.end(), edge_id);
    if (it == edge_ids_.end())
    {
      LOG(ERROR, "Cannot remove edge_id ", edge_id, " from face (not listed).");
      return;
    }
    std::swap(*it, edge_ids_.back());
    edge_ids_.pop_back();
  }

  /// Serialize the face.
  void configure(const utils::Json& json) override
  {
    std::string type_str;
    this->param(json, "type", type_str)
        .description("type of grouped term")
        .default_value("slc");
    if (type_str == "slc")
    {
      type_ = FaceType::SquaredLinearCombination;
    }

    this->param(json, "c", cost_)
        .description("cost coefficient of this face")
        .required();
  }

  utils::Structure render() const override
  {
    utils::Structure rendered;
    switch (type_)
    {
      case FaceType::Combination:
        break;
      case FaceType::SquaredLinearCombination:
        rendered["type"] = "slc";
        break;
    }
    rendered["c"] = cost_;
    return rendered;
  }

  utils::Structure get_status() const override { return edge_ids_; }

  void rescale(double scale_factor)
  {
    assert(scale_factor > 0);
    cost_ *= scale_factor;
  }

  size_t edges_count() const { return edge_ids_.size(); }

  static size_t memory_estimate(size_t num_edges)
  {
    return sizeof(Face) + utils::vector_values_memory_estimate<int>(num_edges);
  }

  struct Get_Cost
  {
    static Cost& get(Face& f) { return f.cost_; }
    static std::string get_key() { return "c"; }
  };

  struct Get_Edge_Ids
  {
    static std::vector<int>& get(Face& f) { return f.edge_ids_; }
    static std::string get_key() { return "ids"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::BasicTypeStreamHandler<Cost>, Face, Get_Cost, true,
      utils::ObjectMemberStreamHandler<utils::VectorStreamHandler<int>,
                                      Get_Edge_Ids, Get_Edge_Ids, true>>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

 private:
  /// Type of grouped cost term captured by the face.
  FaceType type_;
  /// Weight associated to the face (usually term coefficient).
  Cost cost_;
  /// The list of IDs for edges composing the face.
  std::vector<int> edge_ids_;
};
}  // namespace graph
