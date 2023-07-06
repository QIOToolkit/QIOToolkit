
#pragma once

// File for storing error handling related functions.
// For now, we tag only user errors. In the future, we can expand to different
// types of logging.
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace utils
{
// returns codes in form " _<subcode> " in 3 digits and with spaces
// E.g. format_subcode(1) -> " _QTK001 "
// format_subcode(101) -> " _QTK101 "
inline std::string format_subcode(unsigned subcode)
{
  std::ostringstream codeString;
  codeString << std::setw(3) << std::setfill('0') << subcode;

  return " _QTK" + codeString.str() + " ";
}

enum Error
{
  // InsufficientResources (range 1-100)
  MemoryLimited = 1,
  TimeoutInsufficient = 2,

  // InvalidInputData (range 101-200)
  DuplicatedVariable = 101,
  MissingInput = 102,
  InvalidTypes = 103,
  InitialConfigError = 104,
  ParsingError = 105,
  FeatureSwitchError = 106,
  ValueError = 107,
  IndexOutOfRange = 108,
  NoFeasibleSolution = 109,

  //  Runtime Errors (range 201-300)
  NotImplemented = 201,
  KeyDoesNotExist = 202,
  AccessAsIncorrectType = 203,
  PopulationIsEmpty = 204,
  InconsistentValue = 205,
  UnmetPrecondition = 206,

  // File IO errors (301-400)
  FileReadError = 301,
  FileWriteError = 302
};

// User error with specific error code
inline std::string user_error(std::string message, Error subcode)
{
  std::replace(message.begin(), message.end(), '\n', ' ');
  std::string tag = format_subcode(subcode);
  return tag + message;
}

}  // namespace utils
