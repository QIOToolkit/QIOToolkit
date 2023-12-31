
#pragma once


#include <fstream>
#include <string>
#include <vector>

#include "problem.pb.h"

void create_proto_test_problem(QuantumUtil::Problem_ProblemType type)
{
  QuantumUtil::Problem* problem = new QuantumUtil::Problem();
  QuantumUtil::Problem_CostFunction* cost_function =
      problem->mutable_cost_function();
  cost_function->set_version("1.0");
  cost_function->set_type(type);
  for (unsigned i = 0; i < 10; i++)
  {
    QuantumUtil::Problem_Term* term = cost_function->add_terms();
    term->set_c(1.0);
    term->add_ids(i);
    i == 9 ? term->add_ids(0) : term->add_ids(i + 1);
  }
  std::string folder_name = "../../../../data/models_input_problem_pb";
  std::ofstream out;
  out.open(folder_name + "/" + "models_input_problem_pb_0.pb",
           std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
}

void create_proto_test_problem2(QuantumUtil::Problem_ProblemType type)
{
  QuantumUtil::Problem* problem = new QuantumUtil::Problem();
  QuantumUtil::Problem_CostFunction* cost_function =
      problem->mutable_cost_function();
  cost_function->set_version("1.0");
  cost_function->set_type(type);
  for (unsigned i = 0; i < 10; i++)
  {
    QuantumUtil::Problem_Term* term = cost_function->add_terms();
    i == 9 ? term->set_c(-9.5)
           : (i % 2 == 0 ? term->set_c(-1.0 * (i + 1)) : term->set_c(i + 1));
    term->add_ids(i);
    i == 9 ? term->add_ids(0) : term->add_ids(i + 1);
  }

  std::string folder_name = "../../../../data/models_input_problem2_pb";
  std::ofstream out;
  out.open(folder_name + "/" + "models_input_problem2_pb_0.pb",
           std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
}

std::vector<std::string> SolversSupportMemSaving = {
    "populationannealing-parameterfree.cpu",
    "simulatedannealing.qiotoolkit", "paralleltempering.qiotoolkit",
    "populationannealing.cpu"};
