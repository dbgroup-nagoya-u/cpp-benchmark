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
#include <utility>

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
template <class Target, class OperationEngine>
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
   * @param is_running A flag for monitoring benchmarker's status.
   */
  Worker(  //
      Target &target,
      OperationEngine &ops_engine,
      const std::atomic_bool &is_running,
      const size_t thread_id,
      const size_t rand_seed)
      : target_{target},
        ops_engine_{ops_engine},
        iter_{ops_engine_.GetOPIter(thread_id, rand_seed)},
        is_running_{is_running},
        sketch_{OperationEngine::OPType::kTotalNum}
  {
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
   */
  void
  MeasureLatency()
  {
    for (; iter_ && is_running_.load(kRelaxed); ++iter_) {
      const auto &[type, op] = *iter_;
      stopwatch_.Start();
      target_.Execute(type, op);
      stopwatch_.Stop();
      sketch_.AddLatency(type, stopwatch_.GetNanoDuration());
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   */
  void
  MeasureThroughput()
  {
    size_t cnt = 0;
    stopwatch_.Start();
    for (; iter_ && is_running_.load(kRelaxed); ++iter_) {
      const auto &[type, op] = *iter_;
      cnt += target_.Execute(type, op);
    }
    stopwatch_.Stop();

    sketch_.SetTotalExecNum(cnt);
    sketch_.SetTotalExecTime(stopwatch_.GetNanoDuration());
  }

  /**
   * @brief Get measurement results with its ownership.
   *
   * @return Measurement results.
   */
  auto
  MoveSketch()  //
      -> SimpleDDSketch
  {
    return std::move(sketch_);
  }

 private:
  /*############################################################################
   * Internal constants
   *##########################################################################*/

  /// @brief The alias of `std::memory_order_relaxed`.
  static constexpr auto kRelaxed = std::memory_order_relaxed;

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief A benchmark target.
  Target &target_{};

  /// @brief Operation data to be executed by this worker.
  OperationEngine &ops_engine_{};

  /// @brief The iterator of an operation queue.
  OperationEngine::OPIter iter_{};

  const std::atomic_bool &is_running_{};

  /// @brief Measurement results.
  SimpleDDSketch sketch_{};

  /// @brief A stopwatch to measure execution time.
  StopWatch stopwatch_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // DBGROUP_BENCHMARK_COMPONENT_WORKER_HPP_
