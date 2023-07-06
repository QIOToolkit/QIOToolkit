

#include "rapidjson/stream.h"

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

#include "utils/json.h"
#include "utils/file.h"
#include "utils/stream_handler.h"
#include "utils/stream_handler_json.h"
#include "gtest/gtest.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/reader.h"

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

TEST(Stream, StreamEdge)
{
  std::string json = R"({ "c" : 2, "ids" : [100, 101]})";

  using HandlerType = EdgeObjectHandler;
  Edge edge;
  utils::configure_from_json_string<Edge, HandlerType>(json, edge);

  std::vector<int> expected_nodes = {100, 101};
  EXPECT_EQ(edge.nodes, expected_nodes);

  EXPECT_EQ(edge.c, double(2));
}

TEST(Stream, StreamVectorVector)
{
  std::string json = R"([[-1,-1], [2,100]])";
  std::vector<std::vector<int>> edges;
  using HandlerType =
      utils::VectorObjectStreamHandler<utils::VectorStreamHandler<int>>;
  utils::configure_from_json_string<std::vector<std::vector<int>>, HandlerType>(
      json, edges);

  // Check correctness of model load
  ASSERT_EQ(edges.size(), 2);
  EXPECT_EQ(edges[0], std::vector<int>({-1, -1}));
  EXPECT_EQ(edges[1], std::vector<int>({2, 100}));
}

TEST(Stream, StreamPuboModel)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"c":1, "ids":[-1,-1]}, {"c":2, "ids":[2,100]}]
    }
  })";

  Model model;
  using HandlerType = ModelObjectHandler;
  utils::configure_from_json_string<Model, HandlerType>(json, model);

  ASSERT_EQ(model.graph.edges.size(), 2);
  EXPECT_EQ(model.graph.edges[0].c, double(1));
  ASSERT_EQ(model.graph.edges[0].nodes.size(), 2);
  EXPECT_EQ(model.graph.edges[0].nodes[0], -1);
  EXPECT_EQ(model.graph.edges[0].nodes[1], -1);
  ASSERT_EQ(model.graph.edges[1].nodes.size(), 2);
  EXPECT_EQ(model.graph.edges[1].nodes[0], 2);
  EXPECT_EQ(model.graph.edges[1].nodes[1], 100);
}

// Base tests

TEST(Stream, StreamInt)
{
  std::string json = R"(2)";

  using HandlerType = utils::BasicTypeStreamHandler<int>;

  int node = 0;
  utils::configure_from_json_string<int, HandlerType>(json, node);

  EXPECT_EQ(node, 2);
}

TEST(Stream, StreamDouble)
{
  std::string json = R"(2)";
  double c = 0;

  using HandlerType = utils::BasicTypeStreamHandler<double>;
  utils::configure_from_json_string<double, HandlerType>(json, c);
  EXPECT_EQ(c, double(2));
}

TEST(Stream, StreamIntArray)
{
  std::string json = R"([2,100])";
  using HandlerType = utils::VectorStreamHandler<int>;

  std::vector<int> nodes;
  utils::configure_from_json_string<std::vector<int>, HandlerType>(json, nodes);

  std::vector<int> expected_nodes = {2, 100};
  EXPECT_EQ(nodes, expected_nodes);
}

TEST(Stream, StreamIntMap)
{
  std::string json = R"({"0": -1, "2": 1, "3": -1})";
  using HandlerType = utils::PairVectorStreamHandler<int, int>;

  std::vector< std::pair<int, int>> initial_configuration;
  utils::configure_from_json_string<std::vector<std::pair<int, int>>,
                                   HandlerType>(
      json, initial_configuration);

  std::vector<std::pair<int, int>> expected_initial_configuration{
      {0, -1}, {2, 1}, {3, -1}};

  EXPECT_EQ(initial_configuration, expected_initial_configuration);
}

TEST(Stream, SimpleStreamObjectVector)
{
  std::string json = R"([{"c":1}, {"c":2}])";

  class SimpleEdge
  {
   public:
    double c;
    struct Get_C
    {
      static double& get(SimpleEdge& val) { return val.c; }
      static std::string get_key() { return "c"; }
    };
  };

  using SimpleEdgeObjectHandler =
      utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
          utils::BasicTypeStreamHandler<double>, SimpleEdge, SimpleEdge::Get_C>>;

  std::vector<SimpleEdge> edges;
  using HandlerType = utils::VectorObjectStreamHandler<SimpleEdgeObjectHandler>;

  utils::configure_from_json_string<std::vector<SimpleEdge>, HandlerType>(json,
                                                                         edges);

  // Check correctness of model load
  EXPECT_EQ(edges.size(), 2);
  if (edges.size() < 2) return;
  EXPECT_EQ(edges[0].c, double(1));
  EXPECT_EQ(edges[1].c, double(2));
}

TEST(Stream, StreamObjectVector)
{
  std::string json = R"([{"c":1, "ids":[-1,-1]}, {"c":2, "ids":[2,100]}])";

  std::vector<Edge> edges;
  using HandlerType = utils::VectorObjectStreamHandler<EdgeObjectHandler>;

  utils::configure_from_json_string<std::vector<Edge>, HandlerType>(json, edges);

  // Check correctness of model load
  ASSERT_EQ(edges.size(), 2);
  EXPECT_EQ(edges[0].c, double(1));
  ASSERT_EQ(edges[0].nodes.size(), 2);
  EXPECT_EQ(edges[0].nodes[0], -1);
  EXPECT_EQ(edges[0].nodes[1], -1);
  ASSERT_EQ(edges[1].nodes.size(), 2);
  EXPECT_EQ(edges[1].nodes[0], 2);
  EXPECT_EQ(edges[1].nodes[1], 100);
}

TEST(Stream, ModelStreamHandler)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "initial_configuration": { "2": 0, "100": 1, "-1": 1},
      "terms": [{"c":1, "ids":[-1,-1]}, {"c":2, "ids":[2,100]}],
      "version": "1.1"
    }
  })";

  Model model;
  utils::configure_from_json_string<Model, ModelObjectHandler>(json, model);
  // Check correctness of model load
  EXPECT_EQ(model.type, "pubo");
  EXPECT_EQ(model.version, "1.1");
  EXPECT_EQ(model.graph.edges.size(), 2);
}

TEST(Stream, ObserverStreamHandler)
{
  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "terms": [{"c":1, "ids":[-1,-1]}, {"c":2, "ids":[2,100]}],
      "version": "1.1"
    }
  })";

  Model model;
  utils::configure_from_json_string<Model, ModelPreviewObjectHandler>(json,
                                                                     model);
  // Check correctness of model load
  EXPECT_EQ(model.type, "pubo");
  EXPECT_EQ(model.version, "1.1");
  // Streamer should skip load of terms.
  EXPECT_EQ(model.graph.edges.size(), 0);
}

TEST(Stream, ObserverStreamHandlerStop)
{
  // After loading type and version streamer expected to break,
  // despite terms are in incorrect format, errors should not occur.

  std::string json = R"({
    "cost_function": {
      "type": "pubo",
      "version": "1.1",
      "terms": [{"cccc":"dd", "ids": "-1"}, {"c":2, "ids":[2,100]}]
    }
  })";

  Model model;
  utils::configure_from_json_string<Model, ModelPreviewObjectHandler>(json,
                                                                     model);
  // Check correctness of model load
  EXPECT_EQ(model.type, "pubo");
  EXPECT_EQ(model.version, "1.1");
  EXPECT_EQ(model.graph.edges.size(), 0);
}

// Performance test

TEST(Stream, StreamPerformance)
{
  // To set realtive performance, timing of
  // using JSON document and streaming is compared.

  std::string file_name =
      utils::data_path("input_e2e_pipeline/larger_quality_cases/"
      "wishart_planting_N_729_alpha_2.50_inst_1.json");
  Model model_preview_stream, model_stream;

  // Load JSON document
  auto start_time = std::chrono::high_resolution_clock::now();
  auto json = utils::json_from_file(file_name);
  auto end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_configure_json =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();

  // Preview using streaming interface
  start_time = std::chrono::high_resolution_clock::now();
  utils::configure_from_json_file<Model, ModelPreviewObjectHandler>(
      file_name, model_preview_stream);
  end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_preview_stream =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();

  // Configure using streaming interface
  start_time = std::chrono::high_resolution_clock::now();
  utils::configure_from_json_file<Model, ModelObjectHandler>(file_name,
                                                            model_stream);
  end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_configure_stream =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();

  std::cout << "Load from JSON: " << ms_diff_configure_json << " ms."
            << std::endl;
  std::cout << "Stream preview:    " << ms_diff_preview_stream << " ms."
            << std::endl;
  std::cout << "Stream configure:    " << ms_diff_configure_stream << " ms."
            << std::endl;

  EXPECT_LE(ms_diff_preview_stream, ms_diff_configure_json / 10);

#ifdef _DEBUG
  EXPECT_LE(ms_diff_configure_stream, ms_diff_configure_json * 5);
#else
  EXPECT_LE(ms_diff_configure_stream, ms_diff_configure_json * 3);
#endif
}
