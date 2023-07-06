
#include "strategy/gd_parameters.h"

namespace strategy
{
void GDParameters::load(const utils::Structure& training_parameters)
{
  gamma_ = training_parameters[kGamma].get<double>();
  pre_mult_ = training_parameters[kPreMult].get<double>();
  max_relative_change_ = training_parameters[kMaxRelativeChange].get<double>();
  tolerance_ = training_parameters[kTolerance].get<double>();
  num_multistarts_ = training_parameters[kNumMultistarts].get<int>();
  max_num_steps_ = training_parameters[kMaxNumSteps].get<int>();
  max_num_restarts_ = training_parameters[kMaxNumRestarts].get<int>();
  num_steps_averaged_ = training_parameters[kNumStepsAveraged].get<int>();
  max_mc_steps_ = training_parameters[kMaxMCSteps].get<int>();
  num_to_sample_ = training_parameters[kNumToSample].get<int>();
}

void GDParameters::configure(const utils::Json& params)
{
  using ::matcher::GreaterThan;
  this->param(params, strategy::kNumMultistarts, num_multistarts_)
      .description(
          "number of initial guesses to try in multistarted gradient descent "
          "for search process")
      .matches(GreaterThan(0));

  this->param(params, strategy::kMaxNumSteps, max_num_steps_)
      .description(
          "maximum number of gradient descent iterations per restart for "
          "model contruction for search process")
      .matches(GreaterThan(0));

  this->param(params, strategy::kMaxNumRestarts, max_num_restarts_)
      .description(
          "maximum number of gradient descent restarts, the we are allowed "
          "to call gradient descent for search process")
      .matches(GreaterThan(2));

  this->param(params, strategy::kGamma, gamma_)
      .description(
          "exponent controlling rate of step size decrease (see struct docs "
          "or GradientDescentOptimizer) for search process")
      .matches(GreaterThan(0.0));

  this->param(params, strategy::kPreMult, pre_mult_)
      .description("scaling factor for step size for search process")
      .matches(GreaterThan(0.0));

  this->param(params, strategy::kMaxRelativeChange, max_relative_change_)
      .description("max change allowed per GD iteration for search process")
      .matches(GreaterThan(0.0));

  this->param(params, strategy::kTolerance, tolerance_)
      .description(
          "when the magnitude of the gradient falls below this value OR we "
          "will not move farther than tolerance for search process")
      .matches(GreaterThan(0.0));

  this->param(params, strategy::kNumStepsAveraged, num_steps_averaged_)
      .description(
          "when the magnitude of the gradient falls below this value OR we "
          "will not move farther than tolerance for search process");

  this->param(params, strategy::kMaxMCSteps, max_mc_steps_)
      .description("number of monte carlo iterations for search process");

  this->param(params, strategy::kNumToSample, num_to_sample_)
      .description("number samples to be searched at one training iteration")
      .matches(GreaterThan(0));
}

std::string GDParameters::to_string() const
{
  utils::Structure output;
  output[kGamma] = gamma_;
  output[kPreMult] = pre_mult_;
  output[kMaxRelativeChange] = max_relative_change_;
  output[kTolerance] = tolerance_;
  output[kNumMultistarts] = num_multistarts_;
  output[kMaxNumSteps] = max_num_steps_;
  output[kMaxNumRestarts] = max_num_restarts_;
  output[kNumStepsAveraged] = num_steps_averaged_;
  output[kMaxMCSteps] = max_mc_steps_;
  output[kNumToSample] = num_to_sample_;
  return output.to_string();
}
}  // namespace strategy