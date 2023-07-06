
#pragma once

#include "utils/component.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// Base class for a markov transition.
///
/// A transition describes a proposed move from the current to the next state.
/// Depending on the algorithm, this move might be conditionally accepted,
/// in which case the algorithm invokes `apply_transition(transition, &state)`
/// on the model.
///
/// NOTE: When deciding how to represent your transition, try to
/// envision the minimum amount of information required for `apply_transition`
/// to do its work. This is typically also what `calculate_cost_difference`
/// needs to efficiently calculate the resulting change in cost for a
/// transition.
///
/// @see markov::State
/// @see markov::Model::apply_transition
/// @see markov::Model::calculate_cost_difference
///
class Transition : public utils::Component
{
 public:
};

////////////////////////////////////////////////////////////////////////////////
/// Predefined simple markov transition.
///
/// This is a transition class which works for any `State` -- it simply
/// keeps track of the state before and after the transition. This is
/// convenient for initial testing but not efficient because:
///
///   1) New states are repeatedly created for each (potential transition)
///
///   2) `calculate_cost_difference()` cannot exploit knowledge about what
///      changed to partially recalculate the `calculate_cost` (apart from
///      calculating the diff from `state` and `target`).
///
template <class State>
class SimpleTransition : Transition
{
 public:
  /// Create a simple transition to an explicit target state.
  SimpleTransition(State target) : target_(target) {}

  /// Accessor for the `target` state.
  const State& target() const { return target_; }

 private:
  State target_;
};

}  // namespace markov
