
#include "graph/edge.h"

#include <algorithm>
#include <sstream>

#include "utils/config.h"

namespace graph
{
Edge::Edge() {}

Edge::Edge(const std::vector<int>& node_ids) : node_ids_(node_ids) {}

const std::vector<int>& Edge::node_ids() const { return node_ids_; }

void Edge::clear_node_ids() { node_ids_.clear(); }

void Edge::sort_node_ids() { std::sort(node_ids_.begin(), node_ids_.end()); }

void Edge::add_node_id(int node_id) { node_ids_.push_back(node_id); }

void Edge::remove_node_id(int node_id)
{
  // Yes this is "slow", but a vector allows duplicate entries
  // (=cost_function with squared terms) and vector outperforms
  // std::set or similar for small sizes.
  auto it = std::find(node_ids_.begin(), node_ids_.end(), node_id);
  if (it == node_ids_.end())
  {
    LOG(ERROR, "Cannot remove node_id ", node_id, " from edge (not listed).");
    return;
  }
  std::swap(*it, node_ids_.back());
  node_ids_.pop_back();
}

void Edge::configure(const utils::Json& json)
{
  using matcher::Each;
  using matcher::GreaterEqual;

  this->param(json, "ids", node_ids_)
      .description("node ids participating in the edge")
      .required();
}

size_t Edge::num_nodes(const utils::Json& json)
{
  if (json.HasMember("ids"))
  {
    auto it = json.FindMember("ids");
    if (it->value.IsArray()) return it->value.Size();
  }
  return 0;
}

utils::Structure Edge::render() const
{
  utils::Structure rendered;
  rendered["ids"] = node_ids_;
  return rendered;
}

utils::Structure Edge::get_status() const { return node_ids_; }

size_t Edge::nodes_count() const { return node_ids_.size(); }

}  // namespace graph
