CXX			=	g++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++17 -Iinclude -Isrc
LDFLAGS		=

NAME		=	plazza

GREEN		=	\033[1;32m
CYAN		=	\033[1;36m
RED			=	\033[1;31m
RESET		=	\033[0m

SRC			=	src/main.cpp							\
				src/cli/Args.cpp						\

OBJ			=	$(SRC:.cpp=.o)

LIB_SRC		=	$(filter-out src/main.cpp,$(SRC))
LIB_OBJ		=	$(LIB_SRC:.cpp=.o)

TESTS_SRC	=	tests/common/test_bootstrap.cpp			\
				tests/common/test_errors.cpp			\
				tests/cli/test_args.cpp					\

TESTS_OBJ	=	$(TESTS_SRC:.cpp=.o)
TESTS_BIN	=	tests/unit_tests
TESTS_LDFLAGS	=	-lcriterion

all:	$(NAME)

$(NAME):	$(OBJ)
	@printf "$(GREEN)Linking $(NAME)$(RESET)\n"
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o:	%.cpp
	@printf "$(CYAN)Compiling $<$(RESET)\n"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TESTS_BIN):	$(LIB_OBJ) $(TESTS_OBJ)
	@printf "$(GREEN)Linking $(TESTS_BIN)$(RESET)\n"
	$(CXX) -o $@ $^ $(LDFLAGS) $(TESTS_LDFLAGS)

tests_run:	$(TESTS_BIN)
	@printf "$(CYAN)Running tests$(RESET)\n"
	./$(TESTS_BIN)

clean:
	@printf "$(RED)Cleaning objects$(RESET)\n"
	rm -f $(OBJ) $(TESTS_OBJ)

fclean:	clean
	@printf "$(RED)Cleaning binaries$(RESET)\n"
	rm -f $(NAME) $(TESTS_BIN) plazza.log

re:	fclean all

.PHONY:	all clean fclean re tests_run
