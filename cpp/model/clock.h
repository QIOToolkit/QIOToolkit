
#pragma once

#include <string>
#include <vector>

#include "utils/operating_system.h"
#include "markov/state.h"
#include "markov/transition.h"
#include "model/graph_model.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// Clock state representation
///
/// Represents:
///
///   * N discretized clock spins as a vector<int>
///   * M state terms as {x: double, y: double}
///
/// The state terms hold the sum of cosines (sines) of the discretized clock
/// model directions, respectively. I.e.,
///
/// term[j].x = \sum_{i \in j} cos(s_i * 2\pi/q)
/// term[j].y = \sum_{i \in j} sin(s_i * 2\pi/q)
///
class ClockState : public ::markov::State
{
 public:
  /// Create an uninitialized clock state.
  ClockState();
  /// Create a clock state with N spins and M terms.
  ClockState(size_t N, size_t M);

  struct Term
  {
    double x, y;
  };

  std::vector<size_t> spins;
  std::vector<Term> terms;

  void configure(const utils::Json& json) override
  {
    this->param(json, "spins", spins)
        .description("the spins of the clock state")
        .required();
  }

  void copy_state_only(const ClockState& other) { spins = other.spins; }

  utils::Structure render() const override;

  static size_t memory_estimate(size_t N, size_t M)
  {
    return sizeof(ClockState) + utils::vector_values_memory_estimate<size_t>(N) +
           utils::vector_values_memory_estimate<Term>(M);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N, 0);
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Clock transition representation
///
/// Which spin to change and to what value.
struct ClockTransition
{
  size_t spin_id;
  size_t new_value;
  bool operator==(const ClockTransition& trans) const
  {
    return spin_id == trans.spin_id && new_value == trans.new_value;
  }
};

inline bool same_state_value(const ClockState& state1, const ClockState& state2,
                      const ClockTransition& transition)
{
  return state1.spins[transition.spin_id] == state2.spins[transition.spin_id];
}

class ClockConfiguration : public GraphModelConfiguration
{
 public:
  struct Get_Number_of_States
  {
    static size_t& get(ClockConfiguration& config) { return config.q_; }
    static std::string get_key() { return "q"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::BasicTypeStreamHandler<size_t>, ClockConfiguration,
      Get_Number_of_States, true,
      GraphModelConfiguration::MembersStreamHandler>;

  using StreamHandler = ModelStreamHandler<ClockConfiguration>;

 protected:
  size_t q_;
};

class Clock : public GraphModel<ClockState, ClockTransition>
{
 public:
  using State_T = ClockState;
  using Transition_T = ClockTransition;
  using Graph = GraphModel<State_T, Transition_T>;
  using Configuration_T = ClockConfiguration;

  std::string get_identifier() const override { return "clock"; }
  std::string get_version() const override { return "1.0"; }

  Clock() : q_(0) {}

  //////////////////////////////////////////////////////////////////////////////
  /// Calculate the Clock Hamiltonian.
  ///
  /// The standard clock Hamiltonian is defined for edges involving two nodes:
  ///
  /// \$ \mathcal{H}' = \sum_{ij} \mathrm{cost}_{ij}\vec{s_i}\vec{s_j} \$,
  ///
  /// Where \$ \vec{s_i} \$ are equi-distributed 2d vectors on the unit circle.
  /// (Equivalently: \$ \vec{s_i}\vec{s_j} = \cos(\theta_i - \theta_j) \$ with
  /// \$ \theta_i = 2\pi i/q \$.)
  ///
  /// Here we use a generalization to n-node hyperedges using the sum of planar
  /// vectors partcipating in the edge:
  ///
  /// \$ v_e = \frac{1}{|e|} \sum_{i\in e} \vec{s_i} \$,
  ///
  /// Where \$\vec{s_i}\$ is defined as above and \$|e|\$ is the number of nodes
  /// in the hyper-edge. With this our Hamiltonian can be expressed as:
  ///
  /// $ \mathcal{H} = \sum_e \mathrm{cost}_e (2*v_e^2-1),
  ///
  /// where the rescaling ensures that the generalization corresponds to the
  /// standard defintion for edges with 2 nodes (i.e, +1 when aligned, -1 when
  /// anti-aligned, 0 when perpendicular). This means that the contribution is
  ///
  ///   * `+1 * cost_e` when the clock spins are aligned (in any direction) and
  ///   * `-1 * cost_e` when the clock spins add up to zero.
  ///
  double calculate_cost(const State_T& state) const override;

  //////////////////////////////////////////////////////////////////////////////
  /// Compute the difference resulting from `transition`
  ///
  /// This recomputes v_e for each of the terms in which `transition.spin_id` is
  /// involved.
  ///
  double calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override;

  //////////////////////////////////////////////////////////////////////////////
  /// Build a random Clock state.
  ///
  /// This function decides on the number of discretized clock spins and the
  /// possible values of each to return (according to the model configuration).
  /// Partially precomputed terms (`term[j].x`, etc.) are populated here.
  ///
  State_T get_random_state(utils::RandomGenerator& rng) const override;

  //////////////////////////////////////////////////////////////////////////////
  /// Create a random transition for `state`.
  ///
  /// This picks a random `spin_id` and proposes a `new_value` that is
  /// different from the current one.
  ///
  Transition_T get_random_transition(const State_T& state,
                                     utils::RandomGenerator& rng) const override;

  //////////////////////////////////////////////////////////////////////////////
  /// Change `state` according to `transition`.
  ///
  /// This sets the modified spin to its new value and updates the partially
  /// precomputed terms that are affected by this change.
  ///
  void apply_transition(const Transition_T& transition,
                        State_T& state) const override;

  /// Serialize the number of states `q` and the underlying graph.
  void configure(const utils::Json& json) override;

  void configure(Configuration_T& conf);

  /// Initialize the `state.terms` according to the model and current
  /// `state.spins`.
  void initialize_terms(State_T& state) const;

  /// Estimate memory consumption for state
  size_t state_memory_estimate() const override
  {
    return State_T ::memory_estimate(nodes().size(), edges().size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T ::state_only_memory_estimate(nodes().size());
  }

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// This computes the Hamiltonian contribution of the edge `edge_id`.
  ///
  /// If specified, dx and dy are applied to the term (this is used to emulate
  /// a modified configuration within `calculate_cost_difference`).
  double calculate_term(const State_T& state, size_t edge_id, double dx = 0,
                        double dy = 0) const;

  /// Set the number of states to q and precompute the cos/sin values for the
  /// discretized angles corresponding to 1..(q-1) * 2\pi/q.
  void set_number_of_states(size_t q);

 private:
  size_t q_;
  std::vector<double> cos_;
  std::vector<double> sin_;
};
REGISTER_MODEL(Clock);

}  // namespace model

template <>
struct std::hash<model::ClockTransition>
{
  std::size_t operator()(const model::ClockTransition& trans) const noexcept
  {
    return utils::get_combined_hash(trans.spin_id, trans.new_value);
  }
};
