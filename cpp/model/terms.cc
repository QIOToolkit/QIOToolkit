// Copyright (c) Microsoft. All Rights Reserved.
#include "model/terms.h"

#include "utils/config.h"
#include "utils/timing.h"

namespace model
{
Terms::Stats::Stats() { clear(); }
void Terms::Stats::clear()
{
  terms = 0;
  variables = 0;

  max_vars_in_term = 0;
  max_terms_for_var = 0;

  min_locality = 0;
  max_locality = 0;
  avg_locality = 0;

  constant_terms = 0;
  unary_terms = 0;
  binary_terms = 0;
  terms8 = 0;
  terms32 = 0;

  constant_cost = 0;
}

bool Terms::Variable::operator<(const Variable& other) const
{
  for (size_t i = 0; i < term_ids.size() && i < other.term_ids.size(); i++)
  {
    size_t a = term_ids[i];
    size_t b = other.term_ids[i];
    if (a == b) continue;
    return a < b;
  }
  // return term_ids.size() > other.term_ids.size();
  return false;
}

utils::Structure Terms::Variable::render() const
{
  utils::Structure s;
  s["id"] = id;
  s["name"] = name;
  s["term_ids"] = term_ids;
  return s;
}

void Terms::Term::configure(const utils::Json& json)
{
  this->param(json, "c", cost)
      .description("the cost coefficient of this term")
      .required();
  this->param(json, "ids", variable_names)
      .description("the ids of variables participating in this term")
      .required();
}

bool Terms::Term::operator<(const Term& other) const
{
  return variable_names.size() > other.variable_names.size();
}

utils::Structure Terms::Term::render() const
{
  utils::Structure s;
  s["id"] = id;
  s["cost"] = cost;
  s["variable_ids"] = variable_ids;
  return s;
}

void Terms::configure(const utils::Json& json)
{
  // Read the terms.
  {
    utils::ScopedTimer timer("configure terms");
    this->param(json, "terms", terms)
        .description("the terms of the cost function")
        .required();
  }
  init();
}

void Terms::configure(Configuration_T& config)
{
  // Move the terms
  utils::ScopedTimer timer("configure terms");
  terms = std::move(Configuration_T::Get_Terms::get(config));
  init();
}

void Terms::init()
{
  // sort terms by decreasing arity.

  stats.clear();
  stats.terms = terms.size();
  // sort terms by arity.
  {
    utils::ScopedTimer timer("sort terms");
    std::stable_sort(terms.begin(), terms.end());
  }

  std::map<int, size_t> name_map;
  {
    utils::ScopedTimer timer("iterate terms");
    for (size_t j = 0; j < terms.size(); j++)
    {
      auto& term = terms[j];
      terms[j].id = j;
      size_t locality = term.variable_names.size();
      stats.max_vars_in_term =
          std::max(stats.max_vars_in_term, term.variable_names.size());
      if (stats.min_locality == 0 || locality < stats.min_locality)
      {
        stats.min_locality = locality;
      }
      if (stats.max_locality == 0 || locality > stats.max_locality)
      {
        stats.max_locality = locality;
      }
      stats.avg_locality += static_cast<double>(locality);
      switch (term.variable_names.size())
      {
        case 0:
          stats.constant_terms++;
          stats.constant_cost += term.cost;
          break;
        case 1:
          stats.unary_terms++;
          break;
        case 2:
          stats.binary_terms++;
          break;
        default:
          if (term.variable_names.size() <= 255)
          {
            stats.terms8++;
          }
          else if (term.variable_names.size() <=
                   std::numeric_limits<uint32_t>::max())
          {
            stats.terms32++;
          }
          else
          {
            throw utils::NotImplementedException(
                "Terms with more than 4294967295 variables are not supported.");
          }
          break;
      }
      for (const auto& name : term.variable_names)
      {
        name_map[name] = 0;
      }
    }
  }
  stats.avg_locality /= static_cast<double>(stats.terms - stats.constant_terms);

  stats.variables = name_map.size();
  variables.resize(name_map.size());
  size_t i = 0;
  {
    utils::ScopedTimer timer("label variables");
    for (auto& it : name_map)
    {
      auto& var = variables[i];
      var.name = it.first;
      it.second = i++;
    }
  }

  {
    utils::ScopedTimer timer("associate terms to ids");
    for (auto& term : terms)
    {
      for (const auto& name : term.variable_names)
      {
        variables[name_map[name]].term_ids.push_back(term.id);
      }
      term.variable_ids.reserve(term.variable_names.size());
      term.variable_names.clear();
    }
  }
  name_map.clear();

  {
    utils::ScopedTimer timer("sort variables");
    std::stable_sort(variables.begin(), variables.end());
  }
  i = 0;

  {
    utils::ScopedTimer timer("list variable ids in terms");
    for (auto& var : variables)
    {
      stats.max_terms_for_var =
          std::max(stats.max_vars_in_term, var.term_ids.size());
      var.id = i++;
      for (auto term_id : var.term_ids)
      {
        terms[term_id].variable_ids.push_back(var.id);
      }
    }
  }
}

}  // namespace model
