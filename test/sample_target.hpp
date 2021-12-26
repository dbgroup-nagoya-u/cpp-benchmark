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

#include <atomic>
#include <mutex>

#include "common.hpp"
#include "sample_operation.hpp"

template <class Implementation>
class SampleTarget
{
 public:
  constexpr SampleTarget() = default;

  void Execute(SampleOperation ops);

  [[nodiscard]] auto
  GetSum() const  //
      -> size_t
  {
    return sum_;
  }

 private:
  /// target data
  size_t sum_{0};

  /// mutex for lock-based incrementor
  std::mutex mtx_{};
};

template <>
inline void
SampleTarget<std::mutex>::Execute(const SampleOperation ops)
{
  const std::unique_lock<std::mutex> guard{mtx_};

  sum_ += ops.val;
}

template <>
inline void
SampleTarget<std::atomic_size_t>::Execute(const SampleOperation ops)
{
  reinterpret_cast<std::atomic_size_t*>(&sum_)->fetch_add(ops.val);
}

#endif  // CPP_BENCHMARK_TEST_SAMPLE_TARGET_H_
