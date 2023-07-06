
#include "utils/log.h"

#include "utils/operating_system.h"

#if defined(__GNUG__)
#include <cxxabi.h>

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace utils
{
std::string Logger::Context::demangle(const char* name)
{
  int status = -1;
  std::unique_ptr<char, void (*)(void*)> res{
      abi::__cxa_demangle(name, NULL, NULL, &status), std::free};
  if (status == 0)
  {
    std::string s = res.get();
    // remove space before " >" in template class
    while (s.find(" >") != std::string::npos)
    {
      s.replace(s.find(" >"), 2, ">");
    }
    return s;
  }
  return name;
}

#elif defined(_MSC_VER)

namespace utils
{
std::string Logger::Context::demangle(const char* name)
{
  std::string s(name);
  // remove class keyword
  while (s.find("class ") != std::string::npos)
  {
    s.replace(s.find("class "), 6, "");
  }
  // remove space before " >" in template class
  while (s.find(" >") != std::string::npos)
  {
    s.replace(s.find(" >"), 2, ">");
  }

  return s;
}

#else

namespace utils
{
std::string Logger::Context::demangle(const char* name) { return name; }

#endif

std::string str_level(LogLevel level)
{
  switch (level)
  {
    case ALL:
      return "ALL";
    case INFO:
      return "INFO";
    case WARN:
      return "WARN";
    case ERROR:
      return "ERROR";
    case FATAL:
      return "FATAL";
    case OFF:
      return "OFF";
  }
  return "UNKNOWN";
}

std::string Logger::color(LogLevel level)
{
  if (!Logger::use_color()) return "";
  switch (level)
  {
    case ALL:
      return "\033[0m";
    case INFO:
      return "\033[0;34m";
    case WARN:
      return "\033[0;33m";
    case ERROR:
      return "\033[0;31m";
    case FATAL:
      return "\033[0;31m";
    case OFF:
      return "\033[0m";
  }
  return "\033[0m";
}

std::string Logger::context_color()
{
  return Logger::use_color() ? "\033[0;37m" : "";
}

std::string Logger::normal_color()
{
  return Logger::use_color() ? "\033[0m" : "";
}

std::string make_repo_path(const std::string& path)
{
  std::string repo_path_start =
      path_separator() + std::string("cpp") + path_separator();
  auto pos = path.rfind(repo_path_start);
  if (pos != std::string::npos)
  {
    return path.substr(pos + repo_path_start.length());
  }
  else
  {
    return path;
  }
}

std::string remove_dotdot(std::string path)
{
  std::string dotdot = path_separator() + std::string("..") + path_separator();
  for (;;)
  {
    auto pos = path.find(dotdot);
    if (pos == std::string::npos) break;
    size_t i = pos;
    do
    {
      i--;
    } while (i > 0 && path[i] != path_separator());
    path = path.substr(0, i) + path.substr(pos + 3);
  }
  return path;
}

std::string Logger::get_prefix(LogLevel level, const Context& context)
{
  auto file = remove_dotdot(make_repo_path(context.get_file()));
  std::stringstream s;
  s << "[" << color(level) << str_level(level) << normal_color() << "] "
    << context_color() << file << ":" << context.get_line() << normal_color()
    << ": ";
  return s.str();
}

std::string Logger::get_debug_prefix(const Context& context)
{
  auto file = remove_dotdot(make_repo_path(context.get_file()));
  std::stringstream s;
  std::string debug_color = "\033[0;35m";
  s << "[" << debug_color << "DEBUG" << normal_color() << "] "
    << context_color() << file << ":" << context.get_line() << normal_color()
    << ": ";
  return s.str();
}

bool Logger::should_log(LogLevel level, const Context&)
{
  return level >= level_;
}

std::string Logger::to_string(LogLevel level)
{
  switch (level)
  {
    case ALL:
      return "ALL";
    case INFO:
      return "INFO";
    case WARN:
      return "WARN";
    case ERROR:
      return "ERROR";
    case FATAL:
      return "FATAL";
    case OFF:
      return "OFF";
  }
  return "";
}

bool Logger::set_level(std::string str_level)
{
  for (int i = ALL; i <= OFF; i++)
  {
    LogLevel level = static_cast<LogLevel>(i);
    if (to_string(level) == str_level)
    {
      set_level(level);
      return true;
    }
  }
  auto before = level_;
  level_ = WARN;
  LOG(WARN, "Unknown log level ", str_level);
  level_ = before;
  return false;
}

// Adjust default logging level according to compile flags
// (Disable debug output when NDEBUG is given).
bool Logger::is_default_level_ = true;
#ifdef NDEBUG
LogLevel Logger::level_ = WARN;
#else
LogLevel Logger::level_ = INFO;
#endif

bool Logger::color_ = true;
std::ostream* Logger::stream_ = &std::cerr;

std::vector<std::pair<std::string, size_t>> MemoryUsageLog::checkpoint_usage_;

void MemoryUsageLog::clear() { checkpoint_usage_.clear(); }

void MemoryUsageLog::add(const std::string& checkpoint)
{
  MemoryUsageLog::checkpoint_usage_.push_back(
      std::pair<std::string, size_t>(checkpoint, get_memory_usage()));
}

const std::vector<std::pair<std::string, size_t>>& MemoryUsageLog::get()
{
  return MemoryUsageLog::checkpoint_usage_;
}

}  // namespace utils
