#pragma once

#include <cstddef>
#include <cassert>
#include <cstring>
#include <sstream>
#include <system_error>
#include <iostream>
#include <iterator>
#include <iomanip>
#include "file.h"

#ifdef __MSC_VER
#include <nmmintrin.h>
#define __builtin_popcountll _mm_popcnt_u64
#endif
namespace sparsedb
{
// A simple append only vector that can contain up to 64 integral values and
// that reallocates memory exactly as required.
template <class T>
class SparseVector
{
    static_assert(std::is_integral<T>::value, "T must be an integer type");

    std::uint64_t bitmap_ = 0;
    T *p_ = nullptr;

   public:
    using return_type = std::pair<T, bool>;
    using bitmap_type = std::uint64_t;
    using value_type = T;

    enum
    {
        SIZE = 64,
        MAX_POS = SIZE - 1
    };

    SparseVector(const bitmap_type bitmap = 0) : bitmap_(bitmap)
    {
        resize(num_nonempty());
    }

    // TODO: Is this correct?
    SparseVector(const SparseVector &sv)
    {
        bitmap_ = sv.bitmap_;
        p_ = sv.p_;
    }

    SparseVector &operator=(const SparseVector &) = delete;

    ~SparseVector()
    {
        std::free(p_);
        p_ = nullptr;
    }

    std::size_t max_size() const { return SIZE; }
    std::size_t num_nonempty() const { return __builtin_popcountll(bitmap_); }
    std::uint64_t bitmap() const { return bitmap_; }
    T *ptr() const { return p_; }
    std::size_t size() const { return num_nonempty() * sizeof(T); }

    void clear()
    {
        resize(0);
        bitmap_ = 0;
    }

    void resize(std::size_t const newSize)
    {
        auto rounded = (newSize % 2 == 0) ? newSize : newSize + 1;
        if (!rounded)
        {
            std::free(p_);
            p_ = nullptr;
        }
        else
        {
            void *mem = std::realloc(p_, rounded * sizeof(T));
            if (!mem)
                throw std::bad_alloc();
            p_ = static_cast<T *>(mem);
        }
    }

    bool has(std::size_t const pos) const
    {
        assert(pos <= MAX_POS);
        return bitmap_ & (1ULL << pos);
    }

    // Inserts a new value at pos. Return the previous value and true if one
    // exists. Position must be less than 64.
    return_type insert(std::size_t const pos, T const value)
    {
        assert(pos <= MAX_POS);
        auto exists = has(pos);
        auto offset = get_offset(pos);
        T previous = 0;
        if (exists)
        {
            previous = p_[offset];
        }
        else
        {
            auto count = num_nonempty();
            if (count % 2 == 0)
                resize(count + 2);
            if (count > 0)
                // faster than
                // std::copy_backward(p + offset, p + count, p + count + 1);
                for (std::size_t i = count; i > offset; i--)
                    std::memcpy(p_ + i, p_ + i - 1, sizeof(T));
            set_pos(pos);
        }
        p_[offset] = value;
        return return_type{exists, previous};
    }

    // If pos is occupied return value and true, otherwise return 0 and
    // false. Position must be less than 64.
    return_type get(std::size_t const pos) const
    {
        assert(pos <= MAX_POS);
        if (has(pos))
            return return_type{p_[get_offset(pos)], true};
        return return_type{0, false};
    }

    bool operator==(SparseVector<T> const &rhs) const
    {
        return num_nonempty() == rhs.num_nonempty() && size() == rhs.size() &&
               std::memcmp(ptr(), rhs.ptr(), size()) == 0;
    }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << std::setw(2) << num_nonempty() << "/" << std::setw(2) << size()
           << " : [";
        for (std::size_t i = 0; i <= MAX_POS; i++)
        {
            if (has(i))
                ss << std::to_string(p_[get_offset(i)]);
            else
                ss << "-";
            if (i < MAX_POS)
                ss << ",";
        }
        ss << "]";
        return ss.str();
    }

   private:
    void set_pos(std::size_t const pos) { bitmap_ |= 1ULL << pos; }

    std::size_t get_offset(std::size_t const pos) const
    {
        std::uint64_t mask = (1ULL << pos) - 1ULL;
        return __builtin_popcountll(bitmap_ & mask);
    }
};
}  // namespace sparsedb
