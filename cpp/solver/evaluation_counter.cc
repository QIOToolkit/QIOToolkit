// Copyright (c) Microsoft. All Rights Reserved.
#include "solver/evaluation_counter.h"

solver::EvaluationCounter operator+(const solver::EvaluationCounter& left,
                                    const solver::EvaluationCounter& right)
{
  solver::EvaluationCounter result(left);
  result += right;
  return result;
}

namespace solver
{
EvaluationCounter::EvaluationCounter(const EvaluationCounter& other)
{
  function_evaluations_ = other.function_evaluations_;
  difference_evaluations_ = other.difference_evaluations_;
  accepted_transitions_ = other.accepted_transitions_;
}

EvaluationCounter::EvaluationCounter() { reset(); }

void EvaluationCounter::reset()
{
  function_evaluations_ = 0;
  difference_evaluations_ = 0;
  accepted_transitions_ = 0;
}

const EvaluationCounter& EvaluationCounter::operator+=(
    const EvaluationCounter& other)
{
  function_evaluations_ += other.function_evaluations_;
  difference_evaluations_ += other.difference_evaluations_;
  accepted_transitions_ += other.accepted_transitions_;
  return *this;
}

const EvaluationCounter& EvaluationCounter::operator-=(
    const EvaluationCounter& other)
{
  function_evaluations_ -= other.function_evaluations_;
  difference_evaluations_ -= other.difference_evaluations_;
  accepted_transitions_ -= other.accepted_transitions_;
  return *this;
}

utils::Structure EvaluationCounter::render() const
{
  utils::Structure s;
  s["function_evaluations"] = function_evaluations_;
  s["difference_evaluations"] = difference_evaluations_;
  s["accepted_transitions"] = accepted_transitions_;
  return s;
}

uint64_t EvaluationCounter::get_function_evaluation_count() const
{
  return function_evaluations_;
}

uint64_t EvaluationCounter::get_difference_evaluation_count() const
{
  return difference_evaluations_;
}

uint64_t EvaluationCounter::get_accepted_transition_count() const
{
  return accepted_transitions_;
}

}  // namespace solver
