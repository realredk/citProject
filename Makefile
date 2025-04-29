# Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
# hereby granted to students registered for University of Pennsylvania
# CIT 5950 for use solely during Spring Semester 2025 for purposes of
# the course.  No other use, copying, distribution, or modification
# is permitted without prior written consent. Copyrights for
# third-party components of this work must be honored.  Instructors
# interested in reusing these course materials should contact the
# author.

.PHONY: clean all tidy-check format

# define the commands we will use for compilation and library building
CXX = clang++-15
# define useful flags to cc/ld/etc.
CXXFLAGS = -g3 -gdwarf-4 -Wall -Wpedantic -std=c++2b -pthread -I. -O0

# Common object files (your modules)
COMMON_OBJS := \
    ThreadPool.o \
    ServerSocket.o \
    HttpSocket.o \
    WordIndex.o \
    HttpUtils.o \
    CrawlFileTree.o \
    FileReader.o

# All headers (for dependencies, tidy/format)
HEADERS := \
    ThreadPool.hpp \
    ServerSocket.hpp \
    HttpSocket.hpp \
    WordIndex.hpp \
    HttpUtils.hpp \
    CrawlFileTree.hpp \
    FileReader.hpp \
    Result.hpp \
    catch.hpp

# Test object files (Catch2 tests + catch main)
TEST_OBJS := \
    test_wordindex.o \
    test_serversocket.o \
    test_crawlfiletree.o \
    test_httpsocket.o \
    test_httputils.o \
    test_threadpool.o \
    test_suite.o \
    catch.o

# All .cpp sources (for tidy & format)
CPP_SOURCE_FILES := \
    FileReader.cpp \
    HttpUtils.cpp \
    CrawlFileTree.cpp \
    WordIndex.cpp \
    HttpSocket.cpp \
    ServerSocket.cpp \
    ThreadPool.cpp \
    searchserver.cpp \
    catch.cpp \
    test_wordindex.cpp \
    test_serversocket.cpp \
    test_crawlfiletree.cpp \
    test_httpsocket.cpp \
    test_httputils.cpp \
    test_threadpool.cpp \
    test_suite.cpp

# All .hpp headers (for tidy & format)
HPP_SOURCE_FILES := \
    FileReader.hpp \
    HttpUtils.hpp \
    CrawlFileTree.hpp \
    WordIndex.hpp \
    HttpSocket.hpp \
    ServerSocket.hpp \
    ThreadPool.hpp \
    Result.hpp \
    catch.hpp

# Default target: build both the application and the tests
all: searchserver test_suite

# Link the searchserver executable
searchserver: searchserver.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Link the Catch2 test suite executable
test_suite: $(TEST_OBJS) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile catch.cpp (defines main for tests)
catch.o: catch.cpp catch.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test_suite.cpp
test_suite.o: test_suite.cpp catch.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Individual test compilation rules
test_wordindex.o: test_wordindex.cpp catch.hpp WordIndex.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_serversocket.o: test_serversocket.cpp catch.hpp ServerSocket.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_crawlfiletree.o: test_crawlfiletree.cpp catch.hpp CrawlFileTree.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_httpsocket.o: test_httpsocket.cpp catch.hpp HttpSocket.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_httputils.o: test_httputils.cpp catch.hpp HttpUtils.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_threadpool.o: test_threadpool.cpp catch.hpp ThreadPool.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generic rule for .cpp -> .o
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up all binaries and object files
clean:
	rm -f *.o searchserver test_suite

# Static analysis with clang-tidy
tidy-check:
	clang-tidy-15 \
	  --extra-arg=--std=c++2b \
	  -warnings-as-errors=* \
	  -header-filter='.*' \
	  $(CPP_SOURCE_FILES) $(HPP_SOURCE_FILES)

# Auto-format with clang-format (Chromium style)
format:
	clang-format-15 -i --verbose --style=Chromium \
	  $(CPP_SOURCE_FILES) $(HPP_SOURCE_FILES)
