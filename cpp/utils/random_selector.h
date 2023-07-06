
#pragma once

#include <stdint.h>

#include <vector>

namespace utils
{
class RandomSelector
{
 public:
  RandomSelector();

  void reset();

  // Insert identifier `id` with relative probability `weight`
  // into the selector. After insertion, a call to select_and_remove
  // will return `id` with probability `weigth/total_weight`,
  // assuming `id` was added only once.
  void insert(size_t id, double weight);

  // Expects a uniform \in [0,1) and returns a random `id`
  // with probability `weight[id]/total_weight` of all those
  // which were previously added. The entry remains in the
  // selection.
  size_t select(double uniform) const;

  // Expects a uniform \in [0,1) and returns a random `id`
  // with probability `weight[id]/total_weight` of all those
  // which were previously added. The entry is subsequently
  // removed from the selector (future calls will not return
  // this entry).
  size_t select_and_remove(double uniform);

  double get_weight(size_t position) const;
  double get_cumulative_weight(size_t position) const;

 private:
  size_t select_position(double uniform) const;
  void remove_position(size_t position);
  void add_weight(size_t position, double weight);

  std::vector<size_t> ids_;
  std::vector<double> weight_;
  double total_weight_;
};

}  // namespace utils
