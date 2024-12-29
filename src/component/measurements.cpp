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

// the corresponding header
#include "dbgroup/benchmark/component/measurements.hpp"

// C++ standard libraries
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace dbgroup::benchmark::component
{

SimpleDDSketch::SimpleDDSketch(  //
    const size_t ops_num)
    : min_(ops_num, ~0UL),
      max_(ops_num, 0UL),
      exec_nums_(ops_num, 0UL),
      bins_(ops_num, std::array<uint32_t, kBinNum>{})
{
}

void
SimpleDDSketch::operator+=(  //
    const SimpleDDSketch &rhs)
{
  total_exec_num_ += rhs.total_exec_num_;
  total_exec_time_nano_ += rhs.total_exec_time_nano_;

  const auto ops_num = bins_.size();
  for (size_t ops_id = 0; ops_id < ops_num; ++ops_id) {
    exec_nums_[ops_id] += rhs.exec_nums_[ops_id];
    for (size_t i = 0; i < kBinNum; ++i) {
      bins_[ops_id][i] += rhs.bins_[ops_id][i];
    }
  }
}

void
SimpleDDSketch::Add(  //
    const size_t ops_id,
    const size_t cnt,
    const size_t lat)
{
  total_exec_num_ += cnt;
  total_exec_time_nano_ += lat;

  auto &min = min_[ops_id];
  if (lat < min) [[unlikely]] {
    min = lat;
  }
  auto &max = max_[ops_id];
  if (lat > max) [[unlikely]] {
    max = lat;
  }

  const auto pos = (lat == 0) ? 0 : static_cast<size_t>(std::ceil(std::log(lat) / denom_));
  ++bins_[ops_id][pos];
  ++exec_nums_[ops_id];
}

auto
SimpleDDSketch::HasLatency(     //
    const size_t ops_id) const  //
    -> bool
{
  return exec_nums_.at(ops_id) > 0;
}

auto
SimpleDDSketch::Quantile(  //
    const size_t ops_id,
    const double q) const  //
    -> size_t
{
  if (q == 0) return min_[ops_id];
  if (q >= 1.0) return max_[ops_id];

  const auto bound = static_cast<size_t>(q * static_cast<double>(exec_nums_[ops_id] - 1));
  size_t cnt = bins_[ops_id][0];
  size_t i = 0;
  while (i < kBinNum - 1 && cnt <= bound) {
    cnt += bins_[ops_id][++i];
  }
  return static_cast<size_t>(2 * std::pow(kGamma, i) / (kGamma + 1));
}

}  // namespace dbgroup::benchmark::component
