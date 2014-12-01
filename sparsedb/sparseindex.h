#pragma once

#include <cstddef>
#include <cassert>
#include <vector>
#include <string>
#include "file.h"

namespace sparsedb
{
// Simple wrapper around a group of a Vector class, such as SparseVector.
// Implements serialization for the group.
template <class T>
class SparseIndex
{
   private:
    using return_type = typename T::return_type;
    using value_type = typename T::value_type;
    std::size_t size_;
    std::vector<T> groups_;

   public:
    explicit SparseIndex(std::size_t const size)
        : size_(size), groups_((size + T::SIZE - 1) / T::SIZE)
    {
    }

    SparseIndex(const SparseIndex &) = delete;
    SparseIndex &operator=(const SparseIndex &) = delete;

    return_type insert(std::size_t const pos, const value_type value)
    {
        return groups_[group_for_pos(pos)].insert(pos_in_group(pos), value);
    }

    return_type get(std::size_t const pos) const
    {
        return groups_[group_for_pos(pos)].get(pos_in_group(pos));
    }

    bool has(std::size_t const pos) const
    {
        return groups_[group_for_pos(pos)].has(pos_in_group(pos));
    }

    void clear()
    {
        for (auto &g : groups_) g.clear();
    }

    std::size_t num_nonempty() const
    {
        std::size_t count = 0;
        for (auto const &g : groups_) count += g.num_nonempty();
        return count;
    }

    bool operator==(const SparseIndex<T> &rhs)
    {
        return size() == rhs.size() &&
               std::equal(groups_.cbegin(), groups_.cend(),
                          rhs.groups_.cbegin(), rhs.groups_.cend());
    }

    std::error_condition read(File &file)
    {
        if (auto err = file.Read(&size_, sizeof(size_)))
            return err;
        std::size_t groupSize;
        if (auto err = file.Read(&groupSize, sizeof(groupSize)))
            return err;
        groups_.reserve(groupSize);
        groups_.resize(0);
        std::vector<typename T::bitmap_type> v;
        v.reserve(1024 * 1024);
        for (std::size_t i = 0; i < groupSize; i += v.size())
        {
            v.resize(std::min(v.capacity(), groupSize - i));
            if (auto err = file.Read(v))
                return err;
            for (auto const &bitmap : v) groups_.emplace_back(bitmap);
        }
        std::vector<FileVector> fv;
        fv.reserve(1024);
        for (const auto &g : groups_)
        {
            fv.emplace_back(g.ptr(), g.size());
            if (fv.size() == fv.capacity())
            {
                if (auto err = file.ReadVector(fv))
                    return err;
                fv.resize(0);
            }
        }
        return file.ReadVector(fv);
    }

    std::error_condition write(File &file) const
    {
        if (auto err = file.Write(&size_, sizeof(size_)))
            return err;
        auto groupSize = groups_.size();
        if (auto err = file.Write(&groupSize, sizeof(groupSize)))
            return err;
        std::vector<typename T::bitmap_type> v;
        v.reserve(1024 * 1024);
        for (auto &g : groups_)
        {
            v.emplace_back(g.bitmap());
            if (v.size() == v.capacity())
            {
                if (auto err = file.Write(v))
                    return err;
                v.resize(0);
            }
        }
        if (auto err = file.Write(v))
            return err;
        std::vector<FileVector> fv;
        fv.reserve(1024);
        for (const auto &g : groups_)
        {
            fv.emplace_back(g.ptr(), g.size());
            if (fv.size() == fv.capacity())
            {
                if (auto err = file.WriteVector(fv))
                    return err;
                fv.resize(0);
            }
        }
        return file.WriteVector(fv);
    }

    std::size_t size() const { return size_; }

    friend std::ostream &operator<<(std::ostream &stream,
                                    const SparseIndex &index)
    {
        for (auto const &g : index.groups_)
            stream << g.to_string() << std::endl;
        return stream;
    }

   private:
    std::size_t group_for_pos(std::size_t const pos) const
    {
        assert(pos < size_);
        return pos / T::SIZE;
    }

    std::size_t pos_in_group(std::size_t const pos) const
    {
        assert(pos < size_);
        return pos % T::SIZE;
    }
};
}  // namespace sparsedb
