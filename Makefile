CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
TARGET := big_squares
SRCS := main.cpp

all: $(TARGET)

SVGWriter.o: SVGWriter.cpp SVGWriter.h Utils.h
	$(CXX) $(CXXFLAGS) -c SVGWriter.cpp

big_squares.o: main.cpp SVGWriter.h
	$(CXX) $(CXXFLAGS) -c main.cpp	-o big_squares.o

$(TARGET): big_squares.o SVGWriter.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) big_squares.o SVGWriter.o

clean:
	rm -f $(TARGET)
	rm -rf *.o

.PHONY: all clean

test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET)  test1.csv test1.svg
	# @diff -q test_output.txt expected_output.txt && echo "All tests passed!" || echo "Tests failed!"



