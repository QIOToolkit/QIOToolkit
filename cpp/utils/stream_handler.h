
#pragma once

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "utils/exception.h"
#include "utils/log.h"

namespace utils
{
////////////////////////////////////////////////////////////////////////////////
/// Classes for implementation of handlers, which consume the events
/// (via function calls) from the steam readers.
/// These handles configure objects directly from events information.
/// The reader publishes events to the handler sequentially.
/// Example: for JSON string { "c" = 2.0 } the following events will be called:
/// StartObject(), Key("c"), Double(2), EndObject(1)
/// from which object of type class { double c; } will be configured.

////////////////////////////////////////////////////////////////////////////////
/// BaseStreamHandler
///
/// This is a base class for stream handlers. It has default failure handling
/// implementation for unexpected events due to incorrect format of data.
/// Boolean result of operations indicates that parsing can continue.
template <typename T>
class BaseStreamHandler
{
 public:
  template <typename SignalType>
  bool Value(SignalType, T&)
  {
    return fail();
  }

  bool StartObject() { return fail(); }
  bool Key(const std::string&) { return fail(); }
  bool EndObject(size_t, T&) { return fail(); }
  bool StartArray() { return fail(); }
  bool EndArray(size_t, T&) { return fail(); }

  BaseStreamHandler()
  {
    complete_ = false;
    failed_ = false;
    error_message_.clear();
    error_code_ = Error::ParsingError;
  }

  void reset()
  {
    complete_ = false;
    failed_ = false;
    error_message_.clear();
    error_code_ = Error::ParsingError;
  }

  void reset(T&) { reset(); }

  bool started() const { return false; }
  bool complete() const { return complete_; }
  bool missing_inputs() const { return !complete_; }
  bool fail_at_missing_inputs() const { return !complete_; }
  bool failed() const { return failed_; }
  Error error_code() const { return error_code_; }
  std::string error_message() const { return error_message_; }

 protected:
  bool complete_;
  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    failed_ = true;
    error_code_ = error_code;
    error_message_ = meassage;
    return false;
  }

  bool failed_;
  Error error_code_;
  std::string error_message_;
};

////////////////////////////////////////////////////////////////////////////////
/// ObserverStreamHandler
///
/// This is a stream handler for skipping to load corresponding member,
/// used for configure passes when only part of information is needed.
template <typename T>
class ObserverStreamHandler : public BaseStreamHandler<T>
{
 public:
  bool Key(const std::string&) { return true; }

  template <typename SignalType>
  bool Value(SignalType, T&)
  {
    return state_check();
  }
  bool StartObject()
  {
    object_bracket_count_++;
    return state_check();
  }
  bool EndObject(size_t, T&)
  {
    object_bracket_count_--;
    return state_check();
  }
  bool StartArray()
  {
    array_bracket_count_++;
    return state_check();
  }
  bool EndArray(size_t, T&)
  {
    array_bracket_count_--;
    return state_check();
  }

  ObserverStreamHandler()
  {
    object_bracket_count_ = 0;
    array_bracket_count_ = 0;
  }

  void reset(T& val)
  {
    object_bracket_count_ = 0;
    array_bracket_count_ = 0;
    BaseStreamHandler<T>::reset(val);
  }

  bool started() const
  {
    return object_bracket_count_ > 0 || array_bracket_count_ > 0;
  }

 protected:
  int object_bracket_count_;
  int array_bracket_count_;

  bool state_check()
  {
    if (object_bracket_count_ == 0 && array_bracket_count_ == 0)
    {
      this->complete_ = true;
    }
    if (object_bracket_count_ < 0 || array_bracket_count_ < 0)
    {
      return this->fail();
    }
    return true;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Type mismatch error messgae
template <class T>
std::string get_type_name();

template <typename Expected_T, typename Actual_T>
std::string type_mismatch_message(Actual_T val)
{
  std::stringstream err;
  err << "expected " << get_type_name<Expected_T>() << ", found " << val << " ("
      << get_type_name<Actual_T>() << ")";
  return err.str();
}

////////////////////////////////////////////////////////////////////////////////
/// BasicTypeStreamHandler<..>
///
/// Particular implementations of stream handlers for values of
/// simple, non class, types like int, double, string...
///
template <typename T>
class BasicTypeStreamHandler : public BaseStreamHandler<T>
{
 public:
  using Type_T = T;
};

template <>
class BasicTypeStreamHandler<bool> : public BaseStreamHandler<bool>
{
 public:
  using Type_T = bool;

  template <typename SignalType>
  bool Value(SignalType signal_val, bool&)
  {
    return this->fail(type_mismatch_message<bool, SignalType>(signal_val),
                      Error::ValueError);
  }

  bool Value(bool b, bool& val)
  {
    val = b;
    complete_ = true;
    return true;
  }
};

template <>
class BasicTypeStreamHandler<int> : public BaseStreamHandler<int>
{
 public:
  using Type_T = int;

  template <typename SignalType>
  bool Value(SignalType signal_val, int&)
  {
    return this->fail(type_mismatch_message<int, SignalType>(signal_val),
                      Error::ValueError);
  }

  bool Value(int64_t i, int& val)
  {
    val = i;
    complete_ = true;
    return true;
  }
};

template <>
class BasicTypeStreamHandler<size_t> : public BaseStreamHandler<size_t>
{
 public:
  using Type_T = size_t;

  template <typename SignalType>
  bool Value(SignalType signal_val, size_t&)
  {
    return this->fail(type_mismatch_message<size_t, SignalType>(signal_val),
                      Error::ValueError);
  }

  bool Value(int64_t i, size_t& val)
  {
    if (i < 0)
    {
      return this->fail(type_mismatch_message<size_t, int64_t>(i),
                        Error::ValueError);
    }
    val = i;
    complete_ = true;
    return true;
  }
};

template <>
class BasicTypeStreamHandler<double> : public BaseStreamHandler<double>
{
 public:
  using Type_T = double;

  template <typename SignalType>
  bool Value(SignalType signal_val, double&)
  {
    return this->fail(type_mismatch_message<double, SignalType>(signal_val),
                      Error::ValueError);
  }

  bool Value(int64_t i, double& val)
  {
    val = double(i);
    complete_ = true;
    return true;
  }

  bool Value(double d, double& val)
  {
    val = d;
    complete_ = true;
    return true;
  }
};

template <>
class BasicTypeStreamHandler<std::string>
    : public BaseStreamHandler<std::string>
{
 public:
  using Type_T = std::string;

  template <typename SignalType>
  bool Value(SignalType signal_val, std::string&)
  {
    return this->fail(
        type_mismatch_message<std::string, SignalType>(signal_val),
        Error::ValueError);
  }

  bool Value(std::string str, std::string& val)
  {
    val = str;
    complete_ = true;
    return true;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// EnumStreamHandler
///
/// Implementations of stream handlers for enum
/// expressed as fixed string values
///
template <typename EnumType, class StringEnumMap>
class EnumStreamHandler : public BaseStreamHandler<EnumType>
{
 public:
  using Type_T = std::string;

  template <typename SignalType>
  bool Value(SignalType signal_val, EnumType&)
  {
    return this->fail(
        type_mismatch_message<std::string, SignalType>(signal_val),
        Error::ValueError);
  }

  bool Value(std::string str, EnumType& val)
  {
    auto it = StringEnumMap::get().find(str);
    if (it != StringEnumMap::get().end())
    {
      val = it->second;
      this->complete_ = true;
      return true;
    }
    else
    {
      std::stringstream err;
      err << "expected ";
      for (const auto& it : StringEnumMap::get())
      {
        err << it.first << ", ";
      }
      err << "found " << str;
      return this->fail(err.str(), Error::ValueError);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
/// VectorStreamHandler
///
/// Implementations of stream handlers for vector of values
/// of simple, non class type like bool, int...
///
template <typename T>
class VectorStreamHandler : public BaseStreamHandler<std::vector<T>>
{
 public:
  using Type_T = std::vector<T>;
  using Base_T = BaseStreamHandler<std::vector<T>>;

  template <typename SignalType>
  bool Value(SignalType signal_value, Type_T& vec)
  {
    if (!started_ || complete_) return fail();
    if (!val_stream_.Value(signal_value, val_)) return fail();
    vec.push_back(val_);
    return true;
  }

  bool StartArray()
  {
    started_ = true;
    return !complete_;
  }

  bool EndArray(size_t elementCount, Type_T& vec)
  {
    if (!started_) return fail();
    complete_ = true;
    started_ = false;
    vec.shrink_to_fit();
    return elementCount == vec.size();
  }

  bool complete() const { return complete_; }

  bool started() const { return started_; }

  void reset(Type_T& vec)
  {
    Base_T::reset(vec);
    vec.clear();
    val_stream_.reset(val_);
    started_ = false;
    complete_ = false;
  }

 protected:
  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    if (val_stream_.failed())
    {
      return Base_T::fail(val_stream_.error_message(),
                          val_stream_.error_code());
    }
    else
    {
      return Base_T::fail(meassage, error_code);
    }
  }
  BasicTypeStreamHandler<T> val_stream_;
  T val_;
  bool started_;
  bool complete_;
};

////////////////////////////////////////////////////////////////////////////////
/// SetStreamHandler
///
/// Implementations of stream handlers for set of values
/// of simple, non class type like bool, int...
///
template <typename T>
class SetStreamHandler : public BaseStreamHandler<std::set<T>>
{
 public:
  using Type_T = std::set<T>;
  using Base_T = BaseStreamHandler<std::set<T>>;

  template <typename SignalType>
  bool Value(SignalType signal_value, Type_T& vec)
  {
    if (!started_ || complete_) return fail();
    if (!val_stream_.Value(signal_value, val_)) return fail();
    vec.insert(val_);
    return true;
  }

  bool StartArray()
  {
    started_ = true;
    return !complete_;
  }

  bool EndArray(size_t elementCount, Type_T& vec)
  {
    if (!started_) return fail();
    complete_ = true;
    started_ = false;
    return elementCount == vec.size();
  }

  bool started() const { return started_; }

  bool complete() const { return complete_; }

  void reset(Type_T& vec)
  {
    Base_T::reset(vec);
    vec.clear();
    val_stream_.reset(val_);
    started_ = false;
    complete_ = false;
  }

 protected:
  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    if (val_stream_.failed())
    {
      return Base_T::fail(val_stream_.error_message(),
                          val_stream_.error_code());
    }
    else
    {
      return Base_T::fail(meassage, error_code);
    }
  }
  BasicTypeStreamHandler<T> val_stream_;
  T val_;
  bool started_;
  bool complete_;
};

////////////////////////////////////////////////////////////////////////////////
/// PairVectorStreamHandler
///
/// Implementations of stream handlers for set of values
/// of simple, non class type like bool, int...
///
template <typename Key_T, typename T>
class PairVectorStreamHandler : public BaseStreamHandler<std::vector<std::pair<Key_T, T>>>
{
 public:
  using Type_T = std::vector<std::pair<Key_T, T>>;
  using Base_T = BaseStreamHandler<Type_T>;

  bool StartObject()
  {
    started_ = true;
    return !complete_;
  }

  bool Key(const std::string& key)
  {
    if (!started_ || complete_ || key_is_ready_)
    {
      return fail();
    }

    std::stringstream ss(key);
    try
    {
      ss >> key_;
      key_is_ready_ = true;
    }
    catch (const std::exception&)
    {
      return fail();
    }
    return true;
  }

  template <typename SignalType>
  bool Value(SignalType signal_value, Type_T& vec)
  {
    if (!started_ || complete_ || !key_is_ready_)
    {
      return fail();
    }
    if (!val_stream_.Value(signal_value, val_))
    {
      return fail();
    }
    vec.push_back(std::pair<Key_T, T>(key_, val_));
    key_is_ready_ = false;
    return true;
  }

  bool EndObject(size_t elementCount, Type_T& vec)
  {
    if (!started_ || key_is_ready_)
    {
      return fail();
    }
    complete_ = true;
    started_ = false;
    return elementCount == vec.size();
  }

  bool started() const { return started_; }

  bool complete() const { return complete_; }

  void reset(Type_T& vec)
  {
    Base_T::reset(vec);
    vec.clear();
    val_stream_.reset(val_);
    started_ = false;
    complete_ = false;
    key_is_ready_ = false;
  }

 protected:
  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    if (val_stream_.failed())
    {
      return Base_T::fail(val_stream_.error_message(),
                          val_stream_.error_code());
    }
    else
    {
      return Base_T::fail(meassage, error_code);
    }
  }
  BasicTypeStreamHandler<T> val_stream_;
  bool key_is_ready_;
  Key_T key_;
  T val_;
  bool started_;
  bool complete_;
};

////////////////////////////////////////////////////////////////////////////////
/// Stream handler for object is implemented as nested classes of key value and
/// utility stream handlers:
///
/// ObjectStreamHandler<
///     ObjectMemberStreamHandler<..,
///     ObjectMemberStreamHandler<..,
///     FinalObjectMemberStreamHandler
/// >>>
///
/// Each ObjectMemberStreamHandler represents key value entry of object.
/// FinalObjectMemberStreamHandler is the last one in the chain.
///

////////////////////////////////////////////////////////////////////////////////
/// FinalObjectMemberStreamHandler
///
/// FinalObjectMemberStreamHandler is the last one in the members stream chain.
/// It is used to simplify ObjectMemberStreamHandler logic.
///
template <class OuterClass>
class FinalObjectMemberStreamHandler : public BaseStreamHandler<OuterClass>
{
 public:
  using Out_Type_T = OuterClass;
  bool complete() const { return true; }
  bool started() const { return false; }
  bool fail_at_missing_inputs() { return true; }
  bool missing_inputs() { return false; }
};

////////////////////////////////////////////////////////////////////////////////
/// AnyKeyMemberStreamHandler
///
/// Stream handler for handling extra fields, which are not to be used for
/// configure. These fields could have any key value.
///
template <class OuterClass>
class AnyKeyObjectMemberStreamHandler : public BaseStreamHandler<OuterClass>
{
 public:
  using Out_Type_T = OuterClass;
  using Base_T = BaseStreamHandler<OuterClass>;

  AnyKeyObjectMemberStreamHandler() : val_(0) {}

  bool Key(const std::string& key)
  {
    if (started_)
    {
      if (stream_handler_.complete())
      {
        stream_handler_.reset(val_);
        Base_T::reset();
        key_ = key;
      }
      else
      {
        return stream_handler_.Key(key) ? true : fail();
      }
    }
    else
    {
      key_ = key;
      started_ = true;
    }
    return true;
  }

  template <typename SignalType>
  bool Value(SignalType signal_value, OuterClass&)
  {
    if (!started_ || stream_handler_.complete())
    {
      return fail();
    }
    else
    {
      return stream_handler_.Value(signal_value, val_) ? true : fail();
    }
  }

  bool StartObject()
  {
    if (!started_ || stream_handler_.complete())
    {
      return fail();
    }
    else
    {
      return stream_handler_.StartObject() ? true : fail();
    }
  }

  bool EndObject(size_t elementCount, OuterClass&)
  {
    if (!started_ || stream_handler_.complete())
    {
      return fail();
    }
    else
    {
      return stream_handler_.EndObject(elementCount, val_) ? true : fail();
    }
  }

  bool StartArray()
  {
    if (!started_ || stream_handler_.complete())
    {
      return fail();
    }
    else
    {
      return stream_handler_.StartArray() ? true : fail();
    }
  }

  bool EndArray(size_t elementCount, OuterClass&)
  {
    if (!started_ || stream_handler_.complete())
    {
      return fail();
    }
    else
    {
      return stream_handler_.EndArray(elementCount, val_) ? true : fail();
    }
  }

  bool complete() const { return stream_handler_.complete(); }

  void reset(OuterClass&)
  {
    started_ = false;
    stream_handler_.reset(val_);
    Base_T::reset();
  }

  bool fail_at_missing_inputs() { return true; }

  bool missing_inputs() { return false; }

  bool started() const { return started_; }

 protected:
  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    if (stream_handler_.failed())
    {
      return Base_T::fail(
          "parameter `" + key_ + "`: " + stream_handler_.error_message(),
          stream_handler_.error_code());
    }
    else
    {
      return Base_T::fail(meassage, error_code);
    }
  }

  ObserverStreamHandler<int> stream_handler_;
  bool started_;
  std::string key_;
  int val_;
};

////////////////////////////////////////////////////////////////////////////////
/// ObjectMemberStreamHandler
///
/// Stream handler for key value entry.
/// GetMember::get_key() is the key of object entry.
/// GetMember::get(val) return reference to the member it is streaming to.
///
template <class StreamHandler, class OuterClass, class GetMember,
          bool required_ = true,
          class NextObjectMemberStreamHandler =
              FinalObjectMemberStreamHandler<OuterClass>>
class ObjectMemberStreamHandler : public NextObjectMemberStreamHandler
{
 public:
  using Out_Type_T = OuterClass;
  bool Key(const std::string& key)
  {
    if (started_)
    {
      if (stream_handler_.complete())
        return NextObjectMemberStreamHandler::Key(key);
      else
        return stream_handler_.Key(key) ? true : fail();
    }
    else
    {
      if (key == GetMember::get_key())
      {
        started_ = true;
        return true;
      }
      else
        return NextObjectMemberStreamHandler::Key(key);
    }
  }

  template <typename SignalType>
  bool Value(SignalType signal_value, OuterClass& val)
  {
    if (!started_ || stream_handler_.complete())
      return NextObjectMemberStreamHandler::Value(signal_value, val);
    else
      return stream_handler_.Value(signal_value, GetMember::get(val)) ? true
                                                                      : fail();
  }

  bool StartObject()
  {
    if (!started_ || stream_handler_.complete())
      return NextObjectMemberStreamHandler::StartObject();
    else
      return stream_handler_.StartObject() ? true : fail();
  }

  bool EndObject(size_t elementCount, OuterClass& val)
  {
    if (!started_ || stream_handler_.complete())
      return NextObjectMemberStreamHandler::EndObject(elementCount, val);
    else
      return stream_handler_.EndObject(elementCount, GetMember::get(val))
                 ? true
                 : fail();
  }

  bool StartArray()
  {
    if (!started_ || stream_handler_.complete())
      return NextObjectMemberStreamHandler::StartArray();
    else
      return stream_handler_.StartArray() ? true : fail();
  }

  bool EndArray(size_t elementCount, OuterClass& val)
  {
    if (!started_ || stream_handler_.complete())
      return NextObjectMemberStreamHandler::EndArray(elementCount, val);
    else
      return stream_handler_.EndArray(elementCount, GetMember::get(val))
                 ? true
                 : fail();
  }

  bool complete() const
  {
    return stream_handler_.complete() &&
           NextObjectMemberStreamHandler::complete();
  }

  void reset(OuterClass& val)
  {
    started_ = false;
    stream_handler_.reset(GetMember::get(val));
    NextObjectMemberStreamHandler::reset(val);
  }

  bool fail_at_missing_inputs()
  {
    if (required_ && !stream_handler_.complete())
    {
      if (!started_)
      {
        return NextObjectMemberStreamHandler::fail(
            "parameter `" + GetMember::get_key() + "` is required",
            Error::MissingInput);
      }
      else
      {
        if (!stream_handler_.fail_at_missing_inputs()) return false;
      }
    }
    return NextObjectMemberStreamHandler::fail_at_missing_inputs();
  }

  bool missing_inputs()
  {
    if (required_)
    {
      if (!stream_handler_.complete())
      {
        if (stream_handler_.missing_inputs())
        {
          return true;
        }
      }
    }
    return NextObjectMemberStreamHandler::missing_inputs();
  }

 protected:
  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    if (stream_handler_.failed())
      return NextObjectMemberStreamHandler::fail(
          "parameter `" + GetMember::get_key() +
              "`: " + stream_handler_.error_message(),
          stream_handler_.error_code());
    else
      return NextObjectMemberStreamHandler::fail(meassage, error_code);
  }

  StreamHandler stream_handler_;
  bool started_;
};

////////////////////////////////////////////////////////////////////////////////
/// ObjectStreamHandler
///
/// Stream handler for object.
///.stop_early_ - stop streaming when all required parameters are loaded,
/// to skip streaming of large members.
///
template <class MemberStreamHandler, bool stop_early_ = false,
          typename Out_Type = typename MemberStreamHandler::Out_Type_T>
class ObjectStreamHandler : public MemberStreamHandler
{
 public:
  using Type_T = Out_Type;

  bool Key(const std::string& key)
  {
    if (stop_early_)
    {
      if (!MemberStreamHandler::missing_inputs())
      {
        complete_ = true;
        return false;  // stop parsing without setting fail flags
      }
    }
    return MemberStreamHandler::Key(key);
  }

  bool StartObject()
  {
    if (complete_) return this->fail();

    if (started_)
      return MemberStreamHandler::StartObject();
    else
      started_ = true;
    return true;
  }

  bool EndObject(size_t elementCount,
                 typename MemberStreamHandler::Out_Type_T& val)
  {
    if (complete_) return this->fail();

    if (started_)
    {
      if (MemberStreamHandler::complete())
      {
        complete_ = true;
        started_ = false;
        return true;
      }
      else
      {
        if (MemberStreamHandler::EndObject(elementCount, val)) return true;
        if (!MemberStreamHandler::fail_at_missing_inputs()) return false;
        // Only optional parameters are not set
        complete_ = true;
        started_ = false;
        return true;
      }
    }
    return this->fail();
  }

  bool complete() const { return complete_; }

  bool started() const { return started_; }

  void reset(Type_T& val)
  {
    started_ = false;
    complete_ = false;
    MemberStreamHandler::reset(val);
  }

 private:
  bool started_;
  bool complete_;
};

template <class StreamHandler>
class VectorObjectStreamHandler
    : public BaseStreamHandler<std::vector<typename StreamHandler::Type_T>>
{
 public:
  using Value_T = typename StreamHandler::Type_T;
  using Type_T = std::vector<Value_T>;

  bool StartArray()
  {
    if (complete_) return fail();

    if (started_)
    {
      if (!val_stream_.StartArray()) return fail();
    }
    else
      started_ = true;
    return true;
  }

  template <typename SignalType>
  bool Value(SignalType signal_value, Type_T&)
  {
    if (!started_ || complete_) return fail();
    if (!val_stream_.Value(signal_value, val_)) return fail();
    return true;
  }

  bool StartObject()
  {
    if (!started_ || complete_) return fail();
    if (!val_stream_.StartObject()) return fail();
    return true;
  }

  bool Key(const std::string& key)
  {
    if (!started_ || complete_) return fail();
    if (!val_stream_.Key(key)) return fail();
    return true;
  }

  bool EndObject(size_t elementCount, Type_T& vec)
  {
    if (!started_ || complete_) return fail();
    if (!val_stream_.EndObject(elementCount, val_)) return fail();
    if (val_stream_.complete()) push_back_value(vec);
    return true;
  }

  bool EndArray(size_t elementCount, Type_T& vec)
  {
    if (!started_ || complete_) return fail();

    if (val_stream_.started())
    {
      if (!val_stream_.EndArray(elementCount, val_)) return fail();
      if (val_stream_.complete()) push_back_value(vec);
    }
    else
    {
      vec.shrink_to_fit();
      complete_ = true;
      started_ = false;
    }
    return true;
  }

  virtual bool complete() const { return complete_; }

  virtual bool started() const { return started_; }

  void reset(Type_T& vec)
  {
    vec.clear();
    val_ = Value_T();
    started_ = false;
    complete_ = false;
    val_stream_.reset(val_);
  }

 protected:
  void push_back_value(Type_T& vec)
  {
    vec.push_back(std::move(val_));
    val_ = Value_T();
    val_stream_.reset(val_);
  }

  bool fail(const std::string& meassage = "parsing error",
            Error error_code = Error::ParsingError)
  {
    if (val_stream_.failed())
    {
      return BaseStreamHandler<std::vector<typename StreamHandler::Type_T>>::
          fail(val_stream_.error_message(), val_stream_.error_code());
    }
    else
    {
      return BaseStreamHandler<
          std::vector<typename StreamHandler::Type_T>>::fail(meassage,
                                                             error_code);
    }
  }

  StreamHandler val_stream_;
  Value_T val_;
  bool started_;
  bool complete_;
};

template <class StreamHandler>
class OwnerClassStreamHandler : public StreamHandler
{
 public:
  template <typename SignalType>
  bool Value(SignalType signal_value)
  {
    return StreamHandler::Value(signal_value, val_);
  }

  bool EndObject(size_t elementCount)
  {
    return StreamHandler::EndObject(elementCount, val_);
  }

  bool EndArray(size_t sz) { return StreamHandler::EndArray(sz, val_); }

  typename StreamHandler::Type_T& get_value() { return val_; }

  void reset() { StreamHandler::reset(val_); }

 protected:
  typename StreamHandler::Type_T val_;
};

}  // namespace utils
