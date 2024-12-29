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

#ifndef DBGROUP_BENCHMARK_VALIDATOR_HPP_
#define DBGROUP_BENCHMARK_VALIDATOR_HPP_

// C++ standard libraries
#include <iostream>
#include <stdexcept>
#include <string>

namespace dbgroup::benchmark
{
/*############################################################################*
 * Validators for gflags
 *############################################################################*/

template <class Number>
static auto
ValidatePositiveValue(  //
    const char *flagname,
    const Number value)  //
    -> bool
{
  if (value > 0) return true;

  std::cerr << "ERROR: A value must be positive for " << flagname << "\n";
  return false;
}

static auto
ValidateSkewParameter(  //
    [[maybe_unused]] const char *flagname,
    const double skew)  //
    -> bool
{
  if (skew >= 0) return true;

  std::cerr << "ERROR: A skew parameter must be larther than or equal to zero.\n";
  return false;
}

static auto
ValidateStr2UInt(  //
    [[maybe_unused]] const char *flagname,
    const std::string &str)  //
    -> bool
{
  if (str.empty()) return true;

  try {
    std::stoul(str);
  } catch (const std::invalid_argument &) {
    std::cerr << "ERROR: A string must be unsigned integers\n";
    return false;
  }
  return true;
}

}  // namespace dbgroup::benchmark

#endif  // DBGROUP_BENCHMARK_VALIDATOR_HPP_
