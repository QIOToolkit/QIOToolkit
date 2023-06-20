// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include "graph/graph.h"

namespace graph
{
class CompactGraphVisitor
{
 public:
  uint64_t elem_pos_;
  uint64_t coefficient_pos_;
  CompactGraphVisitor() : elem_pos_(0), coefficient_pos_() {}
  void reset()
  {
    elem_pos_ = 0;
    coefficient_pos_ = 0;
  }
};
/// Save graph mode of ising and pubo mode in compact format (BitStream)
///
template <typename ELEMTYPE>
class CompactGraph : public utils::Component
{
 public:
  using Edge_T = graph::CostEdge<double>;
  using Configuration_T = GraphConfiguration<Edge_T, graph::Node>;

  CompactGraph()
      : num_nodes_(0),
        num_edges_(0),
        node_end_value_(0),
        edge_end_value_(0),
        properties_()
  {
    static_assert(sizeof(ELEMTYPE) < 8,
                  "Can not be larger than or equal to 8 bytes");
    uint32_t bits = sizeof(ELEMTYPE) * 8;
    const uint64_t const_one = 1;
    node_end_value_ = (ELEMTYPE)((const_one << bits) - 1);
    edge_end_value_ = (ELEMTYPE)((const_one << bits) - 2);
  }

  void set_allow_dup_merge(bool value) { properties_.allow_dup_merge_ = value; }

  double get_const_cost() const { return properties_.const_cost_; }

  bool is_empty() const { return num_nodes_ == 0; }

  void configure(Configuration_T& config)
  {
    LOG_MEMORY_USAGE("begin of graph configure");
    edges_ = std::move(Configuration_T::Get_Edges::get(config));
    LOG_MEMORY_USAGE("end of graph configure");
  }

  void configure(const utils::Json&) override
  {
    throw utils::NotImplementedException(
        "configure from json file in DOM mode is not implemented yet.");
  }

  void init()
  {
    std::vector<std::vector<size_t>> local_nodes;
    normalize_edges<Edge_T>(edges_, local_nodes, properties_, node_name_to_id_,
                            node_id_to_name_);
    num_nodes_ = local_nodes.size();
    num_edges_ = edges_.size();

    size_t elem_size = properties_.accumulated_dependent_vars_ +
                       properties_.accumulated_dependent_terms_;
    coefficients_.reserve(properties_.accumulated_dependent_terms_);
    node_edges_associates_.reserve(elem_size);

    for (int node_id = 0; (size_t)node_id < num_nodes_; node_id++)
    {
      assert(local_nodes[node_id].size() > 0);
      for (size_t i = 0; i < local_nodes[node_id].size(); i++)
      {
        size_t edge_id = local_nodes[node_id][i];
        assert(edge_id < edges_.size());
        coefficients_.push_back(edges_[edge_id].cost());

        for (int cur_node_id : edges_[edge_id].node_ids())
        {
          if (cur_node_id != node_id)
          {
            node_edges_associates_.push_back((ELEMTYPE)cur_node_id);
          }
        }
        if (i == (local_nodes[node_id].size() - 1))
        {
          // This is the end of one variable
          node_edges_associates_.push_back(node_end_value_);
        }
        else
        {
          // This is end of one term
          node_edges_associates_.push_back(edge_end_value_);
        }
      }
    }
  }

  size_t nodes_size() const { return num_nodes_; }

  size_t edges_size() const { return num_edges_; }

  bool is_rescaled() const { return properties_.is_rescaled_; }

  void rescale() { properties_.rescale(); }

  double get_scale_factor() const { return properties_.scale_factor_; }

  int map_output(int internal_id) const
  {
    assert((size_t)internal_id < nodes_size());
    return node_id_to_name_.at(internal_id);
  }

  const std::map<int, int>& output_map() const { return node_id_to_name_; }

  const Edge_T& edge(size_t edge_id) const { return edges_[edge_id]; }

  inline uint64_t read_next_neig(CompactGraphVisitor& reader) const
  {
    assert(reader.elem_pos_ < node_edges_associates_.size());
    uint64_t value = node_edges_associates_[reader.elem_pos_];
    reader.elem_pos_++;
    return value;
  }

  inline double read_next_coeff(CompactGraphVisitor& reader) const
  {
    assert(reader.coefficient_pos_ < coefficients_.size());
    double result = coefficients_[reader.coefficient_pos_];
    reader.coefficient_pos_++;

    return result;
  }

  inline bool is_edge_end(uint32_t value) const
  {
    return value == edge_end_value_;
  }

  inline bool is_node_end(uint32_t value) const
  {
    return value == node_end_value_;
  }

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

  double estimate_max_cost_diff() const
  {
    double max_diff = 0;
    CompactGraphVisitor reader;
    for (int node_id = 0; (size_t)node_id < num_nodes_; node_id++)
    {
      bool end_of_node = false;
      double cost_sum_abs = 0;
      do
      {
        uint64_t value = read_next_neig(reader);
        end_of_node = (value == node_end_value_);
        if (end_of_node || (value == edge_end_value_))
        {
          cost_sum_abs += std::abs(read_next_coeff(reader));
        }
      } while (!end_of_node);
      max_diff = std::max(max_diff, cost_sum_abs);
    }
    return max_diff;
  }

  const std::map<int, int>& get_node_name_to_id_map() const
  {
    return node_name_to_id_;
  }

 private:
  // Map of node name to node internal id (which is continous int based from 0)
  std::map<int, int> node_name_to_id_;

  // Mpa of node internal id to node name (reverse of node_name_to_id_)
  std::map<int, int> node_id_to_name_;

  // Save all the edges.
  std::vector<Edge_T> edges_;

  // List of coefficients of each term associated with variable.
  // [{"c":1.0, "ids":[0,1]}, {"c":2.0, "ids":[1,2]}] would be
  // [1.0,1.0,2.0,2.0]
  std::vector<double> coefficients_;

  // List of variable id associated with each node when they are in the same
  // term the end of a term (edge) is marked by edge_end_value_, and the end of
  // terms of one variable (node) is marked by node_end_value_
  std::vector<ELEMTYPE> node_edges_associates_;

  size_t num_nodes_;
  size_t num_edges_;

  // This marks the end of a node (variable) in node_edges_associates_
  ELEMTYPE node_end_value_;

  // This marks the end of a edge of a node (variable) in node_edges_associates_
  ELEMTYPE edge_end_value_;

  // Graph properties.
  GraphProperties properties_;
};
}  // namespace graph