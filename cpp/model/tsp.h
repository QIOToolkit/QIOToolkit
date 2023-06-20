// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include "utils/exception.h"
#include "utils/random_generator.h"
#include "utils/utils.h"
#include "markov/model.h"
#include "model/permutation.h"

namespace model
{
class TspTransition : public ::markov::Transition
{
 public:
  enum Type
  {
    MOVE_NODE = 0,
    SWAP_NODES = 1,
    SWAP_EDGES = 2,
  };

  TspTransition() : type_(Type::MOVE_NODE), a_(0), b_(0) {}

  TspTransition(Type type, size_t a, size_t b) : type_(type), a_(a), b_(b) {}

  static TspTransition random(size_t N, utils::RandomGenerator& rng);

  void apply(Permutation& permutation) const;

  std::string get_class_name() const override;

  inline Type type() const { return type_; }
  inline size_t a() const { return a_; }
  inline size_t b() const { return b_; }
  
  bool operator==(const TspTransition& trans) const
  {
    return type_ == trans.type_ && a_ == trans.a_ && b_ == trans.b_;
  }

 private:
  Type type_;
  size_t a_;
  size_t b_;
};

inline bool same_state_value(const Permutation& p1, const Permutation& p2,
                      const TspTransition& t)
{
  return p1[t.a()] == p2[t.a()];
}

class TspConfiguration : public model::BaseModelConfiguration
{
 public:
  struct Get_Model
  {
    static TspConfiguration& get(TspConfiguration& config) { return config; }
    static std::string get_key() { return utils::kCostFunction; }
  };

  struct Get_Dist
  {
    static std::vector<std::vector<double>>& get(TspConfiguration& config)
    {
      return config.dist_;
    }
    static std::string get_key() { return "disks"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::VectorObjectStreamHandler<utils::VectorStreamHandler<double>>,
      TspConfiguration, Get_Dist, true,
      BaseModelConfiguration::MembersStreamHandler>;

  using StreamHandler =
      utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
          utils::ObjectStreamHandler<MembersStreamHandler>, TspConfiguration,
          Get_Model, true>>;

 protected:
  std::vector<std::vector<double>> dist_;
};

class Tsp : public ::markov::Model<Permutation, TspTransition>
{
 public:
  // Interface for external model configuration functions.
  using Configuration_T = TspConfiguration;

  std::string get_identifier() const override { return "tsp"; }
  std::string get_version() const override { return "1.0"; }

  Tsp() {}

  double calculate_cost(const Permutation& p) const override;

  double calculate_cost_difference(const Permutation& p,
                                   const TspTransition& t) const override;

  Permutation get_random_state(utils::RandomGenerator& rng) const override;

  TspTransition get_random_transition(
      const Permutation& p, utils::RandomGenerator& rng) const override;

  void apply_transition(const TspTransition& transition,
                        Permutation& p) const override;

  void configure(const utils::Json& json) override;

  void configure(Configuration_T& config);

  std::string get_class_name() const override { return "Tsp"; }

  double get_const_cost() const override
  {
    // for now assume that tsp model does not have const term.
    return 0;
  }

  bool is_empty() const override { return dist_.empty(); }

  size_t state_memory_estimate() const override
  {
    return Permutation::memory_estimate(dist_.size());
  }

  size_t state_only_memory_estimate() const override
  {
    return Permutation::state_only_memory_estimate(dist_.size());
  }

 private:
  std::vector<std::vector<double>> dist_;
};
REGISTER_MODEL(Tsp);

}  // namespace model


template <>
struct std::hash<model::TspTransition>
{
  std::size_t operator()(const model::TspTransition& trans) const noexcept
  {
    return utils::get_combined_hash((int)trans.type(), trans.a(), trans.b());
  }
};
