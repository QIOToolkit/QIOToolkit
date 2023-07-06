---
title: Configuration
description: This explains setting up configuration infrastructure.
topic: article
uid: qiotoolkit.dev.configuration
---

Configuration
=============

qiotoolkit includes boiler plate to initialize C++ objects from json. For this,
the class should be derived from `utils::Component` and override the
```cpp
  virtual void configure(const utils::Json& json) {...}
```
method inherited from the base class.

Required Parameters
----------
A `Parameter` can be used to read a variable value from configuration.
The chained call to `required()` signals that an exception should be
thrown if the specified field name is not found:

```cpp
#include "utils/component.h"

class MyModel : public utils::Component {
 public:

  void configure(const utils::Json& json) {
    this->param(json, "my_param", my_param_).required();
  }

 private:
  int my_param_;
};
```

This will read an integer from the JSON field `my_param` (see sample input
below) and store its value in the class field `my_param_`. It throws a
`ConfigurationException` if the field is not set or cannot be parsed as an
integer:

```json
{
  "my_param": 42
}
```

Matchers
--------
Depending on the meaning of `my_param`, not all integer values might make
sense as input. For this reason, you maybe specify additional requirements
to be checked on a `ParameterBuilder` using `matches()`:

```cpp
this->param(json, "my_param", my_param_)
    .matches(::matchers::GreaterThan(0))
    .required();
```

This will throw a configuration exception if `my_param` is not specified,
not an integer *OR* if its value is not greater than 0.

Optional Parameters
-------------------
Parameters are implicitly optional (if `required()` is not specifeid).
In this case, you can decide how to handle the absence of the parameter
in the input:
```cpp
this->param(json, "my_param", my_param_);
```
This does not throw a `ConfigurationException` if `my_param` is not set
and does not change value of `my_param_` (unless `my_param_` is of `std::optional<T>` type,
in which case it is reset).

Specifying a default value:
```cpp
this->param(json, "my_param", my_param_)
    .default_value(3);
```
assigns a specific default value (3) instead.

For optional `Parameter`s, you can also combine a `matches()` and `default_value()`:

```cpp
this->param(json, "my_param", my_param_)
    .matches(::matcher::GreaterThan(0))
    .default_value(3);
```
which tells the program to read from the configuration if the value is set (and check
that it is greater than 0), or assign 3 otherwise.

> [!NOTE]
> The default value is only checked against the matcher if it is chained before.

Vector Parameters
-----------------
Configuration works for certain container data types in the same way as it does for
primitive types. For instance, you can denote a `std::vector<int>` as an expected
input parameter:

```cpp
#include "../utils/component.h"

class MyComponent : public utils::Component {
 public:
  void configure(const utils::Json& json) {
    this->param(json, "numbers", numbers).required();
  }

 private:
  std::vector<int> numbers_;
};
```

Expects the input to have an array-type field `numbers`, the contents of which must
all be integers. During `configure(...)`, these numbers are filled into the `std::vector`:

```json
{
  "numbers": [1,2,3]
}
```

Complex Parameters
------------------
Configuration works for any `utils::Component` (by invoking its `configure` method). This
means that you can use components as parameter types:

```cpp
class MyParamType : public utils::Component {
 public:
  void configure(const utils::Json& json) override {
    this->param(json, "a", a_).required();
    this->param(json, "b", b_).required();
  }

 private:
  int a_, b_;
};

class MyModel : public utils::Component {
 public:
  void configure(utils::Config& config) override {
    this->param(json, "my_param", my_param_).required();
  }
 
 private:
  MyParamType my_param_;
}
```

The json input for `MyModel` is expected to have a field which
contains the input for a `MyParamType`:

```json
{
  "my_param": {
    "a": 13,
    "b": 42
  }
}
```
