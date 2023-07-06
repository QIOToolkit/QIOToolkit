
#pragma once

#include <map>
#include <memory>
#include <string>

#include "utils/component.h"
#include "utils/structure.h"
#include "observe/observable.h"

namespace observe
{
class Observer : public utils::ComponentWithOutput
{
 public:
  Observer() : restart_step_(0) {}

  void restart(size_t step) { restart_step_ = step; }

  bool is_watching(const std::string& identifier) const;

  template <class T>
  void set_observable_label(const std::string& identifier, const T& value)
  {
    labels_[identifier] = value;
  }
  void clear_observable_label(const std::string& identifier);

  class ScopedLabel
  {
   public:
    ScopedLabel(Observer* observer, const std::string& label)
        : observer_(observer), label_(label)
    {
    }
    ScopedLabel(const ScopedLabel&) = default;
    ScopedLabel& operator=(const ScopedLabel& copy) = default;

    virtual ~ScopedLabel() { observer_->clear_observable_label(label_); }

   private:
    Observer* observer_;  // not owned
    std::string label_;
  };

  template <class T>
  ScopedLabel scoped_observable_label(const std::string& label, T value)
  {
    set_observable_label(label, value);
    return ScopedLabel(this, label);
  }

  void observe(const std::string& identifier, double value,
               double weight = 1.0);

  void configure(const utils::Json& json) override;

  utils::Structure render() const override;

 private:
  class Group : public utils::Component
  {
   public:
    using Label = std::pair<std::string, utils::Structure>;

    Group* select_subgroup(const Label& label);
    bool has_observable(const std::string& identifier);
    Observable& add_observable(const std::string& identifier,
                               std::unique_ptr<Observable>&& observable);
    Observable& select_observable(const std::string& identifier);

    utils::Structure render() const override;

   private:
    std::map<std::string, std::map<utils::Structure, Group>> groups_;
    std::map<std::string, std::unique_ptr<Observable>> observables_;
  };

  // "observer": {
  //   "average_over": { "step": 100 },
  //   "group_by": "replica",
  //   "constant": ["T", "beta"],
  //   "average": ["cost", "delta_beta", "population",
  //      "family_count" "rho_t", "sigma", "tau", "zeta"]
  //   "histogram": {
  //     "family_size": { "bin_width": 1 }
  //   }
  // }

  class ObservableFactory
  {
   public:
    virtual ~ObservableFactory() {}
    virtual std::unique_ptr<Observable> create_observable() const = 0;
  };

  template <class T>
  class Factory : public ObservableFactory
  {
   public:
    virtual std::unique_ptr<Observable> create_observable() const
    {
      return std::unique_ptr<Observable>(new T());
    }
  };

  class Protocol : public utils::Component
  {
   public:
    void configure(const utils::Json& json) override;
    bool is_watching(const std::string& name) const;
    Group* select_group(Group& groups, size_t restart_step,
                        std::map<std::string, utils::Structure> labels_) const;
    std::unique_ptr<Observable> create_observable(
        const std::string& name) const;

   private:
    std::map<std::string, size_t> average_over_;
    std::set<std::string> group_by_;
    std::map<std::string, ObservableFactory*> observables_;
  };

  size_t restart_step_;
  utils::Structure config_;
  std::map<std::string, utils::Structure> labels_;
  Group observables_;
  std::vector<Protocol> protocols_;
};

}  // namespace observe
