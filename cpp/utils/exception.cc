
#include "utils/exception.h"

namespace utils
{
ConfigurationException::ConfigurationException(std::string message,
                                               Error error_code)
    : std::invalid_argument(message), error_code_(error_code)
{
}

std::string ConfigurationException::get_error_message() const { return what(); }

std::string ConfigurationException::get_error_code_message() const
{
  return user_error(what(), error_code_);
}

Error ConfigurationException::get_error_code() const { return error_code_; }

RuntimeException::RuntimeException(std::string message, Error error_code)
    : std::runtime_error(message), error_code_(error_code)
{
}

std::string RuntimeException::get_error_code_message() const
{
  return user_error(what(), error_code_);
}

Error RuntimeException::get_error_code() const { return error_code_; }

}  // namespace utils
