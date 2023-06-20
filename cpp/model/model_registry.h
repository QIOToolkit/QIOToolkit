// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <map>
#include <memory>
#include <string>

#include "utils/exception.h"
#include "utils/log.h"
#include "model/base_model.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// Interface for an individual Model's registry entry
///
/// For now this only allows instantiating an instance of the model.
///
class ModelRegistrationInterface
{
 public:
  virtual ~ModelRegistrationInterface() {}
  /// Create an instance of the model
  virtual BaseModel* create() const = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// Registry for model selection
///
/// The model registry allows models to be instantiated from their identifier
///
/// Usage:
///
///   * Registering a new model:
///
///     ```c++
///     #include "model/model/model_registry.h"
///
///     class MyModel : public model::BaseModel {
///      public:
///       // Must have a constructor without arguments.
///       MyModel() = default;
///
///       // This is the identifier the model is registered with.
///       std::string get_identifier() const override {
///         return "my_model";
///       }
///       ...
///     }
///     REGISTER_MODEL(MyModel);
///     ```
///
///   * Instantiating a model by identifier:
///
///     ```c++
///     auto* my_model = model::ModelRegistry::create("my_model");
///     ```
///
class ModelRegistry
{
 public:
  /// Check whether a model with that identifier is registered.
  static bool has(const std::string& identifier)
  {
    return instance().registry_.find(identifier) != instance().registry_.end();
  }

  /// Find and return the model registration
  static const ModelRegistrationInterface* get(const std::string& identifier)
  {
    auto found = instance().registry_.find(identifier);
    if (found == instance().registry_.end())
    {
      THROW(utils::KeyDoesNotExistException,
            "There is no model with identifier '", identifier, "'.");
    }
    return found->second.get();
  }

  /// Find the model registration and create an instance.
  static BaseModel* create(const std::string& identifier)
  {
    auto* registration = get(identifier);
    return registration->create();
  }

  /// Add an entry to the model registry.
  ///
  /// This is used by the ModelRegistrar in the `REGISTER_MODEL` macro,
  /// you should not need to invoke it directly.
  static void add(const std::string& identifier,
                  std::unique_ptr<ModelRegistrationInterface>&& registration)
  {
    if (has(identifier)) return;
    instance().registry_[identifier] = std::move(registration);
  }

 private:
  /// This is a singleton class.
  ModelRegistry() {}

  /// Access the singleton.
  static ModelRegistry& instance()
  {
    static ModelRegistry registry;
    return registry;
  }

  // If we want to register aliases, this might have to become a shared
  // pointer (and add will need to acceept an additional shared pointer,
  // rather than being templated).
  std::map<std::string, std::unique_ptr<ModelRegistrationInterface>> registry_;
};

////////////////////////////////////////////////////////////////////////////////
/// Shared implementation of the Registry entry (templated)
template <class Model_T>
class ModelRegistrationImpl : public ModelRegistrationInterface
{
 public:
  /// Create an instance of the model
  BaseModel* create() const override { return new Model_T(); }
};

/// Helper struct to statically fill the registry.
template <class Model_T>
struct ModelRegistrar
{
  ModelRegistrar(int)
  {
    std::string identifier = Model_T().get_identifier();
    ModelRegistry::add(identifier, std::unique_ptr<ModelRegistrationInterface>(
                                       new ModelRegistrationImpl<Model_T>));
  }
};

#define REGISTER_MODEL(Model_T) \
  static ::model::ModelRegistrar<Model_T> registrar##Model_T(0)

}  // namespace model
