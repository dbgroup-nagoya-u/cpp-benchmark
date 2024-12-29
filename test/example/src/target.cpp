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
#include "target.hpp"

// C++ standard libraries
#include <cstddef>
#include <cstdint>
#include <shared_mutex>

// local sources
#include "constants.hpp"

namespace dbgroup::example
{
/*############################################################################*
 * Specializations for competitors
 *############################################################################*/

template <>
auto
Target<std::shared_mutex>::Execute(  //
    OPTyep type,
    uint32_t pos)  //
    -> size_t
{
  auto &page = pages_[pos];

  if (type == OPTyep::kRead) {
    [[maybe_unused]] const std::shared_lock guard{page.lock};
    size_t sum = 0;
    for (size_t i = 0; i < kElementNum; ++i) {
      sum += page.values[i];
    }
  } else {  // kWrite
    [[maybe_unused]] const std::lock_guard guard{page.lock};
    for (size_t i = 0; i < kElementNum; ++i) {
      ++(page.values[i]);
    }
  }

  return 1;
}

template <>
auto
Target<BackOffLock>::Execute(  //
    OPTyep type,
    uint32_t pos)  //
    -> size_t
{
  auto &page = pages_[pos];

  if (type == OPTyep::kRead) {
    [[maybe_unused]] const auto &guard = page.lock.LockS();
    size_t sum = 0;
    for (size_t i = 0; i < kElementNum; ++i) {
      sum += page.values[i];
    }
  } else {  // kWrite
    [[maybe_unused]] const auto &guard = page.lock.LockX();
    for (size_t i = 0; i < kElementNum; ++i) {
      ++(page.values[i]);
    }
  }

  return 1;
}

template <>
auto
Target<MCSLock>::Execute(  //
    OPTyep type,
    uint32_t pos)  //
    -> size_t
{
  auto &page = pages_[pos];

  if (type == OPTyep::kRead) {
    [[maybe_unused]] const auto &guard = page.lock.LockS();
    size_t sum = 0;
    for (size_t i = 0; i < kElementNum; ++i) {
      sum += page.values[i];
    }
  } else {  // kWrite
    [[maybe_unused]] const auto &guard = page.lock.LockX();
    for (size_t i = 0; i < kElementNum; ++i) {
      ++(page.values[i]);
    }
  }

  return 1;
}

template <>
auto
Target<OptimisticLock>::Execute(  //
    OPTyep type,
    uint32_t pos)  //
    -> size_t
{
  auto &page = pages_[pos];

  if (type == OPTyep::kRead) {
    auto &&guard = page.lock.GetVersion();
    do {
      size_t sum = 0;
      for (size_t i = 0; i < kElementNum; ++i) {
        sum += page.values[i];
      }
    } while (!guard.VerifyVersion());
  } else {  // kWrite
    [[maybe_unused]] const auto &guard = page.lock.LockX();
    for (size_t i = 0; i < kElementNum; ++i) {
      ++(page.values[i]);
    }
  }

  return 1;
}

/*############################################################################*
 * Explicit instantiation definitions
 *############################################################################*/

template class Target<std::mutex>;
template class Target<BackOffLock>;
template class Target<MCSLock>;
template class Target<OptimisticLock>;

}  // namespace dbgroup::example
