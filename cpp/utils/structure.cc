// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/structure.h"

#include <math.h>

#include <iomanip>

namespace utils
{
std::string Structure::to_string(bool pretty) const
{
  return to_string(pretty, 0);
}

void Structure::remove(const std::string& key)
{
  if (!has_key(key))
  {
    THROW(KeyDoesNotExistException, "key:", key, ", does not exist!");
  }
  object_->erase(key);
}

// The following is a poor man's backport of std::quoted
// for compilers before C++14
#if __cplusplus < 201402L || defined(__clang__)
}  // namespace utils

namespace std
{
string quoted(const string& s)
{
  string quoted = "\"";
  size_t i = 0, j = 0;
  while (i < s.size())
  {
    j = s.find_first_of("\\\"", i);
    if (j == std::string::npos) break;
    quoted += s.substr(i, j - i) + "\\" + s[j];
    i = j + 1;
  }
  quoted += s.substr(i, s.size() - i) + "\"";
  return quoted;
}

}  // namespace std

namespace utils
{
#endif

bool is_digits(std::string s)
{
  if (s.empty()) return false;
  auto first_non_digit =
      std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); });
  return first_non_digit == s.end();
}

std::string Structure::to_string(bool pretty, int level) const
{
  std::string indent((size_t)(2 * level), ' ');
  std::stringstream out;
  bool simple = is_simple();
  switch (type_)
  {
    case UNKNOWN_TYPE:
      out << "?";
      break;
    case BOOL:
      out << (bool_ ? "true" : "false");
      break;
    case INT32:
      out << int32_;
      break;
    case UINT32:
      out << uint32_;
      break;
    case INT64:
      out << int64_;
      break;
    case UINT64:
      out << uint64_;
      break;
    case DOUBLE:
      out << (isnan(double_) ? "nan" : std::to_string(double_));
      break;
    case STRING:
      if (string_ != nullptr)
      {
        out << std::quoted(*string_);
      }
      else
      {
        out << std::quoted(std::string());
      }
      break;
    case ARRAY:
      if (array_ == nullptr)
      {
        out << "[]";
        break;
      }
      if (simple || !pretty)
      {
        out << "[";
        for (size_t i = 0; i < array_->size(); i++)
        {
          out << (*array_)[i].to_string(false, level + 1);
          if (i < array_->size() - 1) out << ",";
        }
        out << "]";
      }
      else
      {
        out << "[\n";
        for (size_t i = 0; i < array_->size(); i++)
        {
          out << indent << "  " << (*array_)[i].to_string(pretty, level + 1);
          if (i < array_->size() - 1) out << ",";
          out << "\n";
        }
        out << indent << "]";
      }
      break;
    case OBJECT:
    {
      // objects members need to be printed in a specific order
      // 1) for objects with numeric keys represented as strings,
      //    they should be ordered numerically
      // 2) customers expect a specific order of elements in some
      //    situations. The only known occurrence of this is currently
      //    that "version" needs to be the leading element, so we
      //    print it first.
      if (object_ == nullptr)
      {
        out << "{}";
        break;
      }
      using Iterator = decltype(object_->begin());
      std::vector<Iterator> items;
      items.reserve(object_->size());
      Iterator it = object_->find("version");
      if (it != object_->end()) items.push_back(it);
      bool all_numeric = true;
      for (auto it = object_->begin(); it != object_->end(); it++)
      {
        if (all_numeric) all_numeric = is_digits(it->first);
        if (it->first != "version") items.push_back(it);
      }
      if (all_numeric)
      {
        // Sort strings representing numeric keys numerically.
        // NOTE: this deliberatly does not cover negative numbers
        // (or floats) at this point. This type of information
        // really should not be stored in keys.
        std::sort(items.begin(), items.end(),
                  [](const Iterator& a, const Iterator& b) -> bool {
                    if (a->first.size() != b->first.size())
                      return a->first.size() < b->first.size();
                    return a->first < b->first;
                  });
      }

      if (simple || !pretty)
      {
        out << "{";
        size_t i = 0;
        for (const auto& item : items)
        {
          out << std::quoted(item->first) << ": "
              << item->second.to_string(false, level + 1);
          if (i < object_->size() - 1) out << ", ";
          i++;
        }
        out << "}";
      }
      else
      {
        out << "{\n";
        size_t i = 0;
        for (const auto& item : items)
        {
          out << indent << "  " << std::quoted(item->first) << ": "
              << item->second.to_string(pretty, level + 1);
          if (i < object_->size() - 1) out << ",";
          out << "\n";
          i++;
        }
        out << indent << "}";
      }
      break;
    }
  }
  return out.str();
}

void Structure::deep_copy(const Structure& other)
{
  assert(this != (&other));
  type_ = other.type_;
  if (type_ == STRING)
  {
    assert(string_ == nullptr);
    if (other.string_ != nullptr)
    {
      string_ = new std::string(*other.string_);
    }
  }
  else if (type_ == ARRAY)
  {
    assert(array_ == nullptr);
    if (other.array_ != nullptr)
    {
      array_ = new std::vector<Structure>(*other.array_);
    }
  }
  else if (type_ == OBJECT)
  {
    assert(object_ == nullptr);
    if (other.object_ != nullptr)
    {
      object_ = new std::map<std::string, Structure>(*other.object_);
    }
  }
  else
  {
    uint64_ = other.uint64_;
  }
}

Structure::Structure(const Structure& other) : Structure() { deep_copy(other); }

Structure& Structure::operator=(const Structure& other)
{
  if (this != (&other))
  {
    clear();
    deep_copy(other);
  }

  return *this;
}

bool Structure::operator<(const Structure& rhs) const
{
  if (type_ != rhs.type_) return type_ < rhs.type_;
  switch (type_)
  {
    case UNKNOWN_TYPE:
      return false;
    case BOOL:
      return bool_ < rhs.bool_;
    case INT32:
      return int32_ < rhs.int32_;
    case UINT32:
      return uint32_ < rhs.uint32_;
    case INT64:
      return int64_ < rhs.int64_;
    case UINT64:
      return uint64_ < rhs.uint64_;
    case DOUBLE:
      return double_ < rhs.double_;
    case STRING:
      return *string_ < *(rhs.string_);
    case ARRAY:
      return *array_ < *(rhs.array_);
    case OBJECT:
      return *object_ < *(rhs.object_);
  }
  return false;
}

std::string Structure::type2string(Type type)
{
  switch (type)
  {
    case UNKNOWN_TYPE:
      return "unset";
    case BOOL:
      return "bool";
    case INT32:
      return "int32";
    case UINT32:
      return "uint32";
    case INT64:
      return "int64";
    case UINT64:
      return "uint64";
    case DOUBLE:
      return "double";
    case STRING:
      return "string";
    case ARRAY:
      return "array";
    case OBJECT:
      return "object";
  }
  return "unset";
}

template <>
bool Structure::get<bool>() const
{
  if (type_ != BOOL)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as bool (actual type=", type2string(type_),
          ")");
  }
  return bool_;
}

template <>
int32_t Structure::get<int32_t>() const
{
  if (type_ != INT32)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as int32 (actual type=",
          type2string(type_), ")");
  }
  return int32_;
}

template <>
uint32_t Structure::get<uint32_t>() const
{
  if (type_ != UINT32)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as uint32 (actual type=",
          type2string(type_), ")");
  }
  return uint32_;
}

template <>
int64_t Structure::get<int64_t>() const
{
  if (type_ != INT64)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as int64 (actual type=",
          type2string(type_), ")");
  }
  return int64_;
}

template <>
uint64_t Structure::get<uint64_t>() const
{
  if (type_ != UINT64)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as uint64 (actual type=",
          type2string(type_), ")");
  }
  return uint64_;
}

template <>
double Structure::get<double>() const
{
  if (type_ != DOUBLE)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as double (actual type=",
          type2string(type_), ")");
  }
  return double_;
}

template <>
std::string Structure::get<std::string>() const
{
  if (type_ != STRING)
  {
    THROW(utils::AccessAsIncorrectTypeException,
          "Accessing structure field as string (actual type=",
          type2string(type_), ")");
  }
  if (string_ == nullptr)
  {
    return std::string();
  }
  return *string_;
}

std::ostream& operator<<(std::ostream& out, const Structure& s)
{
  return out << s.to_string(false);
}

}  // namespace utils
