
#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "utils/component.h"
#include "utils/stream_handler.h"
#include "model/base_model.h"

namespace model
{
class Terms : public utils::Component
{
 public:
  class Stats
  {
   public:
    Stats();
    void clear();

    size_t terms;
    size_t variables;

    size_t max_vars_in_term;
    size_t max_terms_for_var;

    size_t constant_terms;
    size_t unary_terms;
    size_t binary_terms;
    size_t terms8;
    size_t terms32;

    size_t min_locality;
    size_t max_locality;
    double avg_locality;

    double constant_cost;
  };

  class Variable : public utils::Component
  {
   public:
    bool operator<(const Variable& other) const;
    utils::Structure render() const override;
    std::string get_class_name() const override { return "Variable"; }

    size_t id;
    int name;
    std::vector<size_t> term_ids;
  };

  class Term : public utils::Component
  {
   public:
    void configure(const utils::Json& json) override;
    bool operator<(const Term& other) const;
    utils::Structure render() const override;
    std::string get_class_name() const override { return "Term"; }

    size_t id;
    double cost;
    std::set<int> variable_names;
    std::vector<size_t> variable_ids;

    struct Get_Cost
    {
      static double& get(Term& term) { return term.cost; }
      static std::string get_key() { return "c"; }
    };

    struct Get_Variable_Names
    {
      static std::set<int>& get(Term& term) { return term.variable_names; }
      static std::string get_key() { return "ids"; }
    };

    using StreamHandler =
        utils::ObjectStreamHandler<utils::ObjectMemberStreamHandler<
            utils::BasicTypeStreamHandler<double>, Term, Get_Cost, true,
            utils::ObjectMemberStreamHandler<utils::SetStreamHandler<int>, Term,
                                            Get_Variable_Names, true>>>;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Terms Configuration
  ///
  /// This is a  class for an intermidiate loading of data for Terms class,
  /// which will be configured form this class by moving its data.
  /// Model configuration is used due to using same data in tests.
  class TermsConfiguration : public BaseModelConfiguration
  {
   public:
    struct Get_Terms
    {
      static std::vector<Term>& get(TermsConfiguration& config)
      {
        return config.terms_;
      }
      static std::string get_key() { return "terms"; }
    };

    using MembersStreamHandler = utils::ObjectMemberStreamHandler<
        utils::VectorObjectStreamHandler<typename Term::StreamHandler>,
        TermsConfiguration, Get_Terms, true,
        BaseModelConfiguration::MembersStreamHandler>;

    using StreamHandler = ModelStreamHandler<TermsConfiguration>;

   private:
    std::vector<Term> terms_;
  };

  using Configuration_T = TermsConfiguration;

  void configure(const utils::Json& json) override;

  void configure(Configuration_T& config);

  Stats stats;
  std::vector<Term> terms;
  std::vector<Variable> variables;

 private:
  void init();
};

}  // namespace model
