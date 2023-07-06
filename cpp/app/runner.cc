
#include "runner.h"

#include <cstring>

#include "../utils/exception.h"
#include "../utils/file.h"
#include "../utils/json.h"
#include "../utils/log.h"
#include "../utils/metadata.h"
#include "../utils/operating_system.h"
#include "../utils/proto_reader.h"
#include "../utils/stream_handler.h"
#include "../utils/stream_handler_json.h"
#include "../utils/timing.h"
#include "../graph/properties.h"
#include "../model/all_models.h"
#include "../model/terms.h"
#include "../solver/all_solvers.h"
#include "rapidjson/document.h"

namespace app
{
using utils::get_build_properties;
using utils::get_cpu_time;
using utils::get_invocation_properties;
using utils::get_max_memory_usage;
using utils::get_wall_time;
using utils::InvalidTypesException;
using utils::MissingInputException;
using graph::get_graph_attributes;
using graph::get_graph_node_count;
using graph::GraphAttributes;

void Runner::configure()
{
  double start_time = get_wall_time();
  double start_cputime = get_cpu_time();
  if (parameter_file_.empty())
    throw MissingInputException("No parameter_file specified");
  LOG(INFO, "Reading file: ", parameter_file_);
  auto params = utils::json_from_file(parameter_file_);

  utils::Component runner;

  std::string log_level;
  runner.param(params, "log", log_level)
      .description("Logging level (INFO, WARN, ERROR, FATAL)");

  if (log_level != "")
  {
    if (utils::Logger::is_default_level())
    {
      if (utils::Logger::set_level(log_level))
      {
        LOG(INFO, "Setting log level to '",
            utils::Logger::to_string(utils::Logger::get_level()), "'.");
      }
    }
    else
    {
      LOG(INFO, "not overwriting explicitly set log level '",
          utils::Logger::to_string(utils::Logger::get_level()), "' from input.");
    }
  }
 
  if (target_.empty())
  {
    runner.param(params, "target", target_)
        .description("identifier of the solver to use")
        .required();
  }

  if (input_file_.empty() && params.HasMember("input_data_uri"))
  {
    runner.param(params, "input_data_uri", input_file_)
        .description("the input file to read")
        .required();
  }

  if (!input_file_.empty())
  {
    if (utils::ends_with(input_file_, ".cnf") ||
        utils::ends_with(input_file_, ".wcnf"))
    {
      LOG(INFO, "Parsing ", input_file_, " as DIMACS CNF.");
      utils::Dimacs dimacs;
      dimacs.read_file(input_file_);
      configure(dimacs, params, target_);
    }
    else
    {
      configure(input_file_, params, target_);
    }
  }
  else
  {
    throw MissingInputException("input file must be specified.");
  }
  configure_time_ms_ = 1e3 * (get_wall_time() - start_time);
  configure_cputime_ms_ = 1e3 * (get_cpu_time() - start_cputime);
}

void Runner::configure(const std::string& json)
{
  double start_time = get_wall_time();
  double start_cputime = get_cpu_time();

  auto config = utils::json_from_string(json);
  std::string target;
  utils::Component runner;
  runner.param(config, "target", target)
      .description("identifier of the solver to use")
      .required();
  if (config.HasMember("input_data_uri"))
  {
    std::string input_data_uri;
    runner.param(config, "input_data_uri", input_data_uri)
        .description("uri to read input data from")
        .required();
    LOG(INFO, "Reading ", input_data_uri);
    configure(input_data_uri, config, target);
  }
  else
  {
    throw MissingInputException("`input_data_uri` must be specified.");
  }
  input_size_bytes_ = (int)json.size();
  configure_time_ms_ = 1e3 * (get_wall_time() - start_time);
  configure_cputime_ms_ = 1e3 * (get_cpu_time() - start_cputime);
}

template <class Model_T>
solver::ModelSolver<Model_T>* create_model_solver(const std::string& target)
{
  solver::ModelSolver<Model_T>* model_solver;
  if (target == "paralleltempering.qiotoolkit")
  {
    model_solver = new ::solver::ParallelTempering<Model_T>();
  }
  else if (target == "simulatedannealing.qiotoolkit")
  {
    model_solver = new ::solver::SimulatedAnnealing<Model_T>();
  }
  else if (target == "populationannealing.cpu")
  {
    model_solver = new ::solver::PopulationAnnealing<Model_T>();
  }
  else if (target == "substochasticmontecarlo.cpu")
  {
    model_solver = new ::solver::SubstochasticMonteCarlo<Model_T>();
  }
  else if (target == "tabu.qiotoolkit")
  {
    model_solver = new ::solver::Tabu<Model_T>();
  }
  else if (target == "quantummontecarlo.qiotoolkit")
  {
    model_solver = new ::solver::QuantumMonteCarlo<Model_T>();
  }
  else if (target == "substochasticmontecarlo-parameterfree.cpu")
  {
    model_solver = new ::solver::ParameterFreeSolver<
        Model_T, ::solver::SSMCParameterFree<Model_T>>();
  }
  else if (target == "populationannealing-parameterfree.cpu")
  {
    model_solver =
        new ::solver::ParameterFreeSolver<Model_T,
                                          ::solver::PAParameterFree<Model_T>>();
  }
  else if (target =="paralleltempering-parameterfree.qiotoolkit")
  {
    model_solver = new ::solver::ParameterFreeLinearSearchSolver<
        Model_T, ::solver::PTParameterFree<Model_T>>();
  }
  else if (target == "simulatedannealing-parameterfree.qiotoolkit")
  {
    model_solver = new ::solver::ParameterFreeLinearSearchSolver<
        Model_T, ::solver::SAParameterFree<Model_T>>();
  }
  else if (target == "tabu-parameterfree.qiotoolkit")
  {
    model_solver = new ::solver::ParameterFreeLinearSearchSolver<
        Model_T, ::solver::TabuParameterFree<Model_T>>();
  }
  else
  {
    THROW(utils::ValueException, "Unknown solver: ", target);
  }
  return model_solver;
}

template <>
solver::ModelSolver<::model::Poly>* create_model_solver(const std::string&)
{
  return new ::solver::Murex<::model::Poly>();
}

#define SELECT_MODEL(type_name, Model_T)                       \
  if (model_type == type_name)                                 \
  {                                                            \
    selected_model = model_type;                               \
    std::unique_ptr<Model_T> model_ptr(new Model_T);           \
    model_ptr->configure(input);                               \
    model_ptr->init();                                         \
    auto* model_solver = create_model_solver<Model_T>(target); \
    model_solver->set_model(model_ptr.get());                  \
    model_ = std::move(model_ptr);                             \
    model_solver->configure(params);                           \
    solver_.reset(model_solver);                               \
  }

/// Configure using streaming
void Runner::configure(const std::string& input_file, const utils::Json& params,
                       const std::string& target)
{
  std::string selected_model = "";

  model::BaseModelPreviewConfiguration input_preview;

  // The protobuf message folder ends with .pb
  // This is constructed internally and we control it's naming
  if (utils::isFolder(input_file))
  {
    LOG(INFO, "Parsing the problem data ", input_file, "in protobuf");
    utils::configure_from_proto_folder(input_file, input_preview);
  }
  else
  {
    utils::configure_from_json_file(input_file, input_preview);
  }

  model_type_ =
      model::BaseModelPreviewConfiguration::Get_Type::get(input_preview);
  const std::string& model_type = model_type_;

  if (model_type == "blume-capel" || model_type == "ising" ||
      model_type == "pubo")
  {
    model::GraphModelConfiguration input;

    utils::memory_check_using_file_size(input_file, 1.0);
    if (utils::isFolder(input_file))
    {
      LOG(INFO, "Parsing problem terms", input_file, "in protobuf");
      utils::configure_from_proto_folder<model::GraphModelConfiguration>(
          input_file, input);
    }
    else
    {
      utils::configure_from_json_file<model::GraphModelConfiguration>(input_file,
                                                                     input);
    }

    // Try instantiating each model (this checks the model identifier
    // against the model.type entry in the configuration) and proceeds
    // with solver selection if it matches (within the SELECT_MODEL macro)

    SELECT_MODEL("blume-capel", ::model::BlumeCapel);

    if (model_type == "ising")
    {
      if (memory_saving_enabled())
      {
        size_t nodes_count = get_graph_node_count(
            model::GraphModelConfiguration::Get_Edges::get(input));
        if (nodes_count <= UINT8_MAX - 2)
        {
          LOG(INFO,
              "use memory saving model model::IsingCompact<uint8_t> for ising");
          SELECT_MODEL("ising", ::model::IsingCompact<uint8_t>);
        }
        else if (nodes_count <= UINT16_MAX - 2)
        {
          LOG(INFO,
              "use memory saving model model::IsingCompact<uint16_t> for "
              "ising");
          SELECT_MODEL("ising", ::model::IsingCompact<uint16_t>);
        }
        else if (nodes_count <= UINT32_MAX - 2)
        {
          LOG(INFO,
              "use memory saving model model::IsingCompact<uint32_t> for "
              "ising");
          SELECT_MODEL("ising", ::model::IsingCompact<uint32_t>);
        }
        else
        {
          THROW(utils::ValueException, "Too many node ids");
        }
      }
      else
      {
        SELECT_MODEL("ising", ::model::IsingTermCached);
      }
    }

    if (model_type == "pubo")
    {
      if (memory_saving_enabled())
      {
        size_t nodes_count = get_graph_node_count(
            model::GraphModelConfiguration::Get_Edges::get(input));
        if (nodes_count <= UINT8_MAX - 2)
        {
          LOG(INFO,
              "use memory saving model model::PuboCompact<uint8_t> for pubo");
          SELECT_MODEL("pubo", ::model::PuboCompact<uint8_t>);
        }
        else if (nodes_count <= UINT16_MAX - 2)
        {
          LOG(INFO,
              "use memory saving model model::PuboCompact<uint16_t> for pubo");
          SELECT_MODEL("pubo", ::model::PuboCompact<uint16_t>);
        }
        else if (nodes_count <= UINT32_MAX - 2)
        {
          LOG(INFO,
              "use memory saving model model::PuboCompact<uint32_t> for pubo");
          SELECT_MODEL("pubo", ::model::PuboCompact<uint32_t>);
        }
        else
        {
          THROW(utils::ValueException, "Too many node ids");
        }
      }
      else
      {
        GraphAttributes graph_attributes = get_graph_attributes(
            model::GraphModelConfiguration::Get_Edges::get(input));

        if (graph_attributes.max_nodes_in_term <=
            std::numeric_limits<uint8_t>::max())
        {
          SELECT_MODEL("pubo", ::model::PuboWithCounter<uint8_t>);
        }
        else if (graph_attributes.max_nodes_in_term <=
                 std::numeric_limits<uint16_t>::max())
        {
          SELECT_MODEL("pubo", ::model::PuboWithCounter<uint16_t>);
        }
        else if (graph_attributes.max_nodes_in_term <=
                 std::numeric_limits<uint32_t>::max())
        {
          SELECT_MODEL("pubo", ::model::PuboWithCounter<uint32_t>);
        }
        else if (graph_attributes.max_nodes_in_term <=
                 std::numeric_limits<uint64_t>::max())
        {
          SELECT_MODEL("pubo", ::model::PuboWithCounter<uint64_t>);
        }
        else
        {
          SELECT_MODEL("pubo", ::model::Pubo);
          // SELECT_MODEL("pubo", ::model::PuboAdaptive<uint32_t>);
        }
      }
    }
  }
  else if (model_type == "ising_grouped" || model_type == "pubo_grouped")
  {
    model::FacedGraphModelConfiguration input;

    utils::memory_check_using_file_size(input_file, 1.0);
    utils::configure_from_json_file(input_file, input);

    SELECT_MODEL("ising_grouped", ::model::IsingGrouped);

    if (model_type == "pubo_grouped")
    {
      GraphAttributes graph_attributes = get_graph_attributes(
          model::FacedGraphModelConfiguration::Get_Edges::get(input),
          model::FacedGraphModelConfiguration::Get_SLC_Terms::get(input));
      if (graph_attributes.max_nodes_in_term <=
          std::numeric_limits<uint8_t>::max())
      {
        SELECT_MODEL("pubo_grouped", ::model::PuboGrouped<uint8_t>);
      }
      else if (graph_attributes.max_nodes_in_term <=
               std::numeric_limits<uint16_t>::max())
      {
        SELECT_MODEL("pubo_grouped", ::model::PuboGrouped<uint16_t>);
      }
      else if (graph_attributes.max_nodes_in_term <=
               std::numeric_limits<uint32_t>::max())
      {
        SELECT_MODEL("pubo_grouped", ::model::PuboGrouped<uint32_t>);
      }
      else if (graph_attributes.max_nodes_in_term <=
               std::numeric_limits<uint64_t>::max())
      {
        SELECT_MODEL("pubo_grouped", ::model::PuboGrouped<uint64_t>);
      }
    }
  }
  else if (model_type == "clock")
  {
    model::ClockConfiguration input;
    utils::memory_check_using_file_size(input_file, 1.0);
    utils::configure_from_json_file(input_file, input);
    SELECT_MODEL("clock", ::model::Clock);
  }
  else if (model_type == "tsp")
  {
    model::TspConfiguration input;
    utils::memory_check_using_file_size(input_file, 1.0);
    utils::configure_from_json_file(input_file, input);
    SELECT_MODEL("tsp", ::model::Tsp);
  }
  else if (model_type == "poly")
  {
    model::PolyConfiguration input;
    utils::memory_check_using_file_size(input_file, 1.0);
    utils::configure_from_json_file(input_file, input);
    SELECT_MODEL("poly", ::model::Poly);
  }
  else if (model_type == "maxsat")
  {
    std::unique_ptr<::model::MaxSat32> maxsat(new ::model::MaxSat32());
    utils::memory_check_using_file_size(input_file, 1.0);
    model::MaxSatConfiguration input;
    utils::configure_from_json_file(input_file, input);
    maxsat->configure(input);
    select_max_sat_implementation(maxsat.get(), params, target);
  }

  check_solver(model_type, selected_model, target);
}

/// Handling of parsed dimacs
void Runner::configure(const utils::Dimacs& dimacs, const utils::Json& params,
                       const std::string& solver_name)
{
  std::unique_ptr<::model::MaxSat32> maxsat(new ::model::MaxSat32());
  maxsat->configure(dimacs);
  select_max_sat_implementation(maxsat.get(), params, solver_name);
}

void Runner::select_max_sat_implementation(::model::MaxSat32* maxsat,
                                           const utils::Json& params,
                                           const std::string& target)
{
  model_type_ = "maxsat";
  const std::string& model_type = model_type_;
  std::string selected_model = "";

  // This `input` will be used in lieu of json by the SELECT_MODEL macro.
  model::BaseModel* input = maxsat;

  // Decide which counter size we need.
  auto max_vars = maxsat->get_max_vars_in_clause();
  if (max_vars <= std::numeric_limits<uint8_t>::max())
  {
    LOG(INFO, "Using MaxSat8 since max_vars_in_clause=", max_vars);
    SELECT_MODEL("maxsat", ::model::MaxSat8);
  }
  else if (max_vars <= std::numeric_limits<uint16_t>::max())
  {
    LOG(INFO, "Using MaxSat16 since max_vars_in_clause=", max_vars);
    SELECT_MODEL("maxsat", ::model::MaxSat16);
  }
  else
  {
    LOG(INFO, "Using MaxSat32 since max_vars_in_clause=", max_vars);
    SELECT_MODEL("maxsat", ::model::MaxSat32);
  }
  check_solver(model_type, selected_model, target);
}

void Runner::check_solver(const std::string& model_type,
                          const std::string& selected_model,
                          const std::string& target)
{
  if (!solver_)
  {
    // If the solver_ is still not initialized at this stage, we either
    // could not find a matching model or solver and can't proceed.
    if (selected_model == "")
    {
      THROW(utils::ValueException, "Unknown model type: '", model_type, "'.");
    }
    else
    {
      THROW(utils::ValueException, "Unknown solver (target): '", target, "'.");
    }
  }
}

#undef SELECT_MODEL

// Whether the memory saving is enabled for current runner
bool Runner::memory_saving_enabled() const
{
  return utils::feature_enabled(utils::Features::FEATURE_USE_MEMORY_SAVING) &&
         target_support_memory_saving() && model_support_memory_saving();
}

// Whether we can retry to use the memory saving model
bool Runner::memory_saving_retry() const
{
  return (!utils::feature_enabled(utils::Features::FEATURE_USE_MEMORY_SAVING)) &&
         target_support_memory_saving() && model_support_memory_saving();
}

// Reset the runner for memory saving.
void Runner::reset_for_memory_saving()
{
  solver_.reset(nullptr);
  model_.reset(nullptr);
}

utils::Structure Runner::get_run_output()
{
  // Run and time the solver
  double start_time = get_wall_time();
  double start_cputime = get_cpu_time();
  try
  {
    solver_->init();
  }
  catch (const utils::MemoryLimitedException& e)
  {
    if (memory_saving_retry())
    {
      LOG(INFO, "Retry to use memory saving model");
      reset_for_memory_saving();
      utils::set_enabled_feature({utils::Features::FEATURE_USE_MEMORY_SAVING});
      configure();
      solver_->init();
    }
    else
    {
      // not suitable for retry memory saving model
      throw;
    }
  }
 
  // Fetch results after initialization
  utils::Structure init_soln;
  if (output_benchmark_) 
  {
    init_soln = solver_->get_result(); 
  }

  double solver_start_time = get_wall_time();
  double preprocess_ms = (solver_start_time - start_time) * 1000;
  solver_->run();
  double solver_end_time = get_wall_time();
  double execution_time_ms = 1000 * (solver_end_time - solver_start_time);
  solver_->finalize();
  auto response = solver_->get_result();

  double execution_cputime_ms = 1000 * (get_cpu_time() - start_cputime);

  response[utils::kBenchmark][utils::kThreads] =
      std::max(1, solver_->get_thread_count());
  response[utils::kBenchmark][utils::kExecutionTimeMs] = execution_time_ms;
  response[utils::kBenchmark][utils::kExecutionCpuTimeMs] = execution_cputime_ms;

  // Time deconstruction + benchmark statistics and add it to the
  // configure_time_ms_
  start_time = get_wall_time();
  start_cputime = get_cpu_time();
  response[utils::kBenchmark][utils::kMaxMemoryUsageBytes] =
      get_max_memory_usage();
  if (!output_benchmark_)
  {
    response[utils::kBenchmark]["solver"] = solver_->get_solver_properties();
  }
  solver_.reset();

  response[utils::kBenchmark][utils::kDiskIOReadBytes] = input_size_bytes_;

  response[utils::kBenchmark].set_extension("build", get_build_properties());
  response[utils::kBenchmark].set_extension("invocation",
                                           get_invocation_properties());

  configure_time_ms_ += get_wall_time() - start_time;
  configure_cputime_ms_ += get_cpu_time() - start_cputime;

  // NOTE: end2end_time_ms is an approximation; since we don't know the time
  // spent between configure and run, as well as the time spend reading the
  // input from disk.
  double end2end_time_ms = execution_time_ms + configure_time_ms_;
  double end2end_cputime_ms = execution_cputime_ms + configure_cputime_ms_;
  response[utils::kBenchmark][utils::kEnd2endTimeMs] = end2end_time_ms;
  response[utils::kBenchmark][utils::kEnd2endCpuTimeMs] = end2end_cputime_ms;

  // Add staged output (to compare solutions immediately after initialization and then after solver has run)
  if (output_benchmark_) {
    // Try/catch in case fields are missing
    try {
      init_soln["solutions"].remove("version");
      init_soln["solutions"].remove("parameters");
      init_soln["solutions"].remove("solutions");
    }
    catch (const utils::RuntimeException &e) {}

    // Assign "stage" identifier to solutions for output
    init_soln["solutions"]["stage"] = "init";
    init_soln["solutions"]["time_in_stage_ms"] = preprocess_ms;

    for (size_t i = 0; i < response["solutions"]["solutions"].get_array_size(); i++)
    {
      response["solutions"]["solutions"][i]["stage"] = Runner::get_target(); 
      response["solutions"]["solutions"][i]["time_in_stage_ms"] = execution_time_ms; 
    }

    // Add parsed initial solution to start of list
    response["solutions"]["solutions"].prepend(std::vector<utils::Structure> {init_soln["solutions"]});
  }

  // Calculate the size of the output and denote both input & output sizes
  // as the io_{read,write} bytes in the benchmark stats.
  int output_size_bytes = (int)(response.to_string().size() +
                                strlen("    \"disk_io_write_bytes\": ,\n"));
  int output_size_digits = (int)floor(log10(output_size_bytes));
  if (output_size_digits < floor(log10(output_size_bytes + output_size_digits)))
  {
    output_size_digits++;
  }
  output_size_bytes += output_size_digits + 1;
  response[utils::kBenchmark][utils::kDiskIOWriteBytes] = output_size_bytes;

  double postprocess_ms = 1000 * (get_wall_time() - solver_end_time);
  response[utils::kBenchmark][utils::kPreprocessingMs] = preprocess_ms;
  response[utils::kBenchmark][utils::kPostprocessingMs] = postprocess_ms;

  return response;
}

std::string Runner::run()
{
  utils::Structure response = get_run_output();

  if (!output_benchmark_)
  {
    metric_to_console(response[utils::kBenchmark]);
    return response["solutions"].to_string();
  }

  // end2end_time_ms also doesn't include serialization of the response.
  return response.to_string();
}

void Runner::copy_if_present(const std::string& key, const utils::Structure& src,
                             utils::Structure& target)
{
  if (src.has_key(key))
    target[key] = src[key];
  else
    target[key] = 0;
}

void Runner::add_common_metrics(utils::Structure& benchmarks,
                                utils::Structure& metrics)
{
  // stamp only the common metrics from in to out
  copy_if_present(utils::kExecutionTimeMs, benchmarks, metrics["common"]);
  copy_if_present(utils::kExecutionCpuTimeMs, benchmarks, metrics["common"]);
  copy_if_present(utils::kMaxMemoryUsageBytes, benchmarks, metrics["common"]);
  copy_if_present(utils::kPreprocessingMs, benchmarks, metrics["common"]);
  copy_if_present(utils::kPostprocessingMs, benchmarks, metrics["common"]);
  metrics["solver"] = benchmarks["solver"];
}

// output benchmark in QIO format to console
void Runner::metric_to_console(utils::Structure& benchmarks)
{
  utils::Structure metrics(utils::Structure::OBJECT);
  add_common_metrics(benchmarks, metrics);
  copy_if_present(utils::kTermCount, benchmarks, metrics["problem"]);
  copy_if_present(utils::kVariableCount, benchmarks, metrics["problem"]);
  copy_if_present(utils::kMaxLocality, benchmarks, metrics["problem"]);
  copy_if_present(utils::kMinLocality, benchmarks, metrics["problem"]);
  copy_if_present(utils::kAvgLocality, benchmarks, metrics["problem"]);
  copy_if_present(utils::kAccumDependentVars, benchmarks, metrics["problem"]);
  copy_if_present(utils::kTotalLocality, benchmarks, metrics["problem"]);
  copy_if_present(utils::kMaxCouplingMagnitude, benchmarks, metrics["problem"]);
  copy_if_present(utils::kMinCouplingMagnitude, benchmarks, metrics["problem"]);

  // Get model properties
  metrics["problem"][utils::kModel] = model_->get_identifier();

  std::cout << utils::kMetricsLabel << metrics.to_string(false)
            << utils::kMetricsLabel << std::endl;
}

}  // namespace app
