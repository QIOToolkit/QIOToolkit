// Copyright (c) Microsoft. All Rights Reserved.
#include "graph/node.h"

#include <algorithm>
#include <sstream>

#include "utils/config.h"
#include "matcher/matchers.h"

namespace graph
{
Node::Node() {}

Node::Node(const std::vector<size_t>& edge_ids) : edge_ids_(edge_ids) {}

const std::vector<size_t>& Node::edge_ids() const { return edge_ids_; }

void Node::clear_edge_ids() { edge_ids_.clear(); }

void Node::sort_edge_ids() { std::sort(edge_ids_.begin(), edge_ids_.end()); }

void Node::add_edge_id(size_t edge_id) { edge_ids_.push_back(edge_id); }

void Node::remove_edge_id(size_t edge_id)
{
  auto it = std::find(edge_ids_.begin(), edge_ids_.end(), edge_id);
  if (it == edge_ids_.end())
  {
    LOG(ERROR, "Cannot remove edge_id ", edge_id, " from node (not listed).");
    return;
  }
  std::swap(*it, edge_ids_.back());
  edge_ids_.pop_back();
}

void Node::configure(const utils::Json& json)
{
  using matcher::Each;
  using matcher::GreaterEqual;

  if (json.HasMember("ids"))
  {
    this->param(json, "ids", edge_ids_)
        .description("edge_ids this node participates in")
        .required();
  }
}

utils::Structure Node::render() const
{
  utils::Structure rendered;
  rendered["ids"] = edge_ids_;
  return rendered;
}

utils::Structure Node::get_status() const
{
  std::stringstream status;
  for (auto edge_id : edge_ids_)
  {
    if (!status.str().empty()) status << " ";
    status << edge_id;
  }
  return status.str();
}

NodeWithFaces::NodeWithFaces(const std::vector<size_t>& edge_ids)
{
  edge_ids_by_face_.push_back(edge_ids);
}

/// The default edge list corresponds to the 0th face, which is typically the
/// catch-all Combination face for non-SLC terms.
const std::vector<size_t>& NodeWithFaces::edge_ids() const
{
  return edge_ids_by_face_[0];
}
const std::vector<size_t>& NodeWithFaces::edge_ids(int face_id) const
{
  return edge_ids_by_face_[face_id];
}
const std::vector<std::vector<size_t>>& NodeWithFaces::edge_ids_by_face() const
{
  return edge_ids_by_face_;
}
const std::vector<int>& NodeWithFaces::face_ids() const { return face_ids_; }

void NodeWithFaces::sort_edge_ids()
{
  for (std::vector<size_t>& edge_ids : edge_ids_by_face_)
  {
    std::sort(edge_ids.begin(), edge_ids.end());
  }
}
void NodeWithFaces::add_edge_id(size_t edge_id)
{
  edge_ids_by_face_[0].push_back(edge_id);
}
void NodeWithFaces::add_edge_id(size_t edge_id, int face_id)
{
  int diff = face_id - (int)edge_ids_by_face_.size();
  for (int i = 0; i <= diff; i++)
  {
    edge_ids_by_face_.push_back({});
  }
  edge_ids_by_face_[face_id].push_back(edge_id);
}

void NodeWithFaces::sort_face_ids()
{
  std::sort(face_ids_.begin(), face_ids_.end());
}
void NodeWithFaces::add_face_id(int face_id) { face_ids_.push_back(face_id); }

bool NodeWithFaces::contains_face_id(int face_id)
{
  return std::find(face_ids_.begin(), face_ids_.end(), face_id) !=
         face_ids_.end();
}

size_t NodeWithFaces::edges_count() const
{
  size_t c = 0;
  for (const std::vector<size_t>& edge_ids : edge_ids_by_face_)
  {
    c += edge_ids.size();
  }
  return c;
}
size_t NodeWithFaces::faces_count() const { return face_ids_.size(); }

}  // namespace graph
