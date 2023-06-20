// Copyright (c) Microsoft. All Rights Reserved.
#pragma once
#include <utility>
#include <vector>

#include "utils/json.h"
#include "utils/structure.h"

namespace solver
{

////////////////////////////////////////////////////////////////////////////////
/// Interface classes for solver to fit in the parameter free framework
///
/// To be added in parameter freeEach solver must implement one of interface: 
/// ParameterFreeAdapterInterface or ParameterFreeLinearSearchAdapterInterface.
///

////////////////////////////////////////////////////////////////////////////////
/// Common interfaces
///
class ParameterFreeAdapterInterfaceBase
{
 public:
  // return number of dimensions in parameter searching space
  virtual size_t parameter_dimensions() const = 0;

  // get the initial set of parameters to start search.
  virtual void get_initial_parameter_values(std::vector<double>& initials) = 0;

  // set up the parameters to the solver associated with this
  virtual void update_parameters(const std::vector<double>& parameters,
                                 double start_time) = 0;

  // in case that the search strategy failed to find next set of parameters, we
  // call this method to forcefully move the procedure
  virtual void update_parameters_linearly(
      std::vector<double>& parameters) const = 0;

  // estimate the execution cost (the less the better) of current parameter
  // settings.
  virtual double estimate_execution_cost() const = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// Interface class for solver to use with Bayesian Global Optimization stategy
///
class ParameterFreeAdapterInterface : public ParameterFreeAdapterInterfaceBase
{
 public:
  // return the search range values of each parameter
  virtual std::vector<std::pair<double, double>> parameter_ranges() const = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// Interface class for solver to use with Linear Search Global
/// Optimization stategy
///
class ParameterFreeLinearSearchAdapterInterface
    : public ParameterFreeAdapterInterfaceBase
{
 public:
  // return the search range values of each parameter
  virtual std::vector<int> parameter_ranges() const = 0;

  // This function is used as back up if search strategy failed to update
  // a new set of parameters. In Linear search that is not possible, so
  // no action is needed.
  void update_parameters_linearly(std::vector<double>&) const override
  {
    return;
  }

  // Particular intitial parameter values is not used in Linear Search strategy
  void get_initial_parameter_values(std::vector<double>& initials) override
  {
    initials.resize(this->parameter_dimensions(), 0.0);
  }
};


}  // namespace solver