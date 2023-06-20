// Copyright (c) Microsoft. All Rights Reserved.
#include "../edge.h"

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

using utils::ConfigurationException;
using utils::Json;
using ::graph::Edge;

TEST(Edge, InitializesEmpty)
{
  Edge edge;
  EXPECT_EQ(edge.node_ids().size(), 0);
}

TEST(Edge, InitializesWithListe)
{
  Edge edge({1, 2, 3});
  ASSERT_EQ(edge.node_ids().size(), 3);
  EXPECT_EQ(edge.node_ids()[0], 1);
  EXPECT_EQ(edge.node_ids()[1], 2);
  EXPECT_EQ(edge.node_ids()[2], 3);
}

TEST(Edge, ClearsNodeIds)
{
  Edge edge({1, 2, 3});
  EXPECT_EQ(edge.node_ids().size(), 3);
  edge.clear_node_ids();
  EXPECT_EQ(edge.node_ids().size(), 0);
}

TEST(Edge, SortsNodeIds)
{
  Edge edge({2, 1, 3, 1});
  ASSERT_EQ(edge.node_ids().size(), 4);
  edge.sort_node_ids();
  EXPECT_EQ(edge.node_ids()[0], 1);
  EXPECT_EQ(edge.node_ids()[1], 1);
  EXPECT_EQ(edge.node_ids()[2], 2);
  EXPECT_EQ(edge.node_ids()[3], 3);
}

TEST(Edge, AddsNodeId)
{
  Edge edge({1, 2, 3});
  edge.add_node_id(1);
  ASSERT_EQ(edge.node_ids().size(), 4);
  EXPECT_EQ(edge.node_ids()[3], 1);
}

TEST(Edge, RemovesNodeId)
{
  Edge edge({1, 2, 3});
  edge.remove_node_id(1);
  ASSERT_EQ(edge.node_ids().size(), 2);
  EXPECT_EQ(edge.node_ids()[0], 3);
  EXPECT_EQ(edge.node_ids()[1], 2);
}

TEST(Edge, Serializes)
{
  Edge edge({1, 2, 3});
  auto rendered = edge.render().to_string();
  EXPECT_EQ(rendered, R"({"ids": [1,2,3]})");

  edge.clear_node_ids();
  utils::configure_from_json_string(rendered, edge);

  ASSERT_EQ(edge.node_ids().size(), 3);
  EXPECT_EQ(edge.node_ids()[0], 1);
  EXPECT_EQ(edge.node_ids()[1], 2);
  EXPECT_EQ(edge.node_ids()[2], 3);
}

TEST(Edge, StreamLoad)
{
  Edge edge;
  std::string json_string = R"({"ids": [1,2,3]})";

  edge.clear_node_ids();
  utils::configure_from_json_string(json_string, edge);

  ASSERT_EQ(edge.node_ids().size(), 3);
  EXPECT_EQ(edge.node_ids()[0], 1);
  EXPECT_EQ(edge.node_ids()[1], 2);
  EXPECT_EQ(edge.node_ids()[2], 3);
}

TEST(Edge, Debugs)
{
  Edge edge({1, 2, 3});
  EXPECT_EQ(edge.get_class_name(), "graph::Edge");
  std::stringstream s;
  s << edge;
  EXPECT_EQ(s.str(), "<graph::Edge: [1,2,3]>");
}

TEST(Edge, VerifiesIdsPresent)
{
  auto json = utils::json_from_string("{}");
  Edge edge;
  EXPECT_THROW_MESSAGE(edge.configure(json), ConfigurationException,
                       "parameter `ids` is required");
}

TEST(Edge, VerifiesIdsPresentStream)
{
  std::string json_str("{}");
  Edge edge;
  EXPECT_THROW_MESSAGE(utils::configure_from_json_string<Edge>(json_str, edge),
                       ConfigurationException, "parameter `ids` is required");
}

TEST(Edge, VerifiesIdsPositive)
{
  auto json = utils::json_from_string(R"({"ids":[1,-1]})");
  Edge edge;
  edge.configure(json);
  ASSERT_EQ(edge.node_ids().size(), 2);
  EXPECT_EQ(edge.node_ids()[0], 1);
  EXPECT_EQ(edge.node_ids()[1], -1);
}

TEST(Edge, VerifiesIdsPositiveStream)
{
  std::string json_str(R"({"ids":[1,-1]})");
  Edge edge;
  utils::configure_from_json_string<Edge>(json_str, edge);
  ASSERT_EQ(edge.node_ids().size(), 2);
  EXPECT_EQ(edge.node_ids()[0], 1);
  EXPECT_EQ(edge.node_ids()[1], -1);
}

TEST(Edge, NumNodes)
{
  auto json = utils::json_from_string(R"({"ids":[1,-1]})");
  size_t n_nodes = Edge::num_nodes(json);
  EXPECT_EQ(n_nodes, 2);

  json = utils::json_from_string(R"({"ids":[]})");
  n_nodes = Edge::num_nodes(json);
  EXPECT_EQ(n_nodes, 0);
}

TEST(Edge, EdgeVectorMemory)
{
  Edge edge({1, 2, 3});
  size_t edge_mem_estimate = Edge::memory_estimate(3);
  EXPECT_EQ(edge_mem_estimate, sizeof(Edge) + 12);
}

TEST(Edge, StreamProtoLoad)
{
  AzureQuantum::Problem_Term term;
  term.set_c(3.4);
  term.add_ids(100);
  term.add_ids(101);
  Edge edge;

  edge.clear_node_ids();
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<Edge::StreamHandler> handler_proxy;

  reader.Parse(term, handler_proxy);

  edge = std::move(handler_proxy.get_value());
  std::vector<int> expected_nodes = {100, 101};
  EXPECT_EQ(edge.node_ids().size(), 2);
  EXPECT_EQ(edge.node_ids()[0], 100);
  EXPECT_EQ(edge.node_ids()[1], 101);
}