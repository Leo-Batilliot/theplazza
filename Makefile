CXX			=	g++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++17 -Iinclude -Isrc
LDFLAGS		=

NAME		=	plazza

GREEN		=	\033[1;32m
CYAN		=	\033[1;36m
RED			=	\033[1;31m
RESET		=	\033[0m

SRC			=	src/main.cpp							\

OBJ			=	$(SRC:.cpp=.o)

all:	$(NAME)

$(NAME):	$(OBJ)
	@printf "$(GREEN)Linking $(NAME)$(RESET)\n"
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o:	%.cpp
	@printf "$(CYAN)Compiling $<$(RESET)\n"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@printf "$(RED)Cleaning objects$(RESET)\n"
	rm -f $(OBJ)

fclean:	clean
	@printf "$(RED)Cleaning binaries$(RESET)\n"
	rm -f $(NAME) plazza.log

re:	fclean all

.PHONY:	all clean fclean re
