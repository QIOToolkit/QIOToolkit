// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <sstream>

template <class T>
void print_recursive(std::ostream& out, const T& arg)
{
  out << arg;
}

template <class T, class... Args>
void print_recursive(std::ostream& out, const T& arg, const Args&... args)
{
  out << arg;
  print_recursive(out, args...);
}

template <class T, class... Args>
std::string concat(const T& arg, const Args&... args)
{
  std::stringstream msg;
  print_recursive(msg, arg, args...);
  return msg.str();
}
