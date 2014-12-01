#pragma once

#include <random>
#include <cstdio>
#include "gtest/gtest.h"
#include "sparsedb/file.h"
#include "sparsedb/sparsevector.h"
#include "sparsedb/sparseindex.h"
#include "sparsedb/xorshift.h"

using namespace sparsedb;

::testing::AssertionResult NoError(std::error_condition err)
{
    if (err)
        return ::testing::AssertionFailure() << err.message();
    return ::testing::AssertionSuccess();
}

template <class T>
void TestInsertAndGet(T& store)
{
    store.clear();
    const auto N = store.size();
    ASSERT_EQ(0ULL, store.num_nonempty());
    std::uint64_t value;
    bool exists;
    for (std::size_t i = 0; i < N; i++)
    {
        std::tie(value, exists) = store.insert(i, i);
        ASSERT_FALSE(exists);
        ASSERT_EQ(0ULL, value);
    }
    ASSERT_EQ(N, store.num_nonempty());
    for (std::size_t i = 0; i < N; i++)
    {
        std::tie(value, exists) = store.get(i);
        ASSERT_TRUE(exists);
        ASSERT_EQ(i, value);
    }
}

template <class T>
void TestRandomInsertAndGet(T& store, std::size_t const factor)
{
    store.clear();
    const auto N = store.size();
    ASSERT_EQ(0ULL, store.num_nonempty());
    std::uniform_int_distribution<uint64_t> posDist(0, N - 1);
    XORShiftEngine gen;
    gen.seed(1234);
    for (size_t i = 0; i < (N / factor); i++)
    {
        store.insert(posDist(gen), i);
    }
    gen.seed(1234);
    bool exists;
    std::uint64_t value;
    for (size_t i = 0; i < (N / factor); i++)
    {
        std::tie(value, exists) = store.get(posDist(gen));
        ASSERT_EQ(exists, true);
    }
}

TEST(SparseVectorTest, Uint64)
{
    SparseVector<std::uint64_t> values;
    TestInsertAndGet(values);
    TestRandomInsertAndGet(values, 1);
    // std::cout << values.to_string();
}

TEST(SparseIndexTest, Uint64)
{
    SparseIndex<SparseVector<std::uint64_t>> index1(1ULL << 24);
    TestInsertAndGet(index1);
    TestRandomInsertAndGet(index1, 4);

    File file("testdb");
    ASSERT_TRUE(NoError(file.Open()));
    ASSERT_TRUE(NoError(index1.write(file)));
    ASSERT_TRUE(NoError(file.Close()));

    SparseIndex<SparseVector<std::uint64_t>> index2(1ULL << 24);
    ASSERT_TRUE(NoError(file.Open()));
    ASSERT_TRUE(NoError(index2.read(file)));
    ASSERT_TRUE(index1 == index2);
    ASSERT_TRUE(NoError(file.Delete()));
    // std::cout << index2;
}