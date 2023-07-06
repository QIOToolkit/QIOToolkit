
#include "utils/random_selector.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "utils/exception.h"

namespace utils
{
RandomSelector::RandomSelector() { reset(); }

void RandomSelector::reset()
{
  ids_ = {};
  weight_ = {0, 0};
  total_weight_ = 0;
}

double RandomSelector::get_weight(size_t position) const
{
  return get_cumulative_weight(position + 1) - get_cumulative_weight(position);
}

// up to but not including position
double RandomSelector::get_cumulative_weight(size_t position) const
{
  size_t p = 0;
  double w = 0;
  for (size_t k = weight_.size() - 1; k > 0; k >>= 1)
  {
    if (position & k)
    {
      p |= k;
      w += weight_[p];
    }
  }
  return w;
}

void RandomSelector::add_weight(size_t position, double weight)
{
  for (size_t k = 1; k < weight_.size(); k <<= 1)
  {
    if (position & k)
    {
      position ^= k;
    }
    else
    {
      weight_[position + k] += weight;
    }
  }
}

void RandomSelector::insert(size_t id, double weight)
{
  size_t position = ids_.size();
  ids_.push_back(id);
  total_weight_ += weight;

  // Resize weight_ if necessary
  if (position + 1 >= weight_.size())
  {
    double total = weight_.back();
    weight_.resize(2 * weight_.size() - 1);
    weight_.back() = total;
  }

  add_weight(position, weight);
}

size_t RandomSelector::select_position(double uniform) const
{
  assert(uniform >= 0);
  assert(uniform < 1);

  if (ids_.size() == 0)
    throw IndexOutOfRangeException("Random selector is empty.");

  if (ids_.size() == 1) return 0;

  double weight = uniform * total_weight_;
  size_t position = 0;
  for (size_t k = weight_.size() - 1; k > 0; k >>= 1)
  {
    if (weight >= weight_[position + k])
    {
      position += k;
      weight -= weight_[position];
    }
  }
  if (position >= ids_.size())
  {
    position = ids_.size() - 1;
  }
  return position;
}

void RandomSelector::remove_position(size_t position)
{
  ids_[position] = ids_.back();
  ids_.pop_back();

  double before = get_weight(position);
  double after = get_weight(ids_.size());
  total_weight_ -= before;
  add_weight(position, after - before);
  add_weight(ids_.size(), -after);
}

size_t RandomSelector::select(double uniform) const
{
  size_t pos = select_position(uniform);
  return ids_[pos];
}

size_t RandomSelector::select_and_remove(double uniform)
{
  size_t pos = select_position(uniform);
  size_t id = ids_[pos];
  remove_position(pos);
  return id;
}

}  // namespace utils
