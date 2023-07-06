
#include "utils/bit_stream.h"

#include "utils/error_handling.h"
#include "utils/random_generator.h"
#include "utils/timing.h"
#include "gtest/gtest.h"

using ::utils::BitStream;
using ::utils::BitStreamVisitor;
using ::utils::Twister;

void run_internal(uint32_t start, uint32_t range, uint32_t bits)
{
  unsigned int seed = utils::get_seed_time();
  Twister rng(seed);
  ASSERT_LE(bits, 63);
  uint64_t module_value = (1ULL) << bits;
  uint64_t value_base = (((uint64_t)(rng.uint32())) << 32) + rng.uint32();
  value_base = value_base % module_value;

  // Random genarate elems between 100 and 200
  uint32_t units = start + (uint32_t)(rng.uniform() * range);
  BitStream stream;
  stream.init(units, bits);
  BitStreamVisitor writer;
  for (uint64_t i = 0; i < units; i++)
  {
    stream.write_next((i + value_base) % module_value, writer);
  }

  BitStreamVisitor reader;
  for (int j = 0; j < 2; j++)
  {
    for (uint64_t i = 0; i < units; i++)
    {
      uint64_t value = stream.read_next(reader);
      EXPECT_EQ((i + value_base) % module_value, value);
    }
  }
}

TEST(BitStream, Elem8Bits) { run_internal(100, 100, 8); }

TEST(BitStream, Elem7Bits) { run_internal(88, 388, 7); }

TEST(BitStream, Elem3Bits) { run_internal(888, 188, 3); }

TEST(BitStream, Elem12Bits) { run_internal(381, 239, 12); }

TEST(BitStream, Elem17Bits) { run_internal(38, 58, 3); }

TEST(BitStream, Elem28Bits) { run_internal(138, 40, 28); }

TEST(BitStream, Elem37Bits) { run_internal(38, 58, 37); }

TEST(BitStream, Elem45Bits) { run_internal(118, 98, 45); }

TEST(BitStream, Elem52Bits) { run_internal(118, 100, 52); }

TEST(BitStream, Elem63Bits) { run_internal(158, 100, 63); }
