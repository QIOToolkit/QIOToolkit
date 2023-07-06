
#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <random>

#include "pcg_random.hpp"

namespace utils
{
////////////////////////////////////////////////////////////////////////////////
/// RandomGenerator
///
/// This is a thin wrapper around the STL mersenne twister which provides
/// the convenience function for the `uniform` = rand \in [0,1) and
/// `poisson(double mean)`
class RandomGenerator
{
 public:
  typedef uint32_t result_type;
  static result_type max() { return std::numeric_limits<result_type>::max(); }
  static result_type min() { return std::numeric_limits<result_type>::min(); }

  RandomGenerator() {}
  virtual ~RandomGenerator() {}

  uint32_t operator()() { return this->uint32(); }

  /// Allow seeding from signed integer
  void seed(int32_t seed) { this->seed(static_cast<uint32_t>(seed)); }

  virtual std::unique_ptr<RandomGenerator> fork() = 0;

  /// Set the seed to the random_number_generator.
  virtual void seed(uint32_t seed) = 0;

  /// Return a random 32bit integer.
  virtual uint32_t uint32() = 0;

  /// Return a uniformly distributed double in [0,1).
  virtual double uniform() = 0;

  /// Return a gaussian distributed double
  virtual double gaussian() = 0;

  /// Return a poisson distributed integer with mean `mean`.
  virtual uint32_t poisson(double mean) = 0;

  /// Return estimation of memory in bytes
  virtual size_t memory_estimate() const = 0;

 protected:
  static std::uniform_real_distribution<double> uniform_;
  static std::normal_distribution<> gaussian_;
};

class Twister : public RandomGenerator
{
 public:
  Twister() {}
  Twister(uint32_t seed) { this->seed(seed); }

  void seed(uint32_t) override;
  std::unique_ptr<RandomGenerator> fork() override;

  uint32_t uint32() override;
  double uniform() override;
  double gaussian() override;
  uint32_t poisson(double mean) override;
  size_t memory_estimate() const override { return sizeof(Twister); }

 private:
  /// Mersenne Twister from the STL.
  std::mt19937 twister_;
};

class PCG : public RandomGenerator
{
 public:
  PCG() {}
  PCG(uint32_t seed) { this->seed(seed); }

  void seed(uint32_t) override;
  std::unique_ptr<RandomGenerator> fork() override;

  uint32_t uint32() override;
  double uniform() override;
  double gaussian() override;
  uint32_t poisson(double mean) override;
  size_t memory_estimate() const override { return sizeof(PCG); }

 private:
  pcg32 pcg_;
};

}  // namespace utils
