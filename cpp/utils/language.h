// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

namespace utils
{
/// Renders the indefinite article 'a' or 'an' for name.
///
/// Helper function to improve the language in exception messages.
///
/// Example:
///
///   ```c++
///   DEBUG("This is", indef_article(model_name), " ", model_name, " model.")
///   // "This is an ising model."
///   // "This is a pubo model."
///   ```
///
/// > [!NOTE]
/// > The limited logic is currently based purely on whether
/// > the first character is a vowel. If need be, exceptions and/or
/// > more detailed rules can be added.
std::string indef_article(const std::string& name);

}  // namespace utils
