
#pragma once

#include <float.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <map>
#include <set>
#include <vector>

#include "utils/config.h"
#include "graph/cost_edge.h"
#include "graph/face.h"
#include "graph/graph.h"
#include "graph/node.h"

namespace graph
{
////////////////////////////////////////////////////////////////////////////////
/// Identifiers used in the input format for QIO optimization problems.
///
/// Fces are called "terms_slc", because they represent
/// interacting variables in the cost function polynomial.
constexpr char kFacesInputIdentifier[] = "terms_slc";

class SCL_Term
{
 public:
  using Cost = double;
  using Edge = ::graph::EdgeWithFace<double>;

  SCL_Term() : cost_(0.0) {}

  struct Get_Cost
  {
    static Cost& get(SCL_Term& val) { return val.cost_; }
    static std::string get_key() { return "c"; }
  };

  struct Get_Edges
  {
    static std::vector<Edge>& get(SCL_Term& config) { return config.edges_; }
    static std::string get_key() { return graph::kEdgesInputIdentifier; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<typename Edge::StreamHandler>, SCL_Term,
      Get_Edges, false,
      utils::ObjectMemberStreamHandler<utils::BasicTypeStreamHandler<Cost>,
                                      SCL_Term, Get_Cost, false>>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

  std::vector<Edge>& get_edges() { return edges_; }
  Cost& get_cost() { return cost_; }

 private:
  std::vector<Edge> edges_;
  Cost cost_;
};

class FacedGraphConfiguration
    : public GraphConfiguration<EdgeWithFace<double>, NodeWithFaces>
{
 public:
  using Base_T = GraphConfiguration<EdgeWithFace<double>, NodeWithFaces>;

  struct Get_SLC_Terms
  {
    static std::vector<SCL_Term>& get(FacedGraphConfiguration& graph_config)
    {
      return graph_config.slc_terms_;
    }
    static std::string get_key() { return graph::kFacesInputIdentifier; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<typename SCL_Term::StreamHandler>,
      FacedGraphConfiguration, Get_SLC_Terms, false,
      Base_T::MembersStreamHandler>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

 private:
  std::vector<SCL_Term> slc_terms_;
};

////////////////////////////////////////////////////////////////////////////////
/// Graph representation as a collection of nodes, edges, and faces.
///
/// It assumes, beyond the assumptions for Graph, that:
///
///   * faces are identified with a face ID `0...(F-1)`,
///
/// where F is the number of faces.
class FacedGraph : public Graph<EdgeWithFace<double>, NodeWithFaces>
{
 public:
  FacedGraph() : Graph<EdgeWithFace<double>, NodeWithFaces>() {}

  using Face = ::graph::Face<double>;
  using Edge = ::graph::EdgeWithFace<double>;
  using Node = ::graph::NodeWithFaces;

  using Configuration_T = FacedGraphConfiguration;

  /// Get a face by ID.
  const Face& face(int face_id) const { return faces_[face_id]; }
  /// Access the vector of all faces.
  const std::vector<Face>& faces() const { return faces_; }

  /// Sort the ID lists of each node, edge, and face.
  /// @see Graph::sort
  void sort() override
  {
    Graph::sort();
    for (auto& node : nodes_) node.sort_face_ids();
    for (auto& face : faces_) face.sort_edge_ids();
  }

  /// Validate the FacedGraph.
  /// Ensure that node, edge, and face references are consistent.
  /// @see validate_face
  /// @see Graph::validate
  bool validate() const override
  {
    bool valid = Graph::validate();
    for (size_t face_id = 0; face_id < faces_.size(); face_id++)
    {
      valid &= validate_face(face_id);
    }
    return valid;
  }

  /// Validate a node, ensuring that:
  ///   * the node participants in at least one edge and face,
  ///   * all edges listed have a valid edge ID, and
  ///   * the node is referenced in those edges.
  bool validate_node(int node_indx) const override
  {
    bool valid = false;
    const auto& node = nodes_[node_indx];
    for (int face_id : node.face_ids())
    {
      switch (face(face_id).type())
      {
        case graph::FaceType::Combination:
          break;
        case graph::FaceType::SquaredLinearCombination:
          if (node.edge_ids(face_id).size() != 1)
          {
            throw utils::ValueException(
                "Like terms not combined in face with type slc!");
          }
          break;
      }
      if (!node.edge_ids(face_id).empty())
      {
        valid = true;
        for (size_t edge_id : node.edge_ids(face_id))
        {
          if (edge_id >= edges_.size())
          {
            LOG(ERROR, "Invalid edge_id ", edge_id, " on node ", node_indx,
                ".");
            return false;
          }
          else
          {
            valid &= validate_counts(node_indx, edge_id);
          }
        }
      }
    }
    return valid;
  }

  /// Validate an edge, ensuring that:
  ///   * edges on certain face types contain a node,
  ///   * all nodes listed have a valid edge ID, and
  ///   * the node is referenced in those edges.
  bool validate_edge(size_t edge_id) const override
  {
    bool valid = true;
    const auto& edge = edges_[edge_id];
    if (edge.node_ids().empty())
    {
      switch (face(edge.face_id()).type())
      {
        case graph::FaceType::Combination:
          LOG(WARN, "Edge ", edge_id, " does not contain any nodes.");
          valid = false;
          break;
        case graph::FaceType::SquaredLinearCombination:
          break;
      }
    }

    valid &= Graph::validate_edge(edge_id);
    return valid;
  }

  /// Validate a face, ensuring that:
  ///   * SquaredLinearCombination faces contain an edge, only linear edges,
  ///     and have like-terms combined;
  ///   * all edges listed have a valid face ID; and
  ///   * the face is referenced in those edges.
  bool validate_face(int face_id) const
  {
    bool valid = true;
    const auto& face = faces_[face_id];
    if (face.edge_ids().empty() && face_id > 0)
    {
      LOG(WARN, "Face ", face_id, " does not contain any edges.");
    }
    else
    {
      switch (face.type())
      {
        case FaceType::Combination:
          break;
        case FaceType::SquaredLinearCombination:
          for (size_t i = 0; i < face.edge_ids().size(); i++)
          {
            if (edges_[face.edge_ids()[i]].node_ids().size() > 1)
            {
              throw utils::ValueException(
                  "Face with slc type contains nonlinear edge.");
            }
          }
          break;
      }
    }
    for (size_t edge_id : face.edge_ids())
    {
      if (edge_id >= edges_.size())
      {
        LOG(ERROR, "Invalid edge_id ", edge_id, " on face ", face_id, ".");
        valid = false;
      }
      else if (edges_[edge_id].face_id() != face_id)
      {
        LOG(ERROR, "Invalid edge_id ", edge_id, " on face ", face_id, ".");
        valid = false;
      }
    }
    return valid;
  }

  bool validate_counts(int node_id, size_t edge_id) const override
  {
    const Node& n = node(node_id);
    const Edge& e = edge(edge_id);
    size_t node_count =
        std::count(e.node_ids().begin(), e.node_ids().end(), node_id);
    size_t edge_count = 0;
    for (const std::vector<size_t>& edge_ids : n.edge_ids_by_face())
    {
      edge_count += std::count(edge_ids.begin(), edge_ids.end(), edge_id);
    }
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

  void configure_memory_check(const utils::Json& json)
  {
    size_t faces_memory_estimation = 1;
    size_t edges_memory_estimation = 0;
    size_t n_connections = 0;
    if (json.HasMember(kEdgesInputIdentifier))
    {
      auto it = json.FindMember(kEdgesInputIdentifier);
      if (it->value.IsArray())
      {
        size_t num_edges = 0;
        size_t num_faces = 0;
        size_t num_dicts = it->value.Size();

        #pragma omp parallel for reduction(+ : edges_memory_estimation, n_connections)
        for (size_t i = 0; i < num_dicts; i++)
        {
          if (it->value[i].HasMember("type"))
          {
            // Grouped term found
            num_faces += 1;
            size_t n_edge_id = it->value[i][kEdgesInputIdentifier].Size();
            num_edges += n_edge_id;
            for (size_t j = 0; j < n_edge_id; j++)
            {
              int n_node_id =
                  Edge::num_nodes(it->value[i][kEdgesInputIdentifier][j]);
              edges_memory_estimation += Edge::memory_estimate(n_node_id);
              n_connections += n_node_id;
            }
            faces_memory_estimation += Face::memory_estimate(n_edge_id);
          }
          else
          {
            // Ungrouped edge found
            num_edges += 1;
            int n_node_id = Edge::num_nodes(it->value[i]);
            edges_memory_estimation += Edge::memory_estimate(n_node_id);
            n_connections += n_node_id;
          }
        }
      }
    }

    size_t nodes_memory_estimation = Node::memory_estimate(n_connections);

    size_t memory_estimation =
        faces_memory_estimation +      // faces_ in populate function
        2 * edges_memory_estimation +  // temp_edges here and edges_
                                       // in populate function
        2 * nodes_memory_estimation;   // nodes_ and local_nodes
                                       // in populate function

    size_t available_memory = utils::get_available_memory();

    if (available_memory < memory_estimation)
      throw utils::MemoryLimitedException(
          "Input problem is too large (too many "
          "terms and/or variables). "
          "Expected to exceed machine's current available memory.");
  }

  /// Configure graph from the input by configuring and organizing its component
  /// nodes, edges, and faces. Input is structured as a dictionary including a
  /// value for "terms" which is a list of dictionaries, where each such
  /// dictionary is either an edge (given with "ids" and "c" keys) or a face
  /// (given with "type", "c", and "terms" keys where "terms" has value of a
  /// list of edge dictionaries).
  void configure(const utils::Json& json) override
  {
    // Gather "term" list of dictionaries
    using matcher::GreaterThan;
    using matcher::SizeIs;

    // Grab list of nodes, if applicable
    if (json.HasMember(kNodesInputIdentifier))
    {
      this->param(json, kNodesInputIdentifier, nodes_)
          .description("nodes of the graph")
          .required()
          .matches(SizeIs(GreaterThan(0)));
    }
    // Grab then sort list of term dictionaries into edges and faces
    if (!(json.HasMember(kEdgesInputIdentifier) ||
          json.HasMember(kFacesInputIdentifier)))
    {
      throw utils::MissingInputException(
          "parameters `" + std::string(kEdgesInputIdentifier) + "` or `" +
          std::string(kFacesInputIdentifier) + "` are required");
    }

    std::vector<Face> temp_faces;
    // Organize edges by face
    std::vector<std::vector<Edge>> temp_edges;
    // First edgeset corresponds to ungrouped edges
    std::vector<Edge> na_edges;
    temp_edges.push_back(na_edges);

    // Configure Edge terms
    if (json.HasMember(kEdgesInputIdentifier))
    {
      const rapidjson::Value& temp_dicts = json[kEdgesInputIdentifier];
      assert(temp_dicts.IsArray());

      for (rapidjson::SizeType i = 0; i < temp_dicts.Size(); i++)
      {
        Edge temp_edge;
        temp_edge.configure(temp_dicts[i]);
        temp_edges[0].push_back(std::move(temp_edge));
      }
    }

    // Configure "slc" face terms
    if (json.HasMember(kFacesInputIdentifier))
    {
      const rapidjson::Value& temp_dicts = json[kFacesInputIdentifier];
      assert(temp_dicts.IsArray());

      for (rapidjson::SizeType i = 0; i < temp_dicts.Size(); i++)
      {
        const rapidjson::Value& dict = temp_dicts[i];
        if (!(dict.IsObject() && dict[kEdgesInputIdentifier].IsArray()))
        {
          throw utils::ValueException(
              "Invalid grouped term formatting; needs a list of terms.");
        }

        Face temp_face = Face(FaceType::SquaredLinearCombination);
        if (dict.HasMember("c"))
        {
          temp_face.set_cost(dict["c"].GetDouble());
        }
        for (rapidjson::SizeType j = 0; j < dict[kEdgesInputIdentifier].Size();
             j++)
        {
          if (dict[kEdgesInputIdentifier][j].HasMember(kEdgesInputIdentifier))
          {
            throw utils::ValueException(
                "Nested grouped terms are not supported.");
          }
        }
        temp_faces.push_back(temp_face);
        // Store edge collection corresponding to face
        std::vector<Edge> edgeset;
        this->param(dict, kEdgesInputIdentifier, edgeset)
            .description("edges of the grouped term")
            .required()
            .matches(SizeIs(GreaterThan(0)));
        temp_edges.push_back(std::move(edgeset));
      }
    }
    assert(temp_faces.size() + 1 == temp_edges.size());
    populate(temp_faces, temp_edges);
  }

  void configure(Configuration_T& config)
  {
    // We are not using nodes yet
    // Grab then sort list of term dictionaries into edges and faces

    auto& ungrouped_terms = Configuration_T::Get_Edges::get(config);

    auto& slc_terms = Configuration_T::Get_SLC_Terms::get(config);

    if (ungrouped_terms.size() == 0 && slc_terms.size() == 0)
    {
      throw utils::ValueException(
          "parameters `" + Configuration_T::Get_Edges::get_key() + "` or `" +
          Configuration_T::Get_SLC_Terms::get_key() +
          "`: size must be greater than 0, found 0");
    }
    std::vector<Face> temp_faces;
    temp_faces.reserve(slc_terms.size());
    // Organize edges by face
    std::vector<std::vector<Edge>> temp_edges;
    temp_edges.reserve(slc_terms.size() + 1);

    // Moving ungrouped edges
    temp_edges.push_back(std::move(Configuration_T::Get_Edges::get(config)));

    // Moving face
    for (rapidjson::SizeType i = 0; i < slc_terms.size(); i++)
    {
      auto& slc_term = slc_terms[i];
      Face temp_face = Face(FaceType::SquaredLinearCombination);
      temp_face.set_cost(slc_term.get_cost());
      temp_faces.push_back(temp_face);
      // Store edge collection corresponding to face
      temp_edges.push_back(std::move(slc_term.get_edges()));
    }
    assert(temp_faces.size() + 1 == temp_edges.size());
    populate(temp_faces, temp_edges);
  }

 protected:
  std::vector<Face> faces_;

  // Build the lists of nodes, edges, and faces from temporary edges and faces.
  void populate(std::vector<Face>& temp_faces,
                std::vector<std::vector<Edge>>& temp_edges)
  {
    node_name_to_id_.clear();
    node_id_to_name_.clear();
    faces_.clear();
    edges_.clear();
    nodes_.clear();
    if (temp_edges.size() == 0 && temp_faces.size() == 0)
    {
      throw utils::ValueException(
          "No terms found in input! Cannot populate graph.");
    }

    // Configure face of regular terms
    Face new_face(1.0);
    populate_face(new_face, temp_edges[0]);
    // Configure faces of grouped terms
    for (size_t i = 0; i < temp_faces.size(); i++)
    {
      Face new_face(temp_faces[i].type(), temp_faces[i].cost());
      populate_face(new_face, temp_edges[i + 1]);
    }

    // Sort node lists and validate lists
    for (Node& node : nodes_)
    {
      node.sort_edge_ids();
      node.sort_face_ids();
    }
    assert(validate());
  }

  void populate_face(Face& new_face, std::vector<Edge>& face_edges)
  {
    // pre-emptively return if the face represents a constant
    // note that uncombined constants in an slc term is not supported.
    if (new_face.type() == FaceType::SquaredLinearCombination &&
        face_edges.size() == 1)
    {
      if (face_edges[0].nodes_count() == 0)
      {
        properties_.const_cost_ +=
            new_face.cost() * face_edges[0].cost() * face_edges[0].cost();
        return;
      }
    }

    for (size_t edge_id = 0; edge_id < face_edges.size(); edge_id++)
    {
      // Manage edge
      Edge new_edge(face_edges[edge_id].cost());
      new_edge.set_face_id((int)faces_.size());

      // Integrate node IDs
      for (size_t i = 0; i < face_edges[edge_id].node_ids().size(); i++)
      {
        int input_id = face_edges[edge_id].node_ids()[i];
        if (node_name_to_id_.find(input_id) == node_name_to_id_.end())
        {
          // Node ID has not yet been processed
          node_name_to_id_.insert({input_id, (int)nodes_.size()});
          node_id_to_name_.insert({(int)nodes_.size(), input_id});
          nodes_.push_back(Node());
        }
        int internal_id = node_name_to_id_[input_id];
        assert(internal_id < (int)nodes_.size());
        Node& node = nodes_[internal_id];
        if (std::find(new_edge.node_ids().begin(), new_edge.node_ids().end(),
                      internal_id) == new_edge.node_ids().end())
        {
          new_edge.add_node_id(internal_id);
          node.add_edge_id((int)edges_.size(), (int)faces_.size());
        }
        else if (!properties_.allow_dup_merge_)
        {
          throw utils::DuplicatedVariableException(
              "Duplicated ids detected in term!");
        }
        if (!node.contains_face_id((int)faces_.size()))
        {
          node.add_face_id((int)faces_.size());
        }
      }
      new_edge.sort_node_ids();
      size_t locality = new_edge.nodes_count();
      if (locality == 0)
      {
        switch (new_face.type())
        {
          case FaceType::Combination:
            // Regular constant terms are not invoked as edges
            properties_.const_cost_ += new_edge.cost();
            break;
          case FaceType::SquaredLinearCombination:
            new_face.add_edge_id((int)edges_.size());
            edges_.push_back(new_edge);
            break;
        }
      }
      else
      {
        new_face.add_edge_id((int)edges_.size());
        edges_.push_back(new_edge);
      }

      // Manage graph statistics based on edge
      double abs_cost = fabs(new_face.cost() * new_edge.cost());
      switch (new_face.type())
      {
        case FaceType::Combination:
          break;
        case FaceType::SquaredLinearCombination:
          // abs_cost *= fabs(new_edge.cost());
          // locality *= 2;
          break;
      }
      if (locality > properties_.max_locality_)
      {
        properties_.max_locality_ = locality;
      }
      if (locality < properties_.min_locality_)
      {
        properties_.min_locality_ = locality;
      }
      if (locality > 0)
      {
        properties_.total_locality_ +=
            locality;  // * new_face.edge_ids().size();
        if (locality > 1)
        {
          properties_.accumulated_dependent_vars_ += locality * (locality - 1);
        }
        if (abs_cost > properties_.max_coupling_magnitude_)
        {
          properties_.max_coupling_magnitude_ = abs_cost;
        }
        if (abs_cost < properties_.min_coupling_magnitude_)
        {
          properties_.min_coupling_magnitude_ = abs_cost;
        }
      }
      properties_.avg_locality_ =
          (double)(properties_.total_locality_) / edges_.size();
    }
    if (new_face.edges_count() > 0)
    {
      new_face.sort_edge_ids();

      // Manage graph statistics based on face
      // and enact type-specific validation
      switch (new_face.type())
      {
        case FaceType::Combination:
          break;
        case FaceType::SquaredLinearCombination:
          int constant_count = 0;
          for (size_t i = 0; i < new_face.edge_ids().size(); i++)
          {
            int edge_id = new_face.edge_ids()[i];
            if (edge(edge_id).nodes_count() > 1)
            {
              throw utils::ValueException(
                  "Face with slc type contains nonlinear edge.");
            }
            else if (edge(edge_id).nodes_count() == 1)
            {
              // linear_count += 1;
              int node_id = edge(edge_id).node_ids()[0];
              if (node(node_id).edge_ids((int)faces_.size()).size() != 1)
              {
                throw utils::ValueException(
                    "Like terms not combined in face with type slc!");
              }
            }
            else
            {
              constant_count += 1;
            }
          }
          if (constant_count > 1)
          {
            throw utils::ValueException(
                "Constant terms not combined in face with type slc!");
          }
          break;
      }
    }
    faces_.push_back(std::move(new_face));
  }
};

}  // namespace graph
