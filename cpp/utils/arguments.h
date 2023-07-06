
#pragma once

#ifdef __linux__
#include <argp.h>
#include <sysexits.h>
#else

#define EX_OK 0

struct argp_option
{
  const char* name;
  int key;
  const char* arg;
  int flags;
  const char* doc;
  int group;
};

struct argp_state
{
  void* input;
};

typedef int error_t;

struct argp
{
  argp_option* options;
  error_t (*parser)(int, char*, argp_state*);
  const char* arg_docs;
  const char* doc;
  const char* unknown1;
  const char* unknown2;
  const char* unknown3;
};

#define ARGP_KEY_ARG 0
#define ARGP_KEY_END 0x1000001
#define ARGP_ERR_UNKNOWN E2BIG

error_t argp_parse(argp* config, int argc, char** argv, int flags, int* index,
                   void* output);

#endif
