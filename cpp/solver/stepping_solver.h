
#pragma once

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>

#include "utils/component.h"
#include "utils/config.h"
#include "utils/log.h"
#include "utils/optional.h"
#include "utils/random_generator.h"
#include "utils/qio_signal.h"
#include "utils/timing.h"
#include "matcher/matchers.h"
#include "observe/observable.h"
#include "observe/observer.h"
#include "omp.h"
#include "solver/evaluation_counter.h"
#include "solver/model_solver.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// Optimization in discretized steps.
///
/// This base class assumes the solver runs in discretized steps t and
/// handles the configuration of `step_limit`, increments `t` and calls
/// `make_step()` for every integer value [0, step_limit].
///
template <class Model_T>
class SteppingSolver : public ModelSolver<Model_T>
{
 public:
  using Base_T = ModelSolver<Model_T>;

  SteppingSolver() : step_(0), steps_accum_(0), seconds_accum_(0) {}
  ~SteppingSolver() override {}

  /// Read the maximum number of steps from configuration.
  void configure(const utils::Json& json) override
  {
    Base_T::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kParams))
    {
      THROW(utils::MissingInputException, "Input field `", utils::kParams,
            "` is required.");
    }
    const utils::Json& params = json[utils::kParams];

    using ::matcher::GreaterThan;

    this->param(params, "step_limit", step_limit_)
        .alias("sweeps")
        .description("Stop the solver after this many steps (sweeps).")
        .default_value(10000)
        .with_output();

    this->param(params, "tick_every", tick_every_)
        .description("Update timings every `tick_every` seconds.")
        .default_value(1.0)
        .matches(GreaterThan(0.0));

    this->param(params, "steps_per_tick", steps_per_tick_)
        .description(
            "How many steps to take between ticks initially. "
            "This value will be adjusted to get the desired "
            "tick duration (`tick_every`).")
        .default_value((uint64_t)10)
        .matches(GreaterThan<uint64_t>(0));

    this->param(params, "seed", seed_)
        .description("seed for the random number generator")
        .default_value(utils::get_seed_time())
        .with_output();

    rng_.reset(new utils::PCG(seed_));
    update_steps_per_tick_ = true;
  }

  // Initialization must be implemented by derived classes.
  virtual void init() override = 0;

  // Stepping must be implemented by derived classes.
  virtual void make_step(uint64_t step) = 0;

  uint64_t get_cost_function_evaluation_count() const
  {
    return this->evaluation_counter_.get_function_evaluation_count() +
           this->evaluation_counter_.get_difference_evaluation_count();
  }

  // Show the current simulation status on std::cerr.
  virtual void status() const
  {
    std::cerr
        << "+----------------------------------------------------------------"
        << std::endl;
    std::cerr << "| " << this->get_class_name() << std::endl;
    std::cerr << "| step: " << step_ << std::endl;
    std::cerr << "| runtime: " << get_runtime() << std::endl;
    std::cerr << "| min_cost: " << Base_T::get_lowest_cost() << std::endl;
    std::cerr
        << "+----------------------------------------------------------------"
        << std::endl;
  }

  /// A stepping solver is run by invoking / make_step(t) for every value of t
  // up to `step_limit`.
  void run() override
  {
    if (this->is_empty())
    {
      // in case the model is empty return
      return;
    }
    using utils::get_wall_time;
    start_time_ = get_wall_time();

    double time_before = get_wall_time(), time_after = time_before, time_diff;
    uint64_t next_steps_per_tick = steps_per_tick_;
    while (true)
    {
      // Perform (and time) the current number of steps scheduled per tick
      for (uint64_t s = 0; s < next_steps_per_tick; s++)
      {
        {
          auto step_label = this->scoped_observable_label("step", step_);
          this->observe("step", static_cast<double>(step_));
          make_step(step_);
        }
        step_++;

        // Record a cost milestone if a better solution was found this round.
        if (this->lowest_cost_.has_value())
        {
#if defined(qiotoolkit_PROFILING) || defined(_DEBUG)
          // ONLY logging this information in profiling build, because this may
          // produce a lot of message on Qiotoolkit's log
          // NOTE: if the solver does not call `update_lowest_cost()` to avoid
          // copying the lowest state, then no milestones will be recorded.
          // and benchmarking time-to-solution WILL NOT WORK.
          this->cost_milestones_.record(*this->lowest_cost_);
#endif

          // Check cost limit in the inner loop to avoid over-running by
          // up to 1 tick duration (this is important because we are
          // using cost_limit to benchmark time-to-solution).
          if (this->cost_limit_.has_value() &&
              *this->lowest_cost_ <= *this->cost_limit_)
          {
            next_steps_per_tick = s + 1;
            break;
          }
        }
      }

      // Get timings for this round of `make_step`.
      time_after = get_wall_time();
      time_diff = time_after - time_before;
      time_before = time_after;  // use as start time to measure next tick
      double step_diff = (double)next_steps_per_tick;

      if (update_steps_per_tick_)
      {
        // Update steps_per_tick_ to aim for the desired tick duration
        //
        // This increases or decreases the number of times `make_step()` is
        // called between calls to `get_wall_time()` when the tick duration
        // `time_diff` is outside a 10% threshold window otf the target ticking
        // period
        // (`tick_every_`).
        if (time_diff < 0.95 * tick_every_)
        {
          steps_per_tick_ = (uint64_t)std::max(
              (double)steps_per_tick_ + 1.0,
              std::min((double)steps_per_tick_ * 1.2,
                       (double)steps_per_tick_ / time_diff * tick_every_));
        }
        else if (time_diff > 1.05 * tick_every_)
        {
          steps_per_tick_ = (uint64_t)std::max(
              1.0, (double)steps_per_tick_ / time_diff * tick_every_);
        }
        // Decide how many steps to take in the next round
        //
        // This is stored in the local value `next_steps_per_tick`, which
        // can differ from the `steps_per_tick_` target when we're nearing
        // the time_limit.
        next_steps_per_tick = steps_per_tick_;
      }

      if (this->time_limit_.has_value())
      {
        double current_runtime = time_after - start_time_;
        double time_left = this->time_limit_.value() - current_runtime;
        double next_tick_duration =
            time_diff * (double)next_steps_per_tick / step_diff;
        if (time_left <= 0)
        {
          std::string output = "Stop due to time_limit.steps : " +
                               std::to_string(step_) +
              ", time_limit:" + std::to_string(this->time_limit_.value());
          exit_reason = output;
          LOG(INFO, output);
          break;  // We are over the time_limit, stop.
        }
        else if (time_left < next_tick_duration)
        {
          // We are nearing the time_limit, slow down in the local variable
          // (but leave the member variable untouched as it will be
          //  relevant for benchmark output).
          next_steps_per_tick = (uint64_t)(step_diff * time_left / time_diff);
        }
      }

      // Check for termination condition (steps)
      if (step_limit_.has_value())
      {
        if (step_ >= step_limit_.value())
        {
          std::string output =
              "Stop due to step limit: " + std::to_string(step_limit_.value()) +
              ", steps:" + std::to_string(step_);
          exit_reason = output;
          LOG(INFO, output);
          break;
        }
        next_steps_per_tick = std::min((uint64_t)(step_limit_.value() - step_),
                                       next_steps_per_tick);
      }

      // Check whether the maximum number of cost evaluations was reached.
      if (this->eval_limit_.has_value() &&
          get_cost_function_evaluation_count() >= this->eval_limit_.value())
      {
        std::string output = "Stop due to Evaluation limit: " + std::to_string(
            this->eval_limit_.value()) + ", evaluations:" + std::to_string(get_cost_function_evaluation_count());
        exit_reason = output;
        LOG(INFO, output);
        break;
      }

      // Check whether the desired cost has been reached.
      if (this->cost_limit_.has_value() &&
          Base_T::get_lowest_cost() <= *this->cost_limit_)
      {
        // We have found a sufficiently low cost state.
        std::string output =
            "Stop due to cost limit: " +
                             std::to_string(this->cost_limit_.value()) +
                             ", current lowest cost:";
        LOG(INFO, output);
        exit_reason = output;
        break;
      }

      // Handle any signals such as USR1
      if (handle_signals())
      {
        // A signal was interpreted to mean 'stop'
        LOG(INFO, "Stop due to signal.");
        break;
      }
    }
  }

  utils::Structure get_solver_properties() const override
  {
    auto s = Solver::get_solver_properties();
    s["cost_milestones"] = this->cost_milestones_.render();
    s["last_step"] = step_;

    if (step_limit_.has_value())
    {
      s["step_limit"] = step_limit_.value();
    }
    if (exit_reason != "")
    {
      s["exit_reason"] = exit_reason;
    }
    return s;
  }

  double get_runtime() const { return utils::get_wall_time() - start_time_; }

  unsigned int get_seed() const { return seed_; }

  virtual uint64_t current_steps() const
  {
    uint64_t steps = this->step_;
    uint64_t threads = static_cast<uint64_t>(this->get_thread_count());
    if (this->target_number_of_states() > threads)
    {
      steps *= (this->target_number_of_states() + threads - 1) / threads;
    }
    return steps;
  }

  void update_accumulated_info()
  {
    uint64_t steps = current_steps();
    steps_accum_ += steps;
    double end_time = utils::get_wall_time();
    seconds_accum_ += (end_time - this->start_time_);
  }

  // Fix the steps_per_tick_ by value
  void fixed_step_per_tick(uint64_t value)
  {
    this->steps_per_tick_ = value;
    this->update_steps_per_tick_ = false;
  }

 protected:
  std::optional<uint64_t> step_limit_;

  void reset(double left_over_time)
  {
    LOG(INFO, "reset time limit:", left_over_time);
    LOG(INFO, "reset steps:", this->step_limit_.value());
    this->time_limit_ = left_over_time;
    this->step_ = 0;
    this->lowest_costs_.clear();
    this->lowest_states_.clear();
    this->lowest_cost_.reset();
  }

  bool handle_signals()
  {
    using utils::Signal;
    while (!Signal::queue().empty())
    {
      auto signal = Signal::queue().front();
      Signal::queue().pop();
      switch (signal.get_type())
      {
        case Signal::HALT:
          return true;
        case Signal::STATUS:
          status();
          break;
        case Signal::UNKNOWN:
        default:
          std::cerr << "Unknown signal, stopping." << std::endl;
          return true;
      }
    }
    return false;
  }

 protected:
  double seconds_per_step() const
  {
    assert(this->steps_accum_ > 0.);
    assert(this->seconds_accum_ > 0.);

    // estimating the timing used by one step.
    double seconds_per_step = this->seconds_accum_ / this->steps_accum_;

    return seconds_per_step;
  }

  unsigned int seed_;
  std::unique_ptr<utils::RandomGenerator> rng_;
  double start_time_;
  uint64_t step_;
  uint64_t steps_accum_;
  double seconds_accum_;

 private:
  std::string exit_reason = "";
  double tick_every_;
  uint64_t steps_per_tick_;
  bool update_steps_per_tick_;
};

}  // namespace solver
