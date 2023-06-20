// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../stream_handler.h"
#include "problem.pb.h"

// Example of simple model
class Edge
{
 public:
  Edge() : c(0) {}
  double c;
  std::vector<int> nodes;

  struct Get_C
  {
    static double& get(Edge& e) { return e.c; }
    static std::string get_key() { return "c"; }
  };

  struct Get_Nodes
  {
    static std::vector<int>& get(Edge& e) { return e.nodes; }
    static std::string get_key() { return "ids"; }
  };
};

class Graph
{
 public:
  std::vector<Edge> edges;
  struct Get_Edges
  {
    static std::vector<Edge>& get(Graph& g) { return g.edges; }
    static std::string get_key() { return "terms"; }
  };
};

class Model
{
 public:
  std::string version;
  std::string type;
  Graph graph;

  struct Get_Model
  {
    static Model& get(Model& m) { return m; }
    static std::string get_key() { return "cost_function"; }
  };

  struct Get_Version
  {
    static std::string& get(Model& m) { return m.version; }
    static std::string get_key() { return "version"; }
  };

  struct Get_Type
  {
    static std::string& get(Model& m) { return m.type; }
    static std::string get_key() { return "type"; }
  };

  struct Get_Edges
  {
    static std::vector<Edge>& get(Model& m) { return m.graph.edges; }
    static std::string get_key() { return "terms"; }
  };
};

// class for streaming Edge object
using EdgeObjectHandler =
    utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
        utils::BasicTypeStreamHandler<double>, Edge, Edge::Get_C, true,
        utils::ObjectMemberStreamHandler<utils::VectorStreamHandler<int>, Edge,
                                        Edge::Get_Nodes>>>;
using GraphObjetHandler =
    utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
        utils::VectorStreamHandler<Edge>, Graph, Graph::Get_Edges>>;

// class for streaming Model object
using ModelObjectHandler =
    utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
        utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
            utils::BasicTypeStreamHandler<std::string>, Model, Model::Get_Type,
            true,
            utils::ObjectMemberStreamHandler<
                utils::BasicTypeStreamHandler<std::string>, Model,
                Model::Get_Version, true,
                utils::ObjectMemberStreamHandler<
                    utils::VectorObjectStreamHandler<EdgeObjectHandler>, Model,
                    Model::Get_Edges, true,
                    utils::AnyKeyObjectMemberStreamHandler<Model>>>>>,
        Model, Model::Get_Model, true>>;

// class for getting version from model object
using ModelPreviewObjectHandler = utils::ObjectStreamHandler<
    utils::ObjectMemberStreamHandler<
        utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
            utils::BasicTypeStreamHandler<std::string>, Model, Model::Get_Type,
            true,
            utils::ObjectMemberStreamHandler<
                utils::BasicTypeStreamHandler<std::string>, Model,
                Model::Get_Version, true,
                utils::AnyKeyObjectMemberStreamHandler<Model>>>>,
        Model, Model::Get_Model, true>,
    true>;

// Function to write tp protobuf folder
unsigned write_to_protobuf_folder(
    std::string& folder_path, std::vector<AzureQuantum::Problem*>& problem_msgs)
{
  // Writing 10 terms per message file
  unsigned file_count = 0;
  std::ofstream out;
  std::string folder_name;
  unsigned found = folder_path.find_last_of("/\\");
  if (found)
  {
    folder_name = folder_path.substr(found + 1);
  }
  else
  {
    // The folder is in the working directory
    folder_name = folder_path;
  }

  for (unsigned i = 0; i < problem_msgs.size(); i++)
  {
    std::string file_name =
        folder_name + "_" + std::to_string(file_count++) + ".pb";
    out.open(folder_path + "/" + file_name, std::ios::out | std::ios::binary);
    problem_msgs[i]->SerializeToOstream(&out);
    out.close();
  }
  return file_count;
}

// Function to create a vector of problem messages for testing
void create_test_proto_msgs(std::vector<AzureQuantum::Problem*>& problem_msgs)
{
  std::ofstream out;
  for (unsigned p = 0; p < 4; p++)
  {
    AzureQuantum::Problem* problem = new AzureQuantum::Problem();
    AzureQuantum::Problem_CostFunction* cost_function =
        problem->mutable_cost_function();
    if (p == 0)
    {
      // Add the version and type for the first message
      cost_function->set_version("1.0");
      cost_function->set_type(AzureQuantum::Problem_ProblemType_ISING);
    }
    for (unsigned i = 0; i < (p + 1); i++)
    {
      AzureQuantum::Problem_Term* term = cost_function->add_terms();
      term->set_c(i + 2.5);
      term->add_ids(i);
      term->add_ids(i + 1);
    }
    problem_msgs.push_back(problem);
  }
}