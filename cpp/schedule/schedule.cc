// Copyright (c) Microsoft. All Rights Reserved.
#include "schedule/schedule.h"

#include <sstream>

#include "schedule/constant.h"
#include "schedule/geometric.h"
#include "schedule/linear.h"
#include "schedule/segments.h"

namespace schedule
{
using utils::ConfigurationException;
using utils::InitialConfigException;
using utils::ValueException;

double Schedule::get_value(double input) const
{
  double start = generator_->get_start();
  double stop = generator_->get_stop();
  if (start == stop) return generator_->get_progress_value(0);
  double progress = (input - start) / (stop - start);
  if (generator_->is_repeated()) progress -= floor(progress);
  return generator_->get_progress_value(std::min(1., std::max(0., progress)));
}

namespace
{
enum CountSource
{
  EXPLICIT_INPUT = 1,
  COUNT_PARAMETER = 2,
  VALUE_RANGE = 3,
};

}  // namespace

std::vector<double> Schedule::get_discretized_values(int count) const
{
  CountSource source = EXPLICIT_INPUT;
  double start = generator_->get_start();
  double stop = generator_->get_stop();
  if (count == -1 && generator_->has_count())
  {
    source = COUNT_PARAMETER;
    count = generator_->get_count();
  }
  if (count == -1)
  {
    source = VALUE_RANGE;
    count = (int)round(std::abs(stop - start) + 1);
    if (count == 2)
    {
      LOG(WARN,
          "assuming discretization to 2 values for schedule. "
          "You can change this by setting `count` in the generator.");
    }
  }
  if (count < 2)
  {
    std::stringstream msg;
    msg << "Can not get_discretized_values(count) for less than 2 values\n";
    switch (source)
    {
      case EXPLICIT_INPUT:
        msg << "  'count=" << count << "' requested explicitly in the call.\n"
            << "  (invoke without arguments to use the \"count\" parameter)";
        break;
      case COUNT_PARAMETER:
        msg << "  'count:" << count
            << "' set as parameter value in the generator configuration.\n"
            << "  (change 'count' to be >= 2 or remove it to use "
               "stop-start+1)";
        break;
      case VALUE_RANGE:
        msg << "  'count=" << count << "' assumed from stop-start+1.\n"
            << "you can set it explicitly via the 'count' parameter.";
        break;
    }
    throw ValueException(msg.str());
  }
  std::vector<double> discretized;
  for (int i = 0; i < count; i++)
  {
    discretized.push_back(
        generator_->get_progress_value(double(i) / (count - 1)));
  }
  return discretized;
}

double Schedule::get_start() const { return generator_->get_start(); }
double Schedule::get_stop() const { return generator_->get_stop(); }
int Schedule::get_count() const { return generator_->get_count(); }
void Schedule::set_stop(int stop) { generator_->set_stop(stop); }

// This macro "registers" the different types of generators with the
// serialization method of `Schedule`. That is, it attempts to
// configure each type sequentally, throwing a `ConfigurationException`
// if none of them succeed.
#define ATTEMPT_SCHEDULE(Schedule)                             \
  try                                                          \
  {                                                            \
    generator_.reset(new Schedule());                          \
    generator_->configure(json);                               \
    return;                                                    \
  }                                                            \
  catch (ConfigurationException & e)                           \
  {                                                            \
    err << std::endl << "  " << #Schedule << ": " << e.what(); \
    generator_.reset();                                        \
  }

void Schedule::configure(const utils::Json& json)
{
  std::stringstream err;
  err << "unable to initialize schedule:";
  ATTEMPT_SCHEDULE(Linear);
  ATTEMPT_SCHEDULE(Geometric);
  ATTEMPT_SCHEDULE(Segments);
  ATTEMPT_SCHEDULE(Constant);
  throw(InitialConfigException(err.str()));
}

#undef ATTEMPT_SCHEDULE

void Schedule::make_linear(double beta_start, double beta_stop)
{
  generator_.reset(new Linear(beta_start, beta_stop));
}

void Schedule::make_geometric(double beta_start, double beta_stop)
{
  generator_.reset(new Geometric(beta_start, beta_stop));
}

void Schedule::make_list(const std::vector<double>& values)
{
  std::unique_ptr<Segments> segs(new Segments(values));
  segs->adjust();
  generator_.reset(segs.release());
}

void Schedule::make_constant(double value)
{
  generator_.reset(new Constant(value));
}
utils::Structure Schedule::render() const { return generator_->render(); }
}  // namespace schedule
