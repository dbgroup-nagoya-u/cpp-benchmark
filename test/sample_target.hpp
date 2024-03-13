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

#ifndef CPP_BENCHMARK_TEST_SAMPLE_TARGET_H_
#define CPP_BENCHMARK_TEST_SAMPLE_TARGET_H_

// C++ standard libraries
#include <atomic>
#include <mutex>

// local sources
#include "common.hpp"
#include "sample_operation.hpp"

template <class Implementation>
class SampleTarget
{
 public:
  constexpr SampleTarget() = default;

  void
  SetUpForWorker()
  {
  }

  void
  TearDownForWorker()
  {
  }

  auto Execute(  //
      SampleOperation ops) -> size_t;

  [[nodiscard]] auto
  GetSum() const  //
      -> size_t
  {
    return sum_;
  }

 private:
  /// @brief Target data
  size_t sum_{0};

  /// @brief Mutex for lock-based incrementor
  std::mutex mtx_{};
};

template <>
inline auto
SampleTarget<std::mutex>::Execute(  //
    const SampleOperation ops)      //
    -> size_t
{
  const std::unique_lock<std::mutex> guard{mtx_};

  sum_ += ops.val;

  return 1;
}

template <>
inline auto
SampleTarget<std::atomic_size_t>::Execute(  //
    const SampleOperation ops)              //
    -> size_t
{
  reinterpret_cast<std::atomic_size_t*>(&sum_)->fetch_add(ops.val);  // NOLINT

  return 1;
}

#endif  // CPP_BENCHMARK_TEST_SAMPLE_TARGET_H_
