
#include "model/base_model.h"

namespace model
{
void BaseModel::configure(const utils::Json& json)
{
  if (!json.IsObject() || !json.HasMember(utils::kCostFunction))
  {
    const auto& model_name = this->get_identifier();
    THROW(utils::MissingInputException, "The configuration for ",
          utils::indef_article(model_name), " \'", this->get_identifier(),
          "\' model must contain a `cost_function` entry.");
  }
  std::string type;
  this->param(json[utils::kCostFunction], utils::kType, type)
      .required()
      .matches(::matcher::EqualTo(get_identifier()));
  std::string version;
  this->param(json[utils::kCostFunction], utils::kVersion, version)
      .description("version is expected")
      .required();
  match_version(version);
}

void BaseModel::configure(BaseModelConfiguration& configuration)
{
  const auto& model_name = BaseModelConfiguration::Get_Type::get(configuration);
  if (model_name != this->get_identifier())
  {
    THROW(utils::ValueException, "parameter `type`: must be equal to '",
          this->get_identifier(), "', found '", model_name, "'");
  }
  match_version(BaseModelConfiguration::Get_Version::get(configuration));
}

void BaseModel::configure(BaseModel* base)
{
  if (base == nullptr)
  {
    THROW(utils::ValueException, this->get_class_name(),
          " cannot be initialized from nullptr.");
  }
  THROW(utils::ValueException, this->get_class_name(),
        " cannot be initialized from ", base->get_class_name(), ".");
}

void BaseModel::init() {}

void BaseModel::match_version(const std::string& version)
{
  if (version.compare(get_version()) == 0)
  {
    return;
  }
  THROW(utils::ValueException, "Expecting `version` equals ", get_version(),
        ", but found: ", version);
}

}  // namespace model
