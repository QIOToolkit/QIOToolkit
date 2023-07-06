
#pragma once
#include "utils/structure.h"
#include "strategy/linear_search.h"
#include "parameter_free_solver.h"
namespace solver
{
template <class Model_T, class SolverAdapter>
class ParameterFreeLinearSearchSolver
    : public ParameterFreeSolver<Model_T, SolverAdapter, strategy::LinearSearchOpt>
{
 public:
  ParameterFreeLinearSearchSolver()
  {
    this->stable_bests_ = 0;
    this->initial_samples_ = 0;
    this->nonimprovement_limit_ = 8;
  }

  ParameterFreeLinearSearchSolver(const ParameterFreeLinearSearchSolver&) = delete;
  ParameterFreeLinearSearchSolver& operator=(const ParameterFreeLinearSearchSolver&) = delete;
  ~ParameterFreeLinearSearchSolver() {}

 protected:

  void training_parameters(const utils::Json&) override
  {
  }

};
}  // namespace solver