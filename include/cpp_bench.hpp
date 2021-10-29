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
#include <chrono>
#include <future>
#include <iomanip>
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

/**
 * @brief A class to run MwCAS benchmark.
 *
 * @tparam Target An implementation of MwCAS algorithms.
 */
template <class Target, class OperationEngine>
class Benchmarker
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using Worker_t = Worker<Target>;

 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  /**
   * @brief Construct a new Mw C A S Bench object.
   *
   * @param total_field_num the total number of target fields.
   * @param target_num the number of MwCAS targets for each operation.
   * @param exec_num the total number of MwCAS operations for benchmarking.
   * @param thread_num the number of execution threads.
   * @param skew_parameter a skew parameter (based on Zipf's law).
   * @param init_thread_num the number of worker threads for initialization.
   * @param random_seed a base random seed.
   * @param measure_throughput a flag to measure throughput (if true) or latency (if false).
   */
  Benchmarker(  //
      const size_t exec_num,
      const size_t thread_num,
      const size_t random_seed,
      Target &bench_target,
      OperationEngine &ops_engine,
      const bool measure_throughput,
      const bool output_as_csv)
      : total_exec_num_{exec_num},
        thread_num_{thread_num},
        random_seed_{random_seed},
        measure_throughput_{measure_throughput},
        output_as_csv_{output_as_csv},
        bench_target_{bench_target},
        ops_engine_{ops_engine}
  {
    Log("*** START BENCHMARK ***");

    Log("...Set up target data for benchmaring.");
    bench_target_.SetUp();
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  /**
   * @brief Destroy the Benchmarker object.
   *
   */
  ~Benchmarker()
  {
    Log("...Tear down target data.");
    bench_target_.TearDown();

    Log("*** FINISH ***\n");
  }

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

    std::vector<std::future<Worker_t *>> futures;

    {  // create a lock to stop workers from running
      const auto lock = std::unique_lock<std::shared_mutex>(mutex_1st_);

      // create workers in each thread
      std::mt19937_64 rand{random_seed_};
      const size_t total_sample_size =
          (total_exec_num_ < kMaxLatencyNum) ? total_exec_num_ : kMaxLatencyNum;
      size_t exec_num = total_exec_num_ / thread_num_;
      size_t sample_num = total_sample_size / thread_num_;
      for (size_t i = 0; i < thread_num_; ++i) {
        if (i == thread_num_ - 1) {
          exec_num = total_exec_num_ - exec_num * (thread_num_ - 1);
          sample_num = total_sample_size - sample_num * (thread_num_ - 1);
        }

        std::promise<Worker_t *> p;
        futures.emplace_back(p.get_future());
        std::thread{&Benchmarker::RunWorker, this, std::move(p), exec_num, sample_num, rand()}
            .detach();
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
     * Output benchmark results
     *--------------------------------------------------------------------------------------------*/
    Log("...Finish running.");

    std::vector<Worker_t *> results;
    results.reserve(thread_num_);
    for (auto &&future : futures) {
      results.emplace_back(future.get());
    }

    if (measure_throughput_) {
      LogThroughput(results);
    } else {
      LogLatency(results);
    }

    for (auto &&worker : results) {
      delete worker;
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
   * @brief Run a worker thread to measure throuput and latency.
   *
   * @param p a promise of a worker pointer that holds benchmark results.
   * @param exec_num the number of operations to be executed by this worker.
   * @param sample_num the number of samples to calculate percentile latency.
   * @param random_seed a random seed.
   */
  void
  RunWorker(  //
      std::promise<Worker_t *> p,
      const size_t exec_num,
      const size_t sample_num,
      const size_t random_seed)
  {
    // prepare a worker
    Worker_t *worker;

    {  // create a lock to stop a main thread
      const auto lock = std::shared_lock<std::shared_mutex>(mutex_2nd_);
      const auto operations = ops_engine_.Generate(exec_num, random_seed);
      worker = new Worker_t{bench_target_, std::move(operations)};
    }  // unlock to notice that this worker has been created

    {  // wait for benchmark to be ready
      const auto guard = std::shared_lock<std::shared_mutex>(mutex_1st_);
      if (measure_throughput_) {
        worker->MeasureThroughput();
      } else {
        worker->MeasureLatency();
      }
    }  // unlock to notice that this worker has measured thuroughput/latency

    if (!measure_throughput_) {
      // wait for all workers to finish
      const auto guard = std::shared_lock<std::shared_mutex>(mutex_2nd_);
      worker->SortLatencies(sample_num);
    }

    p.set_value(worker);
  }

  /**
   * @brief Compute a throughput score and output it to stdout.
   *
   * @param workers worker pointers that hold benchmark results.
   */
  void
  LogThroughput(const std::vector<Worker_t *> &workers) const
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
  LogLatency(const std::vector<Worker_t *> &workers) const
  {
    std::vector<size_t> latencies;
    latencies.reserve(kMaxLatencyNum);

    // sort all execution time
    for (auto &&worker : workers) {
      auto worker_latencies = worker->GetLatencies();
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
  Log(const char *message)
  {
    if (!output_as_csv_) {
      std::cout << message << std::endl;
    }
  }

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// the total number of MwCAS operations for benchmarking
  const size_t total_exec_num_;

  /// the number of execution threads
  const size_t thread_num_;

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
