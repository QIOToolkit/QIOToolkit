// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

#include "utils/exception.h"
#include "utils/log.h"
#include "utils/optional.h"
#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseIterativeFlag
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

namespace utils
{
typedef ::rapidjson::Value Json;
typedef ::rapidjson::Document JsonDocument;

JsonDocument json_from_string(const std::string& buffer);
JsonDocument json_from_string(const char* buffer);
JsonDocument json_from_file(const std::string& filename);
JsonDocument json_from_string_in_situ(char* buffer);

/// Helper function to serialize a json DOM object to string
///
/// > [!NOTE]
/// > This method is intended for debugging purposes as we currently
/// > don't use utils::Json for output (utils::Structure has its own rendering).
std::string json_to_string(const Json& json);

class Component;

template <class T>
typename std::enable_if<!std::is_base_of<utils::Component, T>::value, void>::type
set_from_json(T&, const utils::Json&)
{
  LOG(FATAL, "unimplemented");
}

template <>
void set_from_json(bool& target, const utils::Json& json);
template <>
void set_from_json(int16_t& target, const utils::Json& json);
template <>
void set_from_json(uint16_t& target, const utils::Json& json);
template <>
void set_from_json(int32_t& target, const utils::Json& json);
template <>
void set_from_json(uint32_t& target, const utils::Json& json);
template <>
void set_from_json(int64_t& target, const utils::Json& json);
template <>
void set_from_json(uint64_t& target, const utils::Json& json);
template <>
void set_from_json(float& target, const utils::Json& json);
template <>
void set_from_json(double& target, const utils::Json& json);
template <>
void set_from_json(std::string& target, const utils::Json& json);

template <class T>
void set_from_json(std::vector<T>& target, const utils::Json& json)
{
  if (!json.IsArray())
  {
    throw utils::ValueException("Expected array");
  }
  target.resize(json.Size());
  for (size_t i = 0; i < target.size(); i++)
  {
    set_from_json(target[i], json[i]);
  }
}

template <class T>
void set_from_json(std::set<T>& target, const utils::Json& json)
{
  if (!json.IsArray())
  {
    throw utils::ValueException("Expected array");
  }
  target.clear();
  for (size_t i = 0; i < json.GetArray().Size(); i++)
  {
    T value;
    set_from_json(value, json[i]);
    target.insert(value);
  }
}

template <class T>
void set_from_json(std::map<std::string, T>& target, const utils::Json& json)
{
  if (!json.IsObject())
  {
    throw utils::ValueException("Expected object");
  }
  target.clear();
  for (const auto& it : json.GetObject())
  {
    T t;
    set_from_json(t, it.value);
    target[it.name.GetString()] = t;
  }
}

template <class T>
void set_from_json(std::optional<T>& target, const utils::Json& json)
{
  if (json.IsNull())
  {
    target.reset();
  }
  else
  {
    T value;
    set_from_json(value, json);
    target = value;
  }
}

/// Check if 'mult' * 'file size' bytes will fit in memory.
void memory_check_using_file_size(const std::string& filename, double mult);

}  // namespace utils
