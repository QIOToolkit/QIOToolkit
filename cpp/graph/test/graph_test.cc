
#include "../graph.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/file.h"
#include "../../utils/proto_reader.h"
#include "../../utils/stream_handler_json.h"
#include "../../utils/stream_handler_proto.h"
#include "../cost_edge.h"
#include "../edge.h"
#include "../face.h"
#include "../faced_graph.h"
#include "../node.h"
#include "gtest/gtest.h"
#include "problem.pb.h"

using Graph = ::graph::Graph<::graph::CostEdge<double>, ::graph::Node>;
using GraphSLC = ::graph::FacedGraph;
using utils::ConfigurationException;
using ::utils::Error;
using utils::Json;

void create_graph_proto_test()
{
  QuantumUtil::Problem* problem = new QuantumUtil::Problem();
  QuantumUtil::Problem_CostFunction* cost_function =
      problem->mutable_cost_function();
  cost_function->set_version("1.0");
  cost_function->set_type(QuantumUtil::Problem_ProblemType_ISING);
  for (unsigned i = 0; i < 4; i++)
  {
    QuantumUtil::Problem_Term* term = cost_function->add_terms();
    term->set_c(1.0);
    term->add_ids(i);
    i == 3 ? term->add_ids(0) : term->add_ids(i + 1);
  }
  std::string folder_name = utils::data_path("graph_input_problem_pb");
  std::ofstream out;
  out.open(folder_name + "/" + "graph_input_problem_pb_0.pb",
           std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
}

class GraphTest : public testing::Test
{
 public:
  GraphTest()
  {
    // NOTE: nodes are auto-populated from edges.
    std::string json_str(R"({
      "terms": [
        {"c": 1.000000, "ids": [0,1]},
        {"c": 1.000000, "ids": [1,2]},
        {"c": 1.000000, "ids": [2,3]},
        {"c": 1.000000, "ids": [0,3]}
      ]})");

    utils::configure_with_configuration_from_json_string(json_str, square);
    utils::configure_with_configuration_from_json_string(json_str, square_slc);

    create_graph_proto_test();
    std::string folder_name = utils::data_path("graph_input_problem_pb");

    std::ifstream in(utils::data_path("graph_input_problem_pb_0.pb"));
    QuantumUtil::Problem* prob = new QuantumUtil::Problem();
    prob->ParseFromIstream(&in);
    utils::configure_with_configuration_from_proto_folder(folder_name,
                                                         square_proto);
  }

  std::string AsJson(Graph& g)
  {
    std::string rendered = g.render().to_string();
#ifdef __INTEL_COMPILER
    // gtest with Intel compiler has a bug where it doesn't parse raw
    // strings literals properly in EXPECT_EQ(...):
    // Lines are separated by \\n // instead of \n.
    size_t pos;
    while ((pos = rendered.find("\n")) != std::string::npos)
    {
      rendered.replace(pos, 1, "\\n");
    }
#endif
    return rendered;
  }

 protected:
  Graph empty;
  Graph square;
  Graph square_proto;

  GraphSLC empty_slc;
  GraphSLC square_slc;
};

TEST_F(GraphTest, InitializesEmpty)
{
  EXPECT_EQ(empty.edges().size(), 0);
  EXPECT_EQ(empty.nodes().size(), 0);

  EXPECT_EQ(empty_slc.edges().size(), 0);
  EXPECT_EQ(empty_slc.nodes().size(), 0);
}

TEST_F(GraphTest, Serializes)
{
  EXPECT_EQ(square.edges().size(), 4);
  EXPECT_EQ(square.nodes().size(), 4);

  EXPECT_EQ(AsJson(square), R"({
  "terms": [
    {"c": 1.000000, "ids": [0,1]},
    {"c": 1.000000, "ids": [1,2]},
    {"c": 1.000000, "ids": [2,3]},
    {"c": 1.000000, "ids": [0,3]}
  ],
  "variables": [
    {"ids": [0,3]},
    {"ids": [0,1]},
    {"ids": [1,2]},
    {"ids": [2,3]}
  ]
})");
}

TEST_F(GraphTest, Sorts)
{
  EXPECT_EQ(square.edges().size(), 4);
  EXPECT_EQ(square.nodes().size(), 4);
  square.sort();
  EXPECT_EQ(square.edges().size(), 4);
  EXPECT_EQ(square.nodes().size(), 4);

  EXPECT_EQ(AsJson(square), R"({
  "terms": [
    {"c": 1.000000, "ids": [0,1]},
    {"c": 1.000000, "ids": [1,2]},
    {"c": 1.000000, "ids": [2,3]},
    {"c": 1.000000, "ids": [0,3]}
  ],
  "variables": [
    {"ids": [0,3]},
    {"ids": [0,1]},
    {"ids": [1,2]},
    {"ids": [2,3]}
  ]
})");
}

TEST_F(GraphTest, VerifiesEdgesPresent)
{
  EXPECT_THROW_MESSAGE(empty.configure(utils::json_from_string("{}")),
                       ConfigurationException, "parameter `terms` is required");
}

TEST_F(GraphTest, VerifiesEdgesPresentStream)
{
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string("{}", empty),
      ConfigurationException, "parameter `terms` is required");
}

TEST_F(GraphTest, VerifiesEdgesNonEmpty)
{
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(R"({"terms":[]})",
                                                          empty),
      ConfigurationException,
      "parameter `terms`: size must be greater than 0, found 0");
}

TEST_F(GraphTest, VerifiesEdgesValid)
{
  std::string json_str(R"({"terms":[{"ids":[-1], "c":1}]})");
  utils::configure_with_configuration_from_json_string(json_str, empty);
}

TEST_F(GraphTest, VerifiesNodesNonEmpty)
{
  auto json = utils::json_from_string(
      R"({"terms":[{"ids":[0,1],"c":1}], "variables": []})");
  EXPECT_THROW_MESSAGE(
      empty.configure(json), ConfigurationException,
      "parameter `variables`: size must be greater than 0, found 0");
}

TEST_F(GraphTest, VerifiesNodesValid)
{
  std::string json_str(
      R"({"terms":[{"ids":[0,1], "c":1}], "variables": [{"ids":[-1]}]})");
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json_str, empty),
      ConfigurationException,
      "parameter `variables`: parameter `ids`: expected uint64, found -1 "
      "(int64)");
}

TEST_F(GraphTest, FacesConfigured)
{
  std::string json_faced(R"({
      "terms": [
        {"c": 1.1, "ids": [0,1]},
        {"c": 2.1, "ids": [0,2]}
      ],
      "terms_slc": [
        {"c": 2.0, "terms": [
          {"c": 2.9, "ids": [1]},
          {"c": 8.0, "ids": []}
        ]},
        {"c": 2.0, "terms": [
          {"c": 1.2, "ids": [1]},
          {"c": 1.3, "ids": [2]},
          {"c": 1.4, "ids": [3]}
        ]}
      ]})");
  GraphSLC faced = GraphSLC();
  utils::configure_with_configuration_from_json_string(json_faced, faced);
  EXPECT_EQ(faced.faces().size(), 3);
  EXPECT_EQ(faced.edges().size(), 7);
  EXPECT_EQ(faced.nodes().size(), 4);

  EXPECT_EQ(faced.face(0).edges_count(), 2);
  EXPECT_EQ(faced.face(1).edges_count(), 2);
  EXPECT_EQ(faced.face(2).edges_count(), 3);

  EXPECT_EQ(faced.edge(0).nodes_count(), 2);
  EXPECT_EQ(faced.edge(1).nodes_count(), 2);
  EXPECT_EQ(faced.edge(2).nodes_count(), 1);
  EXPECT_EQ(faced.edge(3).nodes_count(), 0);
  EXPECT_EQ(faced.edge(4).nodes_count(), 1);
  EXPECT_EQ(faced.edge(5).nodes_count(), 1);
  EXPECT_EQ(faced.edge(6).nodes_count(), 1);
}

TEST_F(GraphTest, NonlinearSLCException)
{
  std::string json_nonlinear(R"({
      "terms": [
        {"c": 1.1, "ids": [0,1]},
        {"c": 2.1, "ids": [0,2]}
       ],
      "terms_slc": [
        {"c": 2.0, "terms": [
          {"c": 2.9, "ids": [1]},
          {"c": 8.0, "ids": []}
        ]},
        {"c": 2.0, "terms": [
          {"c": 1.2, "ids": [1,2]},
          {"c": 1.3, "ids": [2]},
          {"c": 1.4, "ids": [3]}
        ]}
      ]})");
  GraphSLC faced_nonlinear = GraphSLC();
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(
                           json_nonlinear, faced_nonlinear),
                       ConfigurationException,
                       "Face with slc type contains nonlinear edge.");
}

TEST_F(GraphTest, NestedSLCException)
{
  std::string json_nested(R"({
      "terms": [
        {"c": 0.1, "ids": [0,1]}
       ],
      "terms_slc": [
        {"c": 2.0, "terms": [
          {"c": 1.0, "terms": [
            {"c": 1, "ids": [1]},
            {"c": 2, "ids": [2]},
            {"c": 3, "ids": [3]}
          ]},
          {"c": 8.0, "ids": []}
        ]}
      ]})");
  GraphSLC faced_nested = GraphSLC();
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string(json_nested,
                                                          faced_nested),
      ConfigurationException,
      "parameter `terms_slc`: parameter `terms`: parsing error");
  // "Nested grouped terms are not supported." message is too specific to face
  // graph.
}

TEST_F(GraphTest, UnsimplifiedSLCException)
{
  std::string json_unsim(R"({
      "terms": [
        {"c": 0.1, "ids": [0,1]}
       ],
      "terms_slc": [
        {"c": 2.0, "terms": [
          {"c": 1, "ids": [3]},
          {"c": 2, "ids": [2]},
          {"c": 3, "ids": [3]},
          {"c": 8.0, "ids": []}
        ]}
      ]})");
  GraphSLC faced_unsim = GraphSLC();
  EXPECT_THROW_MESSAGE(utils::configure_with_configuration_from_json_string(
                           json_unsim, faced_unsim),
                       utils::ValueException,
                       "Like terms not combined in face with type slc!");
}

TEST_F(GraphTest, NAFacesConfigured)
{
  std::string input(R"({
      "terms": [
          {"c": 1, "ids": [0, 1]},
          {"c": 1, "ids": [1, 2]},
          {"c": 1, "ids": [2, 3]},
          {"c": 1, "ids": [3, 4]},
          {"c": 1, "ids": [4, 5]},
          {"c": 1, "ids": [5, 6]},
          {"c": 1, "ids": [6, 7]},
          {"c": 1, "ids": [7, 8]},
          {"c": 1, "ids": [8, 9]},
          {"c": 1, "ids": [9, 0]}
        ]
      })");
  GraphSLC faced = GraphSLC();
  utils::configure_with_configuration_from_json_string(input, faced);
  EXPECT_EQ(faced.faces().size(), 1);
  EXPECT_EQ(faced.edges().size(), 10);
  EXPECT_EQ(faced.nodes().size(), 10);

  EXPECT_EQ(faced.face(0).edges_count(), 10);

  EXPECT_EQ(faced.edge(0).nodes_count(), 2);
  for (size_t i = 0; i < 10; i++)
  {
    EXPECT_EQ(faced.edge(i).nodes_count(), 2);
  }

  EXPECT_EQ(faced.node(0).edges_count(), 2);
  EXPECT_EQ(faced.node(0).faces_count(), 1);
  for (size_t i = 0; i < 10; i++)
  {
    EXPECT_EQ(faced.node(i).edges_count(), 2);
    EXPECT_EQ(faced.node(i).faces_count(), 1);
  }
}

TEST_F(GraphTest, MaxCostDiff)
{
  Graph graph;
  std::string json_str(
      R"({"terms":[{"ids":[0,1], "c":1}, {"ids":[2,1], "c":-1},  {"ids":[2,0], "c":-2}, {"ids":[2], "c":5}, {"ids":[], "c":100000}]})");
  utils::configure_with_configuration_from_json_string(json_str, graph);
  double max_cost = graph.estimate_max_cost_diff();
  EXPECT_EQ(8, max_cost);
}

TEST_F(GraphTest, Serializes_Proto)
{
  EXPECT_EQ(square_proto.edges().size(), 4);
  EXPECT_EQ(square_proto.nodes().size(), 4);
}

TEST_F(GraphTest, Sorts_Proto)
{
  EXPECT_EQ(square_proto.edges().size(), 4);
  EXPECT_EQ(square_proto.nodes().size(), 4);
  square_proto.sort();
  EXPECT_EQ(square_proto.edges().size(), 4);
  EXPECT_EQ(square_proto.nodes().size(), 4);
}