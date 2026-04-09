CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++14
LDFLAGS = -lgtest -lgtest_main -pthread

BIN_DIR = bin

# find all tests files automatically
TEST_SRCS := $(wildcard */test_*.cpp)

TEST_NAMES := $(notdir $(TEST_SRCS:.cpp=))

TEST_BINS := $(addprefix $(BIN_DIR)/,$(TEST_NAMES))

VPATH := $(sort $(dir $(TEST_SRCS)))

# shared sources (no test files here)
SRCS = 	\
		../config/ConfigTokenizer.cpp \
		../logger/Logger.cpp \
		../config/Config.cpp \
		../config/ServerConfig.cpp \
		../config/LocationConfig.cpp \
		# ../server/Server.cpp \
		

all: $(BIN_DIR) $(TEST_BINS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/test_%: test_%.cpp $(SRCS) |$(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Run all tests
test: all
	@for test in $(TEST_BINS); do \
		echo "Running $$test"; \
		./$$test; \
	done

test_%: $(BIN_DIR)/test_%
	./$<

clean:
	rm -rf $(BIN_DIR)

re: clean all

format:
	find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

fclean: clean

.PHONY: all clean fclean re format test