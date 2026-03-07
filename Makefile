CXX := g++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra
TARGET := big_squares
SRCS := main.cpp

all: $(TARGET)

SVGWriter.o: SVGWriter.cpp SVGWriter.h Utils.h MapTools.h
	$(CXX) $(CXXFLAGS) -c SVGWriter.cpp

big_squares.o: main.cpp SVGWriter.h MapTools.h
	$(CXX) $(CXXFLAGS) -c main.cpp	-o big_squares.o

$(TARGET): big_squares.o SVGWriter.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) big_squares.o SVGWriter.o

clean:
	rm -f $(TARGET)
	rm -rf *.o

run: ${TARGET}
	./$(TARGET) test1.csv test1.svg
	@echo Comparing...
	diff -w test1.svg test1-orig.svg


.PHONY: all clean run


test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET)  test1.csv test1.svg
	# @diff -q test_output.txt expected_output.txt && echo "All tests passed!" || echo "Tests failed!"



