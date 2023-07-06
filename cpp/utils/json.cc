
#include "utils/json.h"

#include <cstdio>

#include "utils/exception.h"
#include "utils/operating_system.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace utils
{
JsonDocument json_from_string(const std::string& buffer)
{
  return json_from_string(buffer.c_str());
}
JsonDocument json_from_string(const char* buffer)
{
  JsonDocument d;
  ::rapidjson::ParseResult ok = d.Parse(buffer);
  if (!ok)
  {
    THROW(utils::ValueException,
          "Json parse error: ", ::rapidjson::GetParseError_En(ok.Code()),
          " (offset: ", ok.Offset(), ")");
  }
  // This typically RVO'ed (return of a local variable) or std::move optimized
  // at worst (VC++11 in debug config) => the document is NOT copied.
  return d;
}

void memory_check_using_file_size(const std::string& filename, double mult)
{
  size_t available_memory = utils::get_available_memory();
  bool file_size_found = false;
  size_t file_size = utils::get_file_size(filename, file_size_found);
  size_t memory_estimate = size_t(file_size * mult);

  if (file_size_found && available_memory < memory_estimate)
  {
    throw utils::MemoryLimitedException(
        "File " + filename +
        "is too big."
        "Expected to exceed machine's current available memory.");
  }
}

JsonDocument json_from_file(const std::string& filename)
{
  memory_check_using_file_size(filename, 2.0);

  FILE* fp = fopen(filename.c_str(), "r");

  if (fp == nullptr)
    throw utils::FileReadException("Cannot open " + filename + ".");

  // NOTE: This is a streaming input buffer, it does NOT need to
  // hold the entire file.
  char readBuffer[65536];
  ::rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
  JsonDocument d;
  ::rapidjson::ParseResult ok = d.ParseStream(is);
  if (!ok)
  {
    THROW(utils::ValueException,
          "Json parse error: ", ::rapidjson::GetParseError_En(ok.Code()),
          " (offset: ", ok.Offset(), ")");
  }
  // This typically RVO'ed (return of a local variable) or std::move optimized
  // at worst (VC++11 in debug config) => the document is NOT copied.
  return d;
}

JsonDocument json_from_string_in_situ(char* buffer)
{
  JsonDocument d;
  ::rapidjson::ParseResult ok = d.ParseInsitu(buffer);
  if (!ok)
  {
    THROW(utils::ValueException,
          "Json parse error: ", ::rapidjson::GetParseError_En(ok.Code()),
          " (offset: ", ok.Offset(), ")");
  }
  // This typically RVO'ed (return of a local variable) or std::move optimized
  // at worst (VC++11 in debug config) => the document is NOT copied.
  return d;
}

std::string json_to_string(const Json& json)
{
  rapidjson::StringBuffer buffer;
  buffer.Clear();
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json.Accept(writer);
  return std::string(buffer.GetString());
}

}  // namespace utils
