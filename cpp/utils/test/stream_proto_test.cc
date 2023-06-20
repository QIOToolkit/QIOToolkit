// Copyright (c) Microsoft. All Rights Reserved.

#include <fstream>
#include <typeinfo>
#include <vector>

#include "../../utils/exception.h"
#include "../utils/error_handling.h"
#include "../utils/file.h"
#include "../proto_reader.h"
#include "../stream_handler.h"
#include "../stream_handler_proto.h"
#include "../timing.h"
#include "gtest/gtest.h"
#include "problem.pb.h"
#include "stream_test.h"

TEST(Stream, StreamPROTOEdge)
{
  AzureQuantum::Problem_Term term;
  term.set_c(3.4);
  term.add_ids(100);
  term.add_ids(101.1);
  Edge edge;
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<EdgeObjectHandler> handler_proxy;
  reader.Parse(term, handler_proxy);

  edge = std::move(handler_proxy.get_value());
  std::vector<int> expected_nodes = {100, 101};
  EXPECT_EQ(edge.nodes, expected_nodes);
  EXPECT_EQ(edge.c, 3.4);
}

TEST(Stream, NegativeTestVersion)
{
  AzureQuantum::Problem* problem = new AzureQuantum::Problem();
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<ModelObjectHandler> handler_proxy;
  AzureQuantum::Problem_CostFunction* cost_function =
      problem->mutable_cost_function();
  cost_function->set_version("foo");
  cost_function->set_type(AzureQuantum::Problem_ProblemType_ISING);
  std::ofstream out(utils::data_path("input_problem_pb/input_problem_pb_0.pb"),
                    std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
  problem->Clear();
  EXPECT_THROW_MESSAGE(
      reader.Parse(utils::data_path("input_problem_pb"), handler_proxy),
      utils::ValueException, "Expected version to be 1.0 or 1.1. Provided: foo");
}

TEST(Stream, NegativeTestCF)
{
  AzureQuantum::Problem* problem = new AzureQuantum::Problem();
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<ModelObjectHandler> handler_proxy;
  std::ofstream out(utils::data_path("input_problem_pb/input_problem_pb_0.pb"),
                    std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
  problem->Clear();
  EXPECT_THROW_MESSAGE(
      reader.Parse(utils::data_path("input_problem_pb"), handler_proxy),
      utils::ConfigurationException,
      "Invalid problem message. No cost function found");
}

TEST(Stream, NegativeTestTerms)
{
  AzureQuantum::Problem* problem = new AzureQuantum::Problem();
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<ModelObjectHandler> handler_proxy;
  AzureQuantum::Problem_CostFunction* cost_function =
      problem->mutable_cost_function();
  cost_function->set_version("1.0");
  cost_function->set_type(AzureQuantum::Problem_ProblemType_ISING);
  std::ofstream out(utils::data_path("input_problem_pb/input_problem_pb_0.pb"),
                    std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
  problem->Clear();
  EXPECT_THROW_MESSAGE(
      reader.Parse(utils::data_path("input_problem_pb"), handler_proxy),
      utils::ValueException,
      "Problem terms cannot be 0. Please add problem terms");
}

TEST(Stream, ProtoMessagesTest)
{
  Model model;
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<ModelObjectHandler> handler_proxy;
  std::string input_folder = utils::data_path("input_problem_pb");
  // Create a vector of problem messages
  std::vector<AzureQuantum::Problem*> problem_msgs;
  create_test_proto_msgs(problem_msgs);
  write_to_protobuf_folder(input_folder, problem_msgs);

  bool parse_success = reader.Parse(input_folder, handler_proxy);
  model = std::move(handler_proxy.get_value());
  EXPECT_EQ(parse_success, true);
  EXPECT_EQ(model.graph.edges.size(), 10);
  EXPECT_EQ(model.graph.edges[0].c, 2.5);
  EXPECT_EQ(model.graph.edges[0].nodes, std::vector<int>({0, 1}));
}

TEST(Stream, FileFailureTest)
{
  utils::ProtoReader reader;
  utils::PROTOHandlerProxy<ModelObjectHandler> handler_proxy;

  EXPECT_THROW_MESSAGE(
      reader.Parse(utils::data_path("err_input_problem_pb"), handler_proxy),
      utils::FileReadException,
      "Could not open file: " +
      utils::data_path("err_input_problem_pb/err_input_problem_pb_0.pb"));
}