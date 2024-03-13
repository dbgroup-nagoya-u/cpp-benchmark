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

#ifndef CPP_BENCHMARK_BENCHMARK_COMPONENT_WORKER_HPP
#define CPP_BENCHMARK_BENCHMARK_COMPONENT_WORKER_HPP

// C++ standard libraries
#include <algorithm>
#include <atomic>
#include <memory>
#include <random>
#include <utility>
#include <vector>

// local sources
#include "benchmark/component/common.hpp"
#include "benchmark/component/measurements.hpp"
#include "benchmark/component/stopwatch.hpp"

namespace dbgroup::benchmark::component
{
/**
 * @brief A class of a worker thread for benchmarking.
 *
 * This class acts as a utility wrapper for benchmarking. Actual processing is
 * performed by the Target class.
 *
 * @tparam Target An actual target implementation.
 * @tparam Operation A struct to perform each operation.
 */
template <class Target, class Operation>
class Worker
{
 public:
  /*############################################################################
   * Public constructors/destructors
   *##########################################################################*/

  /**
   * @brief Construct a new Worker object.
   *
   * @param target a referene to a target implementation.
   * @param operations operation data to be performed by this worker.
   */
  Worker(  //
      Target &target,
      const std::vector<Operation> &&operations)
      : target_{target}, operations_{operations}
  {
    measurements_ = std::make_unique<Measurements>();
    target_.SetUpForWorker();
  }

  Worker(const Worker &) = delete;
  Worker(Worker &&) noexcept = default;

  auto operator=(const Worker &obj) -> Worker & = delete;
  auto operator=(Worker &&) noexcept -> Worker & = default;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  /**
   * @brief Destroy the Worker object.
   *
   */
  ~Worker() { target_.TearDownForWorker(); }

  /*############################################################################
   * Public utility functions
   *##########################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   * @param is_running a flag for monitoring benchmarker's status.
   */
  void
  MeasureLatency(const std::atomic_bool &is_running)
  {
    for (auto &&operation : operations_) {
      stopwatch_.Start();

      target_.Execute(operation);

      stopwatch_.Stop();
      measurements_->AddLatency(stopwatch_.GetNanoDuration());

      if (!is_running.load(std::memory_order_relaxed)) break;
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   * @param is_running a flag for monitoring benchmarker's status.
   */
  void
  MeasureThroughput(const std::atomic_bool &is_running)
  {
    stopwatch_.Start();

    size_t executed_num = 0;
    for (auto &&operation : operations_) {
      executed_num += target_.Execute(operation);
      if (!is_running.load(std::memory_order_relaxed)) break;
    }

    stopwatch_.Stop();
    measurements_->SetTotalExecNum(executed_num);
    measurements_->SetTotalExecTime(stopwatch_.GetNanoDuration());
  }

  /**
   * @brief Get measurement results with its ownership.
   *
   * @return measurement results.
   */
  auto
  MoveMeasurements()  //
      -> std::unique_ptr<Measurements>
  {
    return std::move(measurements_);
  }

 private:
  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// a benchmark target
  Target &target_{};

  /// operation data to be executed by this worker
  const std::vector<Operation> operations_{};

  /// measurement results
  std::unique_ptr<Measurements> measurements_{nullptr};

  /// a stopwatch to measure execution time
  StopWatch stopwatch_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // CPP_BENCHMARK_BENCHMARK_COMPONENT_WORKER_HPP
