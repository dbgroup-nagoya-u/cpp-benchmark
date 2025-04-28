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

#ifndef CPP_BENCHMARK_TEST_EXAMPLE_OPERATION_ENGINE_H_
#define CPP_BENCHMARK_TEST_EXAMPLE_OPERATION_ENGINE_H_

// C++ standard libraries
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>

// external libraries
#include "dbgroup/random/zipf.hpp"

// local sources
#include "constants.hpp"

namespace dbgroup::example
{
/**
 * @brief A class for generating benchmark operations.
 *
 * @note Our benchmark template requires this class.
 */
class OperationEngine
{
  /*##########################################################################*
   * Type aliases
   *##########################################################################*/

  using Zipf = ::dbgroup::random::ApproxZipfDistribution<uint32_t>;

 public:
  /*##########################################################################*
   * Public types
   *##########################################################################*/

  /**
   * @brief An enumeration for representing target operations.
   *
   * @note Our benchmark template requires this type.
   */
  enum OPType {
    kRead = 0,
    kWrite,
    kTotalNum,  /// @note This element is mandatory.
  };

  /**
   * @brief A class for representing operation details.
   *
   * @note Our benchmark template requires this type.
   */
  using Operation = uint32_t;

  /**
   * @brief A class for iterating an operation queue.
   *
   * @note Our benchmark template requires this type.
   */
  class OPIter
  {
   public:
    /*########################################################################*
     * Public constructors and assignment operators
     *########################################################################*/

    /**
     * @param rand_seed A random seed.
     */
    explicit OPIter(  //
        const size_t rand_seed)
        : rand_{rand_seed}, pos_{zipf_(rand_)}
    {
    }

    OPIter(const OPIter &) = delete;
    OPIter(OPIter &&) noexcept = default;

    auto operator=(const OPIter &obj) -> OPIter & = delete;
    auto operator=(OPIter &&) noexcept -> OPIter & = default;

    /*########################################################################*
     * Public destructor
     *########################################################################*/

    ~OPIter() = default;

    /*########################################################################*
     * Public APIs
     *########################################################################*/

    /**
     * @retval true if this iterator has other operations.
     * @retval false otherwise.
     * @note Our benchmark template requires this operator.
     */
    [[nodiscard]] constexpr explicit
    operator bool() const
    {
      return cnt_ < kMaxExecNum;
    }

    /**
     * @retval 1st: The current operation type.
     * @retval 2nd: Operation arguments.
     * @note Our benchmark template requires this operator.
     */
    [[nodiscard]] constexpr auto
    operator*() const  //
        -> std::pair<OPType, uint32_t>
    {
      return {type_, pos_};
    }

    /**
     * @brief Advance this iterator.
     *
     * @return Oneself.
     * @note Our benchmark template requires this operator.
     */
    auto
    operator++()  //
        -> OPIter &
    {
      type_ = static_cast<OPType>(static_cast<uint32_t>(type_) ^ 1U);
      pos_ = zipf_(rand_);
      ++cnt_;
      return *this;
    }

   private:
    /*########################################################################*
     * Internal member variables
     *########################################################################*/

    /// @brief A zipf distribution.
    Zipf zipf_{0, kPageNum - 1, 1.0};

    /// @brief A random value generator.
    std::mt19937_64 rand_{};

    /// @brief The position of a target page.
    uint32_t pos_{};

    /// @brief An operation type to be executed.
    /// @note Our benchmark template requires this field.
    OPType type_{};

    /// @brief The number of executed operations.
    size_t cnt_{};
  };

  /*##########################################################################*
   * Public APIs
   *##########################################################################*/

  /**
   * @brief Get the Operation Iter object
   *
   * @param thread_id A unique thread ID.
   * @param rand_seed A random seed.
   * @return An iterator for generating operations.
   * @note Our benchmark template requires this function.
   */
  [[nodiscard]] auto
  GetOPIter(  //
      [[maybe_unused]] const size_t thread_id,
      const size_t rand_seed) const  //
      -> OPIter
  {
    return OPIter{rand_seed};
  }
};

}  // namespace dbgroup::example

#endif  // CPP_BENCHMARK_TEST_EXAMPLE_OPERATION_ENGINE_H_
