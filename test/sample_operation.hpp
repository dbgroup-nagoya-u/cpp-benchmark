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

#ifndef CPP_BENCHMARK_TEST_SAMPLE_OPERATION_H_
#define CPP_BENCHMARK_TEST_SAMPLE_OPERATION_H_

// C++ standard libraries
#include <cstddef>

struct SampleOperation {
  [[nodiscard]] constexpr auto
  GetOpsID() const  //
      -> size_t
  {
    return 0;
  }

  size_t val;
};

#endif  // CPP_BENCHMARK_TEST_SAMPLE_OPERATION_H_
