# C++ Benchmarker

![Unit Tests](https://github.com/dbgroup-nagoya-u/cpp-benchmark/workflows/Unit%20Tests/badge.svg?branch=main)

## Build

Note: this is a header only library. You can use this without pre-build.

### Prerequisites

```bash
sudo apt update && sudo apt install -y build-essential cmake
cd <path_to_your_workspace>
git clone --recursive git@github.com:dbgroup-nagoya-u/cpp-benchmark.git
```

### Build Options

- `CPP_BENCH_BUILD_TESTS`: build unit tests for this repository if `ON` (default: `OFF`).
- `CPP_BENCH_TEST_THREAD_NUM`: the maximum number of threads to perform unit tests (default `8`).

### Build and Run Unit Tests

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCPP_BENCH_BUILD_TESTS=ON ..
make -j
ctest -C Release
```

## Acknowledgments

This work is based on results obtained from project JPNP16007 commissioned by the New Energy and Industrial Technology Development Organization (NEDO). In addition, this work was supported partly by KAKENHI (16H01722 and 20K19804).
