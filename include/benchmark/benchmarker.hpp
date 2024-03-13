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

#ifndef CPP_BENCHMARK_BENCHMARK_BENCHMARKER_HPP
#define CPP_BENCHMARK_BENCHMARK_BENCHMARKER_HPP

// C++ standard libraries
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// local sources
#include "benchmark/component/common.hpp"
#include "benchmark/component/worker.hpp"

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
  /*############################################################################
   * Type aliases
   *##########################################################################*/

  using Worker_t = component::Worker<Target, Operation>;
  using Worker_p = std::unique_ptr<Worker_t>;
  using Result_p = std::unique_ptr<component::Measurements>;

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
   * @param exec_num The number of executions of each worker.
   * @param thread_num The number of worker threads.
   * @param random_seed A base random seed.
   * @param measure_throughput A flag for measuring throughput (true) or latency (false).
   * @param output_as_csv A flag to output benchmarking results as CSV or TEXT.
   * @param timeout_in_sec Seconds to timeout.
   * @param target_latency A set of percentiles for measuring latency.
   */
  Benchmarker(  //
      Target &bench_target,
      const std::string &target_name,
      OperationEngine &ops_engine,
      const size_t exec_num,
      const size_t thread_num,
      const size_t random_seed,
      const bool measure_throughput,
      const bool output_as_csv,
      const size_t timeout_in_sec,
      const char *target_latency = kDefaultLatency)
      : exec_num_{exec_num},
        thread_num_{thread_num},
        random_seed_{random_seed},
        measure_throughput_{measure_throughput},
        output_as_csv_{output_as_csv},
        bench_target_{bench_target},
        ops_engine_{ops_engine},
        timeout_in_sec_{timeout_in_sec}
  {
    if (!measure_throughput_) {
      // check the number of samples is valid
      const auto total_exec_num = exec_num_ * thread_num_;
      if (total_exec_num < kMaxLatencyNum) {
        total_sample_num_ = total_exec_num;
      }

      // prepare percentile latency
      constexpr size_t kDefaultCapacity = 32;
      target_latency_.reserve(kDefaultCapacity);

      // split a given percentile string
      std::string target_lat_str{target_latency};
      size_t begin_pos = 0;
      size_t end_pos = target_lat_str.find_first_of(',');
      while (begin_pos < target_lat_str.size()) {
        // extract a latency string and convert to double
        auto &&lat = target_lat_str.substr(begin_pos, end_pos - begin_pos);
        target_latency_.emplace_back(std::stod(lat));

        // find the next latency string
        begin_pos = end_pos + 1;
        end_pos = target_lat_str.find_first_of(',', begin_pos);
        if (end_pos == std::string::npos) {
          end_pos = target_lat_str.size();
        }
      }
    }

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
  ~Benchmarker() { Log("*** FINISH ***\n"); }

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
    ready_for_benchmarking_ = false;

    std::vector<std::future<void>> prepare_futures{};
    std::vector<std::future<Result_p>> result_futures{};

    // create workers in each thread
    std::mt19937_64 rand{random_seed_};
    for (size_t i = 0; i < thread_num_; ++i) {
      // create promises to manage multi-threaded benchmarking
      std::promise<void> pre_p{};
      std::promise<Result_p> res_p{};
      prepare_futures.emplace_back(pre_p.get_future());
      result_futures.emplace_back(res_p.get_future());

      // create a worker thread
      std::thread t{&Benchmarker::RunWorker, this, std::move(pre_p), std::move(res_p), rand()};
      t.detach();
    }

    // wait for all workers to be created
    for (auto &&future : prepare_futures) {
      future.get();
    }

    /*--------------------------------------------------------------------------
     * Measuring throughput/latency
     *------------------------------------------------------------------------*/
    Log("...Run workers.");

    {
      std::lock_guard x_guard{x_mtx_};
      ready_for_benchmarking_ = true;
    }
    cond_.notify_all();

    std::vector<Result_p> results{};
    results.reserve(thread_num_);
    for (auto &&future : result_futures) {
      const auto status = future.wait_for(timeout_in_sec_);
      if (status != std::future_status::ready) {
        Log("...Interrupting workers.");
        is_running_.store(false, std::memory_order_relaxed);
      }
      results.emplace_back(future.get());
    }

    /*--------------------------------------------------------------------------
     * Output benchmarkings results
     *------------------------------------------------------------------------*/
    Log("...Finish running.");

    if (measure_throughput_) {
      LogThroughput(results);
    } else {
      LogLatency(results);
    }
  }

 private:
  /*############################################################################
   * Internal constants
   *##########################################################################*/

  /// @brief Limit the target of latency calculation.
  static constexpr size_t kMaxLatencyNum = 1e6;

  /// @brief Targets for calculating parcentile latency.
  static constexpr auto kDefaultLatency =
      "0.01,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,"
      "0.55,0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,0.99";

  /*############################################################################
   * Internal utility functions
   *##########################################################################*/

  /**
   * @brief Run a worker thread to measure throughput or latency.
   *
   * @param result_p A promise for notifying a worker is prepared.
   * @param result_p A promise of a worker pointer that holds benchmarking results.
   * @param random_seed A random seed.
   */
  void
  RunWorker(  //
      std::promise<void> prepare_p,
      std::promise<Result_p> result_p,
      const size_t random_seed)
  {
    // prepare a worker
    Worker_p worker{nullptr};
    const auto &operations = ops_engine_.Generate(exec_num_, random_seed);
    worker = std::make_unique<Worker_t>(bench_target_, std::move(operations));

    // the preparation has finished, so wait other workers
    {
      std::unique_lock lock{x_mtx_};
      prepare_p.set_value();  // notify the main thread of the complete of preparation
      cond_.wait(lock, [this] { return ready_for_benchmarking_; });
    }

    // start when all workers are ready for benchmarking
    if (measure_throughput_) {
      worker->MeasureThroughput(is_running_);
    } else {
      worker->MeasureLatency(is_running_);
    }

    result_p.set_value_at_thread_exit(worker->MoveMeasurements());
  }

  /**
   * @brief Compute a throughput score and output it to stdout.
   *
   * @param results benchmarking results.
   */
  void
  LogThroughput(const std::vector<Result_p> &results) const
  {
    size_t exec_num = 0;
    size_t avg_nano_time = 0;
    for (auto &&result : results) {
      exec_num += result->GetTotalExecNum();
      avg_nano_time += result->GetTotalExecTime();
    }
    avg_nano_time /= thread_num_;

    const auto throughput = exec_num / (avg_nano_time / 1E9);

    if (output_as_csv_) {
      std::cout << throughput << std::endl;
    } else {
      std::cout << "Throughput [Ops/s]: " << throughput << std::endl;
    }
  }

  /**
   * @brief Compute percentiled latency and output it to stdout.
   *
   * @param results benchmarking results.
   */
  void
  LogLatency(const std::vector<Result_p> &results) const
  {
    std::vector<size_t> latencies;
    latencies.reserve(kMaxLatencyNum);

    // sort all execution time
    for (size_t i = 0; i < thread_num_; ++i) {
      const size_t n = (total_sample_num_ + i) / thread_num_;
      auto &&worker_latencies = results[i]->GetLatencies(n);
      latencies.insert(latencies.end(), worker_latencies.begin(), worker_latencies.end());
    }
    std::sort(latencies.begin(), latencies.end());

    Log("Percentiled Latency [ns]:");
    for (auto &&percentile : target_latency_) {
      const size_t percentiled_idx = (percentile == 1.0) ? latencies.size() - 1  //
                                                         : latencies.size() * percentile;
      if (!output_as_csv_) {
        std::cout << "  " << std::fixed << std::setprecision(2) << percentile << ": ";
      } else {
        std::cout << percentile << ",";
      }
      std::cout << latencies[percentiled_idx] << std::endl;
    }
  }

  /**
   * @brief Log a message to stdout if the output mode is `text`.
   *
   * @param message An output message.
   */
  void
  Log(const std::string &message) const
  {
    if (!output_as_csv_) {
      std::cout << message << std::endl;
    }
  }

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief The number of executions of each worker.
  const size_t exec_num_{};

  /// @brief The number of worker threads.
  const size_t thread_num_{};

  /// @brief A base random seed.
  const size_t random_seed_{};

  /// @brief A flag to measure throughput (if true) or latency (if false).
  const bool measure_throughput_{};

  /// @brief A flat to output measured results as CSV or TXT.
  const bool output_as_csv_{};

  /// @brief The total number of sampled execution time for computing percentiled latency.
  size_t total_sample_num_{kMaxLatencyNum};

  /// @brief Targets for calculating parcentile latency.
  std::vector<double> target_latency_{};

  /// @brief A benchmaring target.
  Target &bench_target_{};

  /// @brief An target operation generator.
  OperationEngine &ops_engine_{};

  /// @brief An exclusive mutex for waking up worker threads.
  std::mutex x_mtx_{};

  /// @brief A conditional variable for waking up worker threads.
  std::condition_variable cond_{};

  /// @brief A flag for interrupting workers.
  std::atomic_bool is_running_{true};

  /// @brief Seconds to timeout.
  std::chrono::seconds timeout_in_sec_{};

  /// @brief A flag for waking up worker threads.
  bool ready_for_benchmarking_{false};
};

}  // namespace dbgroup::benchmark

#endif  // CPP_BENCHMARK_BENCHMARK_BENCHMARKER_HPP
