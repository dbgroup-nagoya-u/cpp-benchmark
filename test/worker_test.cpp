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

#include "benchmark/component/worker.hpp"

#include <memory>

#include "benchmark/component/stopwatch.hpp"
#include "sample_operation.hpp"
#include "sample_operation_engine.hpp"
#include "sample_target.hpp"

// use GoogleTest for unit testing
#include "gtest/gtest.h"

namespace dbgroup::benchmark::component::test
{
template <class Implementation>
class WorkerFixture : public ::testing::Test
{
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Target_t = SampleTarget<Implementation>;
  using Worker_t = Worker<Target_t, SampleOperation>;

 protected:
  /*####################################################################################
   * Setup/Teardown
   *##################################################################################*/

  void
  SetUp() override
  {
    SampleOperationEngine ops_engine{};

    worker_ = std::make_unique<Worker_t>(target_, ops_engine.Generate(kExecNum, kRandomSeed));
  }

  void
  TearDown() override
  {
  }

  /*####################################################################################
   * Functions for verification
   *##################################################################################*/

  void
  VerifyMeasureThroughput()
  {
    std::atomic_bool is_running{true};
    stopwatch_.Start();
    worker_->MeasureThroughput(is_running);
    stopwatch_.Stop();

    // check the total execution time is reasonable
    const auto wrapperred_exec_time = stopwatch_.GetNanoDuration();
    const auto &results = worker_->MoveMeasurements();
    EXPECT_GE(results->GetTotalExecTime(), 0);
    EXPECT_LE(results->GetTotalExecTime(), wrapperred_exec_time);

    // check the worker performs all the operations
    EXPECT_EQ(kExecNum, target_.GetSum());
  }

  void
  VerifyMeasureLatency()
  {
    std::atomic_bool is_running{true};
    stopwatch_.Start();
    worker_->MeasureLatency(is_running);
    stopwatch_.Stop();

    // check random sampling is performed
    const auto &results = worker_->MoveMeasurements();
    const auto latencies = results->GetLatencies(kSampleNum);
    EXPECT_EQ(kSampleNum, latencies.size());

    // check each latency is reasonable
    const auto wrapperred_exec_time = stopwatch_.GetNanoDuration();
    for (auto &&latency : latencies) {
      EXPECT_GE(latency, 0);
      EXPECT_LE(latency, wrapperred_exec_time);
    }

    // check the worker performs all the operations
    EXPECT_EQ(kExecNum, target_.GetSum());
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  static constexpr size_t kExecNum = 1e3;
  static constexpr double kSampleNum = 1e2;
  static constexpr size_t kRandomSeed = 0;

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  Target_t target_{};

  std::unique_ptr<Worker_t> worker_{};

  StopWatch stopwatch_{};
};

/*######################################################################################
 * Preparation for typed testing
 *####################################################################################*/

using Implementations = ::testing::Types<std::mutex, std::atomic_size_t>;
TYPED_TEST_SUITE(WorkerFixture, Implementations);

/*######################################################################################
 * Unit test definitions
 *####################################################################################*/

TYPED_TEST(WorkerFixture, MeasureThroughputWithSampleIncrementorMeasureReasonableExecutionTime)
{
  TestFixture::VerifyMeasureThroughput();
}

TYPED_TEST(WorkerFixture, MeasureLatencyWithSampleIncrementorMeasureReasonableLatency)
{
  TestFixture::VerifyMeasureLatency();
}

}  // namespace dbgroup::benchmark::component::test
