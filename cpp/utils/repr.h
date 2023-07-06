
#pragma once

#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace std
{
template <class T>
std::ostream& operator<<(std::ostream& ostr, const std::vector<T>& v)
{
  ostr << "[";
  bool first = true;
  for (const auto& e : v)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      ostr << ",";
    }
    ostr << e;
  }
  return ostr << "]";
}

template <class T>
std::ostream& operator<<(std::ostream& ostr, const std::set<T>& v)
{
  ostr << "<";
  bool first = true;
  for (const auto& e : v)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      ostr << ",";
    }
    ostr << e;
  }
  return ostr << ">";
}

template <class K, class V>
std::ostream& operator<<(std::ostream& ostr, const std::map<K, V>& m)
{
  ostr << "{";
  bool first = true;
  for (const auto& e : m)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      ostr << ",";
    }
    ostr << e.first << ":" << e.second;
  }
  return ostr << "}";
}


template <class K, class V>
std::ostream& operator<<(std::ostream& ostr, const std::unordered_map<K, V>& m)
{
  ostr << "{";
  bool first = true;
  for (const auto& e : m)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      ostr << ",";
    }
    ostr << e.first << ":" << e.second;
  }
  return ostr << "}";
}

}  // namespace std

template <class T>
std::string repr(const T& t)
{
  std::stringstream out;
  out << t;
  return out.str();
}

template <>
std::string repr(const std::string& s);
template <>
std::string repr(const char* const& s);
