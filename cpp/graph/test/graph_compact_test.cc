
#include "../graph_compact.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/stream_handler_json.h"
#include "../cost_edge.h"
#include "../edge.h"
#include "../node.h"
#include "gtest/gtest.h"

using CompactGraph = ::graph::CompactGraph<uint8_t>;
using utils::ConfigurationException;
using utils::DuplicatedVariableException;
using utils::ValueException;

TEST(CompactGraphTest, OneTerm)
{
  CompactGraph one;
  std::string json_str(R"({"terms":[{"ids":[-1], "c":1}]})");
  utils::configure_with_configuration_from_json_string(json_str, one);
  one.init();
  EXPECT_EQ(1, one.nodes_size());
  EXPECT_EQ(1, one.edges_size());
}

TEST(CompactGraphTest, MaxDiff)
{
  CompactGraph graph;
  std::string json_str(
      R"({"terms":[{"ids":[0,1], "c":1}, {"ids":[2,1], "c":-1},  {"ids":[2,0], "c":-2}, {"ids":[2], "c":5}, {"ids":[], "c":100000}]})");
  utils::configure_with_configuration_from_json_string(json_str, graph);
  graph.init();
  double max_cost = graph.estimate_max_cost_diff();
  EXPECT_EQ(8, max_cost);
  EXPECT_EQ(100000, graph.get_const_cost());
  EXPECT_EQ(4, graph.edges_size());
  EXPECT_EQ(3, graph.nodes_size());
}

TEST(CompactGraphTest, DuplicatedNodes)
{
  CompactGraph graph;
  std::string json_str(
      R"({"terms":[{"ids":[0,1], "c":1}, {"ids":[2,2], "c":-1},  {"ids":[2,0], "c":-2}, {"ids":[2], "c":5}, {"ids":[], "c":100}, {"ids":[], "c":700}]})");
  utils::configure_with_configuration_from_json_string(json_str, graph);
  graph.set_allow_dup_merge(true);
  graph.init();
  double max_cost = graph.estimate_max_cost_diff();
  EXPECT_EQ(8, max_cost);
  EXPECT_EQ(800, graph.get_const_cost());
  EXPECT_EQ(4, graph.edges_size());
  EXPECT_EQ(3, graph.nodes_size());
  EXPECT_EQ(2, graph.get_locality());
  EXPECT_EQ(0, graph.get_min_locality());
  EXPECT_EQ(1.5, graph.get_avg_locality());
  EXPECT_EQ(4, graph.get_accumulated_dependent_vars());
  EXPECT_EQ(5., graph.get_max_coupling_magnitude());
  EXPECT_EQ(1., graph.get_min_coupling_magnitude());
  EXPECT_EQ(6, graph.get_sum_coefficient_degrees_total());
}

TEST(CompactGraphTest, DuplicatedNodesException)
{
  CompactGraph graph;
  std::string json_str(
      R"({"terms":[{"ids":[0,1], "c":1}, {"ids":[2,2], "c":-1},  {"ids":[2,0], "c":-2}, {"ids":[2], "c":5}, {"ids":[], "c":100}, {"ids":[], "c":700}]})");
  utils::configure_with_configuration_from_json_string(json_str, graph);
  graph.set_allow_dup_merge(false);
  EXPECT_THROW_MESSAGE(graph.init(), DuplicatedVariableException,
                       "Duplicated ids detected in term!");
}

TEST(CompactGraphTest, VerifiesEdgesPresentStream)
{
  CompactGraph empty;
  EXPECT_THROW_MESSAGE(
      utils::configure_with_configuration_from_json_string("{}", empty),
      ConfigurationException, "parameter `terms` is required");
}

TEST(CompactGraphTest, VerifiesEdgesNonEmpty)
{
  CompactGraph empty;
  utils::configure_with_configuration_from_json_string(R"({"terms":[]})", empty);
  EXPECT_THROW_MESSAGE(
      empty.init(), ValueException,
      "parameter `terms`: size must be greater than 0, found 0");
}

TEST(CompactGraphTest, ConstEdges)
{
  CompactGraph empty;
  utils::configure_with_configuration_from_json_string(
      R"({"terms":[{"ids":[], "c":200}, {"ids":[], "c":700}]})", empty);
  empty.init();
  EXPECT_TRUE(empty.is_empty());
  EXPECT_EQ(0, empty.nodes_size());
  EXPECT_EQ(0, empty.edges_size());
  EXPECT_EQ(900, empty.get_const_cost());
}