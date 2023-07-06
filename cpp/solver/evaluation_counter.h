
#pragma once

#include "utils/component.h"

namespace markov
{
template <class T>
class Walker;

template <class T>
class ClusterWalker;
}  // namespace markov

namespace solver
{
class EvaluationCounter : public utils::Component
{
 public:
  EvaluationCounter();
  EvaluationCounter(const EvaluationCounter& other);

  void reset();

  const EvaluationCounter& operator+=(const EvaluationCounter& other);
  const EvaluationCounter& operator-=(const EvaluationCounter& other);
  EvaluationCounter& operator=(const EvaluationCounter& other) = default;

  utils::Structure render() const;

  uint64_t get_function_evaluation_count() const;
  uint64_t get_difference_evaluation_count() const;
  uint64_t get_accepted_transition_count() const;

 private:
  template <class T>
  friend class markov::Walker;
  template <class T>
  friend class markov::ClusterWalker;

  uint64_t function_evaluations_;
  uint64_t difference_evaluations_;
  uint64_t accepted_transitions_;
};

}  // namespace solver

solver::EvaluationCounter operator+(const solver::EvaluationCounter& left,
                                    const solver::EvaluationCounter& right);
