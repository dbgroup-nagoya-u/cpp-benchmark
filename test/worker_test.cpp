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
#include "gtest/gtest.h"

// library headers
#include "dbgroup/benchmark/component/stopwatch.hpp"

// local sources
#include "operation_engine.hpp"
#include "target.hpp"

namespace dbgroup::benchmark::component::test
{
template <class Competitor>
class WorkerFixture : public ::testing::Test
{
  /*############################################################################
   * Type aliases
   *##########################################################################*/

  using Target = ::dbgroup::example::Target<Competitor>;
  using OperationEngine = ::dbgroup::example::OperationEngine;
  using TestWorker = Worker<Target, OperationEngine>;

 protected:
  /*############################################################################
   * Setup/Teardown
   *##########################################################################*/

  void
  SetUp() override
  {
    is_running_ = true;
    worker_ = std::make_unique<TestWorker>(target_, ops_engine_, is_running_, 0, kRandomSeed);
  }

  void
  TearDown() override
  {
    worker_ = nullptr;
  }

  /*############################################################################
   * Functions for verification
   *##########################################################################*/

  void
  VerifyMeasureThroughput()
  {
    stopwatch_.Start();
    worker_->MeasureThroughput();
    stopwatch_.Stop();

    // check the total execution time is reasonable
    const auto wrapperred_exec_time = stopwatch_.GetNanoDuration();
    const auto &sketch = worker_->MoveSketch();
    EXPECT_GE(sketch.GetTotalExecTime(), 0);
    EXPECT_LE(sketch.GetTotalExecTime(), wrapperred_exec_time);
  }

  void
  VerifyMeasureLatency()
  {
    stopwatch_.Start();
    worker_->MeasureLatency();
    stopwatch_.Stop();

    const auto &sketch = worker_->MoveSketch();
    auto prev = sketch.Quantile(0, 0);
    for (size_t i = 1; i < 100; ++i) {  // NOLINT
      auto cur = sketch.Quantile(0, i);
      EXPECT_LE(prev, cur);
      prev = cur;
    }
  }

 private:
  /*############################################################################
   * Internal constants
   *##########################################################################*/

  static constexpr size_t kRandomSeed = 0;

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  Target target_{};

  OperationEngine ops_engine_{};

  std::atomic_bool is_running_{};

  std::unique_ptr<TestWorker> worker_{};

  StopWatch stopwatch_{};
};

/*##############################################################################
 * Preparation for typed testing
 *############################################################################*/

using Competitors = ::testing::Types<  //
    std::shared_mutex,                 //
    example::BackOffLock,              //
    example::MCSLock,                  //
    example::OptimisticLock>;
TYPED_TEST_SUITE(WorkerFixture, Competitors);

/*##############################################################################
 * Unit test definitions
 *############################################################################*/

TYPED_TEST(WorkerFixture, MeasureThroughputWithSampleIncrementorMeasureReasonableExecutionTime)
{
  TestFixture::VerifyMeasureThroughput();
}

TYPED_TEST(WorkerFixture, MeasureLatencyWithSampleIncrementorMeasureReasonableLatency)
{
  TestFixture::VerifyMeasureLatency();
}

}  // namespace dbgroup::benchmark::component::test
