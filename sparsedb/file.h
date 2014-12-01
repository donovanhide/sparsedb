#pragma once

#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstddef>
#include <string>
#include <iostream>
#include <sstream>
#include "error.h"

namespace sparsedb
{
struct FileVector
{
    void* ptr;
    std::size_t length;
    FileVector(void* ptr = nullptr, std::size_t length = 0)
        : ptr(ptr), length(length)
    {
    }
    friend std::ostream& operator<<(std::ostream& os, const FileVector& fv)
    {
        os << "[" << fv.ptr << "," << fv.length << "]";
        return os;
    }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> vec)
{
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<T>(os, ","));
    return os;
}

class File
{
   private:
    std::int32_t fd_ = -1;
    std::string filename_;

   public:
    explicit File(std::string const& filename) : filename_(filename) {}
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    std::error_condition Open(bool const truncate = false)
    {
        return truncate ? open(O_RDWR | O_CREAT | O_TRUNC)
                        : open(O_RDWR | O_CREAT);
    }

    std::error_condition OpenAppend()
    {
        return open(O_RDWR | O_APPEND | O_CREAT);
    }

    std::error_condition OpenSync() { return open(O_RDWR | O_CREAT | O_SYNC); }

    std::error_condition Close()
    {
        if (auto err = Sync())
            return err;
        if (auto err = checkError(::close(fd_)))
            return err;
        fd_ = -1;
        return std::error_condition();
    }

    std::error_condition Delete()
    {
        if (auto err = checkError(::unlink(filename_.c_str())))
            return err;
        fd_ = -1;
        return std::error_condition();
    }

    std::error_condition Sync() const { return checkError(::fsync(fd_)); };

    std::error_condition Truncate() const
    {
        return checkError(::ftruncate(fd_, 0));
    }

    template <class T>
    std::error_condition Read(std::vector<T>& v) const
    {
        return Read(v.data(), v.size() * sizeof(T));
    }

    std::error_condition Read(void* data, std::size_t const length) const
    {
        // std::cout << "File Read: " << data << ":" << length << std::endl;
        return checkIOError(::read(fd_, data, length), length,
                            db_error::short_read);
    }

    std::error_condition ReadAt(std::uint64_t const pos, void* data,
                                std::size_t const length) const
    {
        return checkIOError(::pread(fd_, data, length, pos), length,
                            db_error::short_read);
    }

    std::error_condition ReadVector(std::vector<FileVector> const& v) const
    {
        if (!v.size())
            return std::error_condition();
        return checkIOError(
            ::readv(fd_, reinterpret_cast<const iovec*>(v.data()), v.size()),
            sumLength(v), db_error::short_read);
    }

    template <class T>
    std::error_condition Write(std::vector<T> const& v) const
    {
        return Write(v.data(), v.size() * sizeof(T));
    }

    std::error_condition Write(const void* data, std::size_t const length) const
    {
        return checkIOError(::write(fd_, data, length), length,
                            db_error::short_write);
    }

    std::error_condition WriteAt(std::uint64_t const pos, const void* data,
                                 std::size_t const length) const
    {
        return checkIOError(::pwrite(fd_, data, length, pos), length,
                            db_error::short_write);
    }

    std::error_condition WriteVector(std::vector<FileVector> const& v) const
    {
        if (!v.size())
            return std::error_condition();
        return checkIOError(
            ::writev(fd_, reinterpret_cast<const iovec*>(v.data()), v.size()),
            sumLength(v), db_error::short_read);
    }

    std::error_condition Size(std::uint64_t& size) const
    {
        struct stat sb;
        if (auto err = checkError(::fstat(fd_, &sb)))
            return err;
        size = sb.st_size;
        return std::error_condition();
    }

   private:
    std::error_condition open(std::int32_t const flags)
    {
        fd_ = ::open(filename_.c_str(), flags, 0644);
        return checkError(fd_);
    }

    std::error_condition checkError(ssize_t const err) const
    {
        if (err < 0)
            return std::generic_category().default_error_condition(errno);
        return std::error_condition();
    }

    std::error_condition checkIOError(ssize_t const ret,
                                      ssize_t const expectedLength,
                                      db_error err) const
    {
        if (ret < 0)
            return std::generic_category().default_error_condition(errno);
        if (ret != expectedLength)
            make_error_condition(err);
        return std::error_condition();
    }

    ssize_t sumLength(std::vector<FileVector> const& v) const
    {
        return std::accumulate(v.cbegin(), v.cend(), 0ULL,
                               [](ssize_t sum, FileVector const& fv)
                               {
            return sum + fv.length;
        });
    }
};

}  // namespace sparsedb
