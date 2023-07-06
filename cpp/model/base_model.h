
#pragma once

#include "utils/component.h"
#include "utils/config.h"
#include "utils/language.h"
#include "utils/stream_handler.h"

namespace model
{
template <class ConfigurationType>
struct Get_Model
{
  static ConfigurationType& get(ConfigurationType& m) { return m; }
  static std::string get_key() { return utils::kCostFunction; }
};

template <class ConfigurationType,
          class MembersStreamHandlerType =
              class ConfigurationType::MembersStreamHandler,
          bool stop_early = false>
using ModelStreamHandler = utils::ObjectStreamHandler<
    utils::ObjectMemberStreamHandler<
        utils::ObjectStreamHandler<MembersStreamHandlerType>, ConfigurationType,
        Get_Model<ConfigurationType>, true,
        utils::AnyKeyObjectMemberStreamHandler<ConfigurationType>>,
    stop_early>;

class BaseModelConfiguration
{
 public:
  struct Get_Version
  {
    static std::string& get(BaseModelConfiguration& m) { return m.version_; }
    static std::string get_key() { return "version"; }
  };

  struct Get_Type
  {
    static std::string& get(BaseModelConfiguration& m) { return m.type_; }
    static std::string get_key() { return "type"; }
  };

  using MembersStreamHandler = utils::ObjectMemberStreamHandler<
      utils::BasicTypeStreamHandler<std::string>, BaseModelConfiguration,
      Get_Version, true,
      utils::ObjectMemberStreamHandler<
          utils::BasicTypeStreamHandler<std::string>, BaseModelConfiguration,
          Get_Type, true,
          utils::AnyKeyObjectMemberStreamHandler<BaseModelConfiguration>>>;

 protected:
  std::string type_;
  std::string version_;
};

class BaseModelPreviewConfiguration : public BaseModelConfiguration
{
 public:
  using StreamHandler = ModelStreamHandler<BaseModelPreviewConfiguration,
                                           MembersStreamHandler, true>;
};

/// BaseModel class (non-markov specific)
///
/// Base class implementing methods needed for model selection.
/// A pointer to this base class can be used by the owner of the
/// model (through it needs to be dynamic-cast to a pointer to
/// the specific model type when setting it on the solver).
///
/// > [!NOTE]
/// > This is different from the markov::Model class which has
/// > the interfaces to interact with markov-based optimizers.
class BaseModel : public utils::Component
{
 public:
  BaseModel() = default;
  virtual ~BaseModel() = default;

  /// Returns the identifier string of the model type (e.g., "ising").
  virtual std::string get_identifier() const = 0;

  /// Returns the version of the input format this implementation expects.
  virtual std::string get_version() const = 0;

  /// Populates model internals from the input json.
  void configure(const utils::Json& json) override;

  /// Populates model internals from the input configuration.
  void configure(BaseModelConfiguration& configuration);

  /// Populates the model from a related model
  /// The trelated model is considered useless after this.
  virtual void configure(BaseModel* base);

  /// Initializes internal data structures
  /// (guaranteed to be called after `configure()`).
  virtual void init();

  /// Checks the string argument against the expected version.
  ///
  /// This can be overloaded to accept other version than the one
  /// returned by `get_version()`.
  virtual void match_version(const std::string& version);
};

}  // namespace model
