CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++14 -I../config -I../logger
LDFLAGS = -lgtest -lgtest_main -pthread

SRCS = 	config/test_tokenizer.cpp \
		../config/ConfigTokenizer.cpp \
    	../logger/Logger.cpp

TARGET = run_tests

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean