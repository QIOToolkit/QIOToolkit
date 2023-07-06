---
title: Design: Exceptions
description: This document discusses the guidelines for exceptions in qiotoolkit.
topic: article
uid: qiotoolkit.design.exceptions
---

Design: Exceptions
==================

**Background**: Concise and actionable feedback in exceptions is crucial
to allow customers to amend their input.

**Goal**: Provide guidelines on the content and wording of exceptions
returned to customers.

**Non-goals**:

  * Design and discussion of logging levels for development.

Exceptions vs Logging
---------------------

  * Exceptions are intended as an early-abort mechanism with feedback to
    customers (unless caught and handled)
  * Logging is purely for development (we don't surface any log statements to
    customers).

How to throw
------------

We provide a `THROW` macro which takes care of concatenating multiple
stream-printable contents into an error message:

```c++
if (!my_condition(variable)) {
  THROW(utils::ValueException, "my_condition must hold for variable. Found variable=", variable, ".");
}
```

which is equivalent to `throw(utils::ValueException(msg.str()))` with a manually crafted `msg`.
Which Exception
---------------

We recognize [four classes of exceptions]:

  * Insufficient Resources (1-100)
  * Invalid Input Data (101-200)
  * Runtime Errors (201-300)
  * File IO errors (301-400)

The list of error codes can be expanded if necessary.

Choice of Modal Verb
--------------------

To clarify whether a condition is enforced or merely suggested, we follow the
[Microsoft Style Guide](https://docs.microsoft.com/en-us/style-guide/a-z-word-list-term-collections/s/should-vs-must):

  * the word _"must"_ indicates a **required** condition (the task _will
    abort early_ if not followed and indicate this condition as the reason),
  * the word _"should"_ indicates a **suggestion** (the task _will still
    run_, but we expect results to be inferior).

We currently don't have a mechanism to surface warnings to the user, so the
second instance will _silently_ accept inputs not following the suggestion.

Positive Statement
------------------

Exceptions should be formulated in the _positive_ (i.e., describe the expected
situation), followed by what was encountered (i.e., why the expectation is not
met). Ideally with details on where to find the inconsistency.

**Example**

  * `a` must be greater-equals 0. Found `a=-1` in the json input: `cost_function.params.a`.
  * `alpha` must be decreasing (i.e., the initial value of alpha must be larger than the final value).
    Found `alpha.initial = 0 < 1 = alpha.final`.

> [!NOTE]
> This is in contrast to describing what is wrong (i.e., _"Error: alpha.initial is smaller than alpha.final."_).
