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

#include "dbgroup/benchmark/component/worker.hpp"

// C++ standard libraries
#include <atomic>
#include <cstddef>
#include <memory>
#include <shared_mutex>

// external libraries
#include <gtest/gtest.h>

// external C++ libraries
#include <dbgroup/benchmark/stop_watch.hpp>

// local sources
#include "constants.hpp"
#include "operation_engine.hpp"
#include "target.hpp"

namespace dbgroup::benchmark::component::test
{
template <class Competitor>
class WorkerFixture : public ::testing::Test
{
  /*##########################################################################*
   * Type aliases
   *##########################################################################*/

  using Target = ::dbgroup::example::Target<Competitor>;
  using OperationEngine = ::dbgroup::example::OperationEngine;
  using TestWorker = Worker<Target, OperationEngine>;

 protected:
  /*##########################################################################*
   * Setup/Teardown
   *##########################################################################*/

  void
  SetUp() override
  {
    is_running_ = true;
    worker_ = std::make_unique<TestWorker>(target_, op_engine_, is_running_, 0, kRandomSeed);
  }

  void
  TearDown() override
  {
    worker_ = nullptr;
  }

  /*##########################################################################*
   * Functions for verification
   *##########################################################################*/

  void
  VerifyMeasure()
  {
    using ::dbgroup::example::kMaxExecNum;

    stopwatch_.Start();
    worker_->Measure();
    stopwatch_.Stop();

    const auto sws = worker_->GetMeasurements();
    const auto& read_sw = sws[OperationEngine::kRead];
    const auto& write_sw = sws[OperationEngine::kWrite];
    EXPECT_EQ(read_sw.GetExecNum(), (kMaxExecNum + 1) / 2);
    EXPECT_EQ(write_sw.GetExecNum(), kMaxExecNum / 2);
    EXPECT_EQ(target_.GetWriteNum(), kMaxExecNum / 2);
    EXPECT_TRUE(target_.IsConsistent());

    const auto exec_time = read_sw.GetExecTime() + write_sw.GetExecTime();
    EXPECT_GT(exec_time, 0);
    EXPECT_LE(exec_time, stopwatch_.GetExecTime());

    for (const auto& sw : sws) {
      VerifyQuantiles(sw);
    }
  }

  static void
  VerifyQuantiles(  //
      const StopWatch& sw)
  {
    auto prev = sw.Quantile(0);
    for (size_t i = 1; i < 100; ++i) {  // NOLINT
      auto cur = sw.Quantile(static_cast<double>(i) / 100.0);
      EXPECT_LE(prev, cur);
      prev = cur;
    }
  }

 private:
  /*##########################################################################*
   * Internal constants
   *##########################################################################*/

  static constexpr size_t kRandomSeed = 0;

  /*##########################################################################*
   * Internal member variables
   *##########################################################################*/

  Target target_{};

  OperationEngine op_engine_{};

  std::atomic_bool is_running_{};

  std::unique_ptr<TestWorker> worker_{};

  StopWatch stopwatch_{};
};

/*############################################################################*
 * Preparation for typed testing
 *############################################################################*/

using Competitors = ::testing::Types<  //
    std::shared_mutex,                 //
    example::BackOffLock,              //
    example::MCSLock,                  //
    example::OptimisticLock>;
TYPED_TEST_SUITE(WorkerFixture, Competitors);

/*############################################################################*
 * Unit test definitions
 *############################################################################*/

TYPED_TEST(WorkerFixture, MeasureWithSampleIncrementorMeasureReasonableResults)
{
  TestFixture::VerifyMeasure();
}

}  // namespace dbgroup::benchmark::component::test
