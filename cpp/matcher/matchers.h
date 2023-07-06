
#ifndef MATCHER_MATCHERS_H
#define MATCHER_MATCHERS_H

#include "matcher/all-of.h"
#include "matcher/any-of.h"
#include "matcher/any.h"
#include "matcher/each.h"
#include "matcher/equal-to.h"
#include "matcher/greater-equal.h"
#include "matcher/greater-than.h"
#include "matcher/is-empty.h"
#include "matcher/is-sorted.h"
#include "matcher/less-equal.h"
#include "matcher/less-than.h"
#include "matcher/none-of.h"
#include "matcher/not.h"
#include "matcher/size-is.h"

////////////////////////////////////////////////////////////////////////////////
/// Matchers
///
/// This header provides the following predicates for input verification:
///
///   Any()               -- Matches any value (always true)
///   EqualTo(T t)        -- Compares equal to the value t
///   LessThan(T t)       -- Compares less than the value t
///   LessEqual(T t)      -- Compares less equals value t
///   GreaterThan(T t)    -- Compares greater than the value t
///   GreaterEqual(T t)   -- Compares greater equals value t
///   IsEmpty()           -- Checks that the containes is empty
///   SizeIs(Predicate p) -- Applies p to the size of a container
///   Each(Predicate p)   -- Applies p to each element in a vector
///   IsSorted(Ascending) -- Checks that the contents of a vector are sorted
///
/// They are intended to encapsulate checks one would otherwise perform as part
/// of, e.g., an `assert(...)` in an object which can be associated with an
/// input handler for a specific component to
///
///   1) Check the validity of the input and
///   2) Produce a human-redable explanation of what is wrong.
///
/// As such the goal is brevity in specifying constraints without loss of
/// useful error reporting.
///
/// Example Usage:
///
///   auto matcher = Allof(SizeIs(GreaterThan(0)), Each(GreaterThan(0)));
///   std::vector<int> u = {1,2,3};
///   assert(matcher.verify(v));
///   assert(matcher.explain(v) == "size is greater than 0");
///
///   std::vector<int> w = {1,-2,3};
///   assert(matcher.verify(w) == false);
///   assert(matcher.explain(w) ==
///          "each element must be greater than 0, found -2 at index 1");
///

#endif
