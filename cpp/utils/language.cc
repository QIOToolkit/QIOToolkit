
#include "utils/language.h"

namespace utils
{
std::string indef_article(const std::string& name)
{
  if (name.empty()) return "";
  // Sufficient approximation for now.
  switch (name[0])
  {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
      return "an";
    default:
      return "a";
  }
}

}  // namespace utils
