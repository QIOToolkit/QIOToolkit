
#pragma once

#include <cmath>
#include <vector>

#include "utils/utils.h"
#include "markov/walker.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// Metropolis Algorithm
///
/// Random sampling approximating the Boltzmann distribution using a Markov
/// chain Monte Carlo approach.

template <class Model>
class Metropolis : public Walker<Model>
{
 public:
  using State = typename Model::Model::State_T;
  using Transition = typename Model::Model::Transition_T;

  /// Create a Metropolis instance with uninitialized model and state.
  Metropolis() : temperature_(1.0), beta_(1.0) {}

  /// Get the current temperature for sampling.
  double temperature() const { return temperature_; }

  /// Get the current inverse sampling temperature.
  double beta() const { return beta_; }

  /// Set the sampling temperature (also updates beta_).
  void set_temperature(double temperature)
  {
    temperature_ = temperature;
    if (temperature_ <= 0)
    {
      temperature_ = 0.0;
      beta_ = std::numeric_limits<double>::infinity();
    }
    else
    {
      beta_ = 1.0 / temperature_;
    }
  }

  void set_beta(double beta)
  {
    beta_ = beta;
    if (beta_ <= 0)
    {
      beta_ = 0;
      temperature_ = std::numeric_limits<double>::infinity();
    }
    else
    {
      temperature_ = 1.0 / beta_;
    }
  }

  /// Decide whether to accept a given cost increase.
  bool accept(const typename Model::Cost_T& cost_diff) override
  {
    // original expression is uniform_rnd < exp(-cost_diff * beta_)
    // this is re-expressed here as ln(uniform_rnd) < (-cost_diff * beta_)
    // then a lookup table of ln( [0,1) ) is built at startup
    // and referenced for speed by rnd() & 0xFF
    // The 0th bucket redirects to a new row which zooms in on the previous 0th
    // Then adjacent buckets are interpolated for a trepesoidal approximation
    // Original:
    //  return cost_diff <= 0 ||
    //         Walker<Model>::rng_->uniform() < exp(-cost_diff * beta_);
    //
    if (cost_diff <= 0)
    {
      return true;
    }
    size_t bucket = 0;
    int row = -1;
    utils::RandomGenerator::result_type current_random = 0;
    while (bucket == 0)
    {
      if (row < 3)
      {
        current_random = Walker<Model>::rng_->uint32();
        bucket = current_random & (UNIFORM_LN_BUCKETS - 1);  // mod 256
        row++;
      }
      else
      {
        return true;
      }
    }
    double y0 = utils::uniform_ln[row][bucket - 1];
    double y1 = utils::uniform_ln[row][bucket];
    double distance =
        ((double)(current_random >> UNIFORM_LN_BUCKETS_BITS)) /
        (double)((~((utils::RandomGenerator::result_type)UNIFORM_LN_BUCKETS)) >>
                 UNIFORM_LN_BUCKETS_BITS);
    double value = y0 + distance * (y1 - y0);
    return value < (cost_diff * -beta_);
  }

  /// Estimate memory consumption using model parameters
  static size_t memory_estimate(const Model& model)
  {
    return sizeof(Metropolis<Model>) - sizeof(Walker<Model>) +
           Walker<Model>::memory_estimate(model);
  }

 private:
  double temperature_;
  double beta_;
};

}  // namespace markov
