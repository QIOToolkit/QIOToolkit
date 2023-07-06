
#include "utils/random_generator.h"

#include <sstream>

namespace utils
{
std::uniform_real_distribution<double> RandomGenerator::uniform_ =
    std::uniform_real_distribution<double>(0.0, 1.0);

std::normal_distribution<> RandomGenerator::gaussian_ =
    std::normal_distribution<>(0.0, 1.0);

std::unique_ptr<RandomGenerator> Twister::fork()
{
  // NOTE: This has a 1/2^32 change to produce the same
  // stream as in another call; we might want to use more
  // bits to initialize it?
  std::unique_ptr<RandomGenerator> forked;
  forked.reset(new Twister(uint32()));
  return forked;
}

void Twister::seed(uint32_t seed) { twister_.seed(seed); }

double Twister::uniform() { return uniform_(twister_); }
double Twister::gaussian() { return gaussian_(twister_); }

uint32_t Twister::uint32() { return twister_(); }

uint32_t Twister::poisson(double mean)
{
  std::poisson_distribution<> poisson(mean);
  return poisson(twister_);
}

std::unique_ptr<RandomGenerator> PCG::fork()
{
  // NOTE: This has a 1/2^32 change to produce the same
  // stream as in another call; we might want to use more
  // bits to initialize it?
  std::unique_ptr<RandomGenerator> forked;
  forked.reset(new PCG(uint32()));
  return forked;
}

void PCG::seed(uint32_t seed) { pcg_.seed(seed); }

double PCG::uniform() { return uniform_(pcg_); }
double PCG::gaussian() { return gaussian_(pcg_); }

uint32_t PCG::uint32() { return pcg_(); }

uint32_t PCG::poisson(double mean)
{
  std::poisson_distribution<> poisson(mean);
  return poisson(pcg_);
}

}  // namespace utils
