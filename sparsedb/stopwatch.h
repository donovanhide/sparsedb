#pragma once
#include <iosfwd>
#include <chrono>

namespace sparsedb
{
template <class T>
class StopWatch
{
    using fpSeconds =
        std::chrono::duration<float, std::chrono::seconds::period>;
    typename T::time_point start_;

   public:
    StopWatch() { reset(); }
    void reset() { start_ = T::now(); }
    friend std::ostream& operator<<(std::ostream& stream, const StopWatch& t)
    {
        auto now = T::now();
        stream << fpSeconds(now - t.start_).count();
        return stream;
    }
};
}  // namespace sparsedb
