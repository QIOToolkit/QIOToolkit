
#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "utils/config.h"
#include "utils/structure.h"
#include "graph/edge.h"

namespace graph
{
////////////////////////////////////////////////////////////////////////////////
/// Edge subclass with an associated cost.
///
/// This attaches a `Cost` (default type: double) to each edge in a graph. This
/// is used, for instance, to represent the coefficients in the graph
/// representation of an Ising cost function:
///
/// \f[ \mathcal{H} = \sum_{e\in\mathrm{\tiny{\{edges\}}}} c_e \prod_{i\in e}
/// s_i \f]
///
///
template <typename Cost = double>
class CostEdge : public Edge
{
 public:
  CostEdge() : Edge(), cost_(0) {}
  CostEdge(Cost c) : Edge(), cost_(c) {}
  CostEdge(Cost c, const std::vector<int>& node_ids) : Edge(node_ids), cost_(c)
  {
  }

  /// Accessor for the cost.
  Cost cost() const { return cost_; }

  /// Setter for the cost.
  void set_cost(Cost cost) { cost_ = cost; }

  /// Serialize as a regular edge with an entry "c" for the cost.
  void configure(const utils::Json& json) override
  {
    Edge::configure(json);
    this->param(json, "c", cost_)
        .description("cost coefficient of this edge")
        .required();
  }

  utils::Structure render() const override
  {
    auto rendered = Edge::render();
    rendered["c"] = cost_;
    return rendered;
  }

  utils::Structure get_status() const override { return render(); }

  std::string get_class_name() const override { return "graph::CostEdge"; }

  void rescale(double scale_factor) override
  {
    assert(scale_factor > 0);
    cost_ *= scale_factor;
  }

  static size_t memory_estimate(size_t num_nodes)
  {
    return sizeof(CostEdge) - sizeof(Edge) + Edge::memory_estimate(num_nodes);
  }

  struct Get_Cost
  {
    static Cost& get(CostEdge& e) { return e.cost_; }
    static std::string get_key() { return "c"; }
  };

  using MembersStreamHandler =
      utils::ObjectMemberStreamHandler<utils::BasicTypeStreamHandler<Cost>,
                                      CostEdge, Get_Cost, true,
                                      Edge::MembersStreamHandler>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

 private:
  Cost cost_;
};

/// This edge class extends the CostEdge class.
/// The key difference is that EdgeWithFace objects maintain a
/// face ID to which the edge belongs.
/// No validation on an edge's nodes belonging to the face is handled here.
template <typename Cost = double>
class EdgeWithFace : public CostEdge<Cost>
{
 public:
  /// Constructor with node IDs list and face ID initialization.
  EdgeWithFace() : CostEdge<Cost>(), face_id_(0) {}
  EdgeWithFace(Cost c) : CostEdge<Cost>(c), face_id_(0) {}
  EdgeWithFace(Cost c, const std::vector<int>& node_ids)
      : CostEdge<Cost>(c, node_ids), face_id_(0)
  {
  }

  /// Accessor and setter for the face id.
  /// Each edge has just one face.
  int face_id() const { return face_id_; }
  void set_face_id(int face_id) { face_id_ = face_id; }

  std::string get_class_name() const override { return "graph::EdgeWithFace"; }

  static size_t memory_estimate(size_t num_nodes)
  {
    return sizeof(EdgeWithFace) - sizeof(CostEdge<Cost>) +
           CostEdge<Cost>::memory_estimate(num_nodes);
  }

  using StreamHandler =
      utils::ObjectStreamHandler<typename CostEdge<Cost>::MembersStreamHandler,
                                false, EdgeWithFace>;

 private:
  int face_id_;
};

}  // namespace graph
