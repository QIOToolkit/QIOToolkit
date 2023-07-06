
#pragma once
#include <assert.h>

#include <cstdint>
#include <vector>

namespace utils
{
class BitStream;

/// Class to keep the position information about data to be read/write
/// in BitStream
class BitStreamVisitor
{
 public:
  BitStreamVisitor() { reset(0); }

  // Reset the position to the head of BitStream
  void reset(uint64_t value)
  {
    elem_pos_ = 0;
    unit_pos_ = 0;
    bit_pos_ = 0;
    cur_elem_value_ = value;
  }
  friend class BitStream;

 private:
  // Physical element position of data in elems_
  uint64_t elem_pos_;

  // Logical unit position of data.
  uint64_t unit_pos_;

  // Bit position within one element (uint64_t) in elems_
  uint32_t bit_pos_;

  // Temporary cache the value to be read/write from/to elems_
  // this is to avoid multiple visits of elems_
  uint64_t cur_elem_value_;
};

/// BitStream is used for saving data in a compact format, data are saved in fix
/// sized bits continuouly. To read and write data to and from BitStream, a
/// BitStreamVisitor is required. Currently one sequentially visiting is
/// supported, unless you know how to manually modify position information
/// inside BitStreamVisitor.
class BitStream
{
 public:
  BitStream() : num_units_(0), unit_bits_(0), filled_(false) {}

  // Initialize the space for "num_units" of data, each data element
  // will use "unit_bits" bits
  void init(size_t num_units, uint32_t unit_bits);

  // Read the next data from bitstream, visitor will contain the position of
  // "next" data. and after data is read in the position of visitor will be
  // moved to the next one
  inline uint64_t read_next(BitStreamVisitor& visitor) const;

  // Write the next data to bitstream, visitor will contain the position of
  // "next" data. and after data is written to the position of visitor will be
  // moved to the next one
  inline void write_next(uint64_t value, BitStreamVisitor& visitor);

 private:
  // initialize a visitor for reading
  void init_reader(BitStreamVisitor& visitor) const;

  // Actually write the data to BitStream
  inline void update(BitStreamVisitor& position);

  // Storage to save the data.
  std::vector<uint64_t> elems_;

  // The number of data saved.
  uint64_t num_units_;

  // Number of bits used for each data
  uint32_t unit_bits_;

  // Value mask for read and write data.
  uint64_t value_mask_;

  // Whether data writing has been done.
  bool filled_;
};

inline void BitStream::update(BitStreamVisitor& visitor)
{
  assert(visitor.elem_pos_ < elems_.size());
  elems_[visitor.elem_pos_] = visitor.cur_elem_value_;
  visitor.cur_elem_value_ = 0;
  visitor.elem_pos_++;
}

inline uint64_t BitStream::read_next(BitStreamVisitor& visitor) const
{
  uint64_t value = 0;
  assert(filled_);
  if (visitor.unit_pos_ == 0)
  {
    assert(visitor.elem_pos_ == 0);
    assert(visitor.bit_pos_ == 0);
    visitor.cur_elem_value_ = elems_[0];
  }
  if (visitor.bit_pos_ + unit_bits_ <= 64)
  {
    value = (visitor.cur_elem_value_ >> visitor.bit_pos_) & value_mask_;
    visitor.bit_pos_ += unit_bits_;
    if (visitor.bit_pos_ == 64 && visitor.elem_pos_ + 1 < elems_.size())
    {
      visitor.bit_pos_ = 0;
      visitor.elem_pos_++;
      visitor.cur_elem_value_ = elems_[visitor.elem_pos_];
    }
  }
  else
  {
    value = (visitor.cur_elem_value_ >> visitor.bit_pos_);
    uint32_t read_bits = 64 - visitor.bit_pos_;
    visitor.bit_pos_ = 0;
    visitor.elem_pos_++;
    assert(visitor.elem_pos_ < elems_.size());
    visitor.cur_elem_value_ = elems_[visitor.elem_pos_];
    value |= (visitor.cur_elem_value_ << read_bits) & value_mask_;
    visitor.bit_pos_ = unit_bits_ - read_bits;
  }
  visitor.unit_pos_++;
  if (visitor.unit_pos_ == num_units_)
  {
    visitor.reset(elems_[0]);
  }
  return value;
}

inline void BitStream::write_next(uint64_t value, BitStreamVisitor& visitor)
{
  assert(!filled_);
  assert(value < (1ULL << unit_bits_) || unit_bits_ == 64);

  visitor.cur_elem_value_ |= (value << visitor.bit_pos_);
  if (64 - visitor.bit_pos_ >= unit_bits_)
  {
    visitor.bit_pos_ += unit_bits_;
    if (visitor.bit_pos_ == 64)
    {
      update(visitor);
      visitor.bit_pos_ = 0;
    }
    else if (visitor.unit_pos_ == num_units_ - 1)
    {
      update(visitor);
    }
  }
  else
  {
    uint32_t written_bits = 64 - visitor.bit_pos_;
    uint32_t leftover_bits = unit_bits_ - written_bits;
    assert(leftover_bits > 0);
    update(visitor);

    visitor.cur_elem_value_ |= (value >> written_bits);
    visitor.bit_pos_ = leftover_bits;
    if (visitor.unit_pos_ == num_units_ - 1)
    {
      update(visitor);
    }
  }

  visitor.unit_pos_++;
  filled_ = visitor.unit_pos_ == num_units_;
  if (filled_)
  {
    visitor.reset(elems_[0]);
  }
}
}  // namespace utils
