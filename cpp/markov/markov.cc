

namespace markov
{
#if defined(_MSC_VER)
// MSVC complains when this library is empty; this bogus variable
// silences its complaints. However, good compilers will note the
// unused variable, so we're wrapping it in a MSVC macro.
const int MARKOV_LIBRARY_IS_NOT_EMPTY = 1;
#endif

}  // namespace markov
