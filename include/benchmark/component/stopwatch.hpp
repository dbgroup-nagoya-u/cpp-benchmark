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

#ifndef CPP_BENCHMARK_BENCHMARK_COMPONENT_STOPWATCH_HPP
#define CPP_BENCHMARK_BENCHMARK_COMPONENT_STOPWATCH_HPP

// C++ standard libraries
#include <chrono>

// local sources
#include "benchmark/component/common.hpp"

namespace dbgroup::benchmark::component
{
/**
 * @brief A class to measure time duration.
 *
 */
class StopWatch
{
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Clock_t = ::std::chrono::high_resolution_clock;

 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  /**
   * @brief Create a new StopWatch object.
   *
   */
  constexpr StopWatch() = default;

  constexpr StopWatch(const StopWatch &) = default;
  constexpr StopWatch(StopWatch &&) = default;

  constexpr auto operator=(const StopWatch &obj) -> StopWatch & = default;
  constexpr auto operator=(StopWatch &&) -> StopWatch & = default;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  /**
   * @brief Destroy the StopWatch object.
   *
   */
  ~StopWatch() = default;

  /*####################################################################################
   * Public functions
   *##################################################################################*/

  /**
   * @brief Start this stopwatch to measure time duration.
   *
   */
  void
  Start()
  {
    start_time_ = Clock_t::now();
  }

  /**
   * @brief Stop this stopwatch.
   *
   */
  void
  Stop()
  {
    end_time_ = Clock_t::now();
  }

  /**
   * @brief Get a measured time duration.
   *
   * If Start and Stop functions are not called previously, the return value is
   * undefined.
   *
   * @return a measured time duration [ns].
   */
  [[nodiscard]] constexpr auto
  GetNanoDuration() const  //
      -> size_t
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_ - start_time_).count();
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// a starting timestamp
  Clock_t::time_point start_time_{};

  /// an end timestamp
  Clock_t::time_point end_time_{};
};

}  // namespace dbgroup::benchmark::component

#endif  // CPP_BENCHMARK_BENCHMARK_COMPONENT_STOPWATCH_HPP
