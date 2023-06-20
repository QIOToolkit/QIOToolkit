// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/operating_system.h"

#include <fenv.h>

#include <sstream>
// include OS specific functions

#ifdef __unix__

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>

namespace utils
{
size_t get_max_memory_usage()
{
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return ru.ru_maxrss * 1024;
}

size_t get_memory_usage()
{
  // Values from /proc/self/statm was close to peak memory usage,
  // so max memory usage is used for less confusion until better
  // library functions are available.
  return get_max_memory_usage();
}

size_t get_available_memory()
{
  struct sysinfo system_info;
  sysinfo(&system_info);
  return system_info.freeram;
}

void enable_overflow_divbyzero_exceptions()
{
  feenableexcept(FE_OVERFLOW | FE_DIVBYZERO);
}

void disable_overflow_divbyzero_exceptions()
{
  fedisableexcept(FE_OVERFLOW | FE_DIVBYZERO);
}

char path_separator() { return '/'; }

bool isFolder(std::string input_problem)
{
  struct stat st_buf;
  if (stat(input_problem.c_str(), &st_buf) == 0)
    return S_ISDIR(st_buf.st_mode);
  else
    return false;  // file or dir does not exist
}

#endif

#ifdef _WIN32

// Windows appears to require these to be be included in a specific order.
// clang-format off
#include <windows.h>
#include <psapi.h>
#include <filesystem>
// clang-format on

namespace utils
{
size_t get_max_memory_usage()
{
  PROCESS_MEMORY_COUNTERS_EX pmc;
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc,
                       sizeof(pmc));
  return pmc.PeakWorkingSetSize;
}

size_t get_memory_usage()
{
  PROCESS_MEMORY_COUNTERS_EX pmc;
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc,
                       sizeof(pmc));
  return pmc.WorkingSetSize;
}

size_t get_available_memory()
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  return statex.ullAvailPhys;
}

void enable_overflow_divbyzero_exceptions()
{
  unsigned int control_word;
  _controlfp_s(&control_word, 0, 0);
  _clearfp();
  _controlfp_s(0, control_word & ~(_EM_ZERODIVIDE | _EM_OVERFLOW), _MCW_EM);
}

void disable_overflow_divbyzero_exceptions()
{
  unsigned int control_word;
  _controlfp_s(&control_word, 0, 0);
  _clearfp();
  _controlfp_s(0, control_word | _EM_ZERODIVIDE | _EM_OVERFLOW, _MCW_EM);
}

char path_separator() { return '\\'; }

bool isFolder(std::string input_problem)
{
  struct stat stats;
  if (stat(input_problem.c_str(), &stats) == 0)
    return S_IFDIR & stats.st_mode;
  else
    return false;  // file or dir does not exist
}

#endif

size_t get_file_size(const std::string& filename, bool& success)
{
  success = true;

#if defined(_MSC_VER)
  struct __stat64 buffer;
  if (_stat64(filename.c_str(), &buffer) == 0) return buffer.st_size;
#else
  struct stat buffer;
  if (stat(filename.c_str(), &buffer) == 0) return buffer.st_size;
#endif

  success = false;
  return 0;
}

std::string print_bytes(size_t bytes)
{
  std::string suffix = "B";
  if (bytes > 8 * 1024 * 1024)
  {
    suffix = "MB";
    bytes /= 1024 * 1024;
  }
  else if (bytes > 8 * 1024)
  {
    suffix = "kB";
    bytes /= 1024;
  }
  std::stringstream s;
  s << bytes << suffix;
  return s.str();
}
}  // namespace utils
