
#include "utils/metadata.h"

#include <limits.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>  // for _getcwd
#include <winsock.h>
#define HOSTNAME "COMPUTERNAME"
#define USER "USERNAME"
#endif

#ifdef __linux__
#include <unistd.h>
#define HOSTNAME "HOSTNAME"
#define USER "USER"
#endif

#include "generated/git_version.h"

#define BUFFER_SIZE 256

namespace utils
{
std::string get_hostname()
{
  char* host;
  host = getenv(HOSTNAME);
  if (host) return host;
#ifdef __linux__
  char buffer[BUFFER_SIZE];
  if (gethostname(buffer, BUFFER_SIZE) == 0)
  {
    return buffer;
  }
#endif
  return "?";
}

std::string get_username()
{
  char* user;
  user = getenv(USER);
  if (user) return user;
#ifdef __linux__
  char buffer[BUFFER_SIZE];
  getlogin_r(user, BUFFER_SIZE);
  return buffer;
#else
  return "?";
#endif
}

std::string get_current_directory()
{
  char buffer[BUFFER_SIZE];
#ifdef __linux__
  if (getcwd(buffer, BUFFER_SIZE) == nullptr)
  {
    LOG(WARN,
        "Unable to detect current directory with getcwd(): ", strerror(errno));
  }
  else
  {
    return buffer;
  }
#endif
#ifdef _WIN32
  if (_getcwd(buffer, BUFFER_SIZE) == nullptr)
  {
    LOG(WARN,
        "Unable to detect current directory with _getcwd(): ", strerror(errno));
  }
  else
  {
    return buffer;
  }
#endif
  return "?";
}

#undef HOSTNAME
#undef USER
#undef BUFFER_SIZE

const std::string get_current_datetime()
{
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
  return buf;
}

static std::string startup_time = get_current_datetime();

const std::string& get_startup_datetime() { return startup_time; }

std::string get_qiotoolkit_version() { return qiotoolkit_VERSION; }

std::string get_git_branch() { return GIT_BRANCH; }

std::string get_git_commit_hash() { return GIT_COMMIT_HASH; }

std::string get_cmake_build_type()
{
  std::string build_type = CMAKE_BUILD_TYPE;
  if (!build_type.empty()) return build_type;
#ifdef NDEBUG
  return "Release";
#else
  return "Debug";
#endif
}

std::string get_compiler()
{
  std::stringstream version;
#if defined(_MSC_VER)
  version << "MSVC " << _MSC_VER;
#elif defined(__INTEL_COMPILER)
  version << "icc " << __INTEL_COMPILER;
#elif defined(__MINGW32__)
  version << "mingw32 " << __MINGW32_MAJOR_VERSION << "."
          << __MINGW32_MINOR_VERSION;
#elif defined(__MINGW64__)
  version << "mingw64 " << __MINGW64_VERSION_MAJOR << "."
          << __MINGW64_VERSION_MINOR;
#elif defined(__clang__)
  version << "clang " << __clang_major__ << "." << __clang_minor__ << "."
          << __clang_patchlevel__;
#elif defined(__GNUC__)
  version << "g++ " << __GNUC__ << "." << __GNUC_MINOR__;
#else
  version << "(unknown)";
#endif
  return version.str();
}

utils::Structure get_build_properties()
{
  utils::Structure s;
  s["branch"] = get_git_branch();
  s["commit_hash"] = get_git_commit_hash();
  s["compiler"] = get_compiler();
  s["build_type"] = get_cmake_build_type();
  s["qiotoolkit_version"] = get_qiotoolkit_version();
  return s;
}

utils::Structure get_invocation_properties()
{
  utils::Structure s;
  s["host"] = get_hostname();
  s["user"] = get_username();
  s["datetime"] = get_startup_datetime();
  s["directory"] = get_current_directory();
  return s;
}

}  // namespace utils
