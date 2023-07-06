
#pragma once

#include <stdexcept>
#include <string>

#include "utils/concat.h"
#include "utils/error_handling.h"

namespace utils
{
const std::string kUserErrorTag = "_USER_ERROR";
////////////////////////////////////////////////////////////////////////////////
/// ConfigurationException
///
/// A configuration exception is thrown when the input configuration is not
/// valid (either syntactically or according to the requirements of the
/// components which need to be configured).

class ConfigurationException : public std::invalid_argument
{
 public:
  ConfigurationException(std::string message, Error error_code);
  std::string get_error_message() const;
  std::string get_error_code_message() const;
  Error get_error_code() const;

 private:
  Error error_code_;
};

////////////////////////////////////////////////////////////////////////////////
/// RuntimeException
///
/// A runtime exception is thrown when an unexpected state is encountered
/// during the simulation, which cannot be handled gracefully (i.e., a bug).
///
class RuntimeException : public std::runtime_error
{
 public:
  RuntimeException(std::string message, Error error_code);
  std::string get_error_code_message() const;
  Error get_error_code() const;

 private:
  Error error_code_;
};

template <Error err>
class ConfigurationError : public ConfigurationException
{
 public:
  ConfigurationError(std::string message) : ConfigurationException(message, err)
  {
  }
};

template <Error err>
class RuntimeError : public RuntimeException
{
 public:
  RuntimeError(std::string message) : RuntimeException(message, err) {}
};

typedef ConfigurationError<Error::MemoryLimited> MemoryLimitedException;
typedef ConfigurationError<Error::TimeoutInsufficient>
    TimeoutInsufficientException;

typedef ConfigurationError<Error::DuplicatedVariable>
    DuplicatedVariableException;
typedef ConfigurationError<Error::MissingInput> MissingInputException;
typedef ConfigurationError<Error::InvalidTypes> InvalidTypesException;
typedef ConfigurationError<Error::InitialConfigError> InitialConfigException;
typedef ConfigurationError<Error::ParsingError> ParsingException;
typedef ConfigurationError<Error::FeatureSwitchError> FeatureSwitchException;
typedef ConfigurationError<Error::ValueError> ValueException;
typedef ConfigurationError<Error::IndexOutOfRange> IndexOutOfRangeException;
typedef ConfigurationError<Error::NoFeasibleSolution>
    NoFeasibleSolutionException;

typedef RuntimeError<Error::NotImplemented> NotImplementedException;
typedef RuntimeError<Error::KeyDoesNotExist> KeyDoesNotExistException;
typedef RuntimeError<Error::AccessAsIncorrectType>
    AccessAsIncorrectTypeException;
typedef RuntimeError<Error::PopulationIsEmpty> PopulationIsEmptyException;
typedef RuntimeError<Error::FileReadError> FileReadException;
typedef RuntimeError<Error::FileWriteError> FileWriteException;
typedef RuntimeError<Error::InconsistentValue> InconsistentValueException;
typedef RuntimeError<Error::UnmetPrecondition> UnmetPreconditionException;

/// Catch exceptions in parallel sections
///
/// Omp parallel sections must not leave the parallel section via exceptions
/// This allows such exceptions to be caught and later re-thrown after the
/// parallel section:
///
/// ```c++
/// utils::OmpCatch omp_catch;
/// #pragma omp parallel
/// for (int i = 0; i < N; i++) {
///   omp_catch([=]{
///     // .. code which might throw
///   });
/// }
/// omp_catch.rethrow();
/// ```
class OmpCatch
{
 public:
  OmpCatch() : exception_(nullptr) {}
  ~OmpCatch() {}
  template <typename Function, typename... Parameters>
  void run(Function f, Parameters... params)
  {
    try
    {
      f(params...);
    }
    catch (...)
    {
      #pragma omp critical
      this->exception_ = std::current_exception();
    }
  }
  void rethrow()
  {
    if (this->exception_ != nullptr)
    {
      std::rethrow_exception(this->exception_);
    }
  }

 private:
  std::exception_ptr exception_;
};

}  // namespace utils

#define THROW(EXCEPTION_TYPE, ...) throw(EXCEPTION_TYPE(concat(__VA_ARGS__)))

// Helper macro to test for exceptions to be thrown.
#define EXPECT_THROW_MESSAGE(STATEMENT, EXCEPTION_TYPE, MESSAGE)  \
  try                                                             \
  {                                                               \
    (STATEMENT);                                                  \
    FAIL() << "Expected " #STATEMENT " to throw " #EXCEPTION_TYPE \
              " (not thrown).";                                   \
  }                                                               \
  catch (EXCEPTION_TYPE const& e)                                 \
  {                                                               \
    EXPECT_EQ((std::string)e.what(), MESSAGE);                    \
  }                                                               \
  catch (...)                                                     \
  {                                                               \
    FAIL() << "Expected " #STATEMENT " to throw " #EXCEPTION_TYPE \
              " (throws different exception).";                   \
  }

// Helper macro to test for exceptions to be thrown.
#define ASSERT_THROW_MESSAGE(STATEMENT, EXCEPTION_TYPE, MESSAGE)  \
  try                                                             \
  {                                                               \
    (STATEMENT);                                                  \
    FAIL() << "Expected " #STATEMENT " to throw " #EXCEPTION_TYPE \
              " (not thrown).";                                   \
  }                                                               \
  catch (EXCEPTION_TYPE const& e)                                 \
  {                                                               \
    ASSERT_EQ(std::string(e.what()), MESSAGE);                    \
  }                                                               \
  catch (...)                                                     \
  {                                                               \
    FAIL() << "Expected " #STATEMENT " to throw " #EXCEPTION_TYPE \
              " (throws different exception).";                   \
  }
