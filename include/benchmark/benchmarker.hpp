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

#ifndef CPP_BENCHMARK_BENCHMARK_BENCHMARKER_H
#define CPP_BENCHMARK_BENCHMARK_BENCHMARKER_H

#include <algorithm>
#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include "component/common.hpp"
#include "component/worker.hpp"

namespace dbgroup::benchmark
{
/**
 * @brief A class to run benchmark.
 *
 * @tparam Target An actual target implementation.
 * @tparam Operation A struct to perform each operation.
 * @tparam OperationEngine A class to generate operations.
 */
template <class Target, class Operation, class OperationEngine>
class Benchmarker
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using Worker_t = component::Worker<Target, Operation>;
  using Worker_p = std::unique_ptr<Worker_t>;

 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  /**
   * @brief Construct a new Benchmarker object.
   *
   * @param bench_target a reference to an actual target implementation.
   * @param ops_engine a reference to a operation generator.
   * @param exec_num the total number of executions for benchmarking.
   * @param thread_num the number of worker threads.
   * @param random_seed a base random seed.
   * @param measure_throughput a flag to measure throughput (if true) or latency (if false).
   * @param output_as_csv a flag to output benchmarking results as CSV or TEXT.
   */
  Benchmarker(  //
      Target &bench_target,
      OperationEngine &ops_engine,
      const size_t exec_num,
      const size_t thread_num,
      const size_t random_seed,
      const bool measure_throughput,
      const bool output_as_csv)
      : total_exec_num_{exec_num},
        thread_num_{thread_num},
        total_sample_num_{(total_exec_num_ < kMaxLatencyNum) ? total_exec_num_ : kMaxLatencyNum},
        random_seed_{random_seed},
        measure_throughput_{measure_throughput},
        output_as_csv_{output_as_csv},
        bench_target_{bench_target},
        ops_engine_{ops_engine}
  {
    Log("*** START BENCHMARK ***");
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  /**
   * @brief Destroy the Benchmarker object.
   *
   */
  ~Benchmarker() { Log("*** FINISH ***\n"); }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Run benchmark and output results to stdout.
   *
   */
  void
  Run()
  {
    /*----------------------------------------------------------------------------------------------
     * Preparation of benchmark workers
     *--------------------------------------------------------------------------------------------*/
    Log("...Prepare workers for benchmarking.");

    std::vector<std::future<Worker_p>> futures;

    {  // create a lock to stop workers from running
      const auto lock = std::unique_lock<std::shared_mutex>(mutex_1st_);

      // create workers in each thread
      std::mt19937_64 rand{random_seed_};
      size_t exec_num = total_exec_num_ / thread_num_;
      for (size_t i = 0; i < thread_num_; ++i) {
        if (i == thread_num_ - 1) {
          exec_num = total_exec_num_ - exec_num * (thread_num_ - 1);
        }

        std::promise<Worker_p> p;
        futures.emplace_back(p.get_future());
        std::thread{&Benchmarker::RunWorker, this, std::move(p), exec_num, rand()}.detach();
      }

      // wait for all workers to be created
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      const auto guard = std::unique_lock<std::shared_mutex>(mutex_2nd_);
    }  // unlock to run workers

    /*----------------------------------------------------------------------------------------------
     * Measuring throughput/latency
     *--------------------------------------------------------------------------------------------*/
    Log("...Run workers.");

    {  // create a lock to stop workers from running
      const auto lock = std::unique_lock<std::shared_mutex>(mutex_2nd_);

      // wait for all workers to finish measuring throughput
      const auto guard = std::unique_lock<std::shared_mutex>(mutex_1st_);
    }  // unlock to run workers

    /*----------------------------------------------------------------------------------------------
     * Output benchmarkings results
     *--------------------------------------------------------------------------------------------*/
    Log("...Finish running.");

    std::vector<Worker_p> results;
    results.reserve(thread_num_);
    for (auto &&future : futures) {
      results.emplace_back(future.get());
    }

    if (measure_throughput_) {
      LogThroughput(results);
    } else {
      LogLatency(results);
    }
  }

 private:
  /*################################################################################################
   * Internal constants
   *##############################################################################################*/

  /// limit the target of latency calculation
  static constexpr size_t kMaxLatencyNum = 1e6;

  /// targets for calculating parcentile latency
  static constexpr double kTargetPercentiles[] = {0.00, 0.01, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30,
                                                  0.35, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70,
                                                  0.75, 0.80, 0.85, 0.90, 0.95, 0.99, 1.00};

  /*################################################################################################
   * Internal utility functions
   *##############################################################################################*/

  /**
   * @brief Run a worker thread to measure throughput or latency.
   *
   * @param p a promise of a worker pointer that holds benchmarking results.
   * @param exec_num the number of executions for this worker.
   * @param random_seed a random seed.
   */
  void
  RunWorker(  //
      std::promise<Worker_p> p,
      const size_t exec_num,
      const size_t random_seed)
  {
    // prepare a worker
    Worker_p worker;

    {  // create a lock to stop a main thread
      const auto lock = std::shared_lock<std::shared_mutex>(mutex_2nd_);

      const auto operations = ops_engine_.Generate(exec_num, random_seed);
      worker = std::make_unique<Worker_t>(bench_target_, std::move(operations));
    }  // unlock to notice that this worker has been created

    {  // wait for benchmark to be ready
      const auto guard = std::shared_lock<std::shared_mutex>(mutex_1st_);

      if (measure_throughput_) {
        worker->MeasureThroughput();
      } else {
        worker->MeasureLatency();
      }
    }  // unlock to notice that this worker has measured thuroughput/latency

    p.set_value(std::move(worker));
  }

  /**
   * @brief Compute a throughput score and output it to stdout.
   *
   * @param workers worker pointers that hold benchmark results.
   */
  void
  LogThroughput(const std::vector<Worker_p> &workers) const
  {
    size_t avg_nano_time = 0;
    for (auto &&worker : workers) {
      avg_nano_time += worker->GetTotalExecTime();
    }
    avg_nano_time /= thread_num_;

    const auto throughput = total_exec_num_ / (avg_nano_time / 1E9);

    if (output_as_csv_) {
      std::cout << throughput << std::endl;
    } else {
      std::cout << "Throughput [Ops/s]: " << throughput << std::endl;
    }
  }

  /**
   * @brief Compute percentiled latency and output it to stdout.
   *
   * @param workers worker pointers that hold benchmark results.
   */
  void
  LogLatency(const std::vector<Worker_p> &workers) const
  {
    std::vector<size_t> latencies;
    latencies.reserve(kMaxLatencyNum);

    // sort all execution time
    for (size_t i = 0, sample_num = total_sample_num_ / thread_num_; i < thread_num_; ++i) {
      if (i == thread_num_ - 1) {
        sample_num = total_sample_num_ - sample_num * (thread_num_ - 1);
      }

      auto worker_latencies = workers[i]->GetLatencies(sample_num);
      latencies.insert(latencies.end(), worker_latencies.begin(), worker_latencies.end());
    }
    std::sort(latencies.begin(), latencies.end());

    Log("Percentiled Latencies [ns]:");
    for (auto &&percentile : kTargetPercentiles) {
      const size_t percentiled_idx =
          (percentile == 1.0) ? latencies.size() - 1 : latencies.size() * percentile;
      if (!output_as_csv_) {
        std::cout << "  " << std::fixed << std::setprecision(2) << percentile << ": ";
      }
      std::cout << latencies[percentiled_idx];
      if (!output_as_csv_) {
        std::cout << std::endl;
      } else {
        if (percentile < 1.0) {
          std::cout << ",";
        } else {
          std::cout << std::endl;
        }
      }
    }
  }

  /**
   * @brief Log a message to stdout if the output mode is `text`.
   *
   * @param message an output message
   */
  void
  Log(const char *message) const
  {
    if (!output_as_csv_) {
      std::cout << message << std::endl;
    }
  }

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// the total number of executions for benchmarking
  const size_t total_exec_num_;

  /// the number of worker threads
  const size_t thread_num_;

  /// the total number of sampled execution time for computing percentiled latency
  const size_t total_sample_num_;

  /// a base random seed
  const size_t random_seed_;

  /// a flag to measure throughput (if true) or latency (if false)
  const bool measure_throughput_;

  /// a flat to output measured results as CSV or TEXT
  const bool output_as_csv_;

  /// a benchmaring target
  Target &bench_target_;

  /// an target operation generator
  OperationEngine &ops_engine_;

  /// a mutex to control workers
  std::shared_mutex mutex_1st_;

  /// a mutex to control workers
  std::shared_mutex mutex_2nd_;
};

}  // namespace dbgroup::benchmark

#endif  // CPP_BENCHMARK_BENCHMARK_BENCHMARKER_H
