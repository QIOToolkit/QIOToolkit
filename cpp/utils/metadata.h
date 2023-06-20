// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

#include "utils/structure.h"

namespace utils
{
std::string get_qiotoolkit_version();

std::string get_git_branch();

std::string get_git_commit_hash();

std::string get_cmake_build_type();

std::string get_compiler();

utils::Structure get_build_properties();

utils::Structure get_invocation_properties();

}  // namespace utils
