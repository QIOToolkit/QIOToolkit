
#pragma once

#include <string>

namespace utils
{
// include OS specific functions

/// Process current memory usage in bytes
size_t get_memory_usage();

/// Process peak memory usage in bytes
size_t get_max_memory_usage();

/// Available memory in bytes
size_t get_available_memory();

/// File size in bytes
size_t get_file_size(const std::string& filename, bool& success);

void enable_overflow_divbyzero_exceptions();

void disable_overflow_divbyzero_exceptions();

/// Functions for estimation of memory consumption of
/// values of vector of simple type
template <class Type>
inline size_t vector_values_memory_estimate(size_t size)
{
  size_t data_bytes = sizeof(Type) * size;
  return data_bytes;
}

template <>
inline size_t vector_values_memory_estimate<bool>(size_t size)
{
  // bool vector is implementead as bit field
  const size_t block_size = 4;
  size_t data_bytes = (size + 7) / 8;
  size_t num_blocks = (data_bytes + block_size - 1) / block_size;

  size_t memory_estimation = block_size * num_blocks;
  return memory_estimation;
}

/// Helper function to print byte sizes in a human readable form.
std::string print_bytes(size_t bytes);

/// File path separator: \ or /
char path_separator();

/// Helper function to check if the input problem is a file or folder.
/// Used to differentiate protobuf inputs that come in a folder
bool isFolder(std::string problem_input);

}  // namespace utils
