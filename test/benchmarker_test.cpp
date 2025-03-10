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

#include "dbgroup/benchmark/benchmarker.hpp"

// C++ standard libraries
#include <cstddef>
#include <memory>
#include <shared_mutex>

// external sources
#include "gtest/gtest.h"

// local sources
#include "operation_engine.hpp"
#include "target.hpp"

namespace dbgroup::benchmark::test
{
template <class Competitor>
class BenchmarkerFixture : public ::testing::Test
{
  /*##########################################################################*
   * Type aliases
   *##########################################################################*/

  using Target = ::dbgroup::example::Target<Competitor>;
  using OperationEngine = ::dbgroup::example::OperationEngine;
  using Benchmarker_t = Benchmarker<Target, OperationEngine>;
  using Builder = typename Benchmarker_t::Builder;

 protected:
  /*##########################################################################*
   * Constants
   *##########################################################################*/

  static constexpr size_t kRandomSeed = 0;
  static constexpr bool kThroughput = true;
  static constexpr bool kLatency = false;
  static constexpr size_t kThreadNum = DBGROUP_TEST_THREAD_NUM;

  /*##########################################################################*
   * Setup/Teardown
   *##########################################################################*/

  void
  SetUp() override
  {
  }

  void
  TearDown() override
  {
  }

  /*##########################################################################*
   * Functions for verification
   *##########################################################################*/

  void
  VerifyRunBench(  //
      const size_t thread_num)
  {
    Builder builder{target_, "Bench for testing", op_engine_};
    builder.SetThreadNum(thread_num);
    builder.SetRandomSeed(kRandomSeed);

    benchmarker_ = builder.Build();
    benchmarker_->Run();
  }

 private:
  /*##########################################################################*
   * Internal member variables
   *##########################################################################*/

  Target target_{};

  OperationEngine op_engine_{};

  std::unique_ptr<Benchmarker_t> benchmarker_{};
};

/*############################################################################*
 * Preparation for typed testing
 *############################################################################*/

using Competitors = ::testing::Types<  //
    std::shared_mutex,                 //
    example::BackOffLock,              //
    example::MCSLock,                  //
    example::OptimisticLock>;
TYPED_TEST_SUITE(BenchmarkerFixture, Competitors);

/*############################################################################*
 * Unit test definitions
 *############################################################################*/

TYPED_TEST(BenchmarkerFixture, RunBenchWithSingleWorkerSucceed)
{  //
  TestFixture::VerifyRunBench(1);
}

TYPED_TEST(BenchmarkerFixture, RunBenchWithMultiWorkersSucceed)
{
  TestFixture::VerifyRunBench(TestFixture::kThreadNum);
}

}  // namespace dbgroup::benchmark::test
