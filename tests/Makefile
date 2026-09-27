# Host unit tests for AverageFilter.h
# Usage: make -C tests [CXX=clang++] [STD=c++17]

STD ?= gnu++11
CXXFLAGS = -std=$(STD) -g -O1 -Wall -Wextra -Wpedantic -Wshadow -Werror \
           -fsanitize=address,undefined -fno-sanitize-recover=all -I../src

test: test_averagefilter
	./test_averagefilter

test_averagefilter: test_averagefilter.cpp ../src/AverageFilter.h
	$(CXX) $(CXXFLAGS) $< -o $@ -lm

clean:
	rm -f test_averagefilter

.PHONY: test clean
