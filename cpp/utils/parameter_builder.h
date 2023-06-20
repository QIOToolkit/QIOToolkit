#ifndef UTILS_PARAMETER_BUILDER_H
#define UTILS_PARAMETER_BUILDER_H

#include "exception.h"
#include "json.h"

namespace utils
{
class ComponentWithOutput;

////////////////////////////////////////////////////////////////////////////////
/// ParameterBuilder
///
/// This is the builder object returned by Component::param(). Its initial
/// setup contains the input (json) to read from, the name of the field to
/// read and a reference to the parameter to be filled.
///
/// Additional details on how to read the parameter can be chained onto the
/// Parameter builder:
///
/// ```c++
/// this->param(json, "number", my_number)
///     .alias("my_number")
///     .description("some description")
///     .matches(GreaterEquals(0))
///     .default_value(1)
/// ```
///
/// NOTE: A parameter is considered **optional** unless `required()` is
/// specified explicitly as part of the chain. An optional parameter with no
/// default value will leave the parameter reference untouched (except if the
/// parameter is an std::optional<T>, in which case it will be reset).
///
/// Output Parametes
///
/// Some parameters are intended to be returned as part of the response
/// (read back). Components with this property should be derived from
/// `utils::ComponentWithOutput`, in which case the ParameterBuilder has the
/// additional method `.with_output()` causing the parameter value to be
/// stored in the component's output parameters:
///
/// ```c++
/// this->param(json, "number", my_number)
///     .with_output()
///
/// this->get_output_parameters();  // will contain {"number": my_number}
/// ```
///
template <class Component_T, class Param_T>
class ParameterBuilder
{
 public:
  /// Assign the parameter from the specified field.
  ParameterBuilder(Component_T* component, const utils::Json& json,
                   const std::string& name, Param_T& parameter)
      : assigned_(false),
        component_(component),
        json_(&json),
        name_(name),
        parameter_(&parameter)
  {
    reset_if_optional(parameter);
    assigned_ = try_assign();
  }

  /// If not yet assigned try assigning from the alias field.
  ParameterBuilder& alias(const std::string& name)
  {
    if (assigned_) return *this;
    name_ = name;
    assigned_ = try_assign();
    return *this;
  }

  /// Store the description to be used in error messages.
  ParameterBuilder& description(const std::string& desc)
  {
    desc_ = desc;
    return *this;
  }

  /// Throw an exception if the parameter has not been assigned
  /// (i.e., the field was not present).
  ParameterBuilder& required()
  {
    if (!assigned_)
    {
      throw utils::MissingInputException("parameter `" + name_ +
                                        "` is required");
    }
    return *this;
  }

  /// Assign a default value if the parameter has not ben assigned.
  template <class V>
  ParameterBuilder& default_value(const V& default_value_)
  {
    if (!assigned_)
    {
      *parameter_ = default_value_;
      assigned_ = true;
    }
    return *this;
  }

  /// Store the parameters value in the component's output_parameters
  /// (to be returned to the user).
  template <class C = Component_T>
  typename std::enable_if<std::is_same<C, utils::ComponentWithOutput>::value,
                          ParameterBuilder&>::type
  with_output()
  {
    if (assigned_ && component_ != nullptr)
    {
      component_->set_output_parameter(name_, *parameter_);
    }
    return *this;
  }

  /// If the parameter was assigned, match its value against the matcher.
  template <class M>
  ParameterBuilder& matches(const M& matcher)
  {
    if (assigned_ && !matcher.matches(*parameter_))
    {
      throw utils::ValueException("parameter `" + name_ +
                                 "`: " + matcher.explain(*parameter_));
    }
    return *this;
  }

 private:
  bool try_assign()
  {
    if (!json_->IsObject()) return false;
    auto it = json_->FindMember(name_.c_str());
    if (it == json_->MemberEnd()) return false;
    try
    {
      set_from_json(*parameter_, it->value);
    }
    catch (utils::ConfigurationException& e)
    {
      std::string err = e.what();
      static std::string prefix = "parameter `";
      if (std::string(e.what()).rfind(prefix) == 0)
      {
        throw utils::ConfigurationException(
            prefix + name_ + "." + err.substr(prefix.size()),
            e.get_error_code());
      }
      else
      {
        throw utils::ConfigurationException(prefix + name_ + "`: " + err,
                                           e.get_error_code());
      }
    }
    return true;
  }

  template <class T>
  static void reset_if_optional(T&)
  {
  }

  template <class T>
  static void reset_if_optional(std::optional<T>& parameter)
  {
    parameter.reset();
  }

  bool assigned_;
  Component_T* component_;
  const utils::Json* json_;
  std::string name_;
  Param_T* parameter_;
  std::string desc_;
};

}  // namespace utils

#endif  
