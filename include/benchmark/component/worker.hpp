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

#include <algorithm>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "measurements.hpp"
#include "stopwatch.hpp"

namespace dbgroup::benchmark::component
{
/**
 * @brief A class of a worker thread for benchmarking.
 *
 * This class acts as a utility wrapper for benchmarking. Actual processin is performed
 * by the Target class.
 *
 * @tparam Target An actual target implementation.
 * @tparam Operation A struct to perform each operation.
 */
template <class Target, class Operation>
class Worker
{
 public:
  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

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
  auto operator=(const Worker &obj) -> Worker & = delete;
  Worker(Worker &&) noexcept = default;
  auto operator=(Worker &&) noexcept -> Worker & = default;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  /**
   * @brief Destroy the Worker object.
   *
   */
  ~Worker() { target_.TearDownForWorker(); }

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   */
  void
  MeasureLatency()
  {
    for (auto &&operation : operations_) {
      stopwatch_.Start();

      target_.Execute(operation);

      stopwatch_.Stop();
      measurements_->AddLatency(stopwatch_.GetNanoDuration());
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
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

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
