
#pragma once

#include "utils/random_generator.h"
#include "markov/metropolis.h"

namespace solver
{
/// Energy scale estimator
///
/// This estimator attempts to estimate the relevant energy range of the
/// model it analyzes by collecting a histogram of transition costs in the
/// *random* and *quenched* regime.
///
///   - The *random regime* is sampled from random initial positions. It is
///     used as a proxy for the high temperature regime.
///
///   - The *quenched regime* is sampled from quenched states obtained by
///     peforming several metropolis sweeps at T=0.
///
/// From these histograms we can tune the temperature for expected
/// boltzmann acceptance rates in either regime. This is expressed in terms
/// of the portion of **energy increasing** transitions that would be
/// accepted (transitions which don't change the energy are always accepted
/// and the potion of energy-lowering transitions in the random regime
/// depends on the model).
template <class Model>
class Estimator
{
 public:
  using Walker = markov::Metropolis<Model>;

  /// The number of restarts defines how many initial (random) starting
  /// points we use for sampling (and, after quenching, resulting quenched
  /// points).
  static const int restarts = 64;

  /// The number of samples defines how many random transitions are
  /// generated (and calculated) both in the random and quenched regime.
  static const int samples = 100;

  /// How many sweeps at T=0 to perform to go from the initial random state
  /// to the "quenched" regime.
  static const int quenching_sweeps = 20;

  /// Measure the current cost and samples of the cost of a random transition
  /// for the walker at its current state.
  /// For models derived from model::FacedGraphModel, an extra parameter
  /// for a cost object is necessary to communicate cached calculations.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  gather_statistics(const Model& model, Walker& walker, double* cost,
                    std::vector<double>* deltas)
  {
    *cost += walker.cost();
    for (int j = 0; j < samples; j++)
    {
      auto transition =
          model.get_random_transition(walker.state(), walker.rng());
      deltas->push_back(model.calculate_cost_difference(
          walker.state(), transition, &walker.cost()));
    }
  }

  /// Measure the current cost and samples of the cost of a random transition
  /// for the walker at its current state.
  /// For all other models, no extra cost parameter is needed.
  template <class TM = Model, class TS = typename Model::State_T,
            class TT = typename Model::Transition_T>
  typename std::enable_if<
      !std::is_base_of<model::FacedGraphModel<TS, TT>, TM>::value, void>::type
  gather_statistics(const Model& model, Walker& walker, double* cost,
                    std::vector<double>* deltas)
  {
    *cost += walker.cost();
    for (int j = 0; j < samples; j++)
    {
      auto transition =
          model.get_random_transition(walker.state(), walker.rng());
      deltas->push_back(
          model.calculate_cost_difference(walker.state(), transition));
    }
  }

  void quench(Walker& walker)
  {
    walker.set_temperature(0);
    for (int i = 0; i < quenching_sweeps; i++)
    {
      double cost_before = walker.cost();
      walker.make_sweep();
      if (walker.cost() >= cost_before) break;
    }
  }

  double estimate_accepted(const std::vector<double>& deltas, double T)
  {
    double accepted = 0.0;
    double positive = 0.0;
    for (const double& delta : deltas)
    {
      if (delta <= 0) continue;
      positive++;
      accepted += exp(-delta / T);
    }
    return accepted / positive;
  }

  double find_temperature(const std::vector<double>& deltas,
                          double target_acceptance)
  {
    double T = 1e-30;
    while (estimate_accepted(deltas, T) < target_acceptance)
    {
      T *= 2;
    }
    double t = T / 2;
    while (t > 1e-30)
    {
      if (estimate_accepted(deltas, T - t) > target_acceptance)
      {
        T -= t;
      }
      t /= 2;
    }
    return T;
  }

  utils::Structure analyze(const Model& model, utils::RandomGenerator& rng)
  {
    double e_random = 0.0, e_quenched = 0.0;
    std::vector<double> dE_random, dE_quenched;

    markov::Metropolis<Model> walker;
    walker.set_model(&model);
    walker.set_rng(&rng);
    walker.reset_evaluation_counter();
    for (int i = 0; i < restarts; i++)
    {
      walker.init();
      gather_statistics(model, walker, &e_random, &dE_random);
      quench(walker);
      gather_statistics(model, walker, &e_quenched, &dE_quenched);
      eval_counter_ += walker.get_evaluation_counter();
      walker.get_evaluation_counter();
    }
    e_random /= restarts;
    e_quenched /= restarts;

    utils::Structure result;
    result["initial"] = find_temperature(dE_random, 0.30);
    result["final"] = find_temperature(dE_quenched, 0.05);

    double dE_sum1 = 0, dE_sum2 = 0;
    for (auto dE : dE_random)
    {
      dE_sum1 += dE;
      dE_sum2 += dE_sum2;
    }
    for (auto dE : dE_quenched)
    {
      dE_sum1 += dE;
      dE_sum2 += dE_sum2;
    }
    dE_sum1 /= static_cast<double>(dE_random.size() + dE_quenched.size());
    dE_sum2 /= static_cast<double>(dE_random.size() + dE_quenched.size());
    double dev = sqrt(dE_sum1 * dE_sum1 - dE_sum2);

    result["count"] = (int)round(2 * (e_random - e_quenched) / dev);
    return result;
  }

  solver::EvaluationCounter eval_counter_;
};

}  // namespace solver
