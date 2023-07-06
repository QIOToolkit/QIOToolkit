
#include "../soft_spin.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include "../../utils/exception.h"
#include "../../utils/json.h"
#include "../../utils/file.h"
#include "../../utils/proto_reader.h"
#include "../../utils/random_generator.h"
#include "../../utils/stream_handler_json.h"
#include "../../utils/stream_handler_proto.h"
#include "gtest/gtest.h"
#include "problem.pb.h"

using ::utils::ConfigurationException;
using ::utils::Json;
using ::utils::RandomGenerator;
using ::utils::Twister;
using Model_T = ::examples::SoftSpin;
using State_T = ::examples::SoftSpinState;
using Transition_T = ::examples::SoftSpinTransition;

void create_spin_proto_test()
{
  QuantumUtil::Problem* problem = new QuantumUtil::Problem();
  QuantumUtil::Problem_CostFunction* cost_function =
      problem->mutable_cost_function();
  cost_function->set_version("0.1");
  cost_function->set_type(QuantumUtil::Problem_ProblemType_SOFTSPIN);
  for (unsigned i = 0; i < 3; i++)
  {
    QuantumUtil::Problem_Term* term = cost_function->add_terms();
    term->set_c(1.0);
    term->add_ids(i);
    i == 2 ? term->add_ids(0) : term->add_ids(i + 1);
  }
  std::string folder_name = utils::data_path("ex_input_problem_pb");
  std::ofstream out;
  out.open(folder_name + "/" + "ex_input_problem_pb_0.pb",
           std::ios::out | std::ios::binary);
  problem->SerializeToOstream(&out);
  out.close();
}

class SoftSpinTest : public testing::Test
{
 public:
  SoftSpinTest()
  {
    // A triangle of 3 spins with ferromagnetic 2-spin couplings.
    std::string input(R"({
      "cost_function": {
        "type": "softspin",
        "version": "0.1",
        "terms": [
          {"c": 1, "ids": [0, 1]},
          {"c": 1, "ids": [1, 2]},
          {"c": 1, "ids": [2, 0]}
        ]
      }
    })");
    utils::configure_with_configuration_from_json_string(input, model);
    model.init();
    create_spin_proto_test();
    std::string folder_name = utils::data_path("ex_input_problem_pb");
    utils::configure_with_configuration_from_proto_folder(folder_name,
                                                         model_proto);
  }

  Model_T model;
  Model_T model_proto;
};

TEST_F(SoftSpinTest, SingleUpdates)
{
  State_T state;
  state.spin = {1, 1, 1};

  EXPECT_EQ(model.calculate_cost(state), 3);
  Transition_T transition;
  transition.spin_id = 0;      // setting the first spin
  transition.new_value = 0.0;  // to 0.0;
  EXPECT_EQ(model.calculate_cost_difference(state, transition), -2);

  model.apply_transition(transition, state);  // Apply the transition
  EXPECT_EQ(model.calculate_cost(state), 1);
}

TEST_F(SoftSpinTest, Metropolis)
{
  State_T state;
  state.spin = {1, 1, 1};

  EXPECT_EQ(model.calculate_cost(state), 3);  // Maximum value
  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(5);
  double cost = model.calculate_cost(state);
  double min = cost;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = model.get_random_transition(state, rng);
    auto diff = model.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      model.apply_transition(transition, state);
      cost += diff;
    }
    if (cost < min)
    {
      DEBUG("New best: ", cost, " ", state.spin);
      min = cost;
    }
  }
  double ground_state = -1.0;
  EXPECT_LT(min - ground_state, 0.01);
}

TEST_F(SoftSpinTest, StateMemoryEstimate)
{
  std::string input(R"({
    "cost_function": {
      "type": "softspin",
      "version": "0.1",
      "terms": [
        {"c": 1, "ids": [0, 1]},
        {"c": 1, "ids": [1, 2]},
        {"c": 1, "ids": [2, 0]}
      ]
    }
  })");

  Model_T model;
  utils::configure_with_configuration_from_json_string(input, model);
  size_t state_mem = model.state_memory_estimate();
  size_t expected_state_mem = sizeof(Model_T::State_T) + 8 * 3;
  EXPECT_EQ(state_mem, expected_state_mem);
}

TEST_F(SoftSpinTest, SingleUpdates_proto)
{
  State_T state;
  state.spin = {1, 1, 1};

  EXPECT_EQ(model_proto.calculate_cost(state), 3);
  Transition_T transition;
  transition.spin_id = 0;      // setting the first spin
  transition.new_value = 0.0;  // to 0.0;
  EXPECT_EQ(model_proto.calculate_cost_difference(state, transition), -2);

  model_proto.apply_transition(transition, state);  // Apply the transition
  EXPECT_EQ(model_proto.calculate_cost(state), 1);
}

TEST_F(SoftSpinTest, Metropolis_proto)
{
  State_T state;
  state.spin = {1, 1, 1};

  EXPECT_EQ(model_proto.calculate_cost(state), 3);  // Maximum value
  // Do some metropolis and find the lowest cost.
  double T = 0.5;
  Twister rng;
  rng.seed(5);
  double cost = model_proto.calculate_cost(state);
  double min = cost;
  for (int i = 0; i < 1000; i++)
  {
    auto transition = model_proto.get_random_transition(state, rng);
    auto diff = model_proto.calculate_cost_difference(state, transition);
    if (diff < 0 || rng.uniform() < exp(-diff / T))
    {
      model_proto.apply_transition(transition, state);
      cost += diff;
    }
    if (cost < min)
    {
      DEBUG("New best: ", cost, " ", state.spin);
      min = cost;
    }
  }
  double ground_state = -1.0;
  EXPECT_LT(min - ground_state, 0.01);
}
