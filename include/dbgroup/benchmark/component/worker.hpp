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

#ifndef DBGROUP_BENCHMARK_COMPONENT_WORKER_HPP_
#define DBGROUP_BENCHMARK_COMPONENT_WORKER_HPP_

// C++ standard libraries
#include <atomic>
#include <cstddef>
#include <memory>
#include <random>
#include <utility>
#include <vector>

// local sources
#include "dbgroup/benchmark/component/measurements.hpp"
#include "dbgroup/benchmark/component/stopwatch.hpp"

namespace dbgroup::benchmark::component
{
/**
 * @brief A class of a worker thread for benchmarking.
 *
 * This class acts as a utility wrapper for benchmarking. Actual processing is
 * performed by the `Target` class.
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
   * @param target A referene to a target implementation.
   * @param operations Operation data to be performed by this worker.
   */
  Worker(  //
      Target &target,
      const std::vector<Operation> &&operations,
      const size_t ops_num)
      : target_{target}, operations_{operations}
  {
    sketch_ = std::make_unique<SimpleDDSketch>(ops_num);
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
   * @param is_running A flag for monitoring benchmarker's status.
   */
  void
  MeasureLatency(  //
      const std::atomic_bool &is_running)
  {
    for (auto &&ops : operations_) {
      stopwatch_.Start();

      target_.Execute(ops);

      stopwatch_.Stop();
      sketch_->AddLatency(ops.GetOpsID(), stopwatch_.GetNanoDuration());

      if (!is_running.load(std::memory_order_relaxed)) break;
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   * @param is_running A flag for monitoring benchmarker's status.
   */
  void
  MeasureThroughput(  //
      const std::atomic_bool &is_running)
  {
    stopwatch_.Start();

    size_t executed_num = 0;
    for (auto &&ops : operations_) {
      executed_num += target_.Execute(ops);
      if (!is_running.load(std::memory_order_relaxed)) break;
    }

    stopwatch_.Stop();
    sketch_->SetTotalExecNum(executed_num);
    sketch_->SetTotalExecTime(stopwatch_.GetNanoDuration());
  }

  /**
   * @brief Get measurement results with its ownership.
   *
   * @return Measurement results.
   */
  auto
  MoveMeasurements()  //
      -> std::unique_ptr<SimpleDDSketch>
  {
    return std::move(sketch_);
  }

 private:
  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief A benchmark target.
  Target &target_{};

  /// @brief Operation data to be executed by this worker.
  const std::vector<Operation> operations_{};

  /// @brief Measurement results.
  std::unique_ptr<SimpleDDSketch> sketch_{nullptr};

  /// @brief A stopwatch to measure execution time.
  StopWatch stopwatch_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // DBGROUP_BENCHMARK_COMPONENT_WORKER_HPP_
