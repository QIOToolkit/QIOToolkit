// Copyright (c) Microsoft. All Rights Reserved.

#include "app/runner.h"

#include <memory>

#include "utils/config.h"
#include "utils/exception.h"
#include "utils/structure.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "solver/solver.h"
using ::testing::Throw;

class MockSolver : public solver::Solver
{
 public:
  MOCK_METHOD((std::string), get_identifier, (), (const, override));
  MOCK_METHOD(void, init, (), (override));
  MOCK_METHOD(void, run, (), (override));
  MOCK_METHOD((utils::Structure), get_solutions, (), (const, override));
  MOCK_METHOD(size_t, get_model_sweep_size, (), (const, override));
};

class TestResult : public utils::Component
{
 public:
  double cost_;
  void configure(const utils::Json& json) override
  {
    this->param(json["solutions"], "cost", cost_);
  }
};

