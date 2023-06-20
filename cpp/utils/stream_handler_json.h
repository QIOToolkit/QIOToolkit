// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

#include "utils/exception.h"
#include "utils/stream_handler.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/reader.h"
#include "rapidjson/stream.h"

namespace utils
{
////////////////////////////////////////////////////////////////////////////////
/// JSONHandlerProxy
///
/// Convert and redirect JSON reader signals into qiotoolkit handler.
///
template <class StreamHandler>
class JSONHandlerProxy
{
 public:
  JSONHandlerProxy() { stream_handler_.reset(); }
  bool Null() { return fail(); }
  bool Bool(bool b) { return stream_handler_.Value(b); }

  bool Int(int i) { return stream_handler_.Value(int64_t(i)); }
  bool Uint(unsigned i) { return stream_handler_.Value(int64_t(i)); }
  bool Int64(int64_t i) { return stream_handler_.Value(i); }
  bool Uint64(uint64_t i) { return stream_handler_.Value(int64_t(i)); }

  bool Double(double d) { return stream_handler_.Value(d); }
  bool RawNumber(const char*, int, bool) { return fail(); }
  bool String(const char* str, int len, bool)
  {
    return stream_handler_.Value(std::string(str, len));
  }
  bool StartObject() { return stream_handler_.StartObject(); }
  bool Key(const char* str, int len, bool)
  {
    return stream_handler_.Key(std::string(str, len));
  }
  bool EndObject(int elementCount)
  {
    return stream_handler_.EndObject(elementCount);
  }
  bool StartArray() { return stream_handler_.StartArray(); }
  bool EndArray(int elementCount)
  {
    return stream_handler_.EndArray(elementCount);
  }

  typename StreamHandler::Type_T& get_value()
  {
    return stream_handler_.get_value();
  }

  bool complete() { return stream_handler_.complete(); }
  Error error_code() const { return stream_handler_.error_code(); }
  std::string error_message() const { return stream_handler_.error_message(); }

 private:
  bool fail() const { return false; }
  utils::OwnerClassStreamHandler<StreamHandler> stream_handler_;
};

////////////////////////////////////////////////////////////////////////////////
/// Sream configure from JSON string.
/// 'Type' is the type of value to configure
/// 'Handler' is the type of stream handler for 'Type'.
/// By default use 'StreamHandler' defined in 'Type'.
///
template <typename Type, typename Handler = typename Type::StreamHandler>
void configure_from_json_string(const std::string& json_str, Type& val)
{
  JSONHandlerProxy<Handler> handler_proxy;

  rapidjson::StringStream json_stream(json_str.c_str());

  rapidjson::Reader reader;

  if (!reader.Parse(json_stream, handler_proxy))
  {
    if (!handler_proxy.complete())
    {
      throw ConfigurationException(handler_proxy.error_message(),
                                   handler_proxy.error_code());
    }
  }
  val = std::move(handler_proxy.get_value());
}

/// File
/// Wrapper for using FILE* exception safe way
class File
{
 public:
  File() : fp_(nullptr) {}
  File(const std::string& file_name, const std::string& mode)
  {
    this->open(file_name, mode);
  }
  void open(const std::string& file_name, const std::string& mode)
  {
    fp_ = fopen(file_name.c_str(), mode.c_str());
  }
  FILE* get() { return fp_; }
  ~File()
  {
    if (fp_ != nullptr) fclose(fp_);
  }

 private:
  FILE* fp_;
};

////////////////////////////////////////////////////////////////////////////////
/// Sream configure from JSON file.
/// 'Type' is the type of value to configure
/// 'Handler' is the type of stream handler for 'Type'.
/// By default use 'StreamHandler' defined in 'Type'.
///
template <typename Type, typename Handler = typename Type::StreamHandler>
void configure_from_json_file(const std::string& file_name, Type& val)
{
  JSONHandlerProxy<Handler> handler_proxy;
  // Recommended size of read buffer is taken from:
  // https://rapidjson.org/md_doc_stream.html
  std::vector<char> readBuffer(65536);
  rapidjson::Reader reader;
  bool parse_res;

  {
    utils::File json_file(file_name.c_str(), "rb");
    rapidjson::FileReadStream json_stream(json_file.get(), readBuffer.data(),
                                          readBuffer.size());
    parse_res = reader.Parse(json_stream, handler_proxy);
  }

  if (!parse_res)
  {
    if (!handler_proxy.complete())
    {
      throw ConfigurationException(handler_proxy.error_message(),
                                   handler_proxy.error_code());
    }
  }
  val = std::move(handler_proxy.get_value());
}

////////////////////////////////////////////////////////////////////////////////
/// Sream configure from JSON string.
/// Intermidiate configuration object is created and used.
/// 'Type' is the type of value to configure
/// 'ConfigurationType' is the type of configuration object for 'Type'.
/// By default 'Configuration_T' defined in 'Type' is used.
///
template <class Type,
          typename ConfigurationType = typename Type::Configuration_T>
void configure_with_configuration_from_json_string(const std::string& json_str,
                                                   Type& val)
{
  ConfigurationType configuration;
  configure_from_json_string(json_str, configuration);
  val.configure(configuration);
}

////////////////////////////////////////////////////////////////////////////////
/// Sream configure from JSON file.
/// Intermidiate configuration object is created and used.
/// 'Type' is the type of value to configure
/// 'ConfigurationType' is the type of configuration object for 'Type'.
/// By default 'Configuration_T' defined in 'Type' is used.
///
template <class Type,
          typename ConfigurationType = typename Type::Configuration_T>
void configure_with_configuration_from_json_file(const std::string& file_name,
                                                 Type& val)
{
  ConfigurationType configuration;
  configure_from_json_file(file_name, configuration);
  val.configure(configuration);
}

}  // namespace utils
