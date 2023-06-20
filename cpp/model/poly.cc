// Copyright (c) Microsoft. All Rights Reserved.
#include "model/poly.h"

#include <algorithm>

namespace model
{
utils::Structure PolyState::render() const
{
  utils::Structure s;
  for (size_t i = 0; i < spins.size(); i++)
  {
    s[std::to_string(i)] = spins[i] ? -1 : 1;
  }
  return s;
}

PolyState Poly::create_state(std::vector<int> spins,
                             std::vector<double> parameters) const
{
  PolyState state;
  // Translate spin values to their boolean representaiton.
  for (const auto& spin : spins)
  {
    assert(spin * spin == 1);
    state.spins.push_back(spin == -1);
  }
  // Set default values if parameters are not given.
  state.parameters = parameters;
  if (state.parameters.empty())
  {
    state.parameters.resize(parameter_names_.size(), 1);
  }
  // Calculate the intermediate term values.
  // (This also checks that spin and parameter have the right length.)
  initialize_terms(state);
  return state;
}

PolyState Poly::get_random_state(utils::RandomGenerator& rng) const
{
  PolyState state;
  state.spins.resize(spin_to_terms_.size());
  for (size_t i = 0; i < state.spins.size(); i++)
  {
    state.spins[i] = rng.uniform() < 0.5;
  }
  state.parameters.resize(parameter_names_.size(), 1.0);
  initialize_terms(state);
  return state;
}

PolyTransition Poly::get_random_transition(const PolyState&,
                                           utils::RandomGenerator& rng) const
{
  return create_spin_flip((size_t)floor(
      rng.uniform() * static_cast<double>(spin_to_terms_.size())));
}

PolyTransition Poly::create_spin_flip(size_t spin_id) const
{
  PolyTransition spin_flip;
  spin_flip.set_spin_id(spin_id);
  return spin_flip;
}

PolyTransition Poly::create_parameter_change(
    const std::vector<double> values) const
{
  PolyTransition parameter_change;
  parameter_change.set_parameter_values(values);
  return parameter_change;
}

namespace
{
// Slightly faster version of pow for (small) integers.
double mypow(double d, int e)
{
  switch (e)
  {
    case 0:
      return 1;
    case 1:
      return d;
    case 2:
      return d * d;
    case 3:
      return d * d * d;
    default:
      return pow(d, e);
  }
}

}  // namespace

double Poly::calculate_cost(const PolyState& state) const
{
  return term_factor(state.parameters, 0) *
         mypow(state.terms[0], term_[0].exponent);
}

double Poly::calculate_cost_difference(const PolyState& state,
                                       const PolyTransition& transition) const
{
  auto term_diff = calculate_term_differences(state, transition);
  size_t term_id = 0;
  double old_value = state.terms[0];
  double new_value = old_value + term_diff[0];
  if (transition.is_spin_flip())
  {
    return term_factor(state.parameters, 0) *
           (mypow(new_value, term_[term_id].exponent) -
            mypow(old_value, term_[term_id].exponent));
  }
  else
  {
    return term_factor(transition.parameter_values(), term_id) *
               mypow(new_value, term_[term_id].exponent) -
           term_factor(state.parameters, term_id) *
               mypow(old_value, term_[term_id].exponent);
  }
}

void Poly::apply_transition(const PolyTransition& transition,
                            PolyState& state) const
{
  auto term_differences = calculate_term_differences(state, transition);
  for (auto& term_and_diff : term_differences)
  {
    state.terms[term_and_diff.first] += term_and_diff.second;
  }
  if (transition.is_spin_flip())
  {
    state.spins[transition.spin_id()] = !state.spins[transition.spin_id()];
  }
  else
  {
    state.parameters = transition.parameter_values();
  }
}

void Poly::configure_term(const utils::Json& input, int parent)
{
  size_t term_id = term_.size();
  term_.push_back(Term(parent));
  Term& term = term_.back();
  this->param(input, "constant", term.constant)
      .description("The constant of this term")
      .default_value(1.0);
  std::string parameter_name;
  this->param(input, "parameter", parameter_name)
      .description("Name of the parameter preceding this term");
  if (!parameter_name.empty())
  {
    size_t parameter_id;
    auto it = std::find(parameter_names_.begin(), parameter_names_.end(),
                        parameter_name);
    if (it != parameter_names_.end())
    {
      parameter_id = (size_t)(it - parameter_names_.begin());
    }
    else
    {
      parameter_id = parameter_names_.size();
      parameter_names_.push_back(parameter_name);
      parameter_to_terms_.resize(parameter_names_.size());
    }
    term.parameter_id = (int)parameter_id;
    parameter_to_terms_[parameter_id].push_back(term_id);
  }
  if (input.HasMember("terms"))
  {
    term.is_leaf = false;
    this->param(input, "exponent", term.exponent)
        .description("The exponent to apply to the terms")
        .default_value(1);
    assert(!input.HasMember("ids"));
    for (size_t i = 0; i < input["terms"].Size(); i++)
    {
      configure_term(input["terms"][i], static_cast<int>(term_id));
    }
  }
  else
  {
    term.is_leaf = true;
    term.exponent = 1;
    assert(!input.HasMember("exponent"));
    std::vector<size_t> ids;
    this->param(input, "ids", ids)
        .description("Ids of spins participating in this term");
    for (const auto& id : ids)
    {
      spin_to_terms_.resize(std::max(size_t(id) + 1, spin_to_terms_.size()));
      spin_to_terms_[id].push_back(term_id);
    }
  }
}

void Poly::configure(const utils::Json& json)
{
  markov::Model<PolyState, PolyTransition>::configure(json);
  if (!json.IsObject() || !json.HasMember(utils::kCostFunction))
  {
    THROW(utils::MissingInputException,
          "The configuration of a pubo model must contain a `cost_function` "
          "entry.");
  }
  configure_term(json[utils::kCostFunction]);
}

void Poly::configure_term(PolySpinsConfiguration& input, int parent)
{
  size_t term_id = term_.size();
  term_.push_back(Term(parent));
  Term& term = term_.back();
  term.constant = input.constant;
  for (const auto& id : input.ids)
  {
    spin_to_terms_.resize(std::max(size_t(id) + 1, spin_to_terms_.size()));
    spin_to_terms_[id].push_back(term_id);
  }
}

void Poly::configure_term(PolyTermConfiguration& input, int parent)
{
  size_t term_id = term_.size();
  term_.push_back(Term(parent));
  Term& term = term_.back();
  term.constant = input.constant;
  std::string& parameter_name = input.parameter;

  if (!parameter_name.empty())
  {
    size_t parameter_id;
    auto it = std::find(parameter_names_.begin(), parameter_names_.end(),
                        parameter_name);
    if (it != parameter_names_.end())
    {
      parameter_id = (size_t)(it - parameter_names_.begin());
    }
    else
    {
      parameter_id = parameter_names_.size();
      parameter_names_.push_back(parameter_name);
      parameter_to_terms_.resize(parameter_names_.size());
    }
    term.parameter_id = (int)parameter_id;
    parameter_to_terms_[parameter_id].push_back(term_id);
  }
  if (input.terms.size() > 0)
  {
    term.is_leaf = false;
    term.exponent = input.exponent;
    auto& terms = input.terms;
    assert(input.ids.size() == 0);
    for (size_t i = 0; i < terms.size(); i++)
    {
      configure_term(terms[i], static_cast<int>(term_id));
    }
  }
  else if (input.ids.size() > 0)
  {
    for (const auto& id : input.ids)
    {
      spin_to_terms_.resize(std::max(size_t(id) + 1, spin_to_terms_.size()));
      spin_to_terms_[id].push_back(term_id);
    }
  }
}

void Poly::configure_term(std::vector<PolyTermConfiguration>& input, int parent)
{
  size_t term_id = term_.size();
  term_.push_back(Term(parent));
  Term& term = term_.back();
  term.is_leaf = false;
  for (size_t i = 0; i < input.size(); i++)
  {
    configure_term(input[i], static_cast<int>(term_id));
  }
}

void Poly::configure(Configuration_T& configuration)
{
  markov::Model<PolyState, PolyTransition>::configure(configuration);
  configure_term(configuration.terms);
}

std::string Poly::print() const
{
  std::string result = term_to_string(0);
  while (result.size() >= 2 && result[0] == '(' &&
         result[result.size() - 1] == ')')
  {
    result = result.substr(1, result.size() - 2);
  }
  return result;
}

std::vector<std::string> Poly::get_parameters() const
{
  return parameter_names_;
}

void Poly::initialize_terms(PolyState& state) const
{
  // Check that the state has spins and parameters
  assert(state.spins.size() == spin_to_terms_.size());
  assert(state.parameters.size() == parameter_to_terms_.size());
  // Clear the terms and initialize them to 0 or 1, depending on whether
  // they are leaf nodes (product -> 1) or inner nodes (sum -> 0).
  state.terms.clear();
  state.terms.resize(term_.size());
  for (size_t term_id = 0; term_id < term_.size(); term_id++)
  {
    state.terms[term_id] = term_[term_id].is_leaf ? 1 : 0;
  }
  // Multiply the spin values into their respective leaf terms.
  for (size_t spin_id = 0; spin_id < state.spins.size(); spin_id++)
  {
    if (state.spins[spin_id])
    {
      for (size_t term_id : spin_to_terms_[spin_id])
      {
        state.terms[term_id] *= -1;
      }
    }
  }
  // Add the term values into their parent term's intermediate values.
  for (size_t term_id = term_.size() - 1; term_id > 0; term_id--)
  {
    state.terms[static_cast<size_t>(term_[term_id].parent_id)] +=
        term_factor(state.parameters, term_id) *
        mypow(state.terms[term_id], term_[term_id].exponent);
  }
}

std::map<size_t, double> Poly::calculate_term_differences(
    const PolyState& state, const PolyTransition& transition) const
{
  const std::vector<double>* new_parameters = &state.parameters;
  std::map<size_t, double> term_diff;
  // Denote the initial differences cause by the flip or parameter change.
  if (transition.is_spin_flip())
  {
    for (size_t term_id : spin_to_terms_[transition.spin_id()])
    {
      if (term_diff.find(term_id) == term_diff.end()) term_diff[term_id] = 0;
      term_diff[term_id] += state.spins[transition.spin_id()] ? 2 : -2;
    }
  }
  else
  {
    new_parameters = &transition.parameter_values();
    for (const auto& parameter_terms : parameter_to_terms_)
    {
      for (size_t term_id : parameter_terms)
      {
        // Adding a 0-diff element to term_diffs has the effect of forcing
        // recalculating from that node (parameter changes don't affect the
        // value of the sum in brackets, but how the diff bubbles to
        // parent nodes).
        term_diff[term_id] = 0;
      }
    }
  }
  // Bubble these changes up the term tree.
  for (auto it = --term_diff.end(); it->first != 0; it--)
  {
    size_t term_id = it->first;
    int parent_id = term_[term_id].parent_id;
    double diff = it->second;
    double old_value = state.terms[term_id];
    double new_value = old_value + diff;
    if (parent_id >= 0)
    {
      if (term_diff.find((size_t)parent_id) == term_diff.end())
        term_diff[(size_t)parent_id] = 0;
      term_diff[(size_t)parent_id] +=
          term_factor(*new_parameters, term_id) *
              mypow(new_value, term_[term_id].exponent) -
          term_factor(state.parameters, term_id) *
              mypow(old_value, term_[term_id].exponent);
    }
  }
  return term_diff;
}

double Poly::term_factor(const std::vector<double> parameters,
                         size_t term_id) const
{
  if (term_[term_id].has_parameter())
  {
    size_t parameter_id = static_cast<size_t>(term_[term_id].parameter_id);
    if (parameter_id >= parameters.size())
    {
      return term_[term_id].constant;
    }
    return term_[term_id].constant * parameters[parameter_id];
  }
  else
  {
    return term_[term_id].constant;
  }
}

namespace
{
template <class T>
static std::string join(const std::vector<T>& v, const std::string& delim)
{
  std::stringstream out;
  for (size_t i = 0; i < v.size(); i++)
  {
    if (i) out << delim;
    out << v[i];
  }
  return out.str();
}

std::string to_str(double d)
{
  if (d == round(d))
  {
    return std::to_string(int(d));
  }
  else
  {
    return std::to_string(d);
  }
}

}  // namespace

std::string Poly::term_to_string(size_t term_id) const
{
  std::vector<std::string> factors;
  const auto& term = term_[term_id];
  if (term.has_constant())
  {
    if (term.constant == -1)
    {
      factors.push_back("-");
    }
    else
    {
      factors.push_back(to_str(term.constant));
    }
  }
  if (term.has_parameter())
  {
    factors.push_back(parameter_names_[(size_t)term.parameter_id]);
  }
  if (term.is_leaf_node())
  {
    for (size_t spin_id = 0; spin_id < spin_to_terms_.size(); spin_id++)
    {
      if (std::find(spin_to_terms_[spin_id].begin(),
                    spin_to_terms_[spin_id].end(),
                    term_id) != spin_to_terms_[spin_id].end())
      {
        factors.push_back("s_" + std::to_string(spin_id));
      }
    }
  }
  else
  {
    std::string terms = "(";
    for (size_t j = 0; j < term_.size(); j++)
    {
      if (term_[j].parent_id != (int)term_id)
      {
        continue;
      }
      if (term_[j].constant == 0)
      {
        continue;
      }
      if (terms == "(")
      {
        terms += term_to_string(j);
      }
      else if (term_[j].constant > 0)
      {
        terms += " + " + term_to_string(j);
      }
      else
      {
        terms += " - " + term_to_string(j).substr(1);
      }
    }
    terms += ")";
    if (term.exponent != 1)
    {
      terms += "^" + to_str(term.exponent);
    }
    factors.push_back(terms);
  }
  // Fix the special case of rendering a standalone constant -1
  // (which was rendered without the 1 in anticipation of more factors).
  if (factors.size() == 1 && factors[0] == "-")
  {
    factors[0] = "-1";
  }
  // Fix the special case of rendering a standalone constant 1
  // (which was not rendered in anticipation of more factors).
  if (factors.empty())
  {
    return "1";
  }
  return join(factors, "");
}

}  // namespace model
