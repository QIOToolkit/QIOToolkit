
#include "graph/properties.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/json.h"
#include "utils/stream_handler_json.h"
#include "graph/cost_edge.h"
#include "graph/edge.h"
#include "graph/face.h"
#include "graph/graph.h"
#include "graph/node.h"
#include "gtest/gtest.h"

using WeightedGraph = ::graph::Graph<::graph::CostEdge<double>, ::graph::Node>;
using utils::Json;
using graph::get_graph_attributes;
using graph::GraphAttributes;

class PropertiesTest : public testing::Test
{
 public:
  PropertiesTest() {}
};

TEST_F(PropertiesTest, Sample)
{
  //
  //    (0)--(1)
  //   / |    |
  // (2) |    |
  //   \ |    |
  //    (3)--(4)--(5)
  //
  std::string json_string(R"(
    {
      "terms": [
        {"ids": [0,1], "c":1},
        {"ids": [0,2], "c":0.2},
        {"ids": [0,3], "c":1},
        {"ids": [1,4], "c":1},
        {"ids": [2,3,4], "c":1},
        {"ids": [3], "c":1},
        {"ids": [4,5], "c":-10}
      ]
    }
  )");
  WeightedGraph g;
  utils::configure_with_configuration_from_json_string(json_string, g);
  auto properties = get_properties(g);
  std::cout << properties.to_string() << std::endl;

  auto& size = properties["size"];

  EXPECT_EQ(size["edges"].get<uint64_t>(), 7);
  EXPECT_EQ(3, size[utils::kMaxLocality].get<uint32_t>());
  EXPECT_EQ(size["nodes"].get<uint64_t>(), 6);
  EXPECT_EQ(1, size[utils::kMinLocality].get<uint32_t>());
  EXPECT_EQ(2., size[utils::kAvgLocality].get<double>());
  EXPECT_EQ(16, size[utils::kAccumDependentVars].get<uint64_t>());
  EXPECT_EQ(14, size[utils::kTotalLocality].get<uint64_t>());
  EXPECT_EQ(10., size[utils::kMaxCouplingMagnitude].get<double>());
  EXPECT_EQ(0.2, size[utils::kMinCouplingMagnitude].get<double>());
}

TEST(GraphPropertiesTest, MaxNodesInTerm)
{
  std::string json_string(R"(
    {
      "terms": [
        {"ids": [0,1], "c":1},
        {"ids": [1,4], "c":1},
        {"ids": [2,3,4, 200, 20], "c":1},
        {"ids": [3], "c":1},
        {"ids": [4,5], "c":-10},
        {"ids": [], "c":1}
      ]
    }
  )");
  WeightedGraph::Configuration_T configuration;
  utils::configure_from_json_string(json_string, configuration);

  EXPECT_EQ(5,
            get_graph_attributes(
                WeightedGraph::Configuration_T::Get_Edges::get(configuration))
                .max_nodes_in_term);
}

TEST(GraphPropertiesTest, SLCTermPresent)
{
  auto json = utils::json_from_string(R"(
    {
      "terms": [
        {"ids": [0], "c":1},
        {"ids": [1], "c":1}
       ],
      "terms_slc": [
        {"c": 1, "terms": [
          {"ids": [2], "c":1},
          {"ids": [3], "c":1},
          {"ids": [4], "c":-10},
          {"ids": [], "c":1}
        ]}
      ]
    })");

  EXPECT_EQ(true, get_graph_attributes(json).slc_term_present);
  EXPECT_EQ(2, get_graph_attributes(json).max_nodes_in_term);
}

TEST(GraphPropertiesTest, NodesCount)
{
  std::string json_string(R"(
    {
      "terms": [
        {"ids": [0,1], "c":1},
        {"ids": [1,4], "c":1},
        {"ids": [2,3,4, 200, 20], "c":1},
        {"ids": [3, 3], "c":1},
        {"ids": [4,5], "c":-10},
        {"ids": [], "c":1}
      ]
    }
  )");
  WeightedGraph::Configuration_T configuration;
  utils::configure_from_json_string(json_string, configuration);

  EXPECT_EQ(8,
            get_graph_node_count(
                WeightedGraph::Configuration_T::Get_Edges::get(configuration)));
}
