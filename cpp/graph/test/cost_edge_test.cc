// Copyright (c) Microsoft. All Rights Reserved.
#include "../cost_edge.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/proto_reader.h"
#include "../../utils/stream_handler_json.h"
#include "../../utils/stream_handler_proto.h"
#include "gtest/gtest.h"
#include "problem.pb.h"

using CostEdge = ::graph::CostEdge<double>;
using EdgeWithFace = ::graph::EdgeWithFace<double>;
using utils::ConfigurationException;
using utils::Json;

TEST(CostEdge, InitializesDefault)
{
  CostEdge edge;
  EXPECT_EQ(edge.cost(), 0.0);
  EXPECT_EQ(edge.node_ids().size(), 0);
}

TEST(CostEdge, InitializesCost)
{
  CostEdge edge(42);
  EXPECT_EQ(edge.cost(), 42);
  EXPECT_EQ(edge.node_ids().size(), 0);
}

TEST(CostEdge, InitializesCostAndNodeIds)
{
  CostEdge edge(42, {1, 2, 3});
  EXPECT_EQ(edge.cost(), 42);
  EXPECT_EQ(edge.node_ids().size(), 3);
}

TEST(CostEdge, SetsCost)
{
  CostEdge edge;
  EXPECT_EQ(edge.cost(), 0.0);
  edge.set_cost(42);
  EXPECT_EQ(edge.cost(), 42);
}

TEST(CostEdge, Renders)
{
  CostEdge edge(42, {1, 2, 3});
  auto rendered = edge.render().to_string();
  EXPECT_EQ(rendered, R"({"c": 42.000000, "ids": [1,2,3]})");
  edge.set_cost(0);
  edge.clear_node_ids();
  utils::configure_from_json_string<CostEdge>(rendered, edge);
  EXPECT_EQ(edge.cost(), 42);
  EXPECT_EQ(edge.node_ids().size(), 3);
}

TEST(CostEdge, StreamConfigure)
{
  CostEdge edge;
  std::string json_str(R"({"c": 42.000000, "ids": [1,2,3]})");
  utils::configure_from_json_string<CostEdge>(json_str, edge);
  EXPECT_EQ(edge.cost(), 42);
  EXPECT_EQ(edge.node_ids().size(), 3);
}

TEST(CostEdge, Debugs)
{
  CostEdge edge(42, {1, 2, 3});
  EXPECT_EQ(edge.get_class_name(), "graph::CostEdge");
  std::stringstream s;
  s << edge;
  EXPECT_EQ(s.str(), "<graph::CostEdge: {\"c\": 42.000000, \"ids\": [1,2,3]}>");
}

TEST(CostEdge, VerifiesIdsPresent)
{
  auto json = utils::json_from_string(R"({"c":42})");
  CostEdge edge;
  EXPECT_THROW_MESSAGE(edge.configure(json), ConfigurationException,
                       "parameter `ids` is required");
}

TEST(CostEdge, VerifiesIdsPresentStream)
{
  std::string json_str(R"({"c":42})");
  CostEdge edge;
  EXPECT_THROW_MESSAGE(
      utils::configure_from_json_string<CostEdge>(json_str, edge),
      ConfigurationException, "parameter `ids` is required");
}

TEST(CostEdge, VerifiesCostPresent)
{
  auto json = utils::json_from_string(R"({"ids":[1,2,3]})");
  CostEdge edge;
  EXPECT_THROW_MESSAGE(edge.configure(json), ConfigurationException,
                       "parameter `c` is required");
}

TEST(CostEdge, VerifiesCostPresentStream)
{
  std::string json_str(R"({"ids":[1,2,3]})");
  CostEdge edge;
  EXPECT_THROW_MESSAGE(
      utils::configure_from_json_string<CostEdge>(json_str, edge),
      ConfigurationException, "parameter `c` is required");
}
TEST(EdgeWithFace, InitializesDefault)
{
  EdgeWithFace edge;
  EXPECT_EQ(edge.cost(), 0.0);
  EXPECT_EQ(edge.node_ids().size(), 0);
  EXPECT_EQ(edge.face_id(), 0);
}

TEST(EdgeWithFace, InitializesCost)
{
  EdgeWithFace edge(42);
  EXPECT_EQ(edge.cost(), 42);
  EXPECT_EQ(edge.node_ids().size(), 0);
  EXPECT_EQ(edge.face_id(), 0);
}

TEST(EdgeWithFace, SetsFaceID)
{
  EdgeWithFace edge;
  EXPECT_EQ(edge.face_id(), 0);
  edge.set_face_id(42);
  EXPECT_EQ(edge.face_id(), 42);
}

TEST(EdgeWithFace, Renders)
{
  // The face ID should not affect rendering
  EdgeWithFace edge(42, {1, 2, 3});
  auto rendered = edge.render().to_string();
  EXPECT_EQ(rendered, R"({"c": 42.000000, "ids": [1,2,3]})");

  edge.set_cost(0);
  edge.clear_node_ids();
  auto json = utils::json_from_string(rendered);
  edge.configure(json);
  EXPECT_EQ(edge.cost(), 42);
  EXPECT_EQ(edge.node_ids().size(), 3);
}

TEST(CostEdge, StreamProtoLoad)
{
  AzureQuantum::Problem_Term term;
  term.set_c(3.4);
  term.add_ids(100);
  term.add_ids(101);
  CostEdge edge;

  edge.clear_node_ids();
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<CostEdge::StreamHandler> handler_proxy;

  reader.Parse(term, handler_proxy);

  edge = std::move(handler_proxy.get_value());
  std::vector<int> expected_nodes = {100, 101};
  EXPECT_EQ(edge.cost(), 3.4);
  EXPECT_EQ(edge.node_ids().size(), 2);
  EXPECT_EQ(edge.node_ids()[0], 100);
  EXPECT_EQ(edge.node_ids()[1], 101);
}
