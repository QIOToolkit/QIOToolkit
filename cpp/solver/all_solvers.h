
#pragma once

#include "solver/murex.h"
#include "solver/pa_parameter_free.h"
#include "solver/parallel_tempering.h"
#include "solver/parameter_free_solver.h"
#include "solver/parameter_free_linear_solver.h"
#include "solver/population_annealing.h"
#include "solver/sa_parameter_free.h"
#include "solver/pt_parameter_free.h"
#include "solver/simulated_annealing.h"
#include "solver/ssmc_parameter_free.h"
#include "solver/substochastic_monte_carlo.h"
#include "solver/quantum_monte_carlo.h"
#include "solver/tabu.h"
#include "solver/tabu_parameter_free.h"

namespace solver
{
template <class Model_T>
Solver* create_solver(const std::string& identifier)
{
  Solver* model_solver;
  if (identifier == "paralleltempering.qiotoolkit")
  {
    model_solver = new ::solver::ParallelTempering<Model_T>();
  }
  else if (identifier == "simulatedannealing.qiotoolkit")
  {
    model_solver = new ::solver::SimulatedAnnealing<Model_T>();
  }
  else if (identifier == "populationannealing.cpu")
  {
    model_solver = new ::solver::PopulationAnnealing<Model_T>();
  }
  else if (identifier == "substochasticmontecarlo.cpu")
  {
    model_solver = new ::solver::SubstochasticMonteCarlo<Model_T>();
  }
  else if (identifier == "tabu.qiotoolkit")
  {
    model_solver = new ::solver::Tabu<Model_T>();
  }
  else if (identifier == "quantummontecarlo.qiotoolkit")
  {
    model_solver = new ::solver::QuantumMonteCarlo<Model_T>();
  }
  else if (identifier == "substochasticmontecarlo-parameterfree.cpu")
  {
    model_solver = new ::solver::ParameterFreeSolver<
        Model_T, ::solver::SSMCParameterFree<Model_T>>();
  }
  else if (identifier == "populationannealing-parameterfree.cpu")
  {
    model_solver =
        new ::solver::ParameterFreeSolver<Model_T,
                                          ::solver::PAParameterFree<Model_T>>();
  }
  else if (identifier == "simulatedannealing-parameterfree.qiotoolkit")
  {
    model_solver =
        new ::solver::ParameterFreeLinearSearchSolver<Model_T,
                                          ::solver::SAParameterFree<Model_T>>();
  }
  else if (identifier == "paralleltempering-parameterfree.qiotoolkit")
  {
    model_solver = new ::solver::ParameterFreeLinearSearchSolver<
        Model_T, ::solver::PTParameterFree<Model_T>>();
  }
  else if (identifier == "tabu-parameterfree.qiotoolkit")
  {
    model_solver = new ::solver::ParameterFreeLinearSearchSolver<
        Model_T, ::solver::TabuParameterFree<Model_T>>();
  }
  else
  {
    THROW(utils::ValueException, "Unknown solver: ", identifier);
  }
  return model_solver;
}

}  // namespace solver
