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

#ifndef CPP_BENCHMARK_TEST_SAMPLE_OPERATION_ENGINE_H_
#define CPP_BENCHMARK_TEST_SAMPLE_OPERATION_ENGINE_H_

#include <vector>

#include "common.hpp"
#include "sample_operation.hpp"

class SampleOperationEngine
{
 public:
  constexpr SampleOperationEngine() = default;

  [[nodiscard]] auto
  Generate(  // NOLINT
      const size_t n,
      [[maybe_unused]] const size_t random_seed) const  //
      -> std::vector<SampleOperation>
  {
    return std::vector{n, SampleOperation{1}};
  }
};

#endif  // CPP_BENCHMARK_TEST_SAMPLE_OPERATION_ENGINE_H_
