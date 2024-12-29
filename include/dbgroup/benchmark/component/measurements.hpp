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

#ifndef DBGROUP_BENCHMARK_COMPONENT_MEASUREMENTS_HPP_
#define DBGROUP_BENCHMARK_COMPONENT_MEASUREMENTS_HPP_

// C++ standard libraries
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dbgroup::benchmark::component
{
/**
 * @brief A class for computing approximated quantile.
 *
 * This implementation is based on DDSketch [1] but is simplified. This uses the
 * fixed number of bins and ignores the performance of quantile queries.
 *
 * [1] Charles Masson et al., "DDSketch: A fast and fully-mergeable quantile
 * sketch with relative-error guarantees," PVLDB, Vol. 12, No. 12, pp. 2195-2205,
 * 2019.
 */
class SimpleDDSketch
{
 public:
  /*############################################################################
   * Public constructors and assignment operators
   *##########################################################################*/

  constexpr SimpleDDSketch() = default;

  /**
   * @brief Create a new SimpleDDSketch object.
   *
   * @param ops_num The number of operation types.
   */
  explicit SimpleDDSketch(  //
      size_t ops_num);

  SimpleDDSketch(const SimpleDDSketch &) = default;
  SimpleDDSketch(SimpleDDSketch &&) = default;

  auto operator=(const SimpleDDSketch &obj) -> SimpleDDSketch & = default;
  auto operator=(SimpleDDSketch &&) -> SimpleDDSketch & = default;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  /**
   * @brief Destroy the SimpleDDSketch object.
   *
   */
  ~SimpleDDSketch() = default;

  /*############################################################################
   * Public operators
   *##########################################################################*/

  /**
   * @brief Merge a given sketch into this.
   *
   * @param rhs A sketch to be merged.
   */
  void operator+=(  //
      const SimpleDDSketch &rhs);

  /*############################################################################
   * Public APIs
   *##########################################################################*/

  /**
   * @return The total number of executed operations.
   */
  [[nodiscard]] constexpr auto
  GetTotalExecNum() const  //
      -> size_t
  {
    return total_exec_num_;
  }

  /**
   * @return Total execution time.
   */
  [[nodiscard]] constexpr auto
  GetTotalExecTime() const  //
      -> size_t
  {
    return total_exec_time_nano_;
  }

  /**
   * @brief Add new latency to measuring results.
   *
   * @param ops_id The ID of a target operation.
   * @param cnt The number of executions for throughput.
   * @param lat A measured latency [ns].
   */
  void Add(  //
      size_t ops_id,
      size_t cnt,
      size_t lat);

  /**
   * @param ops_id The ID of a target operation.
   * @retval true if a target operation was executed.
   * @retval false otherwise.
   */
  [[nodiscard]] auto HasLatency(  //
      size_t ops_id) const        //
      -> bool;

  /**
   * @param ops_id The ID of a target operation.
   * @param q A target quantile value.
   * @return The latency of given quantile.
   */
  [[nodiscard]] auto Quantile(  //
      size_t ops_id,
      double q) const  //
      -> size_t;

 private:
  /*############################################################################
   * Internal constants
   *##########################################################################*/

  /// @brief The number of bins.
  static constexpr size_t kBinNum = 2048;

  /// @brief A desired relative error.
  static constexpr double kAlpha = 0.01;

  /// @brief The base value for approximation.
  static constexpr double kGamma = (1.0 + kAlpha) / (1.0 - kAlpha);

  /// @brief The denominator for the logarithm change of base.
  static inline const double denom_ = std::log(kGamma);  // NOLINT

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief The number of executed operations.
  size_t total_exec_num_{};

  /// @brief Total execution time [ns].
  size_t total_exec_time_nano_{};

  /// @brief The number of executions for each operations.
  std::vector<size_t> exec_nums_{};

  /// @brief Execution time for each operation [ns].
  std::vector<std::array<uint32_t, kBinNum>> bins_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // DBGROUP_BENCHMARK_COMPONENT_MEASUREMENTS_HPP_
