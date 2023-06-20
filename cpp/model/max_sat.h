// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <assert.h>

#include <set>

#include "utils/dimacs.h"
#include "utils/operating_system.h"
#include "utils/utils.h"
#include "markov/model.h"
#include "matcher/matchers.h"

namespace model
{
/// Representation of a MaxSat state.
///
/// We store the variable values and a counter for each clause.
template <typename Counter_T = uint32_t>
class MaxSatState
{
 public:
  /// Default constructor for containers.
  MaxSatState() {}

  /// Create a MaxSatState with `nvar` variables and `ncl` clauses.
  ///
  /// > [!NOTE]
  /// > MaxSat will always create an extra clause at the begining such
  /// > that the actual clauses can be accessed with 1-based indices.
  MaxSatState(size_t nvar, size_t ncl) : variables(nvar), clause_counters(ncl)
  {
  }

  /// Copy only the state from another MaxSatState.
  void copy_state_only(const MaxSatState& other)
  {
    variables = other.variables;
  }

  static size_t memory_estimate(size_t variables, size_t clauses)
  {
    return utils::vector_values_memory_estimate<Counter_T>(clauses) +
           utils::vector_values_memory_estimate<bool>(variables);
  }

  static size_t state_only_memory_estimate(size_t variables)
  {
    return memory_estimate(variables, 0);
  }

  /// Render the state by dumping the variable values.
  ///
  /// > [!NOTE]
  /// > To get correct labelling, use the MaxSat::render_state(...) method.
  utils::Structure render() const { return variables; }

  std::vector<bool> variables;
  std::vector<Counter_T> clause_counters;
};

template <typename Counter_T>
inline bool same_state_value(const MaxSatState<Counter_T>& state1,
                      const MaxSatState<Counter_T>& state2,
                      const size_t& transition)
{
  return state1.variables[transition] == state2.variables[transition];
}

///  MaxSat stream configuration
class MaxSatConfiguration : public model::BaseModelConfiguration
{
 public:
  struct Get_Terms
  {
    static std::vector<utils::Dimacs::Clause>& get(MaxSatConfiguration& config)
    {
      return config.terms;
    }
    static std::string get_key() { return "terms"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<utils::Dimacs::Clause::StreamHandler>,
      MaxSatConfiguration, Get_Terms, true,
      BaseModelConfiguration::MembersStreamHandler>;

  using StreamHandler = ModelStreamHandler<MaxSatConfiguration>;

  std::vector<utils::Dimacs::Clause> terms;
};

/// MaxSatModel
///
/// Natively simulates weighted satisfiability problems.
template <typename Counter_T = uint32_t>
class MaxSat : public markov::Model<MaxSatState<Counter_T>, size_t>
{
 public:
  MaxSat() : max_weight_(0), max_vars_in_clause_(0) {}

  using Base_T = markov::Model<MaxSatState<Counter_T>, size_t>;
  using State_T = MaxSatState<Counter_T>;
  using Cost_T = double;
  // Interface for external model configuration functions.
  using Configuration_T = MaxSatConfiguration;

  std::string get_identifier() const override { return "maxsat"; }
  std::string get_version() const override { return "1.0"; }

  /// Create an all-false state with the correct number of variables
  /// and clause-counters for this model.
  State_T create_state() const
  {
    State_T state(affected_.size(), weights_.size());
    for (size_t i = 0; i < affected_.size(); i++)
    {
      for (int j : affected_[i])
        if (j < 0) state.clause_counters[-j]++;
    }
    return state;
  }

  /// Create a state with specific initial variable values.
  State_T create_state(std::vector<bool> variables) const
  {
    auto state = create_state();
    if (variables.size() != affected_.size())
    {
      THROW(utils::ValueException,
            "Wrong number of variables to initialize: got ", variables.size(),
            " but the model has ", affected_.size(), " active variable",
            (affected_.size() != 1) ? "s." : ".");
    }
    for (size_t i = 0; i < variables.size(); i++)
    {
      if (i >= affected_.size()) break;
      if (variables[i]) apply_transition(i, state);
    }
    return state;
  }

  /// Create a state with random initial variable values.
  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    auto state = create_state();
    for (size_t i = 0; i < affected_.size(); i++)
      if (rng.uniform() < 0.5) apply_transition(i, state);
    return state;
  }

  /// Pick a random transition
  size_t get_random_transition(const State_T&,
                               utils::RandomGenerator& rng) const override
  {
    return floor(rng.uniform() * affected_.size());
  }

  /// Calculate the cost (sum of active weights) for state.
  ///
  /// The cost for (weighted) MaxSat is defined as the sum of the weights
  /// of the UNSATISIFIED clauses.
  Cost_T calculate_cost(const State_T& state) const override
  {
    assert(weights_.size() == state.clause_counters.size());
    Cost_T cost = 0;
    for (size_t clause_id = 1; clause_id < weights_.size(); clause_id++)
      if (state.clause_counters[clause_id] == 0) cost += weights_[clause_id];
    return cost;
  }

  /// Calculate the change in cost if `transition` were applied.
  Cost_T calculate_cost_difference(const State_T& state,
                                   const size_t& transition) const override
  {
    Cost_T diff = 0;
    if (state.variables[transition])
    {
      // variable is currently 'true' and switching to 'false'
      for (int clause_id : affected_[transition])
      {
        if (clause_id > 0)  // variable is non-negated in clause_id.
        {
          // The current count for this clause is 1 and switching the
          // non-negated variable `clause_id` from 'true' to 'false' reduces
          // the count to 0, thereby making the overall clause value 'false'
          // (or "inactive"). Since our cost is defined as the sum of weights
          // of inactive clauses, the change adds the weight of this newly
          // inactive clause.
          if (state.clause_counters[clause_id] == 1)
            diff += weights_[clause_id];
        }
        else  // variable appears negated in clause_id.
        {
          // The current count for this clause is 0 and switching the variable
          // `-clause_id` from 'true' to 'false' means its negated value changes
          // 'false' -> 'true'. This increases the count to 1, thereby making
          // the overall clause value 'true' (or "active"). Since our cost is
          // defined as the sum of weights of inactive clauses, the change
          // removes the weight of this newly active clause.
          if (state.clause_counters[-clause_id] == 0)
            diff -= weights_[-clause_id];
        }
      }
    }
    else
    {
      // variable is currently 'false' and switching to 'true'
      for (int clause_id : affected_[transition])
      {
        if (clause_id > 0)  // variable is non-negated in clause_id.
        {
          // The current count for this clause is 0 and switching the
          // non-negated variable `clause_id` from 'false' to 'true' increases
          // the count to 1, thereby making the overall clause value 'true' (or
          // "active"). Since our cost is defined as the sum of weights of
          // inactive clauses, the change removes the weight of this newly
          // active clause.
          if (state.clause_counters[clause_id] == 0)
            diff -= weights_[clause_id];
        }
        else  // variable appears negated in clause id.
        {
          // The current count for this clause is 1 and switching the variable
          // `-clause_id` from 'false' to 'true' means its negated value changes
          // 'true' -> 'false'. This decreases the count to 0, thereby making
          // the overall clause value 'false' (or "inactive"). Since our cost is
          // defined as the sum of weights of inactive clauses, the change
          // adds the weight of this newly inactive clause.
          if (state.clause_counters[-clause_id] == 1)
            diff += weights_[-clause_id];
        }
      }
    }
    return diff;
  }

  /// Apply the effects of `transition` to the variable and counters in state.
  void apply_transition(const size_t& transition, State_T& state) const override
  {
    bool value = state.variables[transition];
    if (value)
    {
      for (int clause_id : affected_[transition])
        if (clause_id > 0)  // variable is non-negated in clause_id.
          state.clause_counters[clause_id]--;
        else  // variable appears negated in clause_id.
          state.clause_counters[-clause_id]++;
    }
    else
    {
      for (int clause_id : affected_[transition])
        if (clause_id > 0)  // variable is non-negated in clause_id.
          state.clause_counters[clause_id]++;
        else  // variable appears negated in clause_id.
          state.clause_counters[-clause_id]--;
    }
    state.variables[transition] = !value;
  }

  /// Read a max-sat problem from json.
  void configure(const utils::Json& json) override
  {
    using matcher::IsEmpty;
    using matcher::Not;
    std::vector<utils::Dimacs::Clause> clauses;
    this->param(json["cost_function"], "terms", clauses)
        .matches(Not(IsEmpty()))
        .required();
    configure(clauses);
  }

  /// Configure using stream configuration.
  void configure(Configuration_T& config)
  {
    Base_T::configure(config);
    for (auto& it : config.terms)
    {
      it.check_variable_names();
    }
    configure(config.terms);
  }

  /// Read a max-sat problem from dimacs.
  void configure(const utils::Dimacs& dimacs)
  {
    configure(dimacs.get_clauses());
  }

  /// Turn a list of clauses into adj-list representation for simulation.
  ///
  /// This uses the position in variables as the variable_id and makes
  /// clauses 1-indexed instead (with a negation in the adj-list denoting
  /// negated participation in a clause).
  void configure(const std::vector<utils::Dimacs::Clause>& clauses)
  {
    // Figure out which clauses are always true (we need not simulate them)
    // and the rest (which we call 'active').
    //
    // Make a list of variables which are
    //
    //   - used in at least one active clause (we need to simulate it)
    //   - used only in clauses which are always true ('free' variables)
    //
    size_t active_clauses = 0;
    std::vector<bool> always_true(clauses.size());
    std::set<int> used_variables;
    std::set<int> potentially_free_variables;
    for (size_t i = 0; i < clauses.size(); i++)
    {
      const auto& clause = clauses[i];
      std::set<int> seen;
      for (const auto& variable : clause.variables)
      {
        if (variable == 0 || variable == std::numeric_limits<int>::min())
        {
          THROW(utils::ValueException, "MaxSat variables cannot have the name ",
                variable, " since it cannot be negated to indicate 'not ",
                variable, "'.");
        }
        if (seen.find(-variable) != seen.end())
        {
          always_true[i] = true;
        }
        seen.insert(variable);
      }
      if (always_true[i])
      {
        for (const auto& variable : seen)
        {
          potentially_free_variables.insert(std::abs(variable));
        }
      }
      else
      {
        active_clauses++;
        for (const auto& variable : seen)
        {
          used_variables.insert(std::abs(variable));
        }
      }
    }
    free_variables_.clear();
    for (int var : potentially_free_variables)
    {
      if (used_variables.find(var) == used_variables.end())
      {
        free_variables_.push_back(var);
      }
    }
    potentially_free_variables.clear();
    variable_names_.resize(used_variables.size());
    std::copy(used_variables.begin(), used_variables.end(),
              variable_names_.begin());
    used_variables.clear();

    weights_.clear();
    affected_.clear();
    if (variable_names_.empty() || active_clauses == 0)
    {
      // The model is "empty": it has only clauses which are always true
      // and all its variable values are free.
      return;
    }

    // Build a map translating each (positive) variable name to its
    // variable_id.
    std::map<int, int> name_to_id;
    for (size_t id = 0; id < variable_names_.size(); id++)
    {
      name_to_id[variable_names_[id]] = id;
    }

    // Clauses (and their weights) use 1-based indexing.
    weights_.resize(active_clauses + 1);
    affected_.resize(variable_names_.size());
    max_weight_ = std::numeric_limits<Cost_T>::min();
    max_vars_in_clause_ = 0;
    size_t clause_id = 1;
    for (size_t i = 0; i < clauses.size(); i++)
    {
      if (always_true[i]) continue;
      const auto& clause = clauses[i];
      weights_[clause_id] = clause.weight;
      max_weight_ = std::max(max_weight_, clause.weight);
      std::set<int> seen;
      for (int var_name : clause.variables)
      {
        int var_id = name_to_id[std::abs(var_name)];
        if (seen.find(std::abs(var_id)) != seen.end()) continue;
        seen.insert(std::abs(var_id));
        affected_[var_id].push_back(var_name < 0 ? -clause_id : clause_id);
      }
      max_vars_in_clause_ = std::max(max_vars_in_clause_, seen.size());
      clause_id++;
    }
  }

  /// Take the configuration from another model.
  /// (this allows changing the counter type after configuration)
  void configure(model::BaseModel* base) override
  {
    MaxSat<uint32_t>* maxsat32 = dynamic_cast<MaxSat<uint32_t>*>(base);
    // Refer to BaseModel to throw exception if the cast was not successful.
    if (maxsat32 == nullptr) ::model::BaseModel::configure(base);

    max_weight_ = maxsat32->max_weight_;
    max_vars_in_clause_ = maxsat32->max_vars_in_clause_;
    std::swap(weights_, maxsat32->weights_);
    std::swap(affected_, maxsat32->affected_);
    std::swap(variable_names_, maxsat32->variable_names_);
    std::swap(free_variables_, maxsat32->free_variables_);
  }

  /// Render a state with the original variable names.
  ///
  /// Example: [1, 2, 3, -4, -5, 6] mean there were six variables in the
  /// original input with consecutive names 1..6 and the solution found has
  /// variables 4 and 5 negated.
  virtual utils::Structure render_state(const State_T& state) const override
  {
    if (state.variables.size() != variable_names_.size())
    {
      THROW(utils::IndexOutOfRangeException,
            "Number of variables in the state does not correspond to the "
            "number of variables in the model: ",
            state.variables.size(), "!=", variable_names_.size());
    }
    utils::Structure rendered(utils::Structure::OBJECT);
    for (size_t i = 0; i < variable_names_.size(); i++)
    {
      auto name = variable_names_[i];
      auto name_str = std::to_string(name);
      assert(!rendered.has_key(name_str));
      rendered[name_str] = state.variables[i] ? 1 : 0;
    }
    for (auto name : free_variables_)
    {
      auto name_str = std::to_string(name);
      assert(!rendered.has_key(name_str));
      rendered[name_str] = 1;
    }

    return rendered;
  }

  bool is_empty() const override { return variable_names_.empty(); }
  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(affected_.size(), get_term_count());
  }
  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(affected_.size());
  }
  size_t get_term_count() const override { return weights_.size() - 1; }
  Cost_T get_max_weight() const { return max_weight_; }
  size_t get_max_vars_in_clause() const { return max_vars_in_clause_; }

  // Estimate max diff caused by one variable flip
  double estimate_max_cost_diff() const override
  {
    Cost_T max_weight_diff_var = 0;
#ifdef _MSC_VER
    // reduction min or max is not implemented yet in all compilers
    #pragma omp parallel
    {
      double max_weight_diff_var_local = 0;
      #pragma omp for
#else
    #pragma omp parallel for reduction(max : max_weight_diff_var)
#endif
      for (size_t var_id = 0; var_id < affected_.size(); var_id++)
      {
        Cost_T negative_sum = 0;
        Cost_T positive_sum = 0;
        for (auto clause_id : affected_[var_id])
        {
          assert(clause_id != 0);
          if (clause_id > 0)
          {
            positive_sum += weights_[clause_id];
          }
          else
          {
            negative_sum += weights_[-clause_id];
          }
        }
        assert(positive_sum >= 0);
        assert(negative_sum >= 0);

        // it is impossible to have cost change both from positive clauses and
        // negative clauses affected by one variable flip, so we estimate the
        // local max diff by max of positive_sum and negative_sum
        Cost_T local_max =
            positive_sum > negative_sum ? positive_sum : negative_sum;
        assert(local_max > 0);
#ifdef _MSC_VER
        max_weight_diff_var_local = local_max > max_weight_diff_var_local
                                        ? local_max
                                        : max_weight_diff_var_local;
      }
      #pragma omp critical
      max_weight_diff_var = max_weight_diff_var_local > max_weight_diff_var
                                ? max_weight_diff_var_local
                                : max_weight_diff_var;
    }
#else
      max_weight_diff_var =
          local_max > max_weight_diff_var ? local_max : max_weight_diff_var;
    }
#endif
    assert(max_weight_diff_var > 0);
    return (double)max_weight_diff_var;
  }

  // the min cost difference is similar from PUBO mode's estimate_min_cost_diff
  double estimate_min_cost_diff() const override
  {
    Cost_T min_weight_diff_var = std::numeric_limits<double>::max();
#ifdef _MSC_VER
    // reduction min or max is not implemented yet in all compilers
    #pragma omp parallel
    {
      double min_weight_diff_var_local = std::numeric_limits<double>::max();
      #pragma omp for
#else
    #pragma omp parallel for reduction(min : min_weight_diff_var)
#endif
      for (size_t var_id = 0; var_id < affected_.size(); var_id++)
      {
        std::multiset<Cost_T> positive_costs;
        std::multiset<Cost_T> negative_costs;
        for (auto clause_id : affected_[var_id])
        {
          assert(clause_id != 0);
          if (clause_id > 0)
          {
            positive_costs.insert(weights_[clause_id]);
          }
          else
          {
            negative_costs.insert(weights_[-clause_id]);
          }
        }

        Cost_T min_cost_elem = std::numeric_limits<double>::max();
        if (positive_costs.size() > 0)
        {
          min_cost_elem = *positive_costs.begin();
        }

        if (negative_costs.size() > 0)
        {
          min_cost_elem = std::min(min_cost_elem, *negative_costs.begin());
        }

        double local_min =
            utils::least_diff_method(positive_costs, negative_costs);
        if (local_min == std::numeric_limits<double>::max())
        {
          // in case the estimated diff is not valid, use the minimal cost.
          local_min = min_cost_elem;
        }
#ifdef _MSC_VER
        min_weight_diff_var_local = local_min < min_weight_diff_var_local
                                        ? local_min
                                        : min_weight_diff_var_local;
      }
      #pragma omp critical
      min_weight_diff_var = min_weight_diff_var_local < min_weight_diff_var
                                ? min_weight_diff_var_local
                                : min_weight_diff_var;
    }
#else
      min_weight_diff_var =
          local_min < min_weight_diff_var ? local_min : min_weight_diff_var;
    }
#endif
    assert(min_weight_diff_var != std::numeric_limits<double>::max());
    return (double)min_weight_diff_var;
  }

 private:
  friend class MaxSat<uint8_t>;
  friend class MaxSat<uint16_t>;

  Cost_T max_weight_;
  size_t max_vars_in_clause_;
  // clauses (and their weights) are 1-indexed
  std::vector<Cost_T> weights_;
  // a negative number indicates that the variable
  // appears negated in the respective clause.
  std::vector<std::vector<int>> affected_;
  // A list of what the variables were originally called.
  std::vector<int> variable_names_;
  // List of variable names in the input that are free.
  std::vector<int> free_variables_;
};

using MaxSat8 = MaxSat<uint8_t>;
using MaxSat16 = MaxSat<uint16_t>;
using MaxSat32 = MaxSat<uint32_t>;

}  // namespace model
