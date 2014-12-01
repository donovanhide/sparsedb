#include <iostream>
#include <cstdint>
#include <cstdio>
#include <random>
#include <sparsedb/stopwatch.h>
#include <sparsedb/xorshift.h>
#include <sparsedb/sparsevector.h>
#include <sparsedb/sparseindex.h>

using namespace sparsedb;

void checkError(std::error_condition err)
{
    if (err)
    {
        std::cerr << err.message() << std::endl;
        std::exit(1);
    }
}

// Pass the filename as the argument
int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cout << "usage: " << argv[0] << " <filename> <width> <factor>"
                  << std::endl;
        std::exit(1);
    }
    const std::string filename(argv[1]);
    const auto width = 1ULL << strtoul(argv[2], 0, 10);
    const auto factor = strtoul(argv[3], 0, 10);
    const auto N = width / factor;

    std::cout << "SparseIndex size: " << width << " factor: " << factor
              << std::endl;

    SparseIndex<SparseVector<std::uint64_t>> index(width);
    XORShiftEngine gen;
    gen.seed(1234);
    std::uniform_int_distribution<uint64_t> prefixDist(0, width - 1);

    StopWatch<std::chrono::steady_clock> t;

    File file(filename.c_str());
    checkError(file.Open(true));

    // Fill the table
    for (size_t i = 0; i < N; i++) index.insert(prefixDist(gen), i);
    std::cout << "Add\t" << N << " keys in " << t << " seconds" << std::endl;

    // Read the table
    t.reset();
    gen.seed(1234);
    for (size_t i = 0; i < N; i++) index.get(prefixDist(gen));
    std::cout << "Get\t" << N << " keys in " << t << " seconds" << std::endl;

    // Write the file
    t.reset();
    index.write(file);
    std::cout << "Write\t" << N << " keys in " << t << " seconds" << std::endl;

    t.reset();
    index.clear();
    std::cout << "Clear\t" << N << " keys in " << t << " seconds" << std::endl;

    checkError(file.Close());
    checkError(file.Open());

    // Read the file;
    t.reset();
    index.read(file);
    std::cout << "Read\t" << N << " keys in " << t << " seconds" << std::endl;

    checkError(file.Close());
    return 0;
}