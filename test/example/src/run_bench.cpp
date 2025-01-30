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

// C++ standard libraries
#include <shared_mutex>

// external libraries
#include "dbgroup/benchmark/benchmarker.hpp"
#include "dbgroup/benchmark/validator.hpp"

// local sources
#include "operation_engine.hpp"
#include "target.hpp"

auto
main(  //
    [[maybe_unused]] const int argc,
    [[maybe_unused]] const char *argv[])  //
    -> int
{
  using Target = ::dbgroup::example::Target<std::shared_mutex>;
  using OperationEngine = ::dbgroup::example::OperationEngine;
  using Benchmarker = ::dbgroup::benchmark::Benchmarker<Target, OperationEngine>;
  using Builder = typename Benchmarker::Builder;

  Target target{};
  OperationEngine op_engine{};
  Builder builder{target, "std::shared_mutex", op_engine};
  // builder.SetThreadNum(...);
  // builder.SetTargetLatency(...);
  // builder.SetTimeOut(...);
  // builder.SetRandomSeed(...);
  // builder.OutputAsCSV(...);
  auto &&bench = builder.Build();
  bench->Run();

  return 0;
}
