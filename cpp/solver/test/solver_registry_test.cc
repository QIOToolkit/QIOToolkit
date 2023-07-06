
#include "solver/solver_registry.h"

#include <string>

#include "gtest/gtest.h"
#include "solver/all_solvers.h"
#include "solver/test/test_model.h"

using ::solver::SolverRegistry;

TEST(SolverRegistry, InstantiatesSimulatedAnnealing)
{
  std::string identifier = "simulatedannealing.qiotoolkit";
  EXPECT_TRUE(SolverRegistry::has(identifier));
  auto* sa = SolverRegistry::create_for_model<TestModel>(identifier);
  EXPECT_EQ(sa->get_class_name(), "solver::SimulatedAnnealing<TestModel>");
}

TEST(SolverRegistry, InstantiatesParallelTempering)
{
  std::string identifier = "paralleltempering.qiotoolkit";
  EXPECT_TRUE(SolverRegistry::has(identifier));
  auto* pt = SolverRegistry::create_for_model<TestModel>(identifier);
  EXPECT_EQ(pt->get_class_name(), "solver::ParallelTempering<TestModel>");
}

TEST(SolverRegistry, ThrowsOnUnknownSolver)
{
  std::string identifier = "unknown";
  EXPECT_FALSE(SolverRegistry::has(identifier));
  EXPECT_THROW(SolverRegistry::create_for_model<TestModel>(identifier),
               utils::KeyDoesNotExistException);
}
