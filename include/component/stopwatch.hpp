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

#include <chrono>

#include "common.hpp"

namespace dbgroup::benchmark::component
{
class StopWatch
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using Clock_t = ::std::chrono::high_resolution_clock;

 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  constexpr StopWatch() : start_time_{}, end_time_{} {}

  constexpr StopWatch(const StopWatch &) = default;
  constexpr StopWatch &operator=(const StopWatch &obj) = default;
  constexpr StopWatch(StopWatch &&) = default;
  constexpr StopWatch &operator=(StopWatch &&) = default;

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  ~StopWatch() = default;

  /*################################################################################################
   * Public functions
   *##############################################################################################*/

  void
  Start()
  {
    start_time_ = Clock_t::now();
  }

  void
  Stop()
  {
    end_time_ = Clock_t::now();
  }

  constexpr size_t
  GetNanoDuration() const
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_ - start_time_).count();
  }

 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  Clock_t::time_point start_time_;

  Clock_t::time_point end_time_;
};

}  // namespace dbgroup::benchmark::component
