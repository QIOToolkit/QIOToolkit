
#pragma once

#include <fstream>
#include <string>

#include "exception.h"
#include "problem.pb.h"
#include "stream_handler.h"
namespace utils
{
////////////////////////////////////////////////////////////////////////////////
/// PROTOHandlerProxy
///
/// Convert and redirect PROTO reader signals into qiotoolkit handler.
///
template <class StreamHandler>
class PROTOHandlerProxy
{
 public:
  PROTOHandlerProxy() { stream_handler_.reset(); }
  bool Null() { return fail(); }
  bool Bool(bool b) { return stream_handler_.Value(b); }

  bool Int(int i) { return stream_handler_.Value(int64_t(i)); }
  bool Uint(unsigned i) { return stream_handler_.Value(int64_t(i)); }
  bool Int64(int64_t i) { return stream_handler_.Value(i); }
  bool Uint64(uint64_t i) { return stream_handler_.Value(int64_t(i)); }

  bool Double(double d) { return stream_handler_.Value(d); }
  bool RawNumber(const char*, int, bool) { return fail(); }
  bool String(std::string str) { return stream_handler_.Value(str); }
  bool StartObject() { return stream_handler_.StartObject(); }
  bool Key(std::string str) { return stream_handler_.Key(str); }
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
  void reset() { stream_handler_.reset(); }

  bool complete() { return stream_handler_.complete(); }
  Error error_code() const { return stream_handler_.error_code(); }
  std::string error_message() const { return stream_handler_.error_message(); }

 private:
  bool fail() const { return false; }
  utils::OwnerClassStreamHandler<StreamHandler> stream_handler_;
};
}  // namespace utils
