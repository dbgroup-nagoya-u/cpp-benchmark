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

#include "worker.hpp"

#include "gtest/gtest.h"
#include "pmwcas.h"

template <class MwCASImplementation>
class WorkerFixture : public ::testing::Test
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using ZipfGenerator = ::dbgroup::random::zipf::ZipfGenerator;
  using Worker_t = Worker<MwCASImplementation>;

 protected:
  /*################################################################################################
   * Setup/Teardown
   *##############################################################################################*/

  void
  SetUp() override
  {
    if constexpr (std::is_same_v<MwCASImplementation, PMwCAS>) {
      // prepare PMwCAS descriptor pool
      pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                          pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
      pmwcas_desc_pool =
          std::make_unique<PMwCAS>(static_cast<uint32_t>(8192 * 1), static_cast<uint32_t>(1));
    }

    target_fields.reserve(kMaxTargetNum);
    for (size_t i = 0; i < kMaxTargetNum; ++i) {
      target_fields.emplace_back(new uint64_t{0});
    }

    zipf_engine = ZipfGenerator{kMaxTargetNum, kSkewParameter};

    worker = std::make_unique<Worker_t>(target_fields, kMaxTargetNum, kExecNum, zipf_engine,
                                        kRandomSeed);
  }

  void
  TearDown() override
  {
    for (auto &&addr : target_fields) {
      delete addr;
    }
  }

  /*################################################################################################
   * Functions for verification
   *##############################################################################################*/

  void
  VerifyMeasureThroughput()
  {
    const auto start_time = Clock::now();
    worker->MeasureThroughput();
    const auto end_time = Clock::now();
    const auto total_time = GetNanoDuration(start_time, end_time);

    EXPECT_GE(worker->GetTotalExecTime(), 0);
    EXPECT_LE(worker->GetTotalExecTime(), total_time);
  }

  void
  VerifyMeasureLatency()
  {
    const auto start_time = Clock::now();
    worker->MeasureLatency();
    const auto end_time = Clock::now();
    const auto total_time = GetNanoDuration(start_time, end_time);

    worker->SortLatencies(kExecNum);
    const auto latencies = worker->GetLatencies();

    // check whether latencies are sorted
    size_t prev_latency = 0;
    for (auto &&latency : latencies) {
      EXPECT_LE(prev_latency, latency);
      prev_latency = latency;
    }
    EXPECT_LE(latencies.back(), total_time);
  }

 private:
  /*################################################################################################
   * Internal constants
   *##############################################################################################*/

  static constexpr size_t kExecNum = 1e3;
  static constexpr double kSkewParameter = 0;
  static constexpr size_t kRandomSeed = 0;

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  std::vector<uint64_t *> target_fields;
  ZipfGenerator zipf_engine;
  std::unique_ptr<Worker_t> worker;
};

/*##################################################################################################
 * Preparation for typed testing
 *################################################################################################*/

using MwCASImplementations = ::testing::Types<MwCAS, PMwCAS, SingleCAS>;
TYPED_TEST_CASE(WorkerFixture, MwCASImplementations);

/*##################################################################################################
 * Unit test definitions
 *################################################################################################*/

TYPED_TEST(WorkerFixture, MeasureThroughput_SwapSameFields_MeasureReasonableExecutionTime)
{
  TestFixture::VerifyMeasureThroughput();
}

TYPED_TEST(WorkerFixture, MeasureLatency_SwapSameFields_MeasureReasonableLatency)
{
  TestFixture::VerifyMeasureLatency();
}
