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

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "random/zipf.hpp"

// declare PMwCAS's descriptor pool globally in order to define a templated worker class
inline std::unique_ptr<PMwCAS> pmwcas_desc_pool = nullptr;

/**
 * @brief A class of a worker thread for benchmarking.
 *
 * @tparam MwCASImplementation An implementation of MwCAS algorithms.
 */
template <class MwCASImplementation>
class Worker
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using ZipfGenerator = ::dbgroup::random::zipf::ZipfGenerator;
  using MwCASTargets = ::std::array<uint64_t *, kMaxTargetNum>;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new Worker object.
   *
   * @param target_fields target fields of MwCAS.
   * @param target_num the number of MwCAS targets for each operation.
   * @param exec_num the number of operations to be executed by this worker.
   * @param zipf_engine a random engine according to Zipf's law.
   * @param random_seed a random seed.
   */
  Worker(  //
      std::vector<uint64_t *> &target_fields,
      const size_t target_num,
      const size_t exec_num,
      ZipfGenerator &zipf_engine,
      const size_t random_seed)
      : total_exec_time_nano_{0}, target_num_{target_num}
  {
    // reserve capacity of dynamic arrays
    latencies_nano_.reserve(exec_num);
    operations_.reserve(exec_num);

    // generate an operation-queue for benchmarking
    std::mt19937_64 rand_engine{random_seed};
    for (size_t i = 0; i < exec_num; ++i) {
      // select target addresses for i-th operation
      MwCASTargets targets;
      for (size_t j = 0; j < target_num; ++j) {
        const auto cur_end = targets.begin() + j;
        uint64_t *addr;
        do {
          addr = target_fields[zipf_engine(rand_engine)];
        } while (std::find(targets.begin(), cur_end, addr) != cur_end);
        targets[j] = addr;
      }

      // sort target addresses for linearization
      std::sort(targets.begin(), targets.begin() + target_num);

      operations_.emplace_back(std::move(targets));
    }
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  /**
   * @brief Destroy the Worker object.
   *
   */
  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   */
  void
  MeasureLatency()
  {
    assert(latencies_nano_.empty());

    for (auto &&operation : operations_) {
      const auto start_time = Clock::now();

      PerformMwCAS(operation);

      const auto end_time = Clock::now();
      latencies_nano_.emplace_back(GetNanoDuration(start_time, end_time));
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   */
  void
  MeasureThroughput()
  {
    const auto start_time = Clock::now();

    for (auto &&operation : operations_) {
      PerformMwCAS(operation);
    }

    const auto end_time = Clock::now();
    total_exec_time_nano_ = GetNanoDuration(start_time, end_time);
  }

  /**
   * @brief Sort execution time to compute percentiled latency.
   *
   * @param sample_num the number of samples to reduce computation cost.
   */
  void
  SortLatencies(const size_t sample_num)
  {
    std::vector<size_t> sampled_latencies;
    sampled_latencies.reserve(sample_num);

    // perform random sampling to reduce sorting targets
    std::uniform_int_distribution<size_t> id_generator{0, latencies_nano_.size() - 1};
    std::mt19937_64 rand_engine{std::random_device{}()};
    for (size_t i = 0; i < sample_num; ++i) {
      const auto sample_idx = id_generator(rand_engine);
      sampled_latencies.emplace_back(latencies_nano_[sample_idx]);
    }

    // sort sampled execution times
    std::sort(sampled_latencies.begin(), sampled_latencies.end());
    latencies_nano_ = std::move(sampled_latencies);
  }

  /**
   * @return the sorted latencies.
   */
  const std::vector<size_t> &
  GetLatencies() const
  {
    return latencies_nano_;
  }

  /**
   * @return total execution time.
   */
  size_t
  GetTotalExecTime() const
  {
    return total_exec_time_nano_;
  }

 private:
  /*################################################################################################
   * Internal utility functions
   *##############################################################################################*/

  /**
   * @brief Perform a MwCAS operation based on its implementation.
   *
   * @param targets target addresses of a MwCAS operation.
   */
  void PerformMwCAS(const MwCASTargets &targets);

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// total execution time [ns]
  size_t total_exec_time_nano_;

  /// execution time for each operation [ns]
  std::vector<size_t> latencies_nano_;

  /// MwCAS target filed addresses for each operation
  std::vector<MwCASTargets> operations_;

  /// the number of MwCAS targets for each operation
  const size_t target_num_;
};

/*##################################################################################################
 * Specializations for each MwCAS implementations
 *################################################################################################*/

template <>
inline void
Worker<MwCAS>::PerformMwCAS(const MwCASTargets &targets)
{
  while (true) {
    MwCAS desc{};
    for (size_t i = 0; i < target_num_; ++i) {
      const auto addr = targets[i];
      const auto old_val = dbgroup::atomic::mwcas::ReadMwCASField<size_t>(addr);
      const auto new_val = old_val + 1;
      desc.AddMwCASTarget(addr, old_val, new_val);
    }

    if (desc.MwCAS()) break;
  }
}

template <>
inline void
Worker<PMwCAS>::PerformMwCAS(const MwCASTargets &targets)
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto desc = pmwcas_desc_pool->AllocateDescriptor();
    auto epoch = pmwcas_desc_pool->GetEpoch();
    epoch->Protect();
    for (size_t i = 0; i < target_num_; ++i) {
      const auto addr = targets[i];
      const auto old_val = reinterpret_cast<PMwCASField *>(addr)->GetValueProtected();
      const auto new_val = old_val + 1;
      desc->AddEntry(addr, old_val, new_val);
    }
    auto success = desc->MwCAS();
    epoch->Unprotect();
    if (success) break;
  }
}

template <>
inline void
Worker<SingleCAS>::PerformMwCAS(const MwCASTargets &targets)
{
  for (size_t i = 0; i < target_num_; ++i) {
    auto target = reinterpret_cast<SingleCAS *>(targets[i]);
    auto old_val = target->load(std::memory_order_relaxed);
    size_t new_val;
    do {
      new_val = old_val + 1;
    } while (!target->compare_exchange_weak(old_val, new_val, std::memory_order_relaxed));
  }
}
