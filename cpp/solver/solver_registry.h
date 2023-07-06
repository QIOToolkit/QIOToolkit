
#pragma once

#include <map>
#include <memory>
#include <string>

#include "utils/exception.h"
#include "utils/log.h"
#include "markov/model.h"
#include "solver/solver.h"

namespace solver
{
// Declaration of the method to actually instantiate the solver.
// (This needs to be defined after all the solvers and it can't be
// integrated into registration mechanism with interface/implementation
// due to the need to template by model).
template <class Model_T>
Solver* create_solver(const std::string& identifier);

// Declaration of the registration helper (used to register solvers
// statically in the macro). Declared here because it needs to be
// a friend class of the SolverRegistration.
template <class Solver_T>
struct SolverRegistrar;

////////////////////////////////////////////////////////////////////////////////
/// Registration of a templateable solver.
///
/// The registration carries only the identifier of the solver for now;
/// the actual instantiation is handled by the templated `create_solver`
/// method declared above.
///
class SolverRegistration
{
 public:
  /// Method to forward solver instantiation to `create_solver`
  ///
  template <class Model_T>
  Solver* create_for_model() const
  {
    return create_solver<Model_T>(identifier_);
  }

 private:
  // Friend class to allow instantiation in the registrar
  template <class Solver_T>
  friend struct SolverRegistrar;
  // Constrained to use from the Registrar for now
  SolverRegistration(const std::string& identifier) : identifier_(identifier) {}
  // The solver identifier associated with this registration.
  // This identifier needs to be handled in `create_solver` for instantiation
  // to work.
  std::string identifier_;
};

////////////////////////////////////////////////////////////////////////////////
/// Registry of available solvers
///
/// The solver registry allows the instantiation of each registered solver for
/// a given model.
///
/// Usage:
///
///   * Registering a new model:
///
///     ```c++
///     #include "solver/solver/solver_registry"
///
///     class MySolver : public solver::Solver {
///      public:
///       // Must have a constructor without arguments
///       MySolver() = default;
///
///       // This is the identifier the solver is register with.
///       std::string get_identifier() const override {
///         return "my_solver";
///       }
///       ...
///     }
///     REGISTER_SOLVER(MySolver);
///     ```
///
///   * Make sure the solver is included in `all_solver.h` and handled in the
///     `create_solver<Model_T>` method.
///
///   * Instantiating a solver for a specific model:
///
///     ```c++
///     auto* my_solver =
///         solver::SolverRegistry::create_for_model<MyModel>("my_solver");
///     ```
class SolverRegistry
{
 public:
  /// Check whether a solver with that identifier is registered.
  static bool has(const std::string& identifier)
  {
    return instance().registry_.find(identifier) != instance().registry_.end();
  }

  /// Find and return the solver registration.
  static const SolverRegistration* get(const std::string& identifier)
  {
    auto found = instance().registry_.find(identifier);
    if (found == instance().registry_.end())
    {
      THROW(utils::KeyDoesNotExistException,
            "There is no solver with identifier '", identifier, "'.");
    }
    return found->second.get();
  }

  /// Find the solver registration and create an instance for Model_T.
  template <class Model_T>
  static Solver* create_for_model(const std::string& identifier)
  {
    auto* entry = get(identifier);
    return entry->create_for_model<Model_T>();
  }

  /// Add an entry to the solver registry.
  ///
  /// This is used by the SolverRegistrar in the REGISTER_SOLVER macro,
  /// you should not need to invoke it directly.
  static void add(const std::string& identifier,
                  std::unique_ptr<SolverRegistration>&& registration)
  {
    if (has(identifier)) return;
    instance().registry_[identifier] = std::move(registration);
  }

 private:
  /// This is a singleton class
  SolverRegistry() {}

  /// Access the singleton
  static SolverRegistry& instance()
  {
    static SolverRegistry registry;
    return registry;
  }

  std::map<std::string, std::unique_ptr<SolverRegistration>> registry_;
};

// Definition of a minimal model satisfying the union current api requirements
// for different solvers.
class EmptyState
{
 public:
  int copy_state_only(EmptyState) const { return 0; }
  utils::Structure render() const { return 0; }
  std::vector<bool> spins;
};

inline bool same_state_value(const EmptyState&, const EmptyState&, const int&)
{
  return true;
}

class EmptyModel : public markov::Model<EmptyState, int>
{
 public:
  EmptyState create_state(const std::vector<int>&) const
  {
    return EmptyState();
  }
  double get_spin_overlap(const EmptyState&, const EmptyState&) const
  {
    return 0;
  }
  double get_term_overlap(const EmptyState&, const EmptyState&) const
  {
    return 0;
  }
  int create_parameter_change(std::vector<double>&) const { return 0; }
  std::vector<std::string> get_parameters() const { return {}; }
};

/// Helper struct to statically fill the registry.
template <class Solver_T>
struct SolverRegistrar
{
  SolverRegistrar(int)
  {
    std::string identifier = Solver_T().get_identifier();
    SolverRegistry::add(identifier, std::unique_ptr<SolverRegistration>(
                                        new SolverRegistration(identifier)));
  }
};

#define REGISTER_SOLVER(Solver_T)                                  \
  static ::solver::SolverRegistrar<Solver_T<::solver::EmptyModel>> \
      registrar##Solver_T(0)

}  // namespace solver
