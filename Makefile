# ─── Variables ────────────────────────────────────────────────────────────────

NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP -g

# ─── Directories ──────────────────────────────────────────────────────────────

OBJ_DIR = obj

# ─── Sources ──────────────────────────────────────────────────────────────────

SRCS = main.cpp \
       config/Config.cpp \
	   config/ConfigBuilder.cpp \
	   config/ConfigParser.cpp \
	   config/ConfigTokenizer.cpp \
       config/ServerConfig.cpp \
       config/LocationConfig.cpp \
	   http/Request.cpp \
	   http/Response.cpp \
	   logger/Logger.cpp \
	   server/Client.cpp \
	   server/Fd.cpp \
	   server/Server.cpp \

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

# ─── Rules ────────────────────────────────────────────────────────────────────

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# ─── clang-format ─────────────────────────────────────────────────────────────
# formats all .cpp and .hpp files in the project
# run : make format

format:
	find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# ─── Phony ────────────────────────────────────────────────────────────────────

.PHONY: all clean fclean re format
