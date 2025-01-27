#include <benchmark/benchmark.h>
#include <tablez/sparse/table.h>

#include <random>
#include "tablez/dense/table.h"

namespace {

std::string random_string(std::mt19937 &rng) {
    std::uniform_int_distribution<char> chars{'0', 'Z'};
    auto size = std::uniform_int_distribution<size_t>{8, 128}(rng);

    std::string str;
    str.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        str.push_back(chars(rng));
    }
    return str;
}

std::tuple<int, bool, double, std::string> generate_tuple(std::mt19937 &rng) {
    auto i = std::uniform_int_distribution<int>{}(rng);
    auto b = std::uniform_int_distribution<uint8_t>{0, 1}(rng);
    auto d = std::uniform_real_distribution<double>{}(rng);
    auto s = random_string(rng);
    return {i, b, d, std::move(s)};
}

std::vector<std::tuple<int, bool, double, std::string>> generate_data(std::mt19937 &rng, size_t size) {
    std::vector<std::tuple<int, bool, double, std::string>> vec;
    vec.reserve(size);
    for (size_t idx = 0; idx < size; ++idx) {
        vec.emplace_back(generate_tuple(rng));
    }

    return vec;
}

std::mt19937 &RNG() {
    static std::mt19937 rng{42};
    return rng;
}

void BM_SparseTableInsert(benchmark::State &state) {
    tablez::sparse::Table<int, bool, double, std::string> table;
    for (auto _ : state) {
        state.PauseTiming();
        auto [i, b, d, s] = generate_tuple(RNG());
        state.ResumeTiming();
        table.insert(i, b, d, std::move(s));
    }
}

void BM_DenseTableInsert(benchmark::State &state) {
    tablez::dense::Table<int, bool, double, std::string> table;
    for (auto _ : state) {
        state.PauseTiming();
        auto [i, b, d, s] = generate_tuple(RNG());
        state.ResumeTiming();
        table.insert(i, b, d, std::move(s));
    }
}

void BM_VecPushBack(benchmark::State &state) {
    std::vector<std::tuple<int, bool, double, std::string>> vec;
    for (auto _ : state) {
        state.PauseTiming();
        auto [i, b, d, s] = generate_tuple(RNG());
        state.ResumeTiming();
        vec.emplace_back(i, b, d, std::move(s));
    }
}

void BM_SparseTableSum(benchmark::State &state) {
    auto data = generate_data(RNG(), state.range(0));
    auto table = tablez::sparse::Table<int, bool, double, std::string>::with_capacity(data.size());
    for (auto &[i, b, d, s] : data) {
        table.insert(i, b, d, std::move(s));
    }

    for (auto _ : state) {
        int64_t sum = 0;
        table.column<int>().for_each([&sum](tablez::Id, int val) { sum += val; });
        benchmark::DoNotOptimize(sum);
    }
}

void BM_DenseTableSum(benchmark::State &state) {
    auto data = generate_data(RNG(), state.range(0));
    auto table = tablez::dense::Table<int, bool, double, std::string>::with_capacity(data.size());
    for (auto &[i, b, d, s] : data) {
        table.insert(i, b, d, std::move(s));
    }

    for (auto _ : state) {
        int64_t sum = 0;
        table.for_each<int>([&sum](tablez::Id, int val) { sum += val; });
        benchmark::DoNotOptimize(sum);
    }
}

void BM_VecSum(benchmark::State &state) {
    std::vector<std::tuple<int, bool, double, std::string>> vec = generate_data(RNG(), state.range(0));

    for (auto _ : state) {
        int64_t sum = 0;
        for (auto &tuple : vec) {
            sum += std::get<int>(tuple);
        }
        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_SparseTableInsert);
BENCHMARK(BM_DenseTableInsert);
BENCHMARK(BM_VecPushBack);

BENCHMARK(BM_SparseTableSum)->RangeMultiplier(2)->Range(1 << 4, 1 << 23);
BENCHMARK(BM_DenseTableSum)->RangeMultiplier(2)->Range(1 << 4, 1 << 23);
BENCHMARK(BM_VecSum)->RangeMultiplier(2)->Range(1 << 4, 1 << 23);

}  // namespace
