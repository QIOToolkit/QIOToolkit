
#pragma once

#include <cctype>
#include <set>
#include <string>
#include <vector>

#include "utils/component.h"
#include "utils/exception.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/stream_handler.h"

namespace utils
{
/// Handler for the DIMACS format
class Dimacs
{
 public:
  /// Constructor: Create an empty Dimacs handler.
  Dimacs();

  /// Representation of a satisfiability clause
  /// For unweighted satisfiability problems, the weight will always be 1.
  class Clause : public utils::Component
  {
   public:
    double weight;
    std::vector<int> variables;

    void configure(const utils::Json& json);

    void check_variable_names();

    // Streaming interfaces

    struct Get_Variables
    {
      static std::vector<int>& get(Clause& config) { return config.variables; }
      static std::string get_key() { return "ids"; }
    };

    struct Get_Weight
    {
      static double& get(Clause& config) { return config.weight; }
      static std::string get_key() { return "c"; }
    };

    using MembersStreamHandler = utils::ObjectMemberStreamHandler<
        utils::VectorStreamHandler<int>, Clause, Get_Variables, true,
        utils::ObjectMemberStreamHandler<utils::BasicTypeStreamHandler<double>,
                                        Clause, Get_Weight, true>>;

    using StreamHandler = utils::ObjectStreamHandler<MembersStreamHandler>;
  };

  /// Parse a string in dimacs format
  void read(const std::string& content);

  /// Read the contents from `filename` and parse them.
  void read_file(const std::string& filename);

  /// Get the type of the parsed problem ('cnf' or 'wcnf')
  /// (or empty string if nothing was parsed).
  std::string get_type() const;

  /// Get the number of variables in the problem.
  size_t get_nvar() const;

  /// Get the number of clauses in the problem.
  size_t get_ncl() const;

  /// Get the value specified as top (minimum weight for a clause to be 'hard')
  /// in the input (return 0 if not specified).
  ///
  /// > [!NOTE]:
  /// > Only type `type='wcnf'` allows `top` as an optional 5th element
  /// > in the problem description line (starting with a 'p').
  double get_top() const;

  /// Get the list of variables actually encountered in the description.
  /// These are all in their positive (non-negated) form, but they don't
  /// need to form a contiguous set of numbers.
  const std::vector<int>& get_variables() const;

  /// Get the list of clauses parsed.
  const std::vector<Clause>& get_clauses() const;

  /// Return the Dimacs parser to its initial state.
  void clear();

 private:
  /// Parse the string pointed to by `content_`
  void parse();

  /// Ignore a full line from the current position `p_`
  void ignore_line();

  /// Parse a problem description line.
  void parse_problem();

  /// Parse a clause (terminated by 0)
  void parse_clause();

  /// Check that the next characters are equivalent to `s` and advance
  /// the position `p_` past them.
  void expect(const std::string& s);

  /// Count the number of characters to the next whitespace.
  size_t to_next_ws();

  /// Read the set of characters to the next whitespace and store them in
  /// target.
  void read_string(std::string* target);

  /// Parse the characters up to the next whitespace as an integer.
  void read_number(int* target);

  /// Parse the characters up to the next whitespace as a double.
  void read_number(double* target);

  /// Parse the characters up to the next whitespace as an integer and
  /// ensure that the number is greater than 0.
  void read_positive_number(uint32_t* target);

  /// Ignore all whitespace from the current position.
  void eat_whitespace();

  /// Return a string describing the current line being parsed.
  std::string on_line() const;

  /// Return a string describing the current line and column
  /// (adding offset to the current column).
  /// > [!NOTE]
  /// > This does not fix the line if col+offset is on a different line;
  /// > it is meant solely to get correct positioning after a number has been
  /// > read successfully but does not meet conditions.
  std::string at_position(int offset = 0) const;

  // internal variables using during parsing.
  size_t p_;           // the current position being parsed
  size_t line_;        // counter for the current line
  size_t line_start_;  // where the last line started
  uint32_t nvar_;      // the number of variables specified in the input
  uint32_t ncl_;       // the number of clauses specified in the input
  const std::string* content_;  // pointer to the string to be parsed.

  // fields holding the problem description
  std::string type_;  // type specified in the input (cnf or wcnf)
  double top_;        // the weight specified as the threshold for 'hard'
  std::vector<Clause> clauses_;  // the clauses that were parsed
  std::vector<int> variables_;   // the variables that were found
};

}  // namespace utils
