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
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Worker_t = component::Worker<Target, Operation>;
  using Worker_p = std::unique_ptr<Worker_t>;
  using Result_p = std::unique_ptr<component::Measurements>;

 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  /**
   * @brief Construct a new Benchmarker object.
   *
   * @param bench_target a reference to an actual target implementation.
   * @param ops_engine a reference to a operation generator.
   * @param exec_num the total number of executions for benchmarking.
   * @param thread_num the number of worker threads.
   * @param random_seed a base random seed.
   * @param measure_throughput a flag for measuring throughput (true) or latency (false).
   * @param output_as_csv a flag to output benchmarking results as CSV or TEXT.
   */
  Benchmarker(  //
      Target &bench_target,
      OperationEngine &ops_engine,
      const size_t exec_num,
      const size_t thread_num,
      const size_t random_seed,
      const bool measure_throughput,
      const bool output_as_csv,
      const std::string &target_name = "NO-NAME TARGET",
      const char *target_latency = kDefaultLatency)
      : total_exec_num_{exec_num},
        thread_num_{thread_num},
        total_sample_num_{(total_exec_num_ < kMaxLatencyNum) ? total_exec_num_ : kMaxLatencyNum},
        random_seed_{random_seed},
        measure_throughput_{measure_throughput},
        output_as_csv_{output_as_csv},
        bench_target_{bench_target},
        ops_engine_{ops_engine}
  {
    if (!measure_throughput_) {
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
  auto operator=(const Benchmarker &obj) -> Benchmarker & = delete;
  Benchmarker(Benchmarker &&) = delete;
  auto operator=(Benchmarker &&) -> Benchmarker & = delete;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  /**
   * @brief Destroy the Benchmarker object.
   *
   */
  ~Benchmarker() { Log("*** FINISH ***\n"); }

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

  /**
   * @brief Run benchmark and output results to stdout.
   *
   */
  void
  Run()
  {
    /*----------------------------------------------------------------------------------
     * Preparation of benchmark workers
     *--------------------------------------------------------------------------------*/
    Log("...Prepare workers for benchmarking.");
    ready_for_benchmarking_ = false;

    std::vector<std::future<Result_p>> futures{};

    // create workers in each thread
    std::mt19937_64 rand{random_seed_};
    for (size_t i = 0; i < thread_num_; ++i) {
      const size_t n = (total_exec_num_ + i) / thread_num_;

      std::promise<Result_p> p{};
      futures.emplace_back(p.get_future());
      std::thread{&Benchmarker::RunWorker, this, std::move(p), n, rand()}.detach();
    }

    // wait for all workers to be created
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    [[maybe_unused]] std::lock_guard s_guard{s_mtx_};

    /*----------------------------------------------------------------------------------
     * Measuring throughput/latency
     *--------------------------------------------------------------------------------*/
    Log("...Run workers.");

    {
      std::lock_guard x_guard{x_mtx_};
      ready_for_benchmarking_ = true;
    }
    cond_.notify_all();

    std::vector<Result_p> results{};
    results.reserve(thread_num_);
    for (auto &&future : futures) {
      results.emplace_back(future.get());
    }

    /*----------------------------------------------------------------------------------
     * Output benchmarkings results
     *--------------------------------------------------------------------------------*/
    Log("...Finish running.");

    if (measure_throughput_) {
      LogThroughput(results);
    } else {
      LogLatency(results);
    }
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  /// limit the target of latency calculation
  static constexpr size_t kMaxLatencyNum = 1e6;

  /// targets for calculating parcentile latency
  static constexpr auto kDefaultLatency =
      "0.01,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,"
      "0.55,0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,0.99";

  /*####################################################################################
   * Internal utility functions
   *##################################################################################*/

  /**
   * @brief Run a worker thread to measure throughput or latency.
   *
   * @param p a promise of a worker pointer that holds benchmarking results.
   * @param exec_num the number of executions for this worker.
   * @param random_seed a random seed.
   */
  void
  RunWorker(  //
      std::promise<Result_p> p,
      const size_t exec_num,
      const size_t random_seed)
  {
    // create a lock to stop a main thread
    std::shared_lock guard{s_mtx_};

    // prepare a worker
    Worker_p worker{nullptr};
    const auto &operations = ops_engine_.Generate(exec_num, random_seed);
    worker = std::make_unique<Worker_t>(bench_target_, std::move(operations));

    // unlock the shared mutex and wait other workers
    {
      std::unique_lock lock{x_mtx_};
      guard.unlock();
      cond_.wait(lock, [this] { return ready_for_benchmarking_; });
    }

    // start when all workers are ready for benchmarking
    if (measure_throughput_) {
      worker->MeasureThroughput();
    } else {
      worker->MeasureLatency();
    }

    p.set_value_at_thread_exit(worker->MoveMeasurements());
  }

  /**
   * @brief Compute a throughput score and output it to stdout.
   *
   * @param results benchmarking results.
   */
  void
  LogThroughput(const std::vector<Result_p> &results) const
  {
    size_t avg_nano_time = 0;
    for (auto &&result : results) {
      avg_nano_time += result->GetTotalExecTime();
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
      const size_t percentiled_idx =
          (percentile == 1.0) ? latencies.size() - 1 : latencies.size() * percentile;  // NOLINT

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
   * @param message an output message
   */
  void
  Log(const std::string &message) const
  {
    if (!output_as_csv_) {
      std::cout << message << std::endl;
    }
  }

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// the total number of executions for benchmarking
  const size_t total_exec_num_{};

  /// the number of worker threads
  const size_t thread_num_{};

  /// the total number of sampled execution time for computing percentiled latency
  const size_t total_sample_num_{};

  /// a base random seed
  const size_t random_seed_{};

  /// a flag to measure throughput (if true) or latency (if false)
  const bool measure_throughput_{};

  /// a flat to output measured results as CSV or TEXT
  const bool output_as_csv_{};

  /// targets for calculating parcentile latency
  std::vector<double> target_latency_{};

  /// a benchmaring target
  Target &bench_target_{};

  /// an target operation generator
  OperationEngine &ops_engine_{};

  /// an exclusive mutex for waking up worker threads
  std::mutex x_mtx_{};

  /// a shared mutex for stopping main process
  std::shared_mutex s_mtx_{};

  /// a conditional variable for waking up worker threads
  std::condition_variable cond_{};

  /// a flag for waking up worker threads
  bool ready_for_benchmarking_{false};
};

}  // namespace dbgroup::benchmark

#endif  // CPP_BENCHMARK_BENCHMARK_BENCHMARKER_HPP
