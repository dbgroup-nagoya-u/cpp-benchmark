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
    latencies_nano_.reserve(operations_.size());
  }

  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &obj) = delete;
  Worker(Worker &&) noexcept = default;
  Worker &operator=(Worker &&) noexcept = default;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  /**
   * @brief Destroy the Worker object.
   *
   */
  ~Worker() = default;

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
   * @brief Get the measured latencies.
   *
   * Note that this function performs random sampling to reduce the cost of computing
   * percentiled latency.
   *
   * @param sample_num the number of desired samples.
   * @return sampled latencies.
   */
  [[nodiscard]] auto
  GetLatencies(const size_t sample_num) const  //
      -> std::vector<size_t>
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
  [[nodiscard]] auto
  GetTotalExecTime() const  //
      -> size_t
  {
    return total_exec_time_nano_;
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// a benchmark target
  Target &target_{};

  /// operation data to be executed by this worker
  const std::vector<Operation> operations_{};

  /// total execution time [ns]
  size_t total_exec_time_nano_{0};

  /// execution time for each operation [ns]
  std::vector<size_t> latencies_nano_{};

  /// a stopwatch to measure execution time
  StopWatch stopwatch_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // CPP_BENCHMARK_BENCHMARK_COMPONENT_WORKER_HPP
