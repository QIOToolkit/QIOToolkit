// Copyright (c) Microsoft. All Rights Reserved.
#include "../node.h"

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
using ::graph::Node;
using ::graph::NodeWithFaces;

TEST(Node, InitializesEmpty)
{
  Node node;
  EXPECT_EQ(node.edge_ids().size(), 0);
}

TEST(Node, InitializesWithListe)
{
  Node node({1, 2, 3});
  ASSERT_EQ(node.edge_ids().size(), 3);
  EXPECT_EQ(node.edge_ids()[0], 1);
  EXPECT_EQ(node.edge_ids()[1], 2);
  EXPECT_EQ(node.edge_ids()[2], 3);
}

TEST(Node, ClearsNodeIds)
{
  Node node({1, 2, 3});
  EXPECT_EQ(node.edge_ids().size(), 3);
  node.clear_edge_ids();
  EXPECT_EQ(node.edge_ids().size(), 0);
}

TEST(Node, SortsNodeIds)
{
  Node node({2, 1, 3, 1});
  EXPECT_EQ(node.edge_ids().size(), 4);
  node.sort_edge_ids();
  EXPECT_EQ(node.edge_ids()[0], 1);
  EXPECT_EQ(node.edge_ids()[1], 1);
  EXPECT_EQ(node.edge_ids()[2], 2);
  EXPECT_EQ(node.edge_ids()[3], 3);
}

TEST(Node, AddsNodeId)
{
  Node node({1, 2, 3});
  node.add_edge_id(1);
  ASSERT_EQ(node.edge_ids().size(), 4);
  EXPECT_EQ(node.edge_ids()[3], 1);
}

TEST(Node, RemovesNodeId)
{
  Node node({1, 2, 3});
  node.remove_edge_id(1);
  ASSERT_EQ(node.edge_ids().size(), 2);
  EXPECT_EQ(node.edge_ids()[0], 3);
  EXPECT_EQ(node.edge_ids()[1], 2);
}

TEST(Node, Serializes)
{
  Node node({1, 2, 3});
  auto rendered = node.render().to_string();
  EXPECT_EQ(rendered, R"({"ids": [1,2,3]})");

  node.clear_edge_ids();
  utils::configure_from_json_string(rendered, node);

  ASSERT_EQ(node.edge_ids().size(), 3);
  EXPECT_EQ(node.edge_ids()[0], 1);
  EXPECT_EQ(node.edge_ids()[1], 2);
  EXPECT_EQ(node.edge_ids()[2], 3);
}

TEST(Node, Debugs)
{
  Node node({1, 2, 3});
  EXPECT_EQ(node.get_class_name(), "graph::Node");
  std::stringstream s;
  s << node;
  EXPECT_EQ(s.str(), "<graph::Node: 1 2 3>");
}

TEST(Node, VerifiesIdsPositive)
{
  auto json = utils::json_from_string(R"({"ids":[1,-1]})");
  Node node;
  EXPECT_THROW_MESSAGE(node.configure(json), ConfigurationException,
                       "parameter `ids`: expected uint64, found -1 (int32)");
}

TEST(Node, VerifiesIdsPositiveStream)
{
  std::string json_str(R"({"ids":[1,-1]})");
  Node node;
  EXPECT_THROW_MESSAGE(utils::configure_from_json_string(json_str, node),
                       ConfigurationException,
                       "parameter `ids`: expected uint64, found -1 (int64)");
}

TEST(Node, NodeMemory)
{
  // Test for memory estimation of small node
  size_t size_of_node = sizeof(Node);
  Node node({1, 2, 3});
  size_t node_mem_estimate = Node::memory_estimate(3);
  EXPECT_EQ(node_mem_estimate, size_of_node + 3 * 8);
}

/// Node has vector of size_t,
/// Test for correct vector size estimation
TEST(Node, VectorMemory)
{
  size_t elem_size = sizeof(size_t);
  size_t vec_size = 10000;
  size_t mem_estimate = utils::vector_values_memory_estimate<size_t>(vec_size);
  EXPECT_EQ(mem_estimate, vec_size * elem_size);
}

TEST(NodeWithFaces, InitializesEmpty)
{
  NodeWithFaces node;
  EXPECT_EQ(node.edge_ids().size(), 0);
  EXPECT_EQ(node.edge_ids_by_face().size(), 1);
}

TEST(NodeWithFaces, InitializesWithListe)
{
  NodeWithFaces node({1, 2, 3});
  EXPECT_EQ(node.edge_ids().size(), 3);
  EXPECT_EQ(node.edge_ids()[0], 1);
  EXPECT_EQ(node.edge_ids()[1], 2);
  EXPECT_EQ(node.edge_ids()[2], 3);
}

TEST(NodeWithFaces, SortsNodeIds)
{
  NodeWithFaces node({2, 1, 3, 1});
  EXPECT_EQ(node.edge_ids().size(), 4);
  node.sort_edge_ids();
  EXPECT_EQ(node.edge_ids()[0], 1);
  EXPECT_EQ(node.edge_ids()[1], 1);
  EXPECT_EQ(node.edge_ids()[2], 2);
  EXPECT_EQ(node.edge_ids()[3], 3);
}

TEST(NodeWithFaces, AddsNodeId)
{
  NodeWithFaces node({1, 2, 3});
  node.add_edge_id(1);
  EXPECT_EQ(node.edge_ids().size(), 4);
  EXPECT_EQ(node.edge_ids()[3], 1);
}

TEST(Node, StreamProtoLoad)
{
  AzureQuantum::Problem_Term term;
  term.set_c(3.4);
  term.add_ids(100);
  term.add_ids(101);
  Node node;

  node.clear_edge_ids();
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<Node::StreamHandler> handler_proxy;
  reader.Parse(term, handler_proxy);

  node = std::move(handler_proxy.get_value());
  std::vector<int> expected_nodes = {100, 101};
  EXPECT_EQ(node.edge_ids().size(), 2);
  EXPECT_EQ(node.edge_ids()[0], 100);
  EXPECT_EQ(node.edge_ids()[1], 101);
}