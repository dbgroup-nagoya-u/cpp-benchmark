/*
 * Copyright 2025 Database Group, Nagoya University
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

#ifndef CPP_UTILITY_DBGROUP_BENCHMARK_UTILITY_HPP_
#define CPP_UTILITY_DBGROUP_BENCHMARK_UTILITY_HPP_

// C++ standard libraries
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <functional>
#include <thread>

namespace dbgroup::benchmark
{
/*############################################################################*
 * Concepts for benchmark implementations
 *############################################################################*/

/**
 * @brief A concept for representing operation types.
 *
 * @tparam T A target class.
 */
template <class T>
concept OPTypeEnum = requires(T &x) {
  // public types
  requires T::kTotalNum > 0;
};

/**
 * @brief A concept for representing operation iterators.
 *
 * @tparam T A target class.
 * @tparam OPType A class for representing operation iterators.
 */
template <class T, class OPType, class Operation>
concept OPIterClass = requires(T &x) {
  // public APIs
  { static_cast<bool>(x) } -> std::convertible_to<bool>;
  { *x } -> std::convertible_to<std::pair<OPType, Operation>>;
  { ++x } -> std::convertible_to<T &>;
};

/**
 * @brief A concept for representing operation generators.
 *
 * @tparam T A target class.
 */
template <class T>
concept OPEngineClass = requires(T &x, size_t thread_id, size_t rand_seed) {
  // public types
  requires OPTypeEnum<typename T::OPType>;
  typename T::Operation;
  requires OPIterClass<typename T::OPIter, typename T::OPType, typename T::Operation>;

  // public APIs
  { x.GetOPIter(thread_id, rand_seed) } -> std::convertible_to<typename T::OPIter>;
};

/**
 * @brief A concept for representing benchmark targets.
 *
 * @tparam T A target class.
 */
template <class T>
concept TargetClass = requires(T &x, typename T::OPType type, typename T::Operation ops) {
  // public types
  requires OPTypeEnum<typename T::OPType>;
  typename T::Operation;

  // public types
  { x.SetUpForWorker() };
  { x.PreProcess() };
  { x.Execute(type, ops) } -> std::convertible_to<size_t>;
  { x.PostProcess() };
  { x.TearDownForWorker() };
};

}  // namespace dbgroup::benchmark

#endif  // CPP_UTILITY_DBGROUP_BENCHMARK_UTILITY_HPP_
