// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

namespace utils
{
bool ends_with(const std::string& fname, const std::string& suffix);

std::string read_file(const std::string& fname);

void write_file(const std::string& fname, const std::string& content);

std::string repo_path(const std::string& rel_path = "");

std::string data_path(const std::string& rel_path = "");

}  // namespace utils
