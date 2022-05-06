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

#ifndef CPP_BENCHMARK_BENCHMARK_COMPONENT_MEASUREMENTS_HPP
#define CPP_BENCHMARK_BENCHMARK_COMPONENT_MEASUREMENTS_HPP

#include <random>
#include <vector>

#include "common.hpp"

namespace dbgroup::benchmark::component
{
/**
 * @brief A class for dealing with benchmarking results.
 *
 */
class Measurements
{
 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  /**
   * @brief Create a new Measurements object.
   *
   */
  constexpr Measurements() = default;

  Measurements(const Measurements &) = default;
  auto operator=(const Measurements &obj) -> Measurements & = default;
  Measurements(Measurements &&) = default;
  auto operator=(Measurements &&) -> Measurements & = default;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  /**
   * @brief Destroy the Measurements object.
   *
   */
  ~Measurements() = default;

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

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

  /**
   * @brief Set a total execution time.
   *
   * @param total_exec_time_nano a total execution time [ns].
   */
  void
  SetTotalExecTime(const size_t total_exec_time_nano)
  {
    total_exec_time_nano_ = total_exec_time_nano;
  }

  /**
   * @brief Add new latency to measuring results.
   *
   * @param latency_nano latency [ns].
   */
  void
  AddLatency(const size_t latency_nano)
  {
    latencies_nano_.emplace_back(latency_nano);
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// total execution time [ns]
  size_t total_exec_time_nano_{0};

  /// execution time for each operation [ns]
  std::vector<size_t> latencies_nano_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // CPP_BENCHMARK_BENCHMARK_COMPONENT_MEASUREMENTS_HPP
