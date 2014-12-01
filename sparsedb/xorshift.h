#pragma once

#include <limits>
#include <stdexcept>

namespace sparsedb
{
// Simple and fast RNG based on:
// http://xorshift.di.unimi.it/xorshift128plus.c
// does not accept seed==0
class XORShiftEngine
{
   public:
    using result_type = std::uint64_t;

    static const result_type default_seed = 1977u;

    explicit XORShiftEngine(result_type val = default_seed) { seed(val); }

    void seed(result_type const seed)
    {
        if (seed == 0)
            throw std::range_error("zero seed supplied");
        s[0] = murmurhash3(seed);
        s[1] = murmurhash3(s[0]);
    }

    result_type operator()()
    {
        result_type s1 = s[0];
        const result_type s0 = s[1];
        s[0] = s0;
        s1 ^= s1 << 23;
        return (s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0;
    }

    static constexpr result_type min()
    {
        return std::numeric_limits<result_type>::min();
    }

    static constexpr result_type max()
    {
        return std::numeric_limits<result_type>::max();
    }

   private:
    result_type s[2];

    static result_type murmurhash3(result_type x)
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        return x ^= x >> 33;
    }
};
}  // namespace sparsedb
