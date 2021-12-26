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

#include "benchmark/benchmarker.hpp"

#include <memory>

#include "benchmark/component/stopwatch.hpp"
#include "benchmark/component/worker.hpp"
#include "sample_operation.hpp"
#include "sample_operation_engine.hpp"
#include "sample_target.hpp"

// use GoogleTest for unit testing
#include "gtest/gtest.h"

namespace dbgroup::benchmark::test
{
template <class Implementation>
class BenchmarkerFixture : public ::testing::Test
{
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Target_t = SampleTarget<Implementation>;
  using Worker_t = component::Worker<Target_t, SampleOperation>;
  using Benchmarker_t = Benchmarker<Target_t, SampleOperation, SampleOperationEngine>;

 protected:
  /*####################################################################################
   * Setup/Teardown
   *##################################################################################*/

  void
  SetUp() override
  {
  }

  void
  TearDown() override
  {
  }

  /*####################################################################################
   * Functions for verification
   *##################################################################################*/

  void
  VerifyMeasureThroughput(const size_t thread_num)
  {
    benchmarker_ = std::make_unique<Benchmarker_t>(target_, ops_engine_, kExecNum, thread_num,
                                                   kRandomSeed, true, false);
    benchmarker_->Run();
  }

  void
  VerifyMeasureLatency(const size_t thread_num)
  {
    benchmarker_ = std::make_unique<Benchmarker_t>(target_, ops_engine_, kExecNum, thread_num,
                                                   kRandomSeed, false, false);
    benchmarker_->Run();
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  static constexpr size_t kExecNum = 1e6;
  static constexpr size_t kRandomSeed = 0;

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  Target_t target_{};

  SampleOperationEngine ops_engine_{};

  std::unique_ptr<Benchmarker_t> benchmarker_{};
};

/*######################################################################################
 * Preparation for typed testing
 *####################################################################################*/

using Implementations = ::testing::Types<std::mutex, std::atomic_size_t>;
TYPED_TEST_SUITE(BenchmarkerFixture, Implementations);

/*######################################################################################
 * Unit test definitions
 *####################################################################################*/

TYPED_TEST(BenchmarkerFixture, RunForMeasuringThroughputWithSingleWorkerSucceed)
{
  TestFixture::VerifyMeasureThroughput(1);
}

TYPED_TEST(BenchmarkerFixture, RunForMeasuringLatencyWithSingleWorkerSucceed)
{
  TestFixture::VerifyMeasureLatency(1);
}

TYPED_TEST(BenchmarkerFixture, RunForMeasuringThroughputWithMultiWorkersSucceed)
{
  TestFixture::VerifyMeasureThroughput(kThreadNum);
}

TYPED_TEST(BenchmarkerFixture, RunForMeasuringLatencyWithMultiWorkersSucceed)
{
  TestFixture::VerifyMeasureLatency(kThreadNum);
}

}  // namespace dbgroup::benchmark::test
