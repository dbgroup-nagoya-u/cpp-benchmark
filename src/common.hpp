/*
 * Copyright 2021 Database Group, Nagoya University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "mwcas/mwcas.h"               // PMwCAS implementation
#include "mwcas/mwcas_descriptor.hpp"  // our MwCAS implementation

/*##################################################################################################
 * Global type aliases
 *################################################################################################*/

using MwCAS = ::dbgroup::atomic::mwcas::MwCASDescriptor;
using PMwCAS = ::pmwcas::DescriptorPool;
using SingleCAS = ::std::atomic_size_t;
using Clock = ::std::chrono::high_resolution_clock;

/*##################################################################################################
 * Global constants and enums
 *################################################################################################*/

#ifdef MWCAS_BENCH_MAX_TARGET_NUM
/// the maximum number of MwCAS targets
constexpr size_t kMaxTargetNum = MWCAS_BENCH_MAX_TARGET_NUM;
#else
constexpr size_t kMaxTargetNum = 8;
#endif

/*##################################################################################################
 * Global variables
 *################################################################################################*/

/// a flag to control output format
bool output_as_csv = true;

/*##################################################################################################
 * Global utility functions
 *################################################################################################*/

/**
 * @brief Log a message to stdout if the output mode is `text`.
 *
 * @param message an output message
 */
void
Log(const char *message)
{
  if (!output_as_csv) {
    std::cout << message << std::endl;
  }
}

/**
 * @tparam TimeStamp a type of timestamps.
 * @param start_time a start timestamp.
 * @param end_time an end timestamp.
 * @return the duration of time in nanoseconds.
 */
template <class TimeStamp>
constexpr size_t
GetNanoDuration(  //
    const TimeStamp &start_time,
    const TimeStamp &end_time)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
}
