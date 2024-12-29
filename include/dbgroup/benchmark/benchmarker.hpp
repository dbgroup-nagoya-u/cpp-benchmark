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

#ifndef DBGROUP_BENCHMARK_BENCHMARKER_HPP_
#define DBGROUP_BENCHMARK_BENCHMARKER_HPP_

// C++ standard libraries
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// local sources
#include "dbgroup/benchmark/component/measurements.hpp"
#include "dbgroup/benchmark/component/worker.hpp"

namespace dbgroup::benchmark
{
/**
 * @brief A class to run benchmark.
 *
 * @tparam Target An actual target implementation.
 * @tparam Operation A struct to perform each operation.
 * @tparam OperationEngine A class to generate operations.
 */
template <class Target, class OperationEngine>
class Benchmarker
{
  /*############################################################################
   * Type aliases
   *##########################################################################*/

  using Worker = component::Worker<Target, OperationEngine>;
  using Sketch = component::SimpleDDSketch;

 public:
  /*############################################################################
   * Public constructors and assignment operators
   *##########################################################################*/

  /**
   * @brief Construct a new Benchmarker object.
   *
   * @param bench_target A reference to an actual target implementation.
   * @param target_name The name of a benchmarking target.
   * @param ops_engine A reference to a operation generator.
   * @param thread_num The number of worker threads.
   * @param rand_seed A base random seed.
   * @param output_as_csv A flag to output benchmarking results as CSV or TEXT.
   * @param measure_throughput A flag for measuring throughput (true) or latency (false).
   * @param timeout_in_sec Seconds to timeout.
   * @param target_latency A set of percentiles for measuring latency.
   */
  Benchmarker(  //
      Target &bench_target,
      const std::string &target_name,
      OperationEngine &ops_engine,
      const size_t thread_num = 1,
      const size_t rand_seed = std::random_device{}(),
      const bool output_as_csv = false,
      const bool measure_throughput = true,
      const size_t timeout_in_sec = 10,
      std::vector<double> target_latency = kDefaultLatency)
      : thread_num_{thread_num},
        random_seed_{rand_seed},
        output_as_csv_{output_as_csv},
        measure_throughput_{measure_throughput},
        target_latency_{std::move(target_latency)},
        bench_target_{bench_target},
        ops_engine_{ops_engine},
        timeout_in_sec_{timeout_in_sec}
  {
    Log("*** START " + target_name + " ***");
  }

  Benchmarker(const Benchmarker &) = delete;
  Benchmarker(Benchmarker &&) = delete;

  auto operator=(const Benchmarker &obj) -> Benchmarker & = delete;
  auto operator=(Benchmarker &&) -> Benchmarker & = delete;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  /**
   * @brief Destroy the Benchmarker object.
   *
   */
  ~Benchmarker() = default;

  /*############################################################################
   * Public utility functions
   *##########################################################################*/

  /**
   * @brief Run benchmark and output results to stdout.
   *
   */
  void
  Run()
  {
    /*--------------------------------------------------------------------------
     * Preparation of benchmark workers
     *------------------------------------------------------------------------*/
    Log("...Prepare workers for benchmarking.");
    is_running_.store(true, kRelaxed);
    ready_for_benchmarking_.store(false, kRelaxed);
    worker_cnt_.store(0, kRelaxed);

    std::vector<std::future<Sketch>> result_futures{};

    // create workers in each thread
    std::mt19937_64 rand{random_seed_};
    for (size_t i = 0; i < thread_num_; ++i) {
      std::promise<Sketch> res_p{};
      result_futures.emplace_back(res_p.get_future());
      std::thread{&Benchmarker::RunWorker, this, std::move(res_p), i, rand()}.detach();
    }
    while (worker_cnt_.load(kRelaxed) < thread_num_) {
      // wait for all workers to be created
    }

    /*--------------------------------------------------------------------------
     * Measuring throughput/latency
     *------------------------------------------------------------------------*/
    std::vector<Sketch> results{};
    results.reserve(thread_num_);

    Log("...Run workers.");
    ready_for_benchmarking_.store(true, kRelaxed);
    const auto &wake_up = std::chrono::high_resolution_clock::now() + timeout_in_sec_;

    for (auto &&future : result_futures) {
      const auto status = future.wait_until(wake_up);
      if (status != std::future_status::ready && is_running_.load(kRelaxed)) {
        Log("...Interrupting workers.");
        is_running_.store(false, kRelaxed);
      }
      results.emplace_back(future.get());
    }

    /*--------------------------------------------------------------------------
     * Output benchmarkings results
     *------------------------------------------------------------------------*/
    Log("...Finish running.");

    auto &&sketch = results[0];
    for (size_t i = 1; i < thread_num_; ++i) {
      sketch += results[i];
    }

    LogThroughput(sketch);
    LogLatency(sketch);
    Log("*** FINISH ***\n");
  }

 private:
  /*############################################################################
   * Internal constants
   *##########################################################################*/

  /// @brief The alias of `std::memory_order_relaxed`.
  static constexpr auto kRelaxed = std::memory_order_relaxed;

  /// @brief Targets for calculating parcentile latency.
  static constexpr auto kDefaultLatency  //
      = {0.0, 0.25, 0.50, 0.75, 0.90, 0.95, 0.99, 0.999, 0.9999, 1.0};

  /*############################################################################
   * Internal utility functions
   *##########################################################################*/

  /**
   * @brief Run a worker thread to measure throughput or latency.
   *
   * @param result_p A promise of a worker pointer that holds benchmarking results.
   * @param thread_id A unique thread ID.
   * @param rand_seed A random seed.
   */
  void
  RunWorker(  //
      std::promise<Sketch> result_p,
      const size_t thread_id,
      const size_t rand_seed)
  {
    Worker worker{bench_target_, ops_engine_, is_running_, thread_id, rand_seed};
    worker_cnt_.fetch_add(1, kRelaxed);
    while (!ready_for_benchmarking_.load(kRelaxed)) {
      // the preparation has finished, so wait other workers
    }

    worker.Measure();
    result_p.set_value(worker.MoveSketch());
  }

  /**
   * @brief Compute a throughput score and output it to stdout.
   *
   * @param results benchmarking results.
   */
  void
  LogThroughput(  //
      const Sketch &sketch) const
  {
    if (output_as_csv_ && !measure_throughput_) return;

    const size_t exec_num = sketch.GetTotalExecNum();
    const size_t avg_nano_time = sketch.GetTotalExecTime() / thread_num_;
    const double throughput = static_cast<double>(exec_num) / (avg_nano_time / 1E9);

    if (output_as_csv_) {
      std::cout << throughput << "\n";
    } else {
      std::cout << "Throughput [OPS/s]: " << throughput << "\n";
    }
  }

  /**
   * @brief Compute percentiled latency and output it to stdout.
   *
   * @param results benchmarking results.
   */
  void
  LogLatency(  //
      const Sketch &sketch) const
  {
    if (output_as_csv_ && measure_throughput_) return;

    Log("Percentile Latency [ns]:");
    for (size_t id = 0; id < OperationEngine::OPType::kTotalNum; ++id) {
      if (!sketch.HasLatency(id)) continue;
      Log(" OPS ID " + std::to_string(id) + ":");
      for (auto &&q : target_latency_) {
        if (!output_as_csv_) {
          std::printf("  %6.2f: %12lu\n", 100 * q, sketch.Quantile(id, q));  // NOLINT
        } else {
          std::cout << id << "," << q << "," << sketch.Quantile(id, q) << "\n";
        }
      }
    }
  }

  /**
   * @brief Log a message to stdout if the output mode is `text`.
   *
   * @param message An output message.
   */
  void
  Log(  //
      const std::string &message) const
  {
    if (!output_as_csv_) {
      std::cout << message << "\n";
    }
  }

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief The number of worker threads.
  const size_t thread_num_{};

  /// @brief A base random seed.
  const size_t random_seed_{};

  /// @brief A flat to output measured results as CSV or TXT.
  const bool output_as_csv_{};

  /// @brief A flag to measure throughput (if true) or latency (if false).
  const bool measure_throughput_{};

  /// @brief Targets for calculating parcentile latency.
  std::vector<double> target_latency_{};

  /// @brief A benchmaring target.
  Target &bench_target_{};

  /// @brief An target operation generator.
  OperationEngine &ops_engine_{};

  /// @brief A flag for interrupting workers.
  std::atomic_bool is_running_{true};

  /// @brief Seconds to timeout.
  std::chrono::seconds timeout_in_sec_{};

  std::atomic_size_t worker_cnt_{};

  /// @brief A flag for waking up worker threads.
  std::atomic_bool ready_for_benchmarking_{false};
};

}  // namespace dbgroup::benchmark

#endif  // DBGROUP_BENCHMARK_BENCHMARKER_HPP_
