

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "utils/arguments.h"
#include "utils/config.h"
#include "utils/exception.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/metadata.h"
#include "app/runner.h"

/// Executable for running qiotoolkit simulations locally.

const char kProgramDocumentation[] = "qiotoolkit Runner";
const char kArgumentDocumentation[] =
    "Instance evaluater built on the qiotoolkit optimization framework";

using utils::ConfigurationException;
using utils::Logger;
using utils::MissingInputException;

static struct argp_option kOptions[] = {
    {"version", 'v', 0, 0, "Show the version of the binary and exit", 0},
    {"log_level", 'l', "STRING", 0,
     "Set explicit log level (INFO, WARN, ERROR, FATAL)", 0},
    {"input", 'i', "FILE", 0, "Input json file to read from", 0},
    {"output", 'o', "FILE", 0, "Output json file to write to", 0},
    {"solver", 's', "STRING", 0, "Name of solver to be executed", 0},
    {"parameters", 'p', "FILE", 0, "Parameter json file of parameters", 0},
    {"no-benchmark", 'n', 0, 0, "Disable output benchmark in result file", 0},
    {"user", 'u', 0, 0,
     "Render user-readable output (as opposed to service-formatted)", 0},
    {"memory-saving", 'm', 0, 0, "Force to run in memory-saving model", 0},
    {nullptr, 0, nullptr, 0, nullptr, 0},
};

struct Config
{
  bool show_version;
  bool user_friendly_mode;
  bool no_benchmark_output;
  bool memory_saving;
  std::string log_level;
  std::string input_file;
  std::string output_file;
  std::string parameter_file;
  std::string solver; 
  std::vector<std::string> positional;
};

static error_t ParseOption(int key, char* value, struct argp_state* state)
{
  Config* config = static_cast<Config*>(state->input);
  switch (key)
  {
    case 'v':
      config->show_version = true;
      break;
    case 'l':
      config->log_level = value;
      break;
    case 'i':
      config->input_file = value;
      break;
    case 'o':
      config->output_file = value;
      break;
    case 's':
      config->solver = value;
      break;
    case 'p':
      config->parameter_file = value;
      break;
    case 'n':
      config->no_benchmark_output = true;
      break;
    case 'u':
      config->user_friendly_mode = true;
      break;
    case 'm':
      config->memory_saving = true;
      break;
    case ARGP_KEY_ARG:
      config->positional.push_back(value);
      break;
    case ARGP_KEY_END:
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  };
  return 0;
}

static app::Runner* get_runner()
{
    return new app::Runner();
}

static struct argp argument_parser = {kOptions,
                                      ParseOption,
                                      kArgumentDocumentation,
                                      kProgramDocumentation,
                                      nullptr,
                                      nullptr,
                                      nullptr};

void print_version()
{
  std::string context = Logger::context_color();
  std::string item = "\n  " + context;
  std::string normal = Logger::normal_color();
  std::string info = Logger::color(INFO);
  Logger::get_stream() << "qiotoolkit " << info << utils::get_qiotoolkit_version()  //
                       << item << "git branch (hash): " << normal
                       << utils::get_git_branch() << " ("
                       << utils::get_git_commit_hash() << ")"  //
                       << item << "compiler (build): " << normal
                       << utils::get_compiler() << " ("
                       << utils::get_cmake_build_type() << ")\n"
                       << std::flush;
}

int main(int argc, char** argv)
{
  Config config;
  config.show_version = false;
  config.log_level = "";
  config.input_file = "";
  config.output_file = "";
  config.parameter_file = "";
  config.solver = "";
  config.user_friendly_mode = false;
  config.no_benchmark_output = false;
  config.memory_saving = false;

  try
  {
    argp_parse(&argument_parser, argc, argv, 0, 0, &config);

    if (config.show_version)
    {
      print_version();
      if (argc > 2)
      {
        LOG(WARN, "Additional arguments were ignored due to -v / --version.");
      }
      return 0;
    }

    if (config.log_level != "")
    {
      if (Logger::set_level(config.log_level))
      {
        LOG(INFO, "Setting log level to '",
            utils::Logger::to_string(utils::Logger::get_level()), "'.");
      }
    }

    // throw MissingInputException("No input file specified (-i)");
    // throw MissingInputException("No solver specified (-s)");

    if (config.parameter_file == "")
    {
      throw MissingInputException("No parameter file specified (-p)");
    }

    utils::initialize_features();

    if (config.memory_saving)
    {
      LOG(INFO, "Force to use memory saving model!");
      utils::set_enabled_feature({utils::Features::FEATURE_USE_MEMORY_SAVING});
    }

    std::unique_ptr<app::Runner> runner_ptr(get_runner());
    auto& runner = *runner_ptr.get();
    runner.set_output_benchmark(!config.no_benchmark_output);
    runner.set_parameter_file(config.parameter_file);
    if (config.input_file != "") runner.set_input_file(config.input_file);
    if (config.solver != "") runner.set_solver(config.solver);

    LOG_MEMORY_USAGE("start of configure");
    runner.configure();

    LOG_MEMORY_USAGE("start of running solver");
    LOG(INFO, "Running solver ", runner.get_solver()->get_identifier());
    std::string result = runner.run();

    if (config.output_file != "")
    {
      LOG(INFO, "Writing ", config.output_file);
      utils::write_file(config.output_file, result);
    }
    else
    {
      LOG(INFO, "Writing Response to stdout");
      std::cout << result << std::endl;
    }

    return EX_OK;
  }
  catch (const utils::ConfigurationException& e)
  {
    if (config.user_friendly_mode)
    {
      // I haven't identified where they are from yet, but it's super
      // unhelpful.
      LOG(FATAL, e.get_error_message());
    }
    else
    {
      std::cerr << utils::kUserErrorTag << e.get_error_code_message()
                << utils::kUserErrorTag << std::endl;
    }
  }
  return 16;
}
