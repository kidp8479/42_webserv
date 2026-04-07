# ─── Variables ────────────────────────────────────────────────────────────────

NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

# ─── Sources ────────────────────────────────────────────────────────────────── 

SRCS = main.cpp \
	   logger/Logger.cpp \

OBJS = $(SRCS:.cpp=.o)

# ─── Rules ────────────────────────────────────────────────────────────────────

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

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
