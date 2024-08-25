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

// C++ standard libraries
#include <cmath>
#include <cstddef>
#include <random>
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

  /**
   * @brief Create a new SimpleDDSketch object.
   *
   * @param ops_num The number of operation types.
   */
  SimpleDDSketch(  //
      const size_t ops_num)
      : bins_{ops_num, std::array<uint32_t, kBinNum>{}}
  {
    for (size_t i = 0; i < ops_num; ++i) {
      exec_nums_.emplace_back(0);
    }
  }

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
  void
  operator+=(  //
      const SimpleDDSketch &rhs)
  {
    total_exec_num_ += rhs.total_exec_num_;
    total_exec_time_nano_ += rhs.total_exec_time_nano_;

    const auto ops_num = bins_.size();
    for (size_t ops_id = 0; ops_id < ops_num; ++ops_id) {
      exec_nums_[ops_id] += rhs.exec_nums_[ops_id];
      for (size_t i = 0; i < kBinNum; ++i) {
        bins_[ops_id][i] += rhs.bins_[ops_id][i];
      }
    }
  }

  /*############################################################################
   * Public APIs for throughput
   *##########################################################################*/

  /**
   * @return The total number of executed operations.
   */
  [[nodiscard]] auto
  GetTotalExecNum() const  //
      -> size_t
  {
    return total_exec_num_;
  }

  /**
   * @return Total execution time.
   */
  [[nodiscard]] auto
  GetTotalExecTime() const  //
      -> size_t
  {
    return total_exec_time_nano_;
  }

  /**
   * @brief Set the total number of executed operations.
   *
   * @param total_exec_num The total number of executed operations.
   */
  void
  SetTotalExecNum(  //
      const size_t total_exec_num)
  {
    total_exec_num_ = total_exec_num;
  }

  /**
   * @brief Set a total execution time.
   *
   * @param total_exec_time_nano A total execution time [ns].
   */
  void
  SetTotalExecTime(  //
      const size_t total_exec_time_nano)
  {
    total_exec_time_nano_ = total_exec_time_nano;
  }

  /*############################################################################
   * Public APIs for latency
   *##########################################################################*/

  /**
   * @brief Add new latency to measuring results.
   *
   * @param ops_id The ID of a target operation.
   * @param lat A measured latency [ns].
   */
  void
  AddLatency(  //
      const size_t ops_id,
      const size_t lat)
  {
    const auto pos = (lat == 0) ? 0 : static_cast<size_t>(std::ceil(std::log(lat) / denom_));
    ++bins_[ops_id][pos];
    ++exec_nums_[ops_id];
  }

  /**
   * @param ops_id The ID of a target operation.
   * @retval true if a target operation was executed.
   * @retval false otherwise.
   */
  auto
  HasLatency(               //
      const size_t ops_id)  //
      -> bool
  {
    return exec_nums_.at(ops_id) > 0;
  }

  /**
   * @param ops_id The ID of a target operation.
   * @param q A target quantile value.
   * @return The latency of given quantile.
   */
  auto
  Quantile(  //
      const size_t ops_id,
      const double q) const  //
      -> size_t
  {
    const size_t bound = q * exec_nums_[ops_id];
    size_t cnt = 0;
    for (size_t i = 0; i < kBinNum; ++i) {
      cnt += bins_[ops_id][i];
      if (cnt > bound) {
        return 2 * std::pow(kGamma, i) / (kGamma + 1);
      }
    }
    return 0;
  }

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
  static inline const double denom_ = std::log(kGamma);

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief The number of executed operations.
  size_t total_exec_num_{0};

  /// @brief Total execution time [ns].
  size_t total_exec_time_nano_{0};

  /// @brief The number of executions for each operations.
  std::vector<size_t> exec_nums_{};

  /// @brief Execution time for each operation [ns].
  std::vector<std::array<uint32_t, kBinNum>> bins_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // CPP_BENCHMARK_BENCHMARK_COMPONENT_MEASUREMENTS_HPP
