
#include "strategy/bayesian.h"

#include <assert.h>

#include <chrono>
#include <cmath>
#include <sstream>

#include "utils/config.h"
#include "utils/log.h"
#include "externals/gpp/gpp_common.hpp"
#include "externals/gpp/gpp_covariance.hpp"
#include "externals/gpp/gpp_domain.hpp"
#include "externals/gpp/gpp_logging.hpp"
#include "externals/gpp/gpp_math.hpp"
#include "externals/gpp/gpp_model_selection.hpp"
#include "externals/gpp/gpp_optimizer_parameters.hpp"
#include "externals/gpp/gpp_random.hpp"
#include "omp.h"

using namespace optimal_learning;

namespace strategy
{
void BayesianOpt::init(size_t dimensions, uint32_t seed,
                       const GDParameters& model_parameters,
                       const GDParameters& search_parameters,
                       size_t reserved_samples)
{
  assert(dimensions_ == 0);
  dimensions_ = dimensions;

  // we really do not know how many iterations of sampling can happen
  // this is just best effort to reserve some memory for vectors
  reserved_samples_ = reserved_samples;
  sample_points_.reserve(reserved_samples_ * dimensions);
  sample_objectives_.reserve(reserved_samples_);

  rng_.seed(seed);

  model_training_ = model_parameters;
  search_training_ = search_parameters;
  LOG(INFO, "Model traning parameters: " + model_parameters.to_string());
  LOG(INFO, "Search traning parameters: " + search_parameters.to_string());

  sample_cur_ = search_parameters.num_to_sample_;
  next_winner_points_.clear();
  next_winner_points_.resize(search_parameters.num_to_sample_ * dimensions_);

  // This is highly model related, other model shall have different settings.
  trained_parameters_.resize(dimensions_ + 2, 1.0);
}

void BayesianOpt::init(size_t dimensions, uint32_t seed,
                       size_t reserved_samples)
{
  init(dimensions, seed, model_training_, search_training_, reserved_samples);
}

void BayesianOpt::set_ranges(
    const std::vector<std::pair<double, double>>& ranges)
{
  ranges_ = ranges;
}

void BayesianOpt::copyout_winner(std::vector<double>& parameters) const
{
  assert(sample_cur_ < search_training_.num_to_sample_);
  parameters.clear();
  parameters.resize(dimensions_);
  for (size_t i = 0; i < dimensions_; i++)
  {
    parameters[i] = next_winner_points_[i + sample_cur_ * dimensions_];
  }
#if defined(_DEBUG) || defined(qiotoolkit_PROFILING)
  log_parameters("Search parameters: ", parameters);
#endif
}

size_t BayesianOpt::num_of_saved_samples() const
{
  assert(sample_points_.size() % dimensions_ == 0);
  return sample_points_.size() / dimensions_;
}

double BayesianOpt::get_sample(int indx,
                               std::vector<double>& sample_point) const
{
  assert(indx >= 0);
  assert((size_t)indx * dimensions_ < sample_points_.size());
  sample_point.clear();
  for (size_t i = 0; i < dimensions_; i++)
  {
    sample_point.push_back(sample_points_[i + dimensions_ * indx]);
  }
  return sample_objectives_[indx];
}

void BayesianOpt::add_new_sample(std::vector<double>& parameters,
                                 double objective)
{
  assert(parameters.size() == dimensions_);
  if (sample_objectives_.size() == 0)
  {
    best_so_far_ = objective;
  }
  if (best_so_far_ > objective)
  {
    best_so_far_ = objective;
  }
  if (sample_objectives_.size() < reserved_samples_)
  {
    // when samples reserved are not reaching reserved_samples_, just push new
    // samples in
    //
    sample_objectives_.push_back(objective);
    for (size_t i = 0; i < dimensions_; i++)
    {
      sample_points_.push_back(parameters[i]);
    }
  }
  else
  {
    for (size_t i = 0; i < dimensions_; i++)
    {
      size_t replace_index = (new_sample_index_ + i) % reserved_samples_;
      if (sample_objectives_[replace_index] > objective)
      {
        // find the oldest (compared with last update) sample whose object is
        // larger (worse) than the new one
        //
        new_sample_index_ = replace_index;
        break;
      }
    }
    sample_objectives_[new_sample_index_] = objective;
    for (size_t i = 0; i < dimensions_; i++)
    {
      sample_points_[i + dimensions_ * new_sample_index_] = parameters[i];
    }
  }
  new_sample_index_ = (new_sample_index_ + 1) % reserved_samples_;

#ifdef qiotoolkit_PROFILING
  update_objective_profile(objective, parameters);
#endif
}

void BayesianOpt::log_hyper_parameters() const
{
  log_parameters("Hyper parameters: ", trained_parameters_);
}

std::unique_ptr<GaussianProcess> create_model(
    double tolerance, std::vector<double>& trained_parameters, int dim,
    std::vector<double>& sample_points, std::vector<double>& sample_objectives,
    std::vector<int>& derivatives, utils::RandomGenerator& rng)
{
  int num_sampled = (int)sample_objectives.size();
  std::unique_ptr<GaussianProcess> result;
  // Matrix of covariance requires inverse operation, so the
  // matrix can not be singular, so we add a noise which modify
  // the value of variance a little to avoid singularity.
  std::vector<double> noise_variance_samples(derivatives.size() + 1, 0.0);

  // Init vector of points and derivatives with values of
  // the already-sampled points, expilcitly setting derivatives to 0.
  std::vector<double> points_sampled_value(
      num_sampled * (derivatives.size() + 1), 0.0);
  for (int i = 0; i < num_sampled; ++i)
  {
    points_sampled_value[i * (derivatives.size() + 1)] = sample_objectives[i];
  }

  double random_base = tolerance;
  for (int retry = 0; retry < dim; retry++)
  {
    try
    {
      for (size_t i = 0; i < noise_variance_samples.size(); i++)
      {
        noise_variance_samples[i] = random_base * rng.uniform();
      }
      random_base += tolerance;
      SquareExponential covariance_opt(dim, trained_parameters[0],
                                       trained_parameters.data() + 1);
      result = std::make_unique<GaussianProcess>(
          covariance_opt, sample_points.data(), points_sampled_value.data(),
          noise_variance_samples.data(), derivatives.data(),
          static_cast<int>(derivatives.size()), dim, num_sampled);
      return result;
    }
    catch (SingularMatrixException& e)
    {
      LOG(WARN, "A SingularMatrixException is thrown, try again");
    }
  }
  return result;
}

bool BayesianOpt::recommend_parameter_values(
    std::vector<double>& parameters_new)
{
  if (sample_cur_ < search_training_.num_to_sample_)
  {
    copyout_winner(parameters_new);
    sample_cur_++;
    return true;
  }

#if defined(_DEBUG) || defined(qiotoolkit_PROFILING)
  log_parameters("Objectives:", sample_objectives_);
#endif
  auto start_time = std::chrono::high_resolution_clock::now();
  using DomainType = TensorProductDomain;
  const int dim = (int)dimensions_;
  const int num_sampled = (int)sample_objectives_.size();
  std::vector<ClosedInterval> domain_bounds;

  UniformRandomGenerator uniform_generator(rng_());  // repeatable results
  domain_bounds.reserve(dim);
  using CovarianceClass =
      SquareExponential;  // see gpp_covariance.hpp for other options
  CovarianceClass covariance_original(dim, trained_parameters_[0],
                                      trained_parameters_.data() + 1);
  std::vector<ClosedInterval> domain_bounds_log10(
      covariance_original.GetNumberOfHyperparameters(), ClosedInterval{0, 1});
  std::vector<int> derivatives(dim, 0);
  std::vector<int> derivatives_parameters(0);
  for (int i = 0; i < dim; i++)
  {
    ClosedInterval interval = {ranges_[i].first, ranges_[i].second};
    domain_bounds.push_back(interval);
    ClosedInterval interval_log10 = {std::log10(ranges_[i].first),
                                     std::log10(ranges_[i].second)};
    domain_bounds_log10[i + 1] = interval_log10;
    derivatives[i] = i;
    // derivatives_parameters[i] = i;
  }

  DomainType domain(domain_bounds.data(), dim);

  using LogLikelihoodEvaluator = LogMarginalLikelihoodEvaluator;

  // log likelihood evaluator object
  LogLikelihoodEvaluator log_marginal_eval(
      sample_points_.data(), sample_objectives_.data(),
      derivatives_parameters.data(),
      static_cast<int>(derivatives_parameters.size()), dim, num_sampled);

  GradientDescentParameters model_gd_parameters(
      model_training_.num_multistarts_, model_training_.max_num_steps_,
      model_training_.max_num_restarts_, model_training_.num_steps_averaged_,
      model_training_.gamma_, model_training_.pre_mult_,
      model_training_.max_relative_change_, model_training_.tolerance_);

  assert(dimensions_ + 1 ==
         (size_t)covariance_original.GetNumberOfHyperparameters());
  int max_num_threads = omp_get_max_threads();
  ThreadSchedule thread_schedule(max_num_threads, omp_sched_dynamic);
  std::vector<double> noise_variance_parameter(1, 0.01);

  for (size_t i = 0; i < noise_variance_parameter.size(); i++)
  {
    domain_bounds_log10.push_back(ClosedInterval{-6, -1});
  }

  // We assume there is no noise
  assert(noise_variance_parameter.size() == 1);
  bool found_flag = false;
  bool model_found = false;
  MultistartGradientDescentHyperparameterOptimization<LogLikelihoodEvaluator>(
      log_marginal_eval, covariance_original, noise_variance_parameter,
      model_gd_parameters,
      domain_bounds_log10.data(),  // need to be changed for log10 and adjust
                                   // for hyper parameters.
      thread_schedule, &model_found, &uniform_generator,
      trained_parameters_.data());
  if (!model_found)
  {
    return found_flag;
  }

#if defined(_DEBUG) || defined(qiotoolkit_PROFILING)
  log_hyper_parameters();
#endif
  auto gp_model =
      create_model(model_training_.tolerance_, trained_parameters_, dim,
                   sample_points_, sample_objectives_, derivatives, rng_);
  if (gp_model.get() == nullptr)
  {
    return found_flag;
  }

  auto end_time_model = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_model = std::chrono::duration_cast<std::chrono::milliseconds>(
                              end_time_model - start_time)
                              .count();
  model_time_ms_ += ms_diff_model;

  const int num_to_sample = search_training_.num_to_sample_;
  const int num_being_sampled = num_to_sample;

  // remaining inputs to EI optimization
  std::vector<double> points_being_sampled(num_being_sampled * dim);

  std::vector<NormalRNG> normal_rng_vec(max_num_threads);
  for (int i = 0; i < max_num_threads; ++i)
  {
    normal_rng_vec[i].SetExplicitSeed(rng_());  // to get repeatable results
  }

  assert(best_so_far_ == *std::min_element(sample_objectives_.begin(),
                                           sample_objectives_.end()));

  GradientDescentParameters gd_params(
      search_training_.num_multistarts_, search_training_.max_num_steps_,
      search_training_.max_num_restarts_, search_training_.num_steps_averaged_,
      search_training_.gamma_, search_training_.pre_mult_,
      search_training_.max_relative_change_, search_training_.tolerance_);
  bool lhc_search_only = false;

  // optimize EI using a model with the optimized hyperparameters

  int num_lhc_samples = num_to_sample;
  ComputeOptimalPointsToSample(
      *gp_model, gd_params, domain, thread_schedule,
      points_being_sampled.data(), num_to_sample, num_being_sampled,
      best_so_far_, search_training_.max_mc_steps_, lhc_search_only,
      num_lhc_samples, &found_flag, &uniform_generator, normal_rng_vec.data(),
      next_winner_points_.data());

  auto end_time = std::chrono::high_resolution_clock::now();
  int64_t ms_diff_learning =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time)
          .count();
  learning_time_ms_ += ms_diff_learning;
  if (found_flag)
  {
    sample_cur_ = 0;
    copyout_winner(parameters_new);
    sample_cur_++;
  }
#ifdef qiotoolkit_PROFILING
  update_profile(ms_diff_model, ms_diff_learning);
#endif
  return found_flag;
}

void BayesianOpt::configure(const utils::Json& params, int thread_count)
{
  using ::matcher::GreaterThan;

  // Please be careful to change the default values of training parameters.
  // They may affect the performance and correctness of parameter free
  // solvers.
  this->model_training_.num_multistarts_ = thread_count;
  this->model_training_.max_num_steps_ = 100;
  this->model_training_.max_num_restarts_ = 12;
  this->model_training_.num_steps_averaged_ = 4;
  this->model_training_.max_mc_steps_ = 100;
  this->model_training_.gamma_ = 1.01;
  this->model_training_.pre_mult_ = 1.0e-3;
  this->model_training_.max_relative_change_ = 1.0;
  this->model_training_.tolerance_ = 1.0e-6;
  this->model_training_.num_to_sample_ = 1;
  if (params.HasMember("model_training"))
  {
    this->model_training_.configure(params["model_training"]);
  }

  this->search_training_.num_multistarts_ = thread_count;
  this->search_training_.max_num_steps_ = 100;
  this->search_training_.max_num_restarts_ = 12;
  this->search_training_.gamma_ = 0.7;
  this->search_training_.pre_mult_ = 1.0;
  this->search_training_.max_relative_change_ = 0.8;
  this->search_training_.tolerance_ = 1.0e-7;
  this->search_training_.num_steps_averaged_ = 4;
  this->search_training_.max_mc_steps_ = 100;
  this->search_training_.num_to_sample_ = 4;
  if (params.HasMember("search_training"))
  {
    this->search_training_.configure(params["search_training"]);
  }

  this->param(params, strategy::kReservedSamples, this->reserved_samples_)
      .description("max number of samples reserved for training")
      .matches(GreaterThan<size_t>(10));
}

}  // namespace strategy
