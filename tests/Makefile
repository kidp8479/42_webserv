CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++14
LDFLAGS = -lgtest -lgtest_main -pthread

SRCS = 	config/test_tokenizer.cpp \
		../config/ConfigTokenizer.cpp \
		../logger/Logger.cpp

TARGET = run_tests

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

re: clean all

format:
	find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

test:
	./run_tests

fclean: clean

clean:
	rm -f $(TARGET)

.PHONY: all clean fclean re format test
