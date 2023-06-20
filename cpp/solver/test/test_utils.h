// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <string>

#include "utils/file.h"
#include "utils/json.h"
#include "utils/stream_handler_json.h"
#include "utils/structure.h"

template <typename Solver_T, typename Model_T>
utils::Structure run_solver(Solver_T& solver, Model_T& model,
                           const std::string& input_file_name,
                           const std::string& param_file_name)
{
  utils::configure_with_configuration_from_json_file<Model_T>(input_file_name,
                                                             model);

  model.init();
  solver.set_model(&model);

  std::string param_str = utils::read_file(param_file_name);
  solver.configure(utils::json_from_string(param_str));
  solver.init();
  solver.run();
  solver.finalize();
  auto result = solver.get_result();
  return result;
}

template <typename Solver_T, typename Model_T>
utils::Structure run_solver_json(Solver_T& solver, Model_T& model,
                                const std::string& input_file_name,
                                const std::string& param_file_name)
{
  std::string input_str = utils::read_file(input_file_name);
  model.configure(utils::json_from_string(input_str));

  model.init();
  solver.set_model(&model);

  std::string param_str = utils::read_file(param_file_name);
  solver.configure(utils::json_from_string(param_str));
  solver.init();
  solver.run();
  solver.finalize();
  auto result = solver.get_result();
  return result;
}
