
#pragma once

#include <cassert>
#include <map>
#include <vector>

#include "utils/operating_system.h"
#include "markov/model.h"
#include "markov/state.h"
#include "markov/transition.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// PolyState
///
/// besides storing the Ising spins, PolyState is also initialized by the model
/// to store intermediate results of the cost function evaluations for each
/// term. This allows efficient evaluation and application of the cost
/// difference a specific update would cause.
///
class PolyState : public ::markov::State
{
 public:
  /// Render the current state of the spins in the model
  /// output format: {"0": +1, "1": -1, ...}
  utils::Structure render() const;

  void copy_state_only(const PolyState& other) { spins = other.spins; }

  /// Values of the ising spins.
  ///
  /// The convention is that the bit denotes the sign (false=+1, true=-1).
  std::vector<bool> spins;

  /// Intermediate value of each term.
  ///
  /// We store as the intermediate value the content of the bracket
  /// expression in recursive terms (\sum_j term_j).
  std::vector<double> terms;

  /// Current values of the parameters of the model.
  std::vector<double> parameters;

  static size_t memory_estimate(size_t n_spins, size_t n_terms,
                                size_t n_parameters)
  {
    return sizeof(PolyState) +
           utils::vector_values_memory_estimate<bool>(n_spins) +
           utils::vector_values_memory_estimate<double>(n_terms) +
           utils::vector_values_memory_estimate<double>(n_parameters);
  }

  static size_t state_only_memory_estimate(size_t n_spins)
  {
    return memory_estimate(n_spins, 0, 0);
  }
};

////////////////////////////////////////////////////////////////////////////////
/// PolyTransition
///
/// We define two types of transitions:
///
/// 1) If spin_id is positive, the transition applies a spin-flip to the
///    spin with this id.
///
/// 2) Otherwise the transition changes the parameter values to the ones
///    denoted in `parameter_values` (all may change concurrently).
class PolyTransition : public ::markov::Transition
{
 public:
  bool is_spin_flip() const { return spin_id_ != -1; }

  size_t spin_id() const { return static_cast<size_t>(spin_id_); }
  void set_spin_id(size_t spin_id) { spin_id_ = static_cast<int>(spin_id); }

  const std::vector<double>& parameter_values() const
  {
    return parameter_values_;
  }
  void set_parameter_values(const std::vector<double>& values)
  {
    spin_id_ = -1;
    parameter_values_ = values;
  }

 private:
  /// The spin to flip (-1 for parameter changes).
  int spin_id_;
  /// The new set of parameters to use.
  std::vector<double> parameter_values_;
};

class PolySpinsConfiguration
{
 public:
  PolySpinsConfiguration() : constant(1.0) {}

  struct Get_Ids
  {
    static std::vector<size_t>& get(PolySpinsConfiguration& config)
    {
      return config.ids;
    }
    static std::string get_key() { return "ids"; }
  };

  struct Get_Constant
  {
    static double& get(PolySpinsConfiguration& config)
    {
      return config.constant;
    }
    static std::string get_key() { return "constant"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorStreamHandler<size_t>, PolySpinsConfiguration, Get_Ids, false,
      utils::ObjectMemberStreamHandler<utils::BasicTypeStreamHandler<double>,
                                      PolySpinsConfiguration, Get_Constant,
                                      false>>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

  double constant;
  std::vector<size_t> ids;
};

class PolyTermConfiguration : public PolySpinsConfiguration
{
 public:
  PolyTermConfiguration() : exponent(1) {}

  struct Get_Terms
  {
    static std::vector<PolySpinsConfiguration>& get(
        PolyTermConfiguration& config)
    {
      return config.terms;
    }
    static std::string get_key() { return "terms"; }
  };

  struct Get_Exponent
  {
    static int& get(PolyTermConfiguration& config) { return config.exponent; }
    static std::string get_key() { return "exponent"; }
  };

  struct Get_Parameter
  {
    static std::string& get(PolyTermConfiguration& config)
    {
      return config.parameter;
    }
    static std::string get_key() { return "parameter"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<PolySpinsConfiguration::StreamHandler>,
      PolyTermConfiguration, Get_Terms, false,
      utils::ObjectMemberStreamHandler<
          utils::BasicTypeStreamHandler<int>, PolyTermConfiguration,
          Get_Exponent, false,
          utils::ObjectMemberStreamHandler<
              utils::BasicTypeStreamHandler<std::string>, PolyTermConfiguration,
              Get_Parameter, false,
              PolySpinsConfiguration::MembersStreamHandler>>>;

  using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;

  std::string parameter;
  int exponent;
  std::vector<PolySpinsConfiguration> terms;
};

class PolyConfiguration : public model::BaseModelConfiguration
{
 public:
  struct Get_Terms
  {
    static std::vector<PolyTermConfiguration>& get(PolyConfiguration& config)
    {
      return config.terms;
    }
    static std::string get_key() { return "terms"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<PolyTermConfiguration::StreamHandler>,
      PolyConfiguration, Get_Terms, true,
      BaseModelConfiguration::MembersStreamHandler>;

  using StreamHandler = ModelStreamHandler<PolyConfiguration>;

  std::vector<PolyTermConfiguration> terms;
};

////////////////////////////////////////////////////////////////////////////////
/// Poly Model
///
/// Defines an Ising model with polynomial cost function. The cost function can
/// be defined via recursive nesting of terms of the form:
///
///   term_i = c_i p_i (\sum_j term_j) ^ e_i
///
///     - or -
///
///   term_i = c_i p_i \prod_j s_j
///
/// where
///
///   c_i  is a numeric constant
///   p_i  is a parameter (which can be changed during the simulation)
///   e_i  is an integer exponent
///   s_i  is an Ising spin
///
/// NOTE: The limitation to Ising spins and products only in non-leaf of such
/// spins is deliberate (this avoids needing to track the number of zeros in
/// products).
///
/// See `Poly::configure()` for the input format definition.
///
class Poly : public ::markov::Model<PolyState, PolyTransition>
{
 public:
  // Interface for external model configuration functions.
  using Configuration_T = PolyConfiguration;

  std::string get_identifier() const override { return "poly"; }
  std::string get_version() const override { return "0.1"; }

  double get_const_cost() const override
  {
    // for now assume that Poly model does not have const term.
    return 0;
  }

  bool is_empty() const override { return term_.empty(); }
  //////////////////////////////////////////////////////////////////////////////
  /// Representation of a recursive term in the polynomial
  ///
  /// A term takes the form
  ///
  ///   constant * parameter (sum_j term_j) ^ exponent
  ///
  ///     - or -
  ///
  ///   constant * parameter (prod_j spin_j)
  ///
  /// The term stores its own properties (constant, parameter_id, exponent)
  /// and the id of its parent term. It does not track a list of its child
  /// terms or spins, since we typically evaluate the cost function bottom-up.
  /// (top-down traversel, as needed by `term_to_string` is inefficient).
  ///
  /// NOTE: For terms of the second kind, there is no exponent.
  ///
  struct Term
  {
    /// Initialize a default term
    Term(int parent_id_ = -1)
        : is_leaf(true),
          parent_id(parent_id_),
          constant(1.0),
          parameter_id(-1),
          exponent(1)
    {
    }

    bool is_leaf_node() const { return is_leaf; }
    bool has_constant() const { return constant != 1; }
    bool has_parameter() const { return parameter_id != -1; }

    /// Whether this is a term of the second kind.
    bool is_leaf;
    /// The parent term this term belongs to
    /// (-1 for the root term)
    int parent_id;
    /// The constant prefactor of this term
    double constant;
    /// The id (index) of the parameter for this term
    /// (-1 if none).
    int parameter_id;
    /// The exponent of the sum of terms in this parameter
    /// (ignored for spins terms at the leaves).
    int exponent;
  };

  /// Create a state with specific spin and parameter values.
  ///
  /// Parameters:
  ///   `spins`  array of initial spin values {-1, 1}
  ///   `parameters`  array of initial parameter values
  ///                 (defaults to all-1).
  ///
  /// Returns:
  ///   An initialized state with intermediate term values populated.
  PolyState create_state(std::vector<int> spins,
                         std::vector<double> parameters = {}) const;

  /// Create a random state
  ///
  /// This fill the state with random spin values {-1, +1} and all
  /// parameters set to 1.0.
  PolyState get_random_state(utils::RandomGenerator& rng) const override;

  /// Create a specific spin flip
  ///
  /// This returns a transition which spins flip `spin_id`.
  PolyTransition create_spin_flip(size_t spin_id) const;

  /// Create a parameter change
  ///
  /// This returns a transition which substitutes the model parameters.
  PolyTransition create_parameter_change(
      const std::vector<double> values) const;

  /// Return a random spin flip
  ///
  /// The default markov transition for Poly is a random spin flip.
  /// @see `create_parameter_change` to manually create a parameter transition.
  PolyTransition get_random_transition(
      const PolyState& state, utils::RandomGenerator& rng) const override;

  /// Get the full cost function.
  ///
  /// We can entirely rely on the root term's intermediate value.
  double calculate_cost(const PolyState& state) const override;

  /// Calculate the cost difference for a transition.
  double calculate_cost_difference(
      const PolyState& state, const PolyTransition& transition) const override;

  /// Apply a transition to a state.
  void apply_transition(const PolyTransition& transition,
                        PolyState& state) const override;

  size_t get_sweep_size() const override { return spin_to_terms_.size(); }

  void configure(const utils::Json& json) override;

  void configure(Configuration_T& configuration);

  std::string print() const;

  std::vector<std::string> get_parameters() const;

  double get_spin_overlap(const PolyState& s, const PolyState& t) const
  {
    assert(s.spins.size() == t.spins.size());
    double agree = 0;
    for (size_t i = 0; i < s.spins.size(); i++)
    {
      agree += (s.spins[i] == t.spins[i]) ? 1 : -1;
    }
    return std::abs(agree / static_cast<double>(s.spins.size()));
  }

  double get_term_overlap(const PolyState& s, const PolyState& t) const
  {
    assert(s.terms.size() == term_.size());
    assert(t.terms.size() == term_.size());
    double leaf_count = 0;
    double agree = 0;
    for (size_t j = 0; j < term_.size(); j++)
    {
      if (term_[j].is_leaf)
      {
        agree += (s.terms[j] == t.terms[j]) ? 1 : -1;
        leaf_count++;
      }
    }
    return std::abs(agree / leaf_count);
  }

  size_t state_memory_estimate() const override
  {
    return PolyState::memory_estimate(spin_to_terms_.size(), term_.size(),
                                      parameter_to_terms_.size());
  }

  size_t state_only_memory_estimate() const override
  {
    return PolyState::state_only_memory_estimate(spin_to_terms_.size());
  }

 private:
  /// Read a term from the input configuration.
  void configure_term(const utils::Json& json, int parent = -1);

  void configure_term(PolySpinsConfiguration& input, int parent = -1);

  void configure_term(PolyTermConfiguration& input, int parent = -1);

  void configure_term(std::vector<PolyTermConfiguration>&, int parent = -1);

  /// Initialize a state
  ///
  /// This takes a state with `spins` and `parameters` set and populates its
  /// intermediate evaluation results in `terms`. This is the only time we
  /// evaluate all of the terms. All subsequent calls to `calculate_cost(state)`
  /// and `calculate_cost_difference(state, transition)` rely on these `term`s.
  void initialize_terms(PolyState& state) const;

  /// Calculate difference for each term
  ///
  /// This calculates all the nodes affected by a transition and bubbles
  /// those changes up to the root node of the poly tree.
  std::map<size_t, double> calculate_term_differences(
      const PolyState& state, const PolyTransition& transition) const;

  /// Return the factor preceding term `term_id`
  ///
  /// This computes the product `constant * parameter` where the parameter
  /// is optional and taken from the set of parameters passed.
  double term_factor(const std::vector<double> parameters,
                     size_t term_id) const;

  /// Render a term's string representation (recursively)
  std::string term_to_string(size_t term_id) const;

  std::vector<Term> term_;
  std::vector<std::string> parameter_names_;
  std::vector<std::vector<size_t>> parameter_to_terms_;
  std::vector<std::vector<size_t>> spin_to_terms_;
};
REGISTER_MODEL(Poly);

}  // namespace model
