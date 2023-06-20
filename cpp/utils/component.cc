// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/component.h"

#include <cmath>

#include "utils/exception.h"
#include "utils/log.h"

namespace utils
{
void Component::configure(const utils::Json&)
{
  throw NotImplementedException(get_class_name() +
                                "::configure() is not implemented");
}

utils::Structure Component::render() const
{
  throw NotImplementedException(get_class_name() +
                                "::render() is not implemented");
}

utils::Structure Component::get_status() const { return this->render(); }

std::string Component::get_class_name() const { return __CLASS__; }

void set_from_json(utils::Component& target, const utils::Json& json)
{
  target.configure(json);
}

std::ostream& operator<<(std::ostream& out, const utils::Component& component)
{
  out << "<" << component.get_class_name();
  auto status = component.get_status();
  if (status.get_type() == utils::Structure::STRING)
  {
    out << ": " << status.get<std::string>();
  }
  else if (!status.is_empty())
  {
    out << ": " << status.to_string();
  }
  return out << ">";
}

template <class T>
std::string get_type_name()
{
  return "unknown";
}

template <>
std::string get_type_name<bool>()
{
  return "boolean";
}

template <>
std::string get_type_name<int16_t>()
{
  return "int16";
}

template <>
std::string get_type_name<uint16_t>()
{
  return "uint16";
}

template <>
std::string get_type_name<int32_t>()
{
  return "int32";
}

template <>
std::string get_type_name<uint32_t>()
{
  return "uint32";
}

template <>
std::string get_type_name<int64_t>()
{
  return "int64";
}

template <>
std::string get_type_name<uint64_t>()
{
  return "uint64";
}

template <>
std::string get_type_name<float>()
{
  return "float";
}

template <>
std::string get_type_name<double>()
{
  return "double";
}

template <>
std::string get_type_name<std::string>()
{
  return "string";
}

std::string get_type_name(const utils::Json& json)
{
  switch (json.GetType())
  {
    case rapidjson::kNullType:
      return "null";
    case rapidjson::kFalseType:
    case rapidjson::kTrueType:
      return "boolean";
    case rapidjson::kArrayType:
      return "array";
    case rapidjson::kObjectType:
      return "object";
    case rapidjson::kStringType:
      return "string";
    case rapidjson::kNumberType:
      if (json.IsInt())
      {
        return "int32";
      }
      else if (json.IsUint())
      {
        return "uint32";
      }
      else if (json.IsInt64())
      {
        return "int64";
      }
      else if (json.IsUint64())
      {
        return "uint64";
      }
      else if (json.IsDouble())
      {
        return "double";
      }
  }
  return "unknown";
}

std::string render(const utils::Json& json)
{
  switch (json.GetType())
  {
    case rapidjson::kNullType:
      return "null";
    case rapidjson::kFalseType:
    case rapidjson::kTrueType:
      return json.GetBool() ? "true" : "false";
    case rapidjson::kArrayType:
      return "array";
    case rapidjson::kObjectType:
      return "object";
    case rapidjson::kStringType:
      return std::string("'") + json.GetString() + "'";
    case rapidjson::kNumberType:
    {
      std::stringstream s;
      if (json.IsInt())
        s << json.GetInt();
      else if (json.IsUint())
        s << json.GetUint();
      else if (json.IsInt64())
        s << json.GetInt64();
      else if (json.IsUint64())
        s << json.GetUint64();
      else
        s << json.GetDouble();
      return s.str();
    }
  }
  return "unknown";
}

template <class T>
utils::ConfigurationException make_type_exception(const utils::Json& json)
{
  std::stringstream err;
  err << "expected " << get_type_name<T>() << ", found ";
  if (json.IsArray() || json.IsObject())
  {
    err << get_type_name(json);
  }
  else
  {
    err << render(json) << " (" << get_type_name(json) << ")";
  }
  return utils::InvalidTypesException(err.str());
}

template <>
void set_from_json(bool& target, const utils::Json& json)
{
  if (!json.IsBool()) throw make_type_exception<bool>(json);
  target = json.GetBool();
}

template <>
void set_from_json(int16_t& target, const utils::Json& json)
{
  if (!json.IsInt()) throw make_type_exception<int16_t>(json);
  target = json.GetInt();
}

template <>
void set_from_json(uint16_t& target, const utils::Json& json)
{
  if (!json.IsUint()) throw make_type_exception<uint16_t>(json);
  target = json.GetUint();
}

template <>
void set_from_json(int32_t& target, const utils::Json& json)
{
  if (!json.IsInt()) throw make_type_exception<int32_t>(json);
  target = json.GetInt();
}

template <>
void set_from_json(uint32_t& target, const utils::Json& json)
{
  if (!json.IsUint()) throw make_type_exception<uint32_t>(json);
  target = json.GetUint();
}

template <>
void set_from_json(int64_t& target, const utils::Json& json)
{
  if (!json.IsInt64()) throw make_type_exception<int64_t>(json);
  target = json.GetInt64();
}

template <>
void set_from_json(uint64_t& target, const utils::Json& json)
{
  if (!json.IsUint64()) throw make_type_exception<uint64_t>(json);
  target = json.GetUint64();
}

template <>
void set_from_json(float& target, const utils::Json& json)
{
  if (json.IsNumber())
    target = json.GetDouble();
  else if (json.IsNull())
    target = NAN;
  else
    throw make_type_exception<float>(json);
}

template <>
void set_from_json(double& target, const utils::Json& json)
{
  if (json.IsNumber())
    target = json.GetDouble();
  else if (json.IsNull())
    target = NAN;
  else
    throw make_type_exception<double>(json);
}

template <>
void set_from_json(std::string& target, const utils::Json& json)
{
  if (!json.IsString()) throw make_type_exception<std::string>(json);
  target = json.GetString();
}

}  // namespace utils
