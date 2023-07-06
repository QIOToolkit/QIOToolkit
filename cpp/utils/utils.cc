
#include "utils/utils.h"

#include <math.h>

#include <limits>

namespace utils
{
/// Create a lookup table of ln / -B. Hits to the 0th bucket redirect to the
/// next row which zooms in towards the asymptote at 0.
static std::vector<std::vector<double>> init_uniform_ln()
{
  int rows = 4;
  std::vector<std::vector<double>> uniform_ln(rows);
  const size_t steps = UNIFORM_LN_BUCKETS;
  for (int current_row = 0; current_row < rows; current_row++)
  {
    uniform_ln[current_row] = std::vector<double>(UNIFORM_LN_BUCKETS);
    double* current_row_head = &uniform_ln[current_row][0];
    for (size_t i = 0; i < steps; i++)
    {
      // the ln function is sampled in the middle of the bucket. Subsequent rows
      // are sized down to one previous bucket
      current_row_head[i] =
          log((i) / pow((double)steps, (double)(current_row + 1)));
    }
  }
  return uniform_ln;
}

std::vector<std::vector<double>> uniform_ln = init_uniform_ln();

double pop_largest(std::multiset<double>& sorted)
{
  auto it = sorted.end();
  it--;
  double value = *it;
  sorted.erase(it);
  return value;
}

double least_diff_method(std::multiset<double>& bbstree)
{
  while (bbstree.size() > 1)
  {
    double first_value = pop_largest(bbstree);
    double second_value = pop_largest(bbstree);
    double diff = first_value - second_value;
    bbstree.insert(diff);
  }
  return *bbstree.begin();
}

double least_diff_method(std::multiset<double>& bbstree_A,
                         std::multiset<double>& bbstree_B)
{
  double min = std::numeric_limits<double>::max();
  if (bbstree_A.size() == 0 && bbstree_B.size() > 0)
  {
    min = *bbstree_B.begin();
  }
  else if (bbstree_A.size() > 0 && bbstree_B.size() == 0)
  {
    min = *bbstree_A.begin();
  }
  else if (bbstree_A.size() > 0 && bbstree_B.size() > 0)
  {
    while (bbstree_A.size() > 0 && bbstree_B.size() > 0)
    {
      double cur_A = pop_largest(bbstree_A);
      double cur_B = pop_largest(bbstree_B);
      double delta = cur_A - cur_B;
      if (delta > 0)
      {
        bbstree_A.insert(delta);
      }
      else if (delta < 0)
      {
        delta = -delta;
        bbstree_B.insert(delta);
      }
      else
      {
        if (bbstree_A.size() < bbstree_B.size())
        {
          bbstree_A.insert(cur_A);
        }
        else
        {
          bbstree_B.insert(cur_B);
        }
      }
      if (min > delta && delta != 0)
      {
        min = delta;
      }
    }
  }
  return min;
}

}  // namespace utils