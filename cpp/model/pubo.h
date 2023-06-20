// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <limits>
#include <set>
#include <sstream>
#include <vector>

#include "utils/operating_system.h"
#include "utils/utils.h"
#include "model/graph_compact_model.h"
#include "model/graph_model.h"

namespace model
{
template <class State_T, class Pubo_T>
State_T get_random_binary_state(utils::RandomGenerator& rng, const Pubo_T& model)
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

template <class State_T, class Pubo_T>
State_T get_initial_configuration_binary_state(const Pubo_T& model)
{
  State_T state(model.node_count(), model.edge_count());
  for (const auto& id_val : model.get_initial_configuration())
  {
    if (id_val.second == 0)
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
            "parameter `initial_configuration` may have 1 and 0 values for pubo models, "
            "found: ",
            id_val.second);
    }
  }
  return state;
}

////////////////////////////////////////////////////////////////////////////////
///
template <class State_T, class Cost_T = double>
class AbstractPubo : public GraphModel<State_T, size_t, Cost_T>
{
 public:
  using Transition_T = size_t;
  using Graph = GraphModel<State_T, size_t, Cost_T>;

  std::string get_identifier() const override { return "pubo"; }
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

  AbstractPubo() { this->graph_.set_allow_dup_merge(true); }
  virtual ~AbstractPubo() {}

  void configure(const utils::Json& json) override { Graph::configure(json); }

  void configure(typename Graph::Configuration_T& configuration)
  {
    Graph::configure(configuration);
  }

  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    return get_random_binary_state<State_T, AbstractPubo<State_T>>(rng, *this);
  }

  State_T get_initial_configuration_state() const override
  {
    return get_initial_configuration_binary_state<State_T, AbstractPubo<State_T>>(*this);
  };

  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    return static_cast<Transition_T>(
        floor(rng.uniform() * static_cast<double>(nodes().size())));
  }

  virtual void apply_transition(const Transition_T& transition,
                                State_T& state) const override = 0;

  utils::Structure render_state(const State_T& state) const override
  {
    return state.render(this->graph_.output_map());
  }

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(nodes().size(), edges().size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(nodes().size());
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
        std::multiset<double> positive_costs;
        std::multiset<double> negative_costs;
        for (auto edge_id : node_i.edge_ids())
        {
          auto edge_i = this->graph_.edge(edge_id);
          double cost = edge_i.cost();

          if (cost > 0)
          {
            positive_costs.insert(cost);
          }
          else if (cost < 0)
          {
            negative_costs.insert(-cost);
          }
        }
        double local_min =
            utils::least_diff_method(positive_costs, negative_costs);
        if (local_min == 0)
        {
          continue;
        }
#ifdef _MSC_VER
        min_diff_local =
            local_min < min_diff_local ? local_min : min_diff_local;
      }
      #pragma omp critical
      min_diff = min_diff_local < min_diff ? min_diff_local : min_diff;
    }
#else
      min_diff = local_min < min_diff ? local_min : min_diff;
    }
#endif

    if (min_diff == std::numeric_limits<double>::max())
    {
      // the only possible that min_diff is not changed, is that all the delta
      // are 0.
      min_diff = (MIN_DELTA_DEFAULT);
    }
    return min_diff;
  }

 protected:
  using Graph::edges;
  using Graph::nodes;
};

class Binary : public ::markov::State
{
 public:
  Binary() {}
  Binary(size_t N, size_t) : spins(N, false) {}
  void copy_state_only(const Binary& other) { spins = other.spins; }

  utils::Structure render() const override;

  utils::Structure render(const std::map<int, int>&) const;

  static size_t memory_estimate(size_t N, size_t)
  {
    return sizeof(Binary) + utils::vector_values_memory_estimate<bool>(N);
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N, 0);
  }

  std::vector<bool> spins;
};

inline bool same_state_value(const Binary& state1, const Binary& state2,
                             size_t transition)
{
  return state1.spins[transition] == state2.spins[transition];
}

class Pubo : public AbstractPubo<Binary>
{
 public:
  double calculate_cost(const State_T& state) const override;

  double calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override;

  void apply_transition(const Transition_T& transition,
                        State_T& state) const override;
};

template <class T>
class BinaryWithCounter : public ::markov::State
{
 public:
  BinaryWithCounter() {}
  BinaryWithCounter(size_t N, size_t M) : spins(N, false), zeros(M, 0) {}
  void copy_state_only(const BinaryWithCounter& other) { spins = other.spins; }

  utils::Structure render() const override
  {
    utils::Structure s(utils::Structure::OBJECT);
    for (size_t i = 0; i < spins.size(); i++)
    {
      s[std::to_string(i)] = spins[i] ? 0 : 1;
    }
    return s;
  }

  utils::Structure render(const std::map<int, int>& ids_map) const
  {
    utils::Structure s(utils::Structure::OBJECT);
    for (size_t i = 0; i < spins.size(); i++)
    {
      s[std::to_string(ids_map.at((int)i))] = spins[i] ? 0 : 1;
    }
    return s;
  }

  static size_t memory_estimate(size_t N, size_t M)
  {
    return sizeof(BinaryWithCounter) +
           utils::vector_values_memory_estimate<bool>(N) +  // spins
           utils::vector_values_memory_estimate<T>(M);      // zeros
  }

  static size_t state_only_memory_estimate(size_t N)
  {
    return memory_estimate(N, 0);
  }

  std::vector<bool> spins;
  std::vector<T> zeros;
};

template <class T>
bool same_state_value(BinaryWithCounter<T>& state1,
                             BinaryWithCounter<T>& state2,
                             size_t transition)
{
  return state1.spins[transition] == state2.spins[transition];
}

template <class T>
class PuboWithCounter : public AbstractPubo<BinaryWithCounter<T>>
{
 public:
  using Base_T = AbstractPubo<BinaryWithCounter<T>>;
  using State_T = BinaryWithCounter<T>;
  using Transition_T = typename Base_T::Transition_T;
  using Graph = typename Base_T::Graph;

  void configure(const utils::Json& json) override { Base_T::configure(json); }

  void configure(typename Base_T::Configuration_T& configuration)
  {
    Base_T::configure(configuration);
  }

  double calculate_cost(const State_T& state) const override
  {
    double cost = 0.0;
    if (state.zeros.empty())
    {
      for (size_t j = 0; j < Graph::edges().size(); j++)
      {
        const auto& e = Graph::edge(j);
        bool zero = false;
        for (size_t i : e.node_ids())
        {
          if (state.spins[i])
          {
            zero = true;
            break;
          }
        }
        if (!zero)
        {
          cost += e.cost();
        }
      }
      return cost;
    }
    for (size_t j = 0; j < edges().size(); j++)
    {
      if (state.zeros[j] == 0)
      {
        cost += edge(j).cost();
      }
    }
    return cost;
  }

  double calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override
  {
    double diff = 0.0;
    const auto& node = Graph::node(transition);
    if (state.spins[transition])
    {
      // spin is currently true, meaning "0". Flipping decreases the zero
      // counter.
      for (auto j : node.edge_ids())
      {
        if (state.zeros[j] == 1)
        {
          // those which currently only have 1 zero remaining will be activated
          diff += edge(j).cost();
        }
      }
    }
    else
    {
      // spin is currently false, meaning "1". Flipping increases the zero
      // counter.
      for (auto j : node.edge_ids())
      {
        if (state.zeros[j] == 0)
        {
          // those which are currently active will be (newly) deactivated
          diff -= edge(j).cost();
        }
      }
    }
    return diff;
  }

  void apply_transition(const Transition_T& transition,
                        State_T& state) const override
  {
    if (state.spins[transition])
    {
      // spin is currently true, meaining "0". Flipping decreases the zero
      // counter
      for (auto j : node(transition).edge_ids())
      {
        state.zeros[j]--;
      }
      state.spins[transition] = false;
    }
    else
    {
      // spin is currently false, meaning "1". Flipping increases the zero
      // counter.
      for (auto j : node(transition).edge_ids())
      {
        state.zeros[j]++;
      }
      state.spins[transition] = true;
    }
  }

 protected:
  using Graph::edge;
  using Graph::edges;
  using Graph::node;
  using Graph::nodes;
};

/// PUBO model state for IsingCompact
class PuboCompactState : public Binary
{
 public:
  PuboCompactState() : Binary() {}

  /// Create an Ising state with N spins and M terms.
  ///
  /// All spins are initialized to 0 ("True").
  /// A true value represents "False".
  ///
  /// As such, the boolean value can be considered as representing the sign.
  PuboCompactState(size_t N, size_t) : Binary(N, N) {}

  // Reader used for accessing the GraphCompactModel
  graph::CompactGraphVisitor reader_;
};

inline bool same_state_value(const PuboCompactState& state1, const PuboCompactState& state2,
                             size_t transition)
{
  return state1.spins[transition] == state2.spins[transition];
}


template <typename Element_T>
class PuboCompact
    : public GraphCompactModel<PuboCompactState, size_t, Element_T>
{
 public:
  using State_T = PuboCompactState;
  using Transition_T = size_t;
  using GraphCompact = GraphCompactModel<State_T, size_t, Element_T, double>;

  PuboCompact() { this->graph_.set_allow_dup_merge(true); }

  std::string get_identifier() const override { return "pubo"; }
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
  double calculate_cost_difference(const State_T& state,
                                   const Transition_T& spin_id) const override
  {
    double diff = 0.0;
    bool var_end = false;
    bool zero = false;
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
        if (state.spins[value])
        {
          zero = true;
        }
      }
      else
      {
        // this is the end of one term/edge
        double coeff = this->graph_.read_next_coeff(reader);

        diff += (state.spins[spin_id] ? coeff : -coeff) * (zero == false);

        // reset term for next one.
        zero = false;
      }
    }
    return diff;
  }
  
  
  /// Return a random Pubo state.
  ///
  /// The number of variables (terms) is determined from the number of
  /// nodes (edges) in the underlying graph.
  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    return get_random_binary_state<State_T, PuboCompact>(rng, *this);
  }

  /// Return a state created from initial configuration.
  State_T get_initial_configuration_state() const override
  {
    return get_initial_configuration_binary_state<State_T, PuboCompact>(*this);
  };

  /// Return a random Single spin update.
  ///
  /// We represent a single spin update as merely the index of the
  /// spin variable that will flip.
  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    return static_cast<Transition_T>(
        rng.uniform() * static_cast<double>(this->graph_.nodes_size()));
  }

  /// Flip the spin specified by spin_id.
  void apply_transition(const Transition_T& transition,
                        State_T& state) const override
  {
    state.spins[transition] = !state.spins[transition];
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

  utils::Structure render_state(const State_T& state) const override
  {
    return state.render(this->graph_.output_map());
  }

  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(this->graph_.nodes_size(),
                                    this->graph_.edges_size());
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(this->graph_.nodes_size());
  }

  double estimate_min_cost_diff() const override
  {
    graph::CompactGraphVisitor reader;
    double min_diff = std::numeric_limits<double>::max();
    for (size_t i = 0; i < this->node_count(); i++)
    {
      std::multiset<double> positive_costs;
      std::multiset<double> negative_costs;
      bool var_end = false;
      while (!var_end)
      {
        uint64_t value = this->graph_.read_next_neig(reader);
        var_end = this->graph_.is_node_end(value);
        bool term_end = this->graph_.is_edge_end(value);
        if (var_end || term_end)
        {
          double cost = this->graph_.read_next_coeff(reader);
          if (cost > 0)
          {
            positive_costs.insert(cost);
          }
          else if (cost < 0)
          {
            negative_costs.insert(-cost);
          }
        }
      }
      double local_min =
          utils::least_diff_method(positive_costs, negative_costs);
      if (local_min == 0)
      {
        continue;
      }
      min_diff = local_min < min_diff ? local_min : min_diff;
    }

    if (min_diff == std::numeric_limits<double>::max())
    {
      // the only possible that min_diff is not changed, is that all the delta
      // are 0.
      min_diff = (MIN_DELTA_DEFAULT);
    }
    return min_diff;
  }

 protected:
  double get_term(const State_T& state, size_t term_id) const
  {
    const auto& edge = this->edge(term_id);
    bool zero = false;
    for (size_t i : edge.node_ids())
    {
      if (state.spins[i])
      {
        zero = true;
        break;
      }
    }
    return zero ? 0. : edge.cost();
  }
};
}  // namespace model
