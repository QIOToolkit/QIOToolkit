// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/dimacs.h"

namespace utils
{
Dimacs::Dimacs() { clear(); }

void Dimacs::Clause::check_variable_names()
{
  std::set<int> seen;
  if (variables.empty())
  {
    THROW(utils::ValueException,
          "List of variables in a clause cannot be empty.");
  }
  for (auto name : variables)
  {
    if (name == 0)
    {
      THROW(utils::ValueException,
            "MaxSat variables cannot have the name 0 since it cannot be "
            "negated to indicate 'not-0'.");
    }
    else if (seen.find(name) != seen.end())
    {
      THROW(utils::ValueException, "Variable name ", name,
            " appears multiple times in clause.");
    }
    else if (seen.find(-name) != seen.end())
    {
      THROW(utils::ValueException, "Variable name ", std::abs(name),
            " appears both positive and negative in clause.");
    }
  }
}

void Dimacs::Clause::configure(const utils::Json& json)
{
  using ::matcher::GreaterThan;
  this->param(json, "c", weight)
      .default_value(1)
      .matches(GreaterThan<double>(0));
  ;
  this->param(json, "ids", variables).required();
  check_variable_names();
}

void Dimacs::read(const std::string& content)
{
  content_ = &content;
  parse();
  content_ = nullptr;
}

void Dimacs::read_file(const std::string& filename)
{
  std::string content = utils::read_file(filename);
  read(content);
}

std::string Dimacs::get_type() const { return type_; }
size_t Dimacs::get_nvar() const { return variables_.size(); }
size_t Dimacs::get_ncl() const { return clauses_.size(); }
double Dimacs::get_top() const { return top_; }
const std::vector<int>& Dimacs::get_variables() const { return variables_; }
const std::vector<Dimacs::Clause>& Dimacs::get_clauses() const
{
  return clauses_;
}

void Dimacs::clear()
{
  type_ = "";
  nvar_ = 0;
  ncl_ = 0;
  clauses_.clear();
  variables_.clear();
}

void Dimacs::parse()
{
  clear();
  p_ = 0;
  line_ = 1;
  line_start_ = 0;
  while (p_ < content_->size())
  {
    switch ((*content_)[p_])
    {
      case 'c':
        ignore_line();
        break;
      case 'p':
        parse_problem();
        break;
      default:
        parse_clause();
        break;
    }
  }
  if (type_.empty())
  {
    THROW(utils::ValueException, "Missing problem line in dimacs input");
  }
  if (clauses_.size() != ncl_)
  {
    THROW(utils::ValueException,
          "Number of clauses parsed does not match: ", clauses_.size(),
          "!=", ncl_);
  }
  std::set<int> variable_set;
  for (const auto& clause : clauses_)
  {
    for (int id : clause.variables)
    {
      variable_set.insert(std::abs(id));
    }
  }
  if (variable_set.size() != nvar_)
  {
    THROW(utils::ValueException,
          "Number of variables found does not match: ", variable_set.size(),
          "!=", nvar_);
  }
  variables_.resize(variable_set.size());
  std::copy(variable_set.begin(), variable_set.end(), variables_.begin());
}

void Dimacs::ignore_line()
{
  while (p_ < content_->size() &&
         ((*content_)[p_] != '\n' && (*content_)[p_] != '\r'))
  {
    p_++;
  }
  eat_whitespace();
}

void Dimacs::parse_problem()
{
  if (!type_.empty())
  {
    THROW(utils::ParsingException, "Multiple problem lines in Dimacs file ",
          on_line());
  }
  expect("p ");
  read_string(&type_);
  if (type_ != "cnf" && type_ != "wcnf")
  {
    THROW(utils::ValueException, "Expected `type` to be 'cnf' or 'wcnf' ",
          at_position(-type_.size()));
  }
  read_positive_number(&nvar_);
  read_positive_number(&ncl_);
  top_ = 0;
  if (type_ == "wcnf")
  {
    // parse optional top_
    while (p_ < content_->size() &&
           ((*content_)[p_] == ' ' || (*content_)[p_] == '\t'))
    {
      p_++;
    }
    if (p_ < content_->size() && !std::isspace((*content_)[p_]))
    {
      read_number(&top_);
    }
  }
  eat_whitespace();
}

void Dimacs::parse_clause()
{
  Clause clause;
  if (type_ == "wcnf")
  {
    read_number(&clause.weight);
  }
  else
  {
    clause.weight = 1;
  }
  int variable = 0;
  while (true)
  {
    read_number(&variable);
    if (variable == 0)
    {
      if (clause.variables.empty())
      {
        THROW(utils::ValueException,
              "List of variables in a clause cannot be empty ", at_position());
      }
      break;
    }
    clause.variables.push_back(variable);
  }
  clause.check_variable_names();
  clauses_.push_back(clause);
  eat_whitespace();
}

void Dimacs::expect(const std::string& s)
{
  for (size_t i = 0; i < s.size(); i++)
  {
    if ((*content_)[p_ + i] != s[i])
    {
      THROW(utils::ParsingException, "Expected '", s.substr(i), "' ",
            at_position());
    }
  }
  p_ += s.size();
}

size_t Dimacs::to_next_ws()
{
  size_t e = p_;
  while (e < content_->size() && !std::isspace((*content_)[e])) e++;
  return e - p_;
}

void Dimacs::read_string(std::string* target)
{
  eat_whitespace();
  size_t length = to_next_ws();
  *target = content_->substr(p_, length);
  p_ += length;
}

void Dimacs::read_number(int* target)
{
  eat_whitespace();
  int length = to_next_ws();
  char* end = nullptr;
  *target = std::strtol(content_->c_str() + p_, &end, 10);
  if (end - (content_->c_str() + p_) != length)
  {
    THROW(utils::ParsingException, "Expected number ", at_position());
  }
  p_ += length;
}

void Dimacs::read_number(double* target)
{
  eat_whitespace();
  int length = to_next_ws();
  char* end = nullptr;
  *target = std::strtold(content_->c_str() + p_, &end);
  if (end - (content_->c_str() + p_) != length)
  {
    THROW(utils::ParsingException, "Expected number ", at_position());
  }
  p_ += length;
}

void Dimacs::read_positive_number(uint32_t* target)
{
  int i;
  int before = p_;
  read_number(&i);
  if (i <= 0)
  {
    THROW(utils::ParsingException, "Expected positive number ",
          at_position(before - int(p_)));
  }
  *target = i;
}

void Dimacs::eat_whitespace()
{
  while (p_ < content_->size() && std::isspace((*content_)[p_]))
  {
    if ((*content_)[p_] == '\n') line_++;
    if ((*content_)[p_] == '\n' || (*content_)[p_] == '\r') line_start_ = p_;
    p_++;
  }
}

std::string Dimacs::on_line() const
{
  std::stringstream s;
  s << "on line " << line_;
  return s.str();
}

std::string Dimacs::at_position(int offset) const
{
  std::stringstream s;
  s << "on line " << line_ << ", col " << (p_ + offset - line_start_ + 1);
  return s.str();
}

}  // namespace utils
