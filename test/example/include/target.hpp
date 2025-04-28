/*
 * Copyright 2024 Database Group, Nagoya University
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

#ifndef CPP_BENCHMARK_TEST_EXAMPLE_TARGET_H_
#define CPP_BENCHMARK_TEST_EXAMPLE_TARGET_H_

// C++ standard libraries
#include <cstddef>
#include <cstdint>

// external libraries
#include "dbgroup/lock/mcs_lock.hpp"
#include "dbgroup/lock/optimistic_lock.hpp"
#include "dbgroup/lock/pessimistic_lock.hpp"

// local sources
#include "constants.hpp"
#include "operation_engine.hpp"

namespace dbgroup::example
{
/*############################################################################*
 * Global type aliases
 *############################################################################*/

using BackOffLock = ::dbgroup::lock::PessimisticLock;
using MCSLock = ::dbgroup::lock::MCSLock;
using OptimisticLock = ::dbgroup::lock::OptimisticLock;

/*############################################################################*
 * Class definitions
 *############################################################################*/

/**
 * @brief A class for representing a benchmark target.
 *
 * @tparam Competitor A benchmark competitor.
 * @note Our benchmark template requires this type.
 */
template <class Competitor>
class Target
{
 public:
  /*##########################################################################*
   * Public types
   *##########################################################################*/

  using OPType = OperationEngine::OPType;

  /*##########################################################################*
   * Public APIs
   *##########################################################################*/

  /**
   * @brief Set up the current thread as a worker.
   *
   * @note Our benchmark template requires this function.
   */
  constexpr void
  SetUpForWorker() const
  {
  }

  /**
   * @brief Preprocess before running benchmark.
   *
   * @note Our benchmark template requires this function.
   */
  constexpr void
  PreProcess() const
  {
  }

  /**
   * @brief Execute operations according to inputs.
   *
   * @param type A desired operation type.
   * @param pos The position of a target page.
   * @return The number of executions.
   * @note Our benchmark template requires this function.
   */
  auto Execute(  //
      OPType type,
      uint32_t pos)  //
      -> size_t;

  /**
   * @brief Postprocess after running benchmark.
   *
   * @note Our benchmark template requires this function.
   */
  constexpr void
  PostProcess() const
  {
  }

  /**
   * @brief Tear the current thread down as the worker.
   *
   * @note Our benchmark template requires this function.
   */
  constexpr void
  TearDownForWorker() const
  {
  }

 private:
  /*##########################################################################*
   * Internal types
   *##########################################################################*/

  /**
   * @brief A class for representing data pages.
   *
   */
  struct alignas(kCachelineSize) Page {
    /// @brief A lock instance for concurrency controls.
    Competitor lock{};

    /// @brief The begin position of contents.
    alignas(kCachelineSize) uint64_t values[kElementNum] = {};
  };

  /*##########################################################################*
   * Internal member variables
   *##########################################################################*/

  /// @brief Target pages.
  Page pages_[kPageNum]{};
};

}  // namespace dbgroup::example

#endif  // CPP_BENCHMARK_TEST_EXAMPLE_TARGET_H_
