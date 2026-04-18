CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
LDFLAGS = -pthread

BIN_DIR = bin
OBJ_DIR = obj

# Google Test (submodule)
GTEST_DIR = googletest/googletest
GTEST_INC = -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_OBJ = $(OBJ_DIR)/gtest-all.o
GTEST_MAIN_OBJ = $(OBJ_DIR)/gtest_main.o

# find all test files automatically
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
		../server/Fd.cpp \
		../server/Client.cpp \
		../http/Request.cpp \
		../http/Response.cpp \
		#../server/Server.cpp \
		

all: $(BIN_DIR) $(OBJ_DIR) $(GTEST_OBJ) $(GTEST_MAIN_OBJ) $(TEST_BINS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(GTEST_OBJ): $(GTEST_DIR)/src/gtest-all.cc | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INC) -c $< -o $@

$(GTEST_MAIN_OBJ): $(GTEST_DIR)/src/gtest_main.cc | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INC) -c $< -o $@

$(BIN_DIR)/test_%: test_%.cpp $(SRCS) $(GTEST_OBJ) $(GTEST_MAIN_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INC) $^ $(LDFLAGS) -o $@

test: all
	@for test in $(TEST_BINS); do \
		echo "Running $$test"; \
		(cd $(BIN_DIR) && ./`basename $$test`); \
	done

test_%: $(BIN_DIR)/test_%
	cd $(BIN_DIR) && ./$(notdir $<)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(BIN_DIR)

re: fclean all

format:
	find . -path "./googletest" -prune \
		-o \( -name "*.cpp" -o -name "*.hpp" \) -print \
		| xargs clang-format -i

.PHONY: all clean fclean re format test test_%
