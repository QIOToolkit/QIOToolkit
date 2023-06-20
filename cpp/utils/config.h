// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#include <cassert>
#include <set>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "utils/log.h"
#include "utils/optional.h"
#include "matcher/matchers.h"

namespace utils
{
const char* const kVersion = "version";
const char* const kCostFunction = "cost_function";
const char* const kType = "type";
const char* const kParams = "params";
const char* const kDisabledFeatures = "disabled_features";
const char* const kEnabledFeatures = "enabled_features";
const char* const kSolver = "solver";

const char* const kBenchmark = "benchmark";
const char* const kExecutionTimeMs = "solver_time_ms";
const char* const kExecutionCpuTimeMs = "cpu_time_ms";
const char* const kMaxMemoryUsageBytes = "max_mem_bytes";
const char* const kTermCount = "num_terms";
const char* const kVariableCount = "num_variables";
const char* const kMaxLocality = "locality";
const char* const kThreads = "threads";
const char* const kDiskIOReadBytes = "disk_io_read_bytes";
const char* const kDiskIOWriteBytes = "disk_io_write_bytes";
const char* const kEnd2endTimeMs = "end2end_time_ms";
const char* const kEnd2endCpuTimeMs = "end2end_cputime_ms";
const char* const kMinLocality = "min_locality";
const char* const kAvgLocality = "avg_locality";
const char* const kAccumDependentVars = "accumulated_dependent_vars";
const char* const kTotalLocality = "sum_coefficient_degrees_total";
const char* const kMaxCouplingMagnitude = "max_coupling_magnitude";
const char* const kMinCouplingMagnitude = "min_coupling_magnitude";
const char* const kPreprocessingMs = "preprocessing_time_ms";
const char* const kPostprocessingMs = "postprocessing_time_ms";

const char* const kModel = "model";

const char* const kMetricsLabel = "_QIO_METRICS";

enum Features
{
  // we do not have this feature in qiotoolkit, but it is used in QIO
  FEATURE_COMPACT_TERMS = 0,
  // we do not have this feature in qiotoolkit, but it is used in QIO
  FEATURE_ACCURATE_TIMEOUT = 1,
  // Exponentially repopulation
  FEATURE_PA_EXP_REPOPULATION = 2,
  // USE memory saving model
  FEATURE_USE_MEMORY_SAVING = 3,
  // new feature much be added right before FEATURE_COUNT
  FEATURE_COUNT = 4
};
void initialize_features();

void set_disabled_feature(const std::set<int>& features);
void set_enabled_feature(const std::set<int>& features);
bool feature_disabled(int feature_id);
bool feature_enabled(int feature_id);

}  // namespace utils
