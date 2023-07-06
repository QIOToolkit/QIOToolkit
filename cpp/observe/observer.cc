
#include "observe/observer.h"

#include "utils/config.h"

namespace observe
{
void Observer::clear_observable_label(const std::string& identifier)
{
  auto found = labels_.find(identifier);
  if (found == labels_.end())
  {
    LOG(WARN, "attempting to clear unset label `", identifier, "`");
    return;
  }
  labels_.erase(found);
}

bool Observer::is_watching(const std::string& name) const
{
  for (const auto& protocol : protocols_)
  {
    if (protocol.is_watching(name)) return true;
  }
  return false;
}

void Observer::observe(const std::string& identifier, double value,
                       double weight)
{
  if (!is_watching(identifier)) return;
  for (const auto& protocol : protocols_)
  {
    if (protocol.is_watching(identifier))
    {
      Group* group =
          protocol.select_group(observables_, restart_step_, labels_);
      if (!group->has_observable(identifier))
      {
        group->add_observable(identifier,
                              protocol.create_observable(identifier));
      }
      group->select_observable(identifier).record(value, weight);
    }
  }
}

utils::Structure Observer::render() const { return observables_.render(); }

void Observer::configure(const utils::Json& json)
{
  if (!json.IsObject() || !json.HasMember("observer")) return;
  if (!json["observer"].IsArray())
  {
    protocols_.resize(1);
    this->param(json, "observer", protocols_[0])
        .description("confiugration of the measurement protocol")
        .required();
  }
  else
  {
    this->param(json, "observer", protocols_)
        .description("configuration of the measurement protocols")
        .required();
  }
}

Observer::Group* Observer::Group::select_subgroup(
    const Observer::Group::Label& label)
{
  return &groups_[label.first][label.second];
}

bool Observer::Group::has_observable(const std::string& identifier)
{
  return observables_.find(identifier) != observables_.end();
}

Observable& Observer::Group::add_observable(
    const std::string& identifier, std::unique_ptr<Observable>&& observable)
{
  observables_[identifier] = std::move(observable);
  return *observable;
}

Observable& Observer::Group::select_observable(const std::string& identifier)
{
  return *observables_[identifier];
}

utils::Structure Observer::Group::render() const
{
  utils::Structure s;
  for (const auto& it : groups_)
  {
    for (const auto& jt : it.second)
    {
      s[it.first].push_back(jt.second.render());
    }
  }
  for (const auto& it : observables_)
  {
    s[it.first] = it.second->render();
  }
  return s;
}

void Observer::Protocol::configure(const utils::Json& json)
{
  if (!json.IsObject())
  {
    throw utils::InvalidTypesException("Expected object");
  }
  if (json.HasMember("average_over"))
  {
    this->param(json, "average_over", average_over_)
        .description("over which labels to average")
        .matches(matcher::Not(matcher::IsEmpty()))
        .required();
  }
  if (json.HasMember("group_by"))
  {
    this->param(json, "group_by", group_by_)
        .description("the labels to group by")
        .matches(matcher::Not(matcher::IsEmpty()))
        .required();
  }
  if (json.HasMember("constant"))
  {
    std::vector<std::string> names;
    this->param(json, "constant", names)
        .description("the observables to measure as constants");
    for (const std::string& name : names)
    {
      observables_[name] = new Factory<observe::Constant>();
    }
  }
  if (json.HasMember("average"))
  {
    std::vector<std::string> names;
    this->param(json, "average", names)
        .description("the observables to measure as constants");
    for (const std::string& name : names)
    {
      observables_[name] = new Factory<observe::Average>();
    }
  }
}

bool Observer::Protocol::is_watching(const std::string& name) const
{
  return observables_.find(name) != observables_.end();
}

Observer::Group* Observer::Protocol::select_group(
    Group& groups, size_t restart_step,
    std::map<std::string, utils::Structure> labels_) const
{
  Group* group = &groups;
  for (const auto& average : average_over_)
  {
    auto label = labels_.find(average.first);
    if (label == labels_.end())
    {
      LOG(ERROR, "observable not grouped by ", average.first);
      continue;
    }
    Group::Label grouped = *label;
    size_t value = label->second.get<size_t>();
    grouped.second = value - (value - restart_step) % average.second;
    group = group->select_subgroup(grouped);
  }
  for (const auto& grouping : group_by_)
  {
    auto label = labels_.find(grouping);
    if (label == labels_.end())
    {
      LOG(ERROR, "observable not grouped by ", grouping);
      continue;
    }
    group = group->select_subgroup(*label);
  }
  return group;
}

std::unique_ptr<Observable> Observer::Protocol::create_observable(
    const std::string& name) const
{
  return observables_.find(name)->second->create_observable();
}

}  // namespace observe
