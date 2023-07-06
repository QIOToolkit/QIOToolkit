
#pragma once
#include <set>
#include <vector>
namespace utils
{
double pop_largest(std::multiset<double>& sorted);

/// Implementation of the least difference method (Karmarkar-Karp)
/// to approximate the least difference among pairs of values
/// from a balanced binary search tree.
double least_diff_method(std::multiset<double>& bbstree);

/// Implementation of the least difference method (Karmarkar-Karp)
/// to approximate the least difference among pairs of values
/// between two balanced binary search tree.
double least_diff_method(std::multiset<double>& bbstree_A,
                         std::multiset<double>& bbstree_B);

#define UNIFORM_LN_BUCKETS_BITS 8
#define UNIFORM_LN_BUCKETS (1 << UNIFORM_LN_BUCKETS_BITS)

extern std::vector<std::vector<double>> uniform_ln;

template<typename T>
inline void hash_combine(size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T1, typename T2>
inline size_t get_combined_hash(const T1& v1, const T2& v2)
{
  std::size_t seed(0);
  hash_combine(seed, v1);
  hash_combine(seed, v2);
  return seed;
}

template <typename T1, typename T2, typename T3>
inline size_t get_combined_hash(const  T1& v1, const T2& v2, const T3& v3)
{
  std::size_t seed(0);
  hash_combine(seed, v1);
  hash_combine(seed, v2);
  hash_combine(seed, v3);
  return seed;
}

}  // namespace utils