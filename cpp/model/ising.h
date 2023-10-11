
#pragma once

#include <limits>
#include <set>
#include <string>
#include <vector>

#include "utils/operating_system.h"
#include "utils/utils.h"
#include "markov/state.h"
#include "model/graph_compact_model.h"
#include "model/graph_model.h"

namespace model
{
/// Return a random Ising state.
///
/// The number of Ising variables (terms) is determined from the number of
/// nodes (edges) in the underlying graph.
/// The input model must have an apply_transition member function
/// with transition and state parameters.
template <class State_T, class Ising_T>
State_T get_random_spin_state(utils::RandomGenerator& rng, const Ising_T& model)
{
  State_T state(model.node_count(), model.edge_count());
  uint32_t random_value = 0;
  for (size_t i = 0; i < model.node_count(); i++)
  {
    if (i % 32 == 0)
    {
      random_value = rng.uint32();
    }
    uint32_t bit = (random_value >> (i % 32)) & 1;
    if (bit == 1)
    {
      model.apply_transition(i, state);
    }
  }
  return state;
}

template <class State_T, class Ising_T>
State_T get_initial_configuration_spin_state(
    const Ising_T& model)
{
  State_T state(model.node_count(), model.edge_count());
  for (const auto& id_val : model.get_initial_configuration())
  {
    if (id_val.second == -1)
    {
      model.apply_transition(id_val.first, state);
    }
    else if (id_val.second == 1)
    {
      // Leave default spin value
    }
    else
    {
      THROW(utils::ValueException,
            "parameter `initial_configuration` may have 1 and -1 values for ising models, "
            "found: ",
            id_val.second);
    }
  }
  return state;
}

////////////////////////////////////////////////////////////////////////////////
/// Ising Model
template <class State_T, class Cost_T = double>
class AbstractIsing : public GraphModel<State_T, size_t, Cost_T>
{
 public:
  using Transition_T = size_t;
  using Graph = GraphModel<State_T, size_t, Cost_T>;

  std::string get_identifier() const override { return "ising"; }
  std::string get_version() const override { return "1.1"; }
  void match_version(const std::string& version) override
  {
    if (version.compare("1.0") == 0 || version.compare(get_version()) == 0)
    {
      return;
    }
    throw utils::ValueException(
        "Expecting `version` equals 1.0 or 1.1, but found: " + version);
  }

  /// Cost function:
  ///
  /// \f[ \mathcal{H} = -\sum_{j\in\mathrm{\{edges\}}} c_j\prod_{i\in j}s_i \f]
  ///
  double calculate_cost(const State_T& state) const override
  {
    double cost = 0.0;
    for (size_t j = 0; j < Graph::edges().size(); j++)
    {
      cost += get_term(state, j);
    }
    return cost;
  }

  /// Cost difference:
  ///
  /// \f[ \Delta_{i\to i'} = 2 \sum_{j\in e_i} c_j\prod_{i\in j}s_i \f]
  ///
  double calculate_cost_difference(const State_T& state,
                                   const Transition_T& spin_id) const override
  {
    double diff = 0.0;
    for (size_t j : Graph::node(spin_id).edge_ids())
    {
      diff += get_term(state, j);
    }
    return 2.0 * (-diff);
  }

  /// Return a random Ising state.
  ///
  /// The number of Ising variables (terms) is determined from the number of
  /// nodes (edges) in the underlying graph.
  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    return get_random_spin_state<State_T, AbstractIsing<State_T>>(rng, *this);
  }

  State_T get_initial_configuration_state() const override
  {
    return get_initial_configuration_spin_state<State_T,
                                                AbstractIsing<State_T>>(*this);
  };

  /// Return a random Single spin update.
  ///
  /// We represent a single spin update as merely the index of the
  /// spin variable that will flip.
  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    return static_cast<Transition_T>(
        rng.uniform() * static_cast<double>(Graph::nodes().size()));
  }

  /// Flip the spin specified by spin_id.
  void apply_transition(const Transition_T& spin_id,
                        State_T& state) const override = 0;

  /// Serialize metadata and the underlying graph.
  ///
  /// The underlying graph represents the "disorder" of the Ising
  /// model (glass) we are simulating.
  void configure(const utils::Json& json) override { Graph::configure(json); }

  void configure(typename Graph::Configuration_T& configuration)
  {
    Graph::configure(configuration);
  }

  utils::Structure render_state(const State_T& state) const override
  {
    return state.render(this->graph_.output_map());
  }

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(this->graph_.nodes().size(),
                                    this->graph_.edges().size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(this->graph_.nodes().size());
  }

  double estimate_max_cost_diff() const override
  {
    return GraphModel<State_T, size_t>::estimate_max_cost_diff() * 2;
  }

  double estimate_min_cost_diff() const override
  {
    auto nodes = this->graph_.nodes();
    double min_diff = std::numeric_limits<double>::max();
#ifdef _MSC_VER
    // reduction min or max is not implemented yet in all compilers
    #pragma omp parallel
    {
      double min_diff_local = std::numeric_limits<double>::max();
      #pragma omp for
#else
    #pragma omp parallel for reduction(min : min_diff)
#endif
      for (size_t i = 0; i < nodes.size(); i++)
      {
        auto node_i = this->graph_.node(i);
        std::multiset<double> sorted_costs;
        for (auto edge_id : node_i.edge_ids())
        {
          auto edge_i = this->graph_.edge(edge_id);
          sorted_costs.insert(std::abs(edge_i.cost()));
        }
        double diff_local = utils::least_diff_method(sorted_costs);
        if (diff_local == 0)
        {
          continue;
        }
#ifdef _MSC_VER
        min_diff_local =
            diff_local < min_diff_local ? diff_local : min_diff_local;
      }
      #pragma omp critical
      min_diff = min_diff_local < min_diff ? min_diff_local : min_diff;
    }
#else
      min_diff = diff_local < min_diff ? diff_local : min_diff;
    }
#endif

    if (min_diff == std::numeric_limits<double>::max())
    {
      min_diff = (MIN_DELTA_DEFAULT);
    }
    return min_diff * 2;
  }

 protected:
  /// Calculate the term with id `term_id`.
  virtual double get_term(const State_T& state, size_t term_id) const = 0;
};

/// Calculate the term with id `term_id`.
template <class State_T, class Ising_T>
double get_ising_term(const State_T& state, size_t term_id,
                      const Ising_T& model)
{
  bool term = false;
  const auto& e = model.edge(term_id);
  for (size_t i = 0; i < e.node_ids().size(); i++)
  {
    term ^= state.spins[e.node_ids()[i]];
  }
  double c = e.cost();
  return term ? -c : c;
}

////////////////////////////////////////////////////////////////////////////////
/// Ising state representation
///
/// We represent an ising state as both the boolean value of each spin
/// and the product of the spins in each term.
///
class IsingState : public ::markov::State
{
 public:
  IsingState() {}

  /// Create an Ising state with N spins and M terms.
  ///
  /// All spins are initialized to 0 ("up" or "+1").
  /// A true value represents "down" or "-1".
  ///
  /// As such, the boolean value can be considered as representing the sign.
  IsingState(size_t N, size_t) : spins(N) {}

  /// Render the spins of the state.
  utils::Structure render() const override;

  utils::Structure render(const std::map<int, int>&) const;

  void copy_state_only(const IsingState& other) { spins = other.spins; }

  static size_t memory_estimate(size_t N, size_t)
  {
    return sizeof(IsingState) + utils::vector_values_memory_estimate<bool>(N);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N, 0);
  }

  std::vector<bool> spins;
};

inline bool same_state_value(const IsingState& state1, const IsingState& state2,
                             size_t transition)
{
  return state1.spins[transition] == state2.spins[transition];
}

class Ising : public AbstractIsing<IsingState>
{
 public:
  /// Flip the spin specified by spin_id.
  void apply_transition(const size_t& spin_id, State_T& state) const override;

 protected:
  /// Calculate the term with id `term_id`.
  double get_term(const IsingState& state, size_t term_id) const override;
};

////////////////////////////////////////////////////////////////////////////////
/// Ising state representation with term cache
class IsingTermCachedState : public IsingState
{
 public:
  IsingTermCachedState() {}
  IsingTermCachedState(size_t N, size_t M) : IsingState(N, M), terms(M) {}
  void copy_state_only(const IsingTermCachedState& other)
  {
    // not copying the terms
    spins = other.spins;
  }

  static size_t memory_estimate(size_t N, size_t M)
  {
    return sizeof(IsingTermCachedState) - sizeof(IsingState) +
           IsingState::memory_estimate(N, M) +
           utils::vector_values_memory_estimate<bool>(M);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N, 0);
  }

  std::vector<bool> terms;
};

class IsingTermCached : public AbstractIsing<IsingTermCachedState>
{
 public:
  /// Flip the spin specified by spin_id.
  void apply_transition(const Transition_T& spin_id,
                        IsingTermCachedState& state) const override;

 protected:
  /// Get the term with id `term_id` from cache.
  double get_term(const IsingTermCachedState& state,
                  size_t term_id) const override;
};

/// ISING model state for IsingCompact
class IsingCompactState : public IsingState
{
 public:
  IsingCompactState() : IsingState() {}

  /// Create an Ising state with N spins and M terms.
  ///
  /// All spins are initialized to 0 ("up" or "+1").
  /// A true value represents "down" or "-1".
  ///
  /// As such, the boolean value can be considered as representing the sign.
  IsingCompactState(size_t N, size_t) : IsingState(N, N) {}

  // Reader used for accessing the GraphCompactModel
  graph::CompactGraphVisitor reader_;
};

inline bool same_state_value(const IsingCompactState& state1,
                             const IsingCompactState& state2,
                             size_t transition)
{
  return state1.spins[transition] == state2.spins[transition];
}

/// Ising model which use GraphCompactModel as model base.
template <typename Element_T>
class IsingCompact
    : public GraphCompactModel<IsingCompactState, size_t, Element_T>
{
 public:
  using State_T = IsingCompactState;
  using GraphCompact = GraphCompactModel<State_T, size_t, Element_T, double>;

  std::string get_identifier() const override { return "ising"; }
  std::string get_version() const override { return "1.1"; }
  void match_version(const std::string& version) override
  {
    if (version.compare("1.0") == 0 || version.compare(get_version()) == 0)
    {
      return;
    }
    throw utils::ValueException(
        "Expecting `version` equals 1.0 or 1.1, but found: " + version);
  }

  /// Cost function:
  ///
  /// \f[ \mathcal{H} = -\sum_{j\in\mathrm{\{edges\}}} c_j\prod_{i\in j}s_i \f]
  ///
  double calculate_cost(const State_T& state) const override
  {
    double cost = 0.0;
    for (size_t j = 0; j < this->edge_count(); j++)
    {
      cost += get_term(state, j);
    }
    return cost;
  }

  /// Cost difference:
  ///
  /// \f[ \Delta_{i\to i'} = 2 \sum_{j\in e_i} c_j\prod_{i\in j}s_i \f]
  ///
  double calculate_cost_difference(const IsingCompactState& state,
                                   const size_t& spin_id) const override
  {
    double diff = 0.0;
    bool var_end = false;
    bool spin_value = state.spins[spin_id];
    bool term = spin_value;

    graph::CompactGraphVisitor& reader =
        const_cast<graph::CompactGraphVisitor&>(state.reader_);
    if (spin_id == 0)
    {
      reader.reset();
    }
    while (!var_end)
    {
      uint64_t value = this->graph_.read_next_neig(reader);
      var_end = this->graph_.is_node_end(value);
      bool term_end = this->graph_.is_edge_end(value);
      if (!(term_end || var_end))
      {
        term ^= state.spins[value];
      }
      else
      {
        // this is the end of one term/edge
        double coeff = this->graph_.read_next_coeff(reader);

        // true -> -1, and false -> 1
        diff += (term ? -coeff : coeff);

        // reset term for next one.
        term = spin_value;
      }
    }

    return 2.0 * (-diff);
  }
  /// Return a random Ising state.
  ///
  /// The number of Ising variables (terms) is determined from the number of
  /// nodes (edges) in the underlying graph.
  IsingCompactState get_random_state(utils::RandomGenerator& rng) const override
  {
    return get_random_spin_state<IsingCompactState, IsingCompact>(rng, *this);
  }

  IsingCompactState get_initial_configuration_state() const override
  {
    return get_initial_configuration_spin_state<IsingCompactState,
                                                IsingCompact>(*this);
  };

  /// Return a random Single spin update.
  ///
  /// We represent a single spin update as merely the index of the
  /// spin variable that will flip.
  size_t get_random_transition(const IsingCompactState&,
                               utils::RandomGenerator& rng) const override
  {
    return static_cast<size_t>(rng.uniform() *
                               static_cast<double>(this->graph_.nodes_size()));
  }

  /// Flip the spin specified by spin_id.
  void apply_transition(const size_t& spin_id,
                        IsingCompactState& state) const override
  {
    state.spins[spin_id] = !state.spins[spin_id];
  }

  /// Serialize metadata and the underlying graph.
  ///
  /// The underlying graph represents the "disorder" of the Ising
  /// model (glass) we are simulating.
  void configure(const utils::Json& json) override
  {
    GraphCompact::configure(json);
  }

  void configure(typename GraphCompact::Configuration_T& configuration)
  {
    GraphCompact::configure(configuration);
  }

  utils::Structure render_state(const IsingCompactState& state) const override
  {
    return state.render(this->graph_.output_map());
  }

  size_t state_memory_estimate() const override
  {
    return IsingCompactState::memory_estimate(this->graph_.nodes_size(),
                                              this->graph_.edges_size());
  }

  size_t state_only_memory_estimate() const override
  {
    return IsingCompactState::state_only_memory_estimate(
        this->graph_.nodes_size());
  }

  double estimate_min_cost_diff() const override
  {
    double min_diff = std::numeric_limits<double>::max();
    graph::CompactGraphVisitor reader;
    for (size_t i = 0; i < this->node_count(); i++)
    {
      std::multiset<double> sorted_costs;
      bool var_end = false;
      while (!var_end)
      {
        uint64_t value = this->graph_.read_next_neig(reader);
        var_end = this->graph_.is_node_end(value);
        bool term_end = this->graph_.is_edge_end(value);
        if (var_end || term_end)
        {
          sorted_costs.insert(std::abs(this->graph_.read_next_coeff(reader)));
        }
      }

      double diff_local = utils::least_diff_method(sorted_costs);
      if (diff_local == 0)
      {
        continue;
      }
      min_diff = diff_local < min_diff ? diff_local : min_diff;
    }

    if (min_diff == std::numeric_limits<double>::max())
    {
      min_diff = (MIN_DELTA_DEFAULT);
    }
    return min_diff * 2;
  }

  double estimate_max_cost_diff() const override
  {
    return GraphCompact::estimate_max_cost_diff() * 2;
  }

 protected:
  double get_term(const State_T& state, size_t term_id) const
  {
    return get_ising_term<State_T, IsingCompact>(state, term_id, *this);
  }
};

}  // namespace model
