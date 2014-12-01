TEST_DIR = tests
TOOLS_DIR = tools
BUILD := debug

# Paths to the fused gtest files.
GTEST_H = $(TEST_DIR)/gtest/gtest.h
GTEST_ALL_C = $(TEST_DIR)/gtest/gtest-all.cc

cxxflags.debug := -g -O3
cxxflags.release := -g -O3 -DNDEBUG

CPPFLAGS += -I$(TEST_DIR) -I. -isystem $(TEST_DIR)/gtest
CXXFLAGS += ${cxxflags.${BUILD}} -Wall -Wextra -Wpedantic -mpopcnt -std=c++1y -DGTEST_LANG_CXX11=1
LDFLAGS += -lpthread -ltcmalloc

all : sparsedb_unittests 

valgrind : all
	valgrind --dsymutil=yes --track-origins=yes ./sparsedb_unittests

check : all
	./sparsedb_unittests

clean :
	rm -rf sparsedb_unittests *.o

gtest-all.o : $(GTEST_H) $(GTEST_ALL_C)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(TEST_DIR)/gtest/gtest-all.cc

unittests.o : $(TEST_DIR)/unittests.cc sparsedb/*.h tests/*.h $(GTEST_H)
	$(CXX) $(CPPFLAGS) $(TESTFLAGS) $(CXXFLAGS) -c $(TEST_DIR)/unittests.cc

sparsedb_unittests : unittests.o gtest-all.o 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

bench.o : $(TOOLS_DIR)/bench.cc sparsedb/*.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(TOOLS_DIR)/bench.cc

bench : bench.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
