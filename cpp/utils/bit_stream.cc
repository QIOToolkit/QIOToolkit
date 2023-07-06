
#include "utils/bit_stream.h"

#include <assert.h>

namespace utils
{
void BitStream::init(std::size_t num_units, uint32_t unit_bits)
{
  assert(!filled_);
  assert(unit_bits > 0);
  assert(unit_bits <= 64);
  unit_bits_ = unit_bits;
  num_units_ = num_units;
  size_t elem_count = (num_units * unit_bits + 63) / 64;
  elems_.resize(elem_count);
  value_mask_ = ((1ULL << unit_bits_) - 1);
}

void BitStream::init_reader(BitStreamVisitor& visitor) const
{
  assert(filled_);
  visitor.reset(elems_[0]);
}
}  // namespace utils
