
#pragma once

#include "utils/component.h"

namespace markov
{
////////////////////////////////////////////////////////////////////////////////
/// Representation of a State in the Markov chain.
///
/// A markov state is one possible configuration of the underlying system. To
/// form a markov chain it is modified according to allowed `Transition`s into
/// "neighboring" states which typically differ from it by a simple operation
/// (such as the change of one variable of the configuration).
///
/// There are no API requirements on the state itself other than that it must
/// be serializable (i.e., implement the `Serializable` interface) to allow
/// checkpointing.
///
/// @see utils::Renderable
/// @see markov::Transition
///
class State : public utils::Component
{
 public:
};

}  // namespace markov
