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

#include <algorithm>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "stopwatch.hpp"

namespace dbgroup::benchmark::component
{
/**
 * @brief A class of a worker thread for benchmarking.
 *
 * @tparam Target An implementation of MwCAS algorithms.
 */
template <class Target, class Operation>
class Worker
{
 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new Worker object.
   *
   * @param target
   * @param operations
   */
  Worker(  //
      Target &target,
      std::vector<Operation> &&operations)
      : target_{target},
        operations_{operations},
        total_exec_time_nano_{0},
        latencies_nano_{},
        stopwatch_{}
  {
    // reserve capacity of dynamic arrays
    latencies_nano_.reserve(operations_.size());
  }

  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &obj) = delete;
  Worker(Worker &&) = default;
  Worker &operator=(Worker &&) = default;

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  /**
   * @brief Destroy the Worker object.
   *
   */
  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   */
  void
  MeasureLatency()
  {
    assert(latencies_nano_.empty());

    for (auto &&operation : operations_) {
      stopwatch_.Start();

      target_.Execute(operation);

      stopwatch_.Stop();
      latencies_nano_.emplace_back(stopwatch_.GetNanoDuration());
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   */
  void
  MeasureThroughput()
  {
    stopwatch_.Start();

    for (auto &&operation : operations_) {
      target_.Execute(operation);
    }

    stopwatch_.Stop();
    total_exec_time_nano_ = stopwatch_.GetNanoDuration();
  }

  /**
   * @return latencies.
   */
  const std::vector<size_t>
  GetLatencies(const size_t sample_num) const
  {
    std::uniform_int_distribution<size_t> id_engine{0, latencies_nano_.size() - 1};
    std::mt19937_64 rand_engine{std::random_device{}()};

    // perform random sampling
    std::vector<size_t> sampled_latencies;
    sampled_latencies.reserve(sample_num);
    for (size_t i = 0; i < sample_num; ++i) {
      sampled_latencies.emplace_back(latencies_nano_[id_engine(rand_engine)]);
    }

    return sampled_latencies;
  }

  /**
   * @return total execution time.
   */
  size_t
  GetTotalExecTime() const
  {
    return total_exec_time_nano_;
  }

 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  Target &target_;

  /// MwCAS target filed addresses for each operation
  std::vector<Operation> operations_;

  /// total execution time [ns]
  size_t total_exec_time_nano_;

  /// execution time for each operation [ns]
  std::vector<size_t> latencies_nano_;

  StopWatch stopwatch_;
};

}  // namespace dbgroup::benchmark::component
