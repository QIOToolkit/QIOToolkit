
#include "model/clock.h"

namespace model
{
ClockState::ClockState() {}
ClockState::ClockState(size_t N, size_t M) : spins(N), terms(M) {}

utils::Structure ClockState::render() const
{
  utils::Structure s;
  s = spins;
  return s;
}

double Clock::calculate_term(const State_T& state, size_t edge_id, double dx,
                             double dy) const
{
  double m = (double)edge(edge_id).node_ids().size();
  const auto& term = state.terms[edge_id];
  double x = term.x + dx;
  double y = term.y + dy;
  double v2 = (x * x + y * y) / m / m;
  // Rescaling such that the case of 2 interacting spins corresponds to the
  // definition of the clock model (scalar product of the 2d vectors):
  //
  //   Examples (q=4):
  //     s_1 = s_2        0  v_e = {1,0}       2*v^2-1 =  1
  //     s_1 = s_2+1  \pi/2  v_e = {1/2,1/2}   2*v^2-1 =  0
  //     s_1 = s_2+2    \pi  v_e = 0           2*v^2-1 = -1
  //
  //
  return edge(edge_id).cost() * (2 * v2 - 1);
}

double Clock::calculate_cost(const State_T& state) const
{
  double cost = 0.0;
  // Calculate the total cost as the sum of edge contributions.
  for (size_t edge_id = 0; edge_id < edges().size(); edge_id++)
  {
    cost += calculate_term(state, edge_id);
  }
  return cost;
}

double Clock::calculate_cost_difference(const State_T& state,
                                        const Transition_T& transition) const
{
  double diff = 0.0;
  size_t old_value = state.spins[transition.spin_id];
  const size_t& new_value = transition.new_value;
  // Calculate difference as sum of changes in affected terms.
  for (size_t edge_id : node(transition.spin_id).edge_ids())
  {
    double dx = -cos_[old_value] + cos_[new_value];
    double dy = -sin_[old_value] + sin_[new_value];
    // subtract the contribution at current configuration
    diff -= calculate_term(state, edge_id);
    // add instead the contribution with modified x,y
    diff += calculate_term(state, edge_id, dx, dy);
  }
  return diff;
}

Clock::State_T Clock::get_random_state(utils::RandomGenerator& rng) const
{
  State_T state(nodes().size(), edges().size());
  // Set each spin to a random direction
  for (size_t node_id = 0; node_id < nodes().size(); node_id++)
  {
    state.spins[node_id] =
        static_cast<size_t>(floor(rng.uniform() * static_cast<double>(q_)));
  }
  // Initialize the partially precomputed terms.
  initialize_terms(state);
  return state;
}

Clock::Transition_T Clock::get_random_transition(
    const State_T& state, utils::RandomGenerator& rng) const
{
  // Choose a random spin
  size_t spin_id = static_cast<size_t>(
      floor(rng.uniform() * static_cast<double>(state.spins.size())));
  // Choose a random **new** direction
  size_t value = (state.spins[spin_id] + 1 +
                  static_cast<size_t>(
                      floor(rng.uniform() * static_cast<double>(q_ - 1)))) %
                 q_;
  return {spin_id, value};
}

void Clock::apply_transition(const Transition_T& transition,
                             State_T& state) const
{
  const size_t old_value = state.spins[transition.spin_id];
  const size_t& new_value = transition.new_value;
  // Update the partially precomputed terms.
  for (size_t edge_id : node(transition.spin_id).edge_ids())
  {
    auto& term = state.terms[edge_id];
    term.x += -cos_[old_value] + cos_[new_value];
    term.y += -sin_[old_value] + sin_[new_value];
  }
  // Update the spin itself.
  state.spins[transition.spin_id] = transition.new_value;
}

void Clock::set_number_of_states(size_t q)
{
  q_ = q;
  // When setting the number of states, also prepare a vector of
  // precomputed cosine and sine values for each q-value.
  cos_.resize(q_);
  sin_.resize(q_);
  double pi = acos(-1);
  for (size_t k = 0; k < q_; k++)
  {
    cos_[k] = cos(static_cast<double>(k) * 2 * pi / static_cast<double>(q_));
    sin_[k] = sin(static_cast<double>(k) * 2 * pi / static_cast<double>(q_));
  }
}

void Clock::initialize_terms(State_T& state) const
{
  // set term.x and term.y to the sum of node contributions.
  state.terms.resize(edges().size());
  for (size_t edge_id = 0; edge_id < edges().size(); edge_id++)
  {
    auto& term = state.terms[edge_id];
    term.x = 0;
    term.y = 0;
    for (size_t node_id : edge(edge_id).node_ids())
    {
      term.x += cos_[state.spins[node_id]];
      term.y += sin_[state.spins[node_id]];
    }
  }
}

void Clock::configure(const utils::Json& json)
{
  Graph::configure(json);
  this->param(json[utils::kCostFunction], "q", q_)
      .description("number_of_states")
      .required()
      .matches(matcher::GreaterThan<size_t>(2));
  set_number_of_states(q_);
}

void Clock::configure(Configuration_T& conf)
{
  Graph::configure(conf);
  q_ = Configuration_T::Get_Number_of_States::get(conf);
  if (q_ <= 2)
  {
    THROW(utils::ValueException, "parameter `q`: must be greater than 2, found ",
          q_);
  }
  set_number_of_states(q_);
}

}  // namespace model
