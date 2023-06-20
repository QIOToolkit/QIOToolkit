// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <map>
#include <string>

#include "utils/json.h"
#include "utils/optional.h"
#include "utils/parameter_builder.h"
#include "utils/structure.h"
#include "matcher/matchers.h"

namespace utils
{
////////////////////////////////////////////////////////////////////////////////
/// Component base class
///
/// The component base class provides quality-of-life functionality for any
/// class derived from it:
///
///   * It can be initialized from json via its `configure()` method
///   * It is rendered in "structured" form via `render()` when assigned
///     to an `utils::Structure`.
///   * Its class name is accessible via `get_class_name()`
///   * When passed to an ostream (or `LOG`), it is represented as
///     '<ClassName: status>' where the status is given by `get_status()`
///
/// While `get_class_name()` works out-of-the-box, the appropriate functionality
/// must be implemented by overloading the others accordingly.
class Component
{
 public:
  Component() {}
  virtual ~Component() {}

  //////////////////////////////////////////////////////////////////////////////
  /// configure the object from input
  ///
  /// Initialize the object's state from the input `utils::Config`. This is done
  /// by declaring which required and optional parameters are associated with
  /// the fields of this object. During initialization, they are checked for
  /// their presence, type and any matchers.
  ///
  /// Example:
  ///
  /// ```c++
  /// MyClass : public Component {
  ///  public:
  ///   void configure(const utils::Json& json) override {
  ///     this->param(json, "number", my_number)
  ///         .description("some description")
  ///         .matches(GreaterEquals(0))
  ///         .required();
  ///     this->param(json, "name", my_name)
  ///         .description("some description")
  ///         .matches(SizeIs(GreaterThan(0)))
  ///         .default_value("no_name");
  ///   }
  ///
  ///  private:
  ///   int my_number;
  ///   std::string my_name;
  /// }
  ///
  /// MyClass my_object;
  /// my_object.configure(utils::json_from_string(R"(
  ///   {
  ///     "number": 42,
  ///     "name": "hello"
  ///   }
  /// )"));
  /// ```
  ///
  /// NOTE: By default, nothing is configured from input. You need to overload
  /// this method if you want to use this functionality. Don't forget to call
  /// the configure method of the parent class if it also needs to be configured
  /// (this is not the case for `utils::Component` itself).
  ///
  /// HINT: Parameters are not limited to scalars and strings; Any component can
  /// be a parameter; in which case it is initialized using its own configure
  /// method.
  ///
  /// @see utils::Json
  ///
  virtual void configure(const utils::Json& json);

  /// Assign a parameter from a json field.
  template <class T>
  ParameterBuilder<utils::Component, T> param(const utils::Json& json,
                                             const std::string& name,
                                             T& parameter)
  {
    return ParameterBuilder<utils::Component, T>(this, json, name, parameter);
  }

  //////////////////////////////////////////////////////////////////////////////
  /// render the object in structured form
  ///
  /// Return a structured representation of the object. This is intended for
  /// output purposes. For instance, the solution your solver finds should
  /// have a render method which allows it to be returned as part of the
  /// result.
  ///
  /// Example:
  ///
  ///   ```c++
  ///   MySolution : public Component {
  ///    public:
  ///     // Represent the internal bool vector as +-1 output.
  ///     utils::Structure render() const override {
  ///       utils::Structure rendered;
  ///       for (bool item : solution_) rendered.push_back(item ? 1 : -1);
  ///       return rendered;
  ///     }
  ///
  ///    private:
  ///     std::vector<bool> solution_;
  ///   }
  ///
  ///   MySolution solution;
  ///   std::cout << solution.render().to_string() << std::endl;
  ///   ```
  ///
  /// @see utils::Structure
  ///
  virtual utils::Structure render() const;

  //////////////////////////////////////////////////////////////////////////////
  /// get_status shows a simplified state representation
  ///
  /// Like `render`, this produces a structured representation of the object's
  /// state. However, it is intended to be simpler in nature with the purpose
  /// of rendering the object during stream-output and logging. By default, it
  /// will fall back to the full `render`, but overloading this allows
  /// distinguising how the object looks during LOG vs full output.
  virtual utils::Structure get_status() const;

  //////////////////////////////////////////////////////////////////////////////
  /// get_class_name shows an identifier of the (derived) class name
  ///
  /// Its primary use is for type identification during stream output and
  /// logging, where a component is rendered as `<ClassName: status>`.
  ///
  /// Example:
  ///
  ///   ```c++
  ///   MyClass : public Component {}
  ///
  ///   // This will render as '<MyClass>'
  ///   MyClass my_object;
  ///   std::cout << my_object << std::endl;
  ///   ```
  ///
  /// NOTE: You do not need to overload this method unless you want to change
  /// the output of the default implementation.
  virtual std::string get_class_name() const;
};

class ComponentWithOutput : public Component
{
 public:
  /// Assign a parameter from a json field.
  template <class T>
  ParameterBuilder<utils::ComponentWithOutput, T> param(const utils::Json& json,
                                                       const std::string& name,
                                                       T& parameter)
  {
    return ParameterBuilder<utils::ComponentWithOutput, T>(this, json, name,
                                                          parameter);
  }

  /// Store a key, value pair in the output_parameters (to be returned to the
  /// user).
  template <class T>
  void set_output_parameter(const std::string& key, const T& value)
  {
    output_parameters_[key] = value;
  }

  /// Store the value of an optional in the output_parameters if it is set.
  template <class T>
  void set_output_parameter(const std::string& key,
                            const std::optional<T>& value)
  {
    if (value.has_value())
    {
      output_parameters_[key] = *value;
    }
  }

  /// Get a structure containing all output_parameters.
  virtual utils::Structure get_output_parameters() const
  {
    return output_parameters_;
  }

 private:
  utils::Structure output_parameters_;
};

/// Helper function to configure a Component from json
/// (by calling its configure method).
void set_from_json(utils::Component& target, const utils::Json& json);

////////////////////////////////////////////////////////////////////////////////
/// Overload for the shift operator in conjunction with streams.
///
/// With this, you can pipe any component into a C++ stream and it will
/// be rendered as either
///
///   * <{class_name}>  if its get_status is empty (default behavior)
///   * <{class_name}: {status}>  if it returns a non empty `get_status`.
std::ostream& operator<<(std::ostream& ostr, const utils::Component& s);

}  // namespace utils
