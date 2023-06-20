// Copyright (c) Microsoft. All Rights Reserved.
#include "utils/arguments.h"

#ifndef __linux__

#include <iostream>
#include <string>

#include "utils/log.h"

static std::string arg_prefix = "-";

bool is_option(std::string arg)
{
  return arg.size() > arg_prefix.size() &&
         arg.substr(0, arg_prefix.size()) == arg_prefix;
}

error_t argp_parse(argp* config, int argc, char** argv, int, int*, void* output)
{
  argp_state state;
  state.input = output;
  for (int i = 1; i < argc; i++)
  {
    std::string argument = argv[i];
    bool handled = false;

    if (is_option(argument))
    {
      std::string name = argument.substr(arg_prefix.size());
      for (int j = 0; config->options[j].name != nullptr; j++)
      {
        auto& option = config->options[j];
        if (name == option.name || (name.size() == 1 && name[0] == option.key))
        {
          if (option.arg)
          {  // has a value
            if (i + 1 >= argc || is_option(argv[i + 1]))
            {
              LOG(ERROR, "Missing value for option ", name);
              exit(1);
              // missing argument value
            }
            (*config->parser)(option.key, argv[i + 1], &state);
            i++;  // jump argument value in iteration
            handled = true;
            break;
          }
          else
          {
            (*config->parser)(option.key, nullptr, &state);
            handled = true;
            break;
          }
        }
      }
      if (!handled)
      {
        // unknown option
        LOG(ERROR, "Unknown option: ", name);
        exit(1);
      }
    }
    else
    {
      (*config->parser)(ARGP_KEY_ARG, argv[i], &state);
    }
  }
  return 0;
}

#endif
