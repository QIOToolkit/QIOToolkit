
#pragma once

#include "utils/stream_handler.h"
#include "utils/structure.h"
#include "markov/model.h"
#include "markov/state.h"

template <typename V>
class ValueState : public ::markov::State
{
 public:
  V value;
  ValueState() : ValueState(0) {}
  ValueState(V v) { value = v; }

  utils::Structure render() const override
  {
    utils::Structure s;
    s = value;
    return s;
  }

  bool operator==(const ValueState& state) const { return value == state.value; }

  void copy_state_only(const ValueState<V>& other) { value = other.value; }

  static size_t memory_estimate() { return sizeof(ValueState<V>); }

  static size_t state_only_memory_estimate() { return memory_estimate(); }
};

template <typename V>
inline bool same_state_value(const ValueState<V>& state1, const ValueState<V>& state2,
                      const ValueState<V>&)
{
  return state1.value == state2.value;
}

class TestModelConfiguration : public model::BaseModelConfiguration
{
 public:
  struct Get_Model
  {
    static TestModelConfiguration& get(TestModelConfiguration& m) { return m; }
    static std::string get_key() { return utils::kCostFunction; }
  };

  using MembersStreamHandler = BaseModelConfiguration::MembersStreamHandler;

  using StreamHandler =
      utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
          utils::ObjectStreamHandler<MembersStreamHandler>,
          TestModelConfiguration, Get_Model, true>>;
};

class TestModel : public ::markov::Model<ValueState<double>, ValueState<double>>
{
 public:
  // Interface for external model configuration functions.
  using Configuration_T = TestModelConfiguration;

  std::string get_identifier() const override { return "toy"; }
  std::string get_version() const override { return "1.0"; }
  static double kInitValue;
  static double kBaseRandom;
  static double kBaseDiff;
  static double kBaseUniform;

  double calculate_cost(const ValueState<double>& state) const override;

  double calculate_cost_difference(
      const ValueState<double>& state,
      const ValueState<double>& transition) const override;

  ValueState<double> get_random_state(
      utils::RandomGenerator& rng) const override;

  ValueState<double> get_random_transition(
      const ValueState<double>& state,
      utils::RandomGenerator& rng) const override;

  void apply_transition(const ValueState<double>& transition,
                        ValueState<double>& state) const override;

  size_t get_sweep_size() const override;

  void configure(const utils::Json&) override
  {
    // do nothing, but implementing this silences the warning.
  }

  void configure(Configuration_T& configuration)
  {
    ::markov::Model<ValueState<double>, ValueState<double>>::configure(
        configuration);
  }

  size_t state_memory_estimate() const override
  {
    return ValueState<double>::memory_estimate();
  }

  size_t state_only_memory_estimate() const override
  {
    return ValueState<double>::state_only_memory_estimate();
  }

  double get_const_cost() const override { return 0; }

  bool is_empty() const override { return false; }
};

template <typename V>
struct std::hash<ValueState<V>>
{
  std::size_t operator()(const ValueState<V>& trans) const noexcept
  {
    std::hash<V> hasher; 
    return hasher(trans.value);
  }
};
