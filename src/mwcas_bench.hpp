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

#include "common.hpp"
#include "worker.hpp"

// use PMwCAS
#include "pmwcas.h"

/**
 * @brief A class to run MwCAS benchmark.
 *
 * @tparam MwCASImplementation An implementation of MwCAS algorithms.
 */
template <class MwCASImplementation>
class MwCASBench
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using Worker_t = Worker<MwCASImplementation>;
  using ZipfGenerator = ::dbgroup::random::zipf::ZipfGenerator;

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
  MwCASBench(  //
      const size_t total_field_num,
      const size_t target_num,
      const size_t exec_num,
      const size_t thread_num,
      const double skew_parameter,
      const size_t init_thread_num,
      const size_t random_seed,
      const bool measure_throughput)
      : target_num_{target_num},
        total_exec_num_{exec_num},
        thread_num_{thread_num},
        random_seed_{random_seed},
        measure_throughput_{measure_throughput},
        target_fields_{total_field_num, nullptr},
        zipf_engine_{total_field_num, skew_parameter}
  {
    // a lambda function to initialize target fields
    auto f = [&](const size_t begin_id, const size_t end_id) {
      for (size_t i = begin_id; i < end_id; ++i) {
        target_fields_[i] = new uint64_t{0};
      }
    };

    // prepare MwCAS target fields
    std::vector<std::thread> threads;
    const size_t field_num = total_field_num / init_thread_num;
    for (size_t i = 0, begin_id = 0; i < init_thread_num; ++i, begin_id += field_num) {
      const auto end_id = (i < init_thread_num - 1) ? begin_id + field_num : total_field_num;
      threads.emplace_back(f, begin_id, end_id);
    }
    for (auto &&t : threads) t.join();

    // prepare descriptor pool for PMwCAS if needed
    if constexpr (std::is_same_v<MwCASImplementation, PMwCAS>) {
      // prepare PMwCAS descriptor pool
      pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                          pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
      pmwcas_desc_pool = std::make_unique<PMwCAS>(static_cast<uint32_t>(8192 * thread_num_),
                                                  static_cast<uint32_t>(thread_num_));
    }

    if constexpr (std::is_same_v<MwCASImplementation, MwCAS>) {
      Log("** Run our MwCAS **");
    } else if constexpr (std::is_same_v<MwCASImplementation, PMwCAS>) {
      Log("** Run PMwCAS **");
    } else if constexpr (std::is_same_v<MwCASImplementation, SingleCAS>) {
      Log("** Run single CAS **");
    }
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  /**
   * @brief Destroy the MwCASBench object.
   *
   */
  ~MwCASBench()
  {
    for (auto &&field : target_fields_) {
      delete field;
    }

    Log("** Finish **\n");
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Run MwCAS benchmark and output results to stdout.
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
      std::mt19937_64 rand_engine{random_seed_};
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
        std::thread{&MwCASBench::RunWorker, this, std::move(p), exec_num, sample_num, rand_engine()}
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
      worker = new Worker_t{target_fields_, target_num_, exec_num, zipf_engine_, random_seed};
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

    if (output_as_csv) {
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
      if (!output_as_csv) {
        std::cout << "  " << std::fixed << std::setprecision(2) << percentile << ": ";
      }
      std::cout << latencies[percentiled_idx];
      if (!output_as_csv) {
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

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// the number of MwCAS targets for each operation
  const size_t target_num_;

  /// the total number of MwCAS operations for benchmarking
  const size_t total_exec_num_;

  /// the number of execution threads
  const size_t thread_num_;

  /// a base random seed
  const size_t random_seed_;

  /// a flag to measure throughput (if true) or latency (if false)
  const bool measure_throughput_;

  /// target fields of MwCAS
  std::vector<uint64_t *> target_fields_;

  /// a random engine according to Zipf's law
  ZipfGenerator zipf_engine_;

  /// a mutex to control workers
  std::shared_mutex mutex_1st_;

  /// a mutex to control workers
  std::shared_mutex mutex_2nd_;
};
