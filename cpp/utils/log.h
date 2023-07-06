
#pragma once

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "utils/concat.h"
#include "utils/repr.h"

#define __CLASS__ ::utils::Logger::Context::demangle(typeid(*this).name())

#ifdef __GNUG__
#define __FUNCTION_SIGNATURE__ __PRETTY_FUNCTION__
#define __METHOD__ (std::string(__CLASS__) + "::" + __FUNCTION__);
#else
#define __FUNCTION_SIGNATURE__ __FUNCSIG__
#define __METHOD__ __FUNCTION__;
#endif

#define LOG_CONTEXT \
  ::utils::Logger::Context(__FILE__, __LINE__, __FUNCTION_SIGNATURE__)

#define LOG(LEVEL, ...) ::utils::Logger::log(LEVEL, LOG_CONTEXT, __VA_ARGS__)

#define DEBUG(...) ::utils::Logger::debug(LOG_CONTEXT, __VA_ARGS__)

enum LogLevel
{
  ALL,
  INFO,
  WARN,
  ERROR,
  FATAL,
  OFF
};

namespace utils
{
////////////////////////////////////////////////////////////////////////////////
/// Logging facilities
///
/// Apart from adjusting logging attributes (level, color, output stream), users
/// should use the LOG(LEVEL, ...) macro to produce log output. The macro takes
/// care of including the source context of where information was logged, while
/// passing the content to `Logger::log`.
class Logger
{
 public:
  /// Store the (code) context of where a LOG was invoked (file, line,
  /// function).
  class Context
  {
   public:
    Context(const std::string& file, int line, const std::string& function)
        : line_(line), file_(file), function_(function)
    {
    }

    /// Translate gcc mangled function name to human-readable form.
    static std::string demangle(const char* name);

    /// Return the source line where the LOG was made.
    int get_line() const { return line_; }

    /// Return the source file path where the LOG was made.
    const std::string& get_file() const { return file_; }

    /// Return the function or method within which the LOG was made.
    const std::string& get_function() const { return function_; }

   private:
    int line_;
    std::string file_, function_;
  };

  /// Log information to the `Logger`
  template <class... Args>
  static void log(LogLevel level, Context context, const Args&... args)
  {
    if (should_log(level, context))
    {
      (*stream_) << get_prefix(level, context);
      print_recursive(*stream_, args...);
      (*stream_) << std::endl;
    }
  }

  template <class... Args>
  static void debug(Context context, Args... args)
  {
    (*stream_) << get_debug_prefix(context);
    print_recursive(*stream_, args...);
    (*stream_) << std::endl;
  }

  /// Set the global log level (minimum level for logs to appear)
  static void set_level(LogLevel level)
  {
    Logger::level_ = level;
    is_default_level_ = false;
  }

  /// Whether the level is still the default value for that build
  /// (used to decide whether to override from input).
  static bool is_default_level() { return is_default_level_; }

  /// Set the global log level from string (minimum level for logs to appear)
  static bool set_level(std::string str_level);

  /// Get the currently set log level
  static LogLevel get_level() { return Logger::level_; }

  /// Turn a log level into its string representation.
  static std::string to_string(LogLevel level);

  /// Set whether logging should use ansi colors
  static void set_color(bool color) { Logger::color_ = color; }

  /// Set where logging should be streamed to (default=std::cerr)
  static void set_stream(std::ostream* stream) { Logger::stream_ = stream; }

  static std::ostream& get_stream() { return *Logger::stream_; }

  static bool use_color() { return color_; }

  static std::string color(LogLevel level);

  static std::string context_color();

  static std::string normal_color();

 private:
  /// Return the prefix that should be at the beginning of each log.
  static std::string get_prefix(LogLevel level, const Context& context);

  /// Return the prefix that should be at the beginning of each debug output.
  static std::string get_debug_prefix(const Context& context);

  /// Decide whether a given call should pass to the output.
  static bool should_log(LogLevel level, const Context& _);

  static bool is_default_level_;
  static LogLevel level_;
  static bool color_;
  static std::ostream* stream_;

 protected:
};

class MemoryUsageLog
{
 public:
  static void clear();
  static void add(const std::string& checkpoint);
  static const std::vector<std::pair<std::string, size_t>>& get();

 private:
  static std::vector<std::pair<std::string, size_t>> checkpoint_usage_;
};

#ifdef qiotoolkit_PROFILING
#define LOG_MEMORY_USAGE(check_point) utils::MemoryUsageLog::add(check_point)
#define CLEAR_LOG_MEMORY_USAGE utils::MemoryUsageLog::clear()
#else
#define LOG_MEMORY_USAGE(check_point)
#define CLEAR_LOG_MEMORY_USAGE
#endif

}  // namespace utils
