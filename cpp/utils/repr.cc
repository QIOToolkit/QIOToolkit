
#include "utils/repr.h"

template <>
std::string repr(const std::string& s)
{
  std::stringstream out;
  out << "'" << s << "'";
  return out.str();
}

template <>
std::string repr(const char* const& s)
{
  std::stringstream out;
  out << "'" << s << "'";
  return out.str();
}
