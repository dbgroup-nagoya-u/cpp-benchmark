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

#ifndef CPP_BENCHMARK_TEST_EXAMPLE_CONSTANTS_H_
#define CPP_BENCHMARK_TEST_EXAMPLE_CONSTANTS_H_

// C++ standard libraries
#include <cstddef>
#include <cstdint>

namespace dbgroup::example
{
/*############################################################################*
 * Global constants
 *############################################################################*/

constexpr size_t kPageNum = 1024;

constexpr size_t kMaxExecNum = 1e7;

constexpr size_t kCachelineSize = 64;

constexpr size_t kElementNum = kCachelineSize / sizeof(uint64_t);

}  // namespace dbgroup::example

#endif  // CPP_BENCHMARK_TEST_EXAMPLE_CONSTANTS_H_
